#include <stdio.h>
#include <uchar.h>
#include <stdbool.h>

extern size_t utf16le_validate_ref(const char16_t buf[], size_t len);
extern size_t utf16le_validate_avx512(const char16_t buf[], size_t len);

#define N_TESTVECTOR 23
const struct testvector {
	unsigned char len, valid; /* valid: length of valid prefix */
	char16_t str[63];
} vectors[N_TESTVECTOR] = {
	{ 0, 0, {}}, /* empty string */
	{ 63, 63, {}}, /* NUL string */
	{ 12, 12, { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' }}, /* ASCII */
	{ 13, 13, { 0xfeff, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' }}, /* ASCII w/ BOM */
	{ 13, 0, { 0xfffe, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!' }}, /* ASCII w/ wrong BOM */
	{ 3, 1, { 0x4242, 0xfde0, 0x55aa }}, /* uncharacter */
	{ 1, 1, { 'f', 0, 0xffff }}, /* check for proper masking */
	{ 2, 2, { 0xd834, 0xdd1e }}, /* correct surrogate pair */
	{ 2, 0, { 0xdd1e, 0xd834 }}, /* flipped surrogate pair */
	{ 3, 3, { 0x4223, 0xd830, 0xdd20 }}, /* in odd position */
	{ 3, 1, { 0x4223, 0xdd1e, 0xd834 }}, /* dito */
	{ 2, 0, { 0xdbff, 0xdfff }}, /* high plane noncharacter */
	{ 3, 1, { 'e', 0xd83f, 0xdffe }}, /* another high plane noncharacter */
	{ 4, 4, { 0xd95f, 0xdffd, 0xda1f, 0xdfff }}, /* two valid high plane characters */
	{ 32, 32, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 }}, /* full block */
	{ 32, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xffff }}, /* error in lookahead */
	{ 32, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xd800 }}, /* error in lookahead */
	{ 32, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xdc00 }}, /* error in lookahead */
	{ 32, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xfde8 }}, /* error in lookahead */
	{ 32, 32, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 0xd801, 0xdc02 }}, /* surrogate split over lookahead */
	{ 32, 30, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 0xdc04, 0xd803 }}, /* surrogate split over lookahead */
	{ 33, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xd805, 0x1234 }}, /* lone high surrogate in lookahead */
	{ 33, 31, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0xdc06, 0x5678 }}, /* lone low surrogate in lookahead */
};

void print_testvector(int i, const struct testvector *v)
{
	int j;

	printf("VECTOR %d  (%d/%d)", i, v->valid, v->len);

	for (j = 0; j < v->len; j++)
		printf("%s%04x", j % 16 == 0 ? "\n\t" : " ", (unsigned)v->str[j]);

	putchar('\n');
}

int test(int i, const struct testvector *v)
{
	size_t ref, avx512;

	ref = utf16le_validate_ref(v->str, v->len);
	avx512 = utf16le_validate_avx512(v->str, v->len);

	if (ref == v->valid && avx512 == v->valid)
		return (0);

	print_testvector(i, v);

	if (ref != v->valid)
		printf("fail (ref): want %d got %zu\n", v->valid, ref);

	if (avx512 != v->valid)
		printf("fail (avx512): want %d got %zu\n", v->valid, avx512);

	putchar('\n');

	return (-1);
}

int main() {
	int i;
	int res = 0;

	for (i = 0; i < N_TESTVECTOR; i++)
		res |= test(i, vectors + i);

	return (res != 0);
}
