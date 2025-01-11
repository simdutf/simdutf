/*-
 * Copyright (c) 2023 The FreeBSD Foundation
 *
 * This software was developed by Robert Clausecker <fuz@FreeBSD.org>
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
 */

#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif
#include <sys/utsname.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "benchmark.h"

#define GOALNS 1000000000LL /* goal time for benchmark */

static struct timespec
addts(const struct timespec a, const struct timespec b)
{
	struct timespec ts;

	ts.tv_sec = a.tv_sec + b.tv_sec;
	ts.tv_nsec = a.tv_nsec + b.tv_nsec;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}

	return (ts);
}

static struct timespec
subts(const struct timespec a, const struct timespec b)
{
	struct timespec ts;

	ts.tv_sec = a.tv_sec - b.tv_sec;
	ts.tv_nsec = a.tv_nsec - b.tv_nsec;
	if (ts.tv_nsec < 0) {
		ts.tv_sec--;
		ts.tv_nsec += 1000000000L;
	}

	return (ts);
}

static long long
tstons(const struct timespec ts)
{
	return (1000000000LL * ts.tv_sec + ts.tv_nsec);
}

/*
 * Start the benchmark timer if it is stopped.
 */
extern void
starttimer(struct B *b) {
	if (b->timeron)
		return;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &b->start);
	b->timeron = true;
}

/*
 * Stop the benchmark timer if it runs.
 */
extern void
stoptimer(struct B *b) {
	struct timespec now;

	if (!b->timeron)
		return;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
	b->duration = addts(b->duration, subts(now, b->start));
	b->timeron = false;
}

/*
 * Reset the benchmark timer without affecting if it runs.
 */
extern void
resettimer(struct B *b) {
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &b->start);
	b->duration.tv_sec = 0;
	b->duration.tv_nsec = 0;
}

/*
 * Returns the number of nanoseconds that elapsed while the benchmark
 * timer was running.
 */
extern long long
elapsed(struct B *b) {
	struct timespec d;

	d = b->duration;
	if (b->timeron) {
		struct timespec now;

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
		d = addts(d, subts(now, b->start));
	}

	return (tstons(d));
}

/*
 * run the benchmark for the specified number of iterations
 */
static void
runn(struct B *b, long n) {
	b->n = n;
	resettimer(b);
	starttimer(b);
	b->func(b, b->payload);
	stoptimer(b);
	b->prevn = n;
	b->prevduration = b->duration;
}

/*
 * Print the result of the last run of b.
 */
static void
printresult(struct B *b)
{
	double ns;

	ns = elapsed(b);
	if (b->bytes > 0) {
		printf("Benchmark%c%-20.20s\t%10ld\t%10.8g ns/op\t%10.8g MB/s\n",
			toupper(b->name[0]), b->name + 1, b->n, ns / b->n,
			(double)b->n * b->bytes * 1e3 / ns); /* 1e3 = 1e9/1e6 */
	} else {
		printf("Benchmark%c%-20.20s\t%10ld\t%10.8g ns/op\t%10s\n",
			toupper(b->name[0]), b->name + 1, b->n, ns / b->n, "-");
	}
}

/*
 * Run the benchmark with the given name and print its status
 * to stdout.
 */
extern void
runbenchmark(const char *name, void (*func)(struct B *, void *), void *payload)
{
	struct B b;
	long long n, duration;

	b.bytes = 0;
	b.name = name;
	b.func = func;
	b.payload = payload;
	b.timeron = false;

	runn(&b, 1); /* initial run to calibrate benchmark */

	n = 1;
	while (duration = tstons(b.prevduration), duration < GOALNS && n < 1000000000) {
		long long last;

		last = n;
		if (duration <= 0)
			duration = 1;

		n = GOALNS * n / duration;
		n += n / 5;
		if (n > 100 * last)
			n = 100 * last;

		if (n <= last)
			n = last + 1;

		if (n > 1000000000)
			n = 1000000000;

		runn(&b, n);
	}

	printresult(&b);
}

/*
 * Print a preamble to the benchmark results with data on the host system.
 */
extern void
preamble(void)
{
	struct utsname uts;
#ifdef __FreeBSD__
	size_t modellen;
	int res, mib[2] = { CTL_HW, HW_MODEL };
	char model[100];
#endif

	uname(&uts);

	printf("os: %s\narch: %s\n", uts.sysname, uts.machine);

#ifdef __FreeBSD__
	modellen = sizeof model - 1;
	res = sysctl(mib, 2, &model, &modellen, NULL, 0);
	if (res == 0) {
		model[modellen] = '\0';
		printf("cpu: %s\n", model);
	}
#endif

	putchar('\n');
}
