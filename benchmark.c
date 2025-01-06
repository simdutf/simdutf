#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <unistd.h>

#include "benchmark.h"
#include "utf16fix.h"

static size_t nparam = 1000;
static char16_t *input, *output;

static void
usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [-n length]\n", argv0);
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

extern int
main(int argc, char *argv[])
{
	size_t i;
	int ch, res;
	char dummy;

	while (ch = getopt(argc, argv, "n:"), ch != -1)
		switch (ch) {
		case 'n':
			res = sscanf(optarg, "%zu %c", &nparam, &dummy);
			if (res != 1)  {
				fprintf(stderr, "can't parse -k argument: %s\n", optarg);
				usage(argv[0]);
			}

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
	printf("buflen: %zu\n", nparam);
	preamble();

	for (i = 0; utf16fix_impls[i].name != NULL; i++)
		runbenchmark(utf16fix_impls[i].name, utf16fix_benchmark, utf16fix_impls + i);

	return (EXIT_SUCCESS);
}
