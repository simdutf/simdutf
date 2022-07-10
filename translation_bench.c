#define _GNU_SOURCE
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <uchar.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef TIME_GOAL
/* run each benchmark for at least this many seconds */
#define TIME_GOAL 2.0
#endif

#ifdef ALIGN
#define memory_allocate(size) aligned_alloc(64, (size) + 63 & ~63)
#else
#define memory_allocate(size) malloc(size)
#endif

typedef size_t utf16le_to_utf8(unsigned char[restrict], const char16_t[restrict], size_t, size_t *);
typedef size_t utf16le_buflen(size_t);
extern utf16le_to_utf8 utf16le_to_utf8_ref, utf16le_to_utf8_avx512, utf16le_to_utf8_avx512i;
extern utf16le_buflen utf16le_to_utf8_buflen_ref, utf16le_to_utf8_buflen_avx512, utf16le_to_utf8_buflen_avx512i;

static const struct utf16le_to_utf8_method {
	const char *name;
	utf16le_to_utf8 *to_utf8;
	utf16le_buflen *buflen;
} to_utf8_methods[] = {
	{ "ref", utf16le_to_utf8_ref, utf16le_to_utf8_buflen_ref },
	{ "avx512", utf16le_to_utf8_avx512, utf16le_to_utf8_buflen_avx512 },
	{ "avx512i", utf16le_to_utf8_avx512i, utf16le_to_utf8_buflen_avx512i },
	{ NULL, NULL, NULL },
};

static int event_group = -1, num_counters = 0;
enum { EVENT_COUNT = 2 };
static struct event {
	uint32_t type;
	int fd;
	uint64_t conf;
} events[EVENT_COUNT] = {
	{ PERF_TYPE_HARDWARE, -1, PERF_COUNT_HW_CPU_CYCLES },
	{ PERF_TYPE_HARDWARE, -1, PERF_COUNT_HW_INSTRUCTIONS },
};

/* set up performance counters so performance measurements can be taken */
static void init_counters()
{
	int i;
	struct perf_event_attr attribs;

	memset(&attribs, 0, sizeof attribs);
	attribs.exclude_kernel = 1;
	attribs.exclude_hv = 1;
	attribs.sample_period = 0;
	attribs.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

	for (i = 0; i < EVENT_COUNT; i++) {
		attribs.type = events[i].type;
		attribs.config = events[i].conf;
		events[i].fd = syscall(SYS_perf_event_open, &attribs, 0, -1, event_group, 0);
		if (events[i].fd == -1) {
			perror("perf_event_open");
			continue;
		}

		num_counters++;

		if (event_group == -1)
			event_group = events[i].fd;
	}
}

/* state of performance counters at one point in time */
struct counters {
	struct timespec ts;
	uint64_t counters[2 * EVENT_COUNT + 1];
};

typedef int testfunc(struct counters *c, void *payload, const char *filename, size_t n, size_t m);

/* initialise hardware perf counters */

/* set counters to their current value */
static int
reset_counters(struct counters *c)
{
	ssize_t count;
	int res;

	res = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &c->ts);
	if (res == -1) {
		perror("clock_gettime");
		return (-1);
	}

	if (event_group != -1) {
		count = read(event_group, c->counters, (2 * num_counters + 1) * sizeof c->counters[0]);
		if (count < 0) {
			perror("read(event_group, ...)");
			return (-1);
		}
	}

	return (0);
}

/* compute the difference between two timespec as a double */
static double
tsdiff(struct timespec start, struct timespec end)
{
	time_t sec;
	long nsec;

	sec = end.tv_sec - start.tv_sec;
	nsec = end.tv_nsec - start.tv_nsec;
	if (nsec < 0) {
		sec--;
		nsec += 1000000000L;
	}

	return (sec + nsec * 1.0e-9);
}

/* compute the difference between the two counter vectors */
static void
counterdiff(uint64_t out[EVENT_COUNT], uint64_t start[], uint64_t end[])
{
	int i, j;

	for (i = 0, j = 0; i < EVENT_COUNT; i++) {
		if (events[i].fd == -1)
			out[i] = 0;
		else {
			out[i] = end[2*j+1] - start[2*j+1];
			j++;
		}
	}
}

/* print test results */
/* https://golang.org/design/14313-benchmark-format */
static void
print_results(
    const char *name,
    struct counters *start, struct counters *end,
    const char *filename, size_t n, size_t m) {
	uint64_t counts[EVENT_COUNT];
	double elapsed;

	elapsed = tsdiff(start->ts, end->ts);
	counterdiff(counts, start->counters, end->counters);

	if (name == NULL || name[0] == '\0')
		name = " ";

	printf("Benchmark%c%s/%s\t%10zu\t"
	    "%.8g ns/op\t%.8g MB/s",
	    toupper(name[0]), name+1, filename, m,
	    (elapsed * 1e9) / m, (1e-6 * n * m) / elapsed);

	if (num_counters == EVENT_COUNT) {
		printf("\t%.8g cy/B\t%.8g ins/B\t%.8g ipc\n",
		    counts[0]/((double)n * m), counts[1]/((double)n * m),
		    (double)counts[1] / counts[0]);
	} else
		putchar('\n');
}

