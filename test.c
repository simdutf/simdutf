#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "utf16fix.h"

enum { MAXLEN = 1024 };

static const struct testcase {
	size_t n;
	const uint16_t *str;
} cases[] = {
	11, u"\x5555\xd000\x0000\x0000\xdc00\xd800\x0000\x0000\x0000\xd800\xaaaa",
	11, u"\x5555\x0000\xdc00\x0000\x0000\x0000\x0000\xdc00\xdc00\xdc00\xaaaa",
	11, u"\x5555\xdc00\xdc00\xdc00\xdc00\xd800\xdc00\xd800\xdc00\xdc00\xaaaa",
	11, u"\x5555\x0000\xffff\xdc00\x0000\xdc00\xd800\xdc00\x0000\x0000\xaaaa",
	-1, NULL,
};

/*
 * Print a UTF-16 string character by character in hexadecimal.
 */
static void
show_utf16(const char16_t *str, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (i % 16 == 0)
			printf("  %08zx  ", i);

		printf("%04x%c", (unsigned)str[i], i % 16 == 15 ? '\n' : ' ');
	}

	if (n % 16 != 0)
		putchar('\n');
}

/*
 * Generate a random test case.
 * Each character in the test case is a low/high surrogate with a
 * probability of 25% each, otherwise some random character (that may
 * be a surrogate too).
 */
static void
random_testcase(char16_t *buf, size_t n)
{
	size_t i;
	uint64_t rng = 0;
	int rngcap = 0, choice;

	for (i = 0; i < n; i++) {
		if (rngcap < 1) {
			rng = rng << 32 | arc4random();
			rngcap += 32;
		}

		choice = rng & 1;
		rng >>= 1;
		rngcap--;

		if (choice) {
			if (rngcap < 11) {
				rng = rng << 32 | arc4random();
				rngcap += 32;
			}

			buf[i] = 0xd800 | rng & 0x07ff;
			rng >>= 11;
			rngcap -= 11;
		} else {
			if (rngcap < 16) {
				rng = rng << 32 | arc4random();
				rngcap += 32;
			}

			buf[i] = rng & 0xffff;
			rng >>= 16;
			rngcap -= 16;
		}
	}
}

/*
 * Test the function with the given name on the given test case.
 * The first and last character of the test case are used as sentinels,
 * they are not translated.  Test both in-place and out-of-place operation.
 * Return 0 on success, -1 on test case failure.  Print an explanation of
 * the test case failure.
 */
static int
test_case(const char *impl, utf16fix *func, const char16_t *input, size_t n)
{
	size_t i;
	char16_t ref_out[MAXLEN], oop_out[MAXLEN], ip_out[MAXLEN];
	bool out_of_place_failed = false, in_place_failed = false, failed;

	assert(2 <= n);
	assert(n <= MAXLEN);

	ref_out[0] = input[0];
	utf16fix_reference(ref_out + 1, input + 1, n - 2);
	ref_out[n - 1] = input[n - 1];

	/* out of place test */
	oop_out[0] = 0xdead;
	memset(oop_out + 1, 0xcc, (n - 2) * sizeof *oop_out);
	oop_out[n - 1] = 0xbeef;
	func(oop_out + 1, input + 1, n - 2);

	if (oop_out[0] != 0xdead || oop_out[n - 1] != 0xbeef) {
		printf("%s: sentinels mutilated (should be DEAD and BEEF)\n", impl);
		out_of_place_failed = true;
	}

	if (memcmp(ref_out + 1, oop_out + 1, (n - 2) * sizeof *ref_out) != 0) {
		printf("%s: output mismatch in out-of-place test\n", impl);
		out_of_place_failed = true;
	}

	/* in-place test */
	memcpy(ip_out, input, n * sizeof *input);
	func(ip_out + 1, ip_out + 1, n - 2);

	if (memcmp(ip_out, ref_out, n * sizeof *ref_out) != 0) {
		printf("%s: output mismatch or sentinels mutilated in in-place test\n", impl);
		in_place_failed = true;
	}

	if (in_place_failed || out_of_place_failed) {
		printf("input:\n");
		show_utf16(input, n);

		printf("wanted output:\n");
		show_utf16(ref_out, n);

		if (out_of_place_failed) {
			printf("out of place output:\n");
			show_utf16(oop_out, n);
		}

		if (in_place_failed) {
			printf("in place output:\n");
			show_utf16(ip_out, n);
		}

		putchar('\n');

		return (-1);
	}

	return (0);
}

/*
 * Run the test suite on the given function.  Return 0 on success, -1
 * on failure.
 */
static int
run_tests(const char *impl, utf16fix *func)
{
	size_t i, n;
	int failed = 0;
	char16_t input[MAXLEN];

	for (i = 0; cases[i].str != NULL; i++)
		failed |= test_case(impl, func, cases[i].str, cases[i].n);

	for (n = 2; n < MAXLEN; n++) {
		for (i = 0; i < 10; i++) {
			random_testcase(input, n);
			failed |= test_case(impl, func, input, n);
		}
	}

	return (failed);
}

int main(int argc, char *argv[])
{
	size_t i;
	int failed = 0;

	for (i = 1; utf16fix_impls[i].name != NULL; i++)
		failed |= run_tests(utf16fix_impls[i].name, utf16fix_impls[i].impl);

	return (failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
