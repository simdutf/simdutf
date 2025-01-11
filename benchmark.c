#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined(__APPLE__)
typedef uint16_t char16_t;
#else
#include <uchar.h>
#endif
#include <unistd.h>

#include "benchmark.h"
#include "utf16fix.h"

#if (defined __FreeBSD__ && __FreeBSD__ >= 9) || \
    (defined __NetBSD__  && __NetBSD_Version__ >= 600000000) || \
    (defined OpenBSD && OpenBSD >= 200805) || \
    (defined __APPLE__ && defined __MACH__ && MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
#define HAVE_ARC4RANDOM_BUF
#endif

#ifndef HAVE_ARC4RANDOM_BUF
uint32_t arc4random(void) {
    static uint32_t state = 123456789;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
void arc4random_buf(void *buf, size_t nbytes) {
    unsigned char *buffer = (unsigned char *)buf;
    for (size_t i = 0; i < nbytes; ++i) {
        buffer[i] = (unsigned char)arc4random();
    }
}
#endif	/* HAVE_ARC4RANDOM_BUF */

static size_t nparam = 1000;
static char16_t *input, *output;

static void
usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [-v] [-n length]\n", argv0);
	exit(EXIT_FAILURE);
}

static void
utf16fix_benchmark(struct B *b, void *payload)
{
	struct utf16fix_impl *impl;
	long i;

	impl = payload;

	b->bytes = nparam * sizeof *input;

	for (i = 0; i < b->n; i++)
		impl->impl(output, input, nparam);
}

/* remove all surrogates from the string -- replace them with 0000 */
static void
make_valid(char16_t *buf, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		if (0xd800 <= buf[i] && buf[i] < 0xe000)
			buf[i] = 0;
}

extern int
main(int argc, char *argv[])
{
	size_t i;
	int ch, res;
	char dummy;
	bool vflag = false;

	while (ch = getopt(argc, argv, "n:v"), ch != -1)
		switch (ch) {
		case 'n':
			res = sscanf(optarg, "%zu %c", &nparam, &dummy);
			if (res != 1)  {
				fprintf(stderr, "can't parse -k argument: %s\n", optarg);
				usage(argv[0]);
			}

			break;

		case 'v':
			vflag = true;
			break;

		default:
			usage(argv[0]);
		}

	if (optind != argc)
		usage(argv[0]);

	input = malloc(nparam * sizeof *input);
	if (input == NULL) {
		perror("malloc(input)");
		return (EXIT_FAILURE);
	}

	output = malloc(nparam * sizeof *output);
	if (output == NULL) {
		perror("malloc(output)");
		return (EXIT_FAILURE);
	}

	arc4random_buf(input, nparam * sizeof *input);
	if (vflag)
		make_valid(input, nparam);

	printf("buflen: %zu\n", nparam);
	preamble();

	for (i = 0; utf16fix_impls[i].name != NULL; i++)
		runbenchmark(utf16fix_impls[i].name, utf16fix_benchmark, utf16fix_impls + i);

	return (EXIT_SUCCESS);
}
