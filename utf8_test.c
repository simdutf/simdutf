#include <stdio.h>
#include <uchar.h>
#include <stdlib.h>
#include <string.h>

extern size_t utf8_to_utf16le_ref(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);
extern size_t utf8_to_utf16le_avx512(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);

extern size_t utf8_to_utf16le_buflen_ref(size_t);
extern size_t utf8_to_utf16le_buflen_avx512(size_t);

//extern size_t utf8_validate_ref(const char16_t in[restrict], size_t len);
//extern size_t utf18_validate_avx512(const char16_t in[restrict], size_t len);

/* all test vectors end in FF to allow embedded NUL characters */
const char *vectors[] = {
	"\xff", /* empty string */
	"Das Pferd frisst keinen Gurkensalat.\xff", /* ASCII string */
	"Fix Schwyz quäkt Jürgen blöd vom Paß.\xff", /* ISO-8859-1 string */
	"ドイツの科学は世界一です！\xff", /* Japanese mixed script string */
	"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق.\xff", /* Arabic */
	"國之語音，異乎中國，與文字不相流通，故愚民有所欲言，而終不得伸其情者多矣。予為此憫然，新制二十八字，欲使人人易習便日用耳。\xff", /* Chinese */
	"모든 인간은 태어날 때부터 자유로우며 그 존엄과 권리에 있어 동등하다. 인간은 천부적으로 이성과 양심을 부여받았으며 서로 형제애의 정신으로 행동하여야 한다.\xff", /* Korean */

	"\0\xff", /* NUL string */
	"A small step for man\0a large step for mankind\0\xff", /* NUL embedded into ASCII */
	"Université\0TÉLUQ\xff", /* NUL embedded into two-byte characters */
	"Germany\0דייטשלאנד\0آلمان\0Германия\xff", /* NUL embedded into three byte characters */

	"𑀤𑁂𑀯𑀸𑀦𑀁𑀧𑀺𑀬𑁂𑀦 𑀧𑀺𑀬𑀤𑀲𑀺𑀦 𑀮𑀸𑀚𑀺𑀦𑀯𑀻𑀲𑀢𑀺𑀯𑀲𑀸𑀪𑀺𑀲𑀺𑀢𑁂𑀦\xff", /* Brahmi script, all surrogates */
	"x😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏\xff", /* Emoji w/ surrogates in odd positions */
	"😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟x\xff", /* Emoji at even positions */
	"🚀🚁🚂🚃🚄🚅🚆🚇🚈🚉🚊🚋🚌🚍🚎🚏🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛🚜🚝🚞🚟\xff" /* map symbols (x32) */
	"no bikes: 🚳, no drinking: 🚱, no littering: 🚯\xff", /* map symbols intermixed with ASCII */

	/* test cases with encoding errors */
	NULL,
};

/* find the number of words before a \xff */
size_t strlen_ff(const char str[])
{
	size_t i;

	for (i = 0; str[i] != '\xff'; i++)
		;

	return (i);
}

void print_vector(int i, const char *vector)
{
	size_t j, len;

	len = strlen_ff(vector);
	printf("VECTOR %d (%zu)\n", i, len);

	for (j = 0; j < len; j++)
		printf("%s%02x", j % 32 == 0 ? "\n\t" : " ", (unsigned)vector[j]);

	putchar('\n');
}

void print_utf16(const char16_t *str, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		printf("%s%04x", i % 16 == 0 ? "\n\t" : " ", (unsigned)str[i]);

	putchar('\n');
}

int test(int i, const char *vector)
{
	char16_t *refbuf, *avx512buf;
	size_t reflen, avx512len, inlen, refvalid, avx512valid;
	size_t refxlat, avx512xlat, refout = -1, avx512out = -1, avx512iout = -1;
	int result = 0;

	printf("TESTCASE %d\n", i);

	inlen = strlen_ff(vector);
	reflen = utf8_to_utf16le_buflen_ref(inlen);
	avx512len = utf8_to_utf16le_buflen_avx512(inlen);

	refbuf = malloc((reflen + 1) * sizeof *refbuf);
	if (refbuf == NULL) {
		perror("malloc(refbuf)");
		return (1);
	}

	avx512buf = malloc((avx512len + 1) * sizeof *refbuf);
	if (avx512buf == NULL) {
		perror("malloc(avx512buf)");
		free(refbuf);
		return (1);
	}

	/* write bullshit to buffers so it's easy to catch uninitialised memory */
	memset(refbuf, 0xff, reflen);
	memset(avx512buf, 0xff, avx512len);

	/* add sentinels to catch buffer overruns */
	refbuf[reflen] = 0xfffe;
	avx512buf[avx512len] = 0xfffe;

#if 0
	/* check for validation failure */
	refvalid = utf8_validate_ref(vector, inlen);
	avx512valid = utf8_validate_avx512(vector, inlen);
	if (refvalid != avx512valid) {
		print_vector(i, vector);
		printf("validation mismatch:\n");
		printf("	validated: %zu (ref) vs. %zu (avx512)\n", refvalid, avx512valid);

		result = 1;
		goto end;
	}
#endif

	refxlat = utf8_to_utf16le_ref(refbuf, vector, inlen, &refout);
	avx512xlat = utf8_to_utf16le_avx512(avx512buf, vector, inlen, &avx512out);

	if (refxlat != avx512xlat || refout != avx512out) {
		print_vector(i, vector);
		printf("length mismatch:\n");
		printf("	converted: %zu (ref) vs %zu (avx512)\n", refxlat, avx512xlat);
		printf("	output length: %zu (ref) vs %zu (avx512)\n", refout, avx512out);

		goto failed;
	}

	if (refout > reflen) {
		print_vector(i, vector);
		printf("implausible output length %zu (ref)\n", refout);
		goto failed;
	}

	if (memcmp(refbuf, avx512buf, refout) != 0) {
		print_vector(i, vector);
		printf("encoding mismatch\n");

	failed:	if (refout > reflen)
			refout = reflen;

		if (avx512out > avx512len)
			avx512out = avx512len;

		printf("OUTPUT %d/ref (%zu)\n", i, refout);
		print_utf16(refbuf, refout);
		printf("OUTPUT %d/avx512 (%zu)\n", i, avx512out);
		print_utf16(avx512buf, avx512out);

		result = 1;
	}

	if (refbuf[reflen] != 0xfffe || avx512buf[avx512len] != 0xfffe) {
		printf("BUFFER OVERRUN (%d)\n", i);
		abort();
	}

end:	free(refbuf);
	free(avx512buf);

	return (result);
}

int main() {
	int i;
	int res = 0;

	for (i = 0; vectors[i] != NULL; i++)
		res |= test(i, vectors[i]);

	return (res != 0);
}