/* run one test case for the specified n and print the result */
static void
run_test(const char *name, testfunc *test, void *payload,
    const char *filename, size_t n)
{
	struct counters start, end;
	size_t m = 1;
	int first_run = 1;

	/* repeatedly run benchmark and adjust m until result is meaningful */
	for (;; first_run = 0) {
		double elapsed;
		size_t newm;
		int res;

		/* printf("RUN %s: n = %zu m = %zu\n", name, n, m); */

		res = reset_counters(&start);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		res = test(&start, payload, filename, n, m);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		res = reset_counters(&end);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		elapsed = tsdiff(start.ts, end.ts);
		if (elapsed < TIME_GOAL) {
			if (elapsed < TIME_GOAL * 0.5)
				m *= 2;
			else {
				/* try to overshoot 1s time goal slightly */
				newm = ceil(m * TIME_GOAL * 1.05 / elapsed);
				m = newm > m ? newm : m + 1;
			}

			continue;
		}

		/* make sure to perform at least one warm-up iteration */
		if (!first_run)
			break;
	}

	print_results(name, &start, &end, filename, n, m);
}

/* test UTF-16 to UTF-8 transcoding by reading a file of n bytes from filename. */
static int
test_utf16le_to_utf8(struct counters *c, void *payload, const char *filename, size_t n, size_t m)
{
	struct utf16le_to_utf8_method *method = (struct utf16le_to_utf8_method *)payload;
	ssize_t count;
	size_t len, i, rem, outlen;
	volatile size_t sum = 0;
	int res, fd;
	char16_t *data;
	unsigned char *out;

	len = n / sizeof *data;
	data = memory_allocate(len * sizeof *data);
	if (data == NULL) {
		perror("memory_allocate");

		return (-1);
	}

	outlen = method->buflen(len);
	out = memory_allocate(outlen);
	if (out == NULL) {
		perror("memory_allocate");

		goto fail1;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror(filename);

		goto fail2;
	}

	rem = n;
	while (rem > 0) {
		count = read(fd, (char *)data + (n - rem), rem);
		if (count < 0) {
			perror(filename);

			goto fail3;
		} else if (count == 0) {
			fprintf(stderr, "%s: file shorter than expected (%zu B < %zu B)\n",
			    filename, n - rem, n);

			goto fail3;
		}

		rem -= count;
	}

	close(fd);

	/* skip initialisation step in benchmark measurements */
	res = reset_counters(c);
	if (res != 0)
		return (-1);

	for (i = 0; i < m; i++)
		sum += method->to_utf8(out, data, len, &outlen);

	if (sum != len * m)
		fprintf(stderr, "Warning (%s/%s): encoding error\n", filename, method->name);

	free(out);
	free(data);

	return (0);

fail3:	close(fd);
fail2:	free(out);
fail1:	free(data);

	return (-1);
}

static void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-r test[,...]] file...\n", argv0);
	exit(EXIT_FAILURE);
}


extern int
main(int argc, char *argv[])
{
	static struct utf16le_to_utf8_method *methods = (struct utf16le_to_utf8_method *)to_utf8_methods;

	int i = 1, j, k, n;

	setlinebuf(stdout);
	init_counters();

	if (argc < 2)
		usage(argv[0]);

	if (strcmp(argv[1], "-r") == 0) {
		char *tok;

		if (argc < 3)
			usage(argv[0]);

		for (n = 0; methods[n].name != NULL; n++)
			;

		methods = malloc((n+1) * sizeof *methods);
		if (methods == NULL) {
			perror("malloc");
			return (EXIT_FAILURE);
		}

		memcpy(methods, to_utf8_methods, (n+1) * sizeof *methods);

		tok = strtok(argv[2], ",");
		j = 0;
		while (tok != NULL) {
			/* find the method and add it to methods */
			for (k = 0; to_utf8_methods[k].name != NULL; k++)
				if (strcmp(tok, to_utf8_methods[k].name) == 0)
					goto found;

			/* not found: */
			fprintf(stderr, "unknown benchmark: %s\n", tok);
			return (EXIT_FAILURE);

		found:	methods[j++] = to_utf8_methods[k];
			tok = strtok(NULL, ",");
		}

		methods[j].name = NULL;
		i = 3;
	}

	for (; i < argc; i++) {
		struct stat st;
		size_t len;
		int res;

		res = stat(argv[i], &st);
		if (res != 0) {
			perror(argv[i]);
			continue;
		}

		len = st.st_size > SIZE_MAX ? SIZE_MAX : (size_t)st.st_size;

		for (j = 0; methods[j].name != NULL; j++)
			run_test(methods[j].name, test_utf16le_to_utf8, (void *)&methods[j], argv[i], len);
	}

	return (EXIT_SUCCESS);
}
