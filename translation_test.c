#include <stdio.h>
#include <uchar.h>
#include <stdlib.h>
#include <string.h>

extern size_t utf16le_to_utf8_ref(unsigned char out[restrict], const char16_t in[restrict], size_t len, size_t *outlen);
extern size_t utf16le_to_utf8_avx512(unsigned char out[restrict], const char16_t in[restrict], size_t len, size_t *outlen);
extern size_t utf16le_to_utf8_avx512i(unsigned char out[restrict], const char16_t in[restrict], size_t len, size_t *outlen);

extern size_t utf16le_to_utf8_buflen_ref(size_t);
extern size_t utf16le_to_utf8_buflen_avx512(size_t);
extern size_t utf16le_to_utf8_buflen_avx512i(size_t);

/* all test vectors end in U+FFFF to allow embedded NUL characters */
const char16_t *vectors[] = {
	u"\uffff", /* empty string */
	u"Das Pferd frisst keinen Gurkensalat.\uffff", /* ASCII string */
	u"Fix Schwyz quäkt Jürgen blöd vom Paß.\uffff", /* ISO-8859-1 string */
	u"ドイツの科学は世界一です！\uffff", /* Japanese mixed script string */
	u"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق.\uffff", /* Arabic */
	u"國之語音，異乎中國，與文字不相流通，故愚民有所欲言，而終不得伸其情者多矣。予為此憫然，新制二十八字，欲使人人易習便日用耳。\uffff", /* Chinese */
	u"모든 인간은 태어날 때부터 자유로우며 그 존엄과 권리에 있어 동등하다. 인간은 천부적으로 이성과 양심을 부여받았으며 서로 형제애의 정신으로 행동하여야 한다.\uffff", /* Korean */

	u"\0\uffff", /* NUL string */
	u"A small step for man\0a large step for mankind\0\uffff", /* NUL embedded into ASCII */
	u"Université\0TÉLUQ\uffff", /* NUL embedded into two-byte characters */
	u"Germany\0דייטשלאנד\0آلمان\0Германия\uffff", /* NUL embedded into three byte characters */

	u"𑀤𑁂𑀯𑀸𑀦𑀁𑀧𑀺𑀬𑁂𑀦 𑀧𑀺𑀬𑀤𑀲𑀺𑀦 𑀮𑀸𑀚𑀺𑀦𑀯𑀻𑀲𑀢𑀺𑀯𑀲𑀸𑀪𑀺𑀲𑀺𑀢𑁂𑀦\uffff", /* Brahmi script, all surrogates */
	u"x😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏\uffff", /* Emoji w/ surrogates in odd positions */
	u"😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟x\uffff", /* Emoji at even positions */
	u"🚀🚁🚂🚃🚄🚅🚆🚇🚈🚉🚊🚋🚌🚍🚎🚏🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛🚜🚝🚞🚟\uffff" /* map symbols (x32) */
	u"no bikes: 🚳, no drinking: 🚱, no littering: 🚯\uffff", /* map symbols intermixed with ASCII */

	/* test cases with encoding errors */
	u"\xd834\xdd1e\uffff", /* flipped surrogates */
	u"x\xd830\xdd20\uffff", /* in odd position */
	u"0123456789abcdef0123456789abcdef\uffff", /* full block */
	u"0123456789abcdef0123456789abcde\xd800\uffff", /* lone high surrogate in lookahead */
	u"0123456789abcdef0123456789abcde\xdc01\uffff", /* lone low surrogate in lookahead */
	u"0123456789abcdef0123456789abcd\xd802\xdc03\uffff", /* surrogate split over lookahead */
	u"0123456789abcdef0123456789abcd\xdc04\xd805\uffff", /* same with flipped surrogate pair */
	u"0123456789abcdef0123456789abcde\xd806\xdc07\uffff", /* matched surrogate in lookahead */
	u"0123456789abcdef0123456789abcde\xdc08\xd809\uffff", /* flipped surrogate in lookahead */
	NULL,
};

/* find the number of words before a \uffff */
size_t str16len_ffff(const char16_t str[])
{
	size_t i;

	for (i = 0; str[i] != u'\uffff'; i++)
		;

	return (i);
}

void print_vector(int i, const char16_t *vector)
{
	size_t j, len;

	len = str16len_ffff(vector);
	printf("VECTOR %d (%zu)\n", i, len);

	for (j = 0; j < len; j++)
		printf("%s%04x", j % 16 == 0 ? "\n\t" : " ", (unsigned)vector[j]);

	putchar('\n');
}

void print_utf8(const unsigned char *str, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		printf("%s%02x", i % 32 == 0 ? "\n\t" : " ", (unsigned)str[i]);

	putchar('\n');
}

int test(int i, const char16_t *vector)
{
	unsigned char *refbuf, *avx512buf, *avx512ibuf;
	size_t reflen, avx512len, avx512ilen, inlen;
	size_t refxlat, avx512xlat, avx512ixlat, refout = -1, avx512out = -1, avx512iout = -1;
	int result = 0;

	printf("TESTCASE %d\n", i);

	inlen = str16len_ffff(vector);
	reflen = utf16le_to_utf8_buflen_ref(inlen);
	avx512len = utf16le_to_utf8_buflen_avx512(inlen);
	avx512ilen = utf16le_to_utf8_buflen_avx512i(inlen);

	refbuf = malloc(reflen + 1);
	if (refbuf == NULL) {
		perror("malloc(refbuf)");
		return (1);
	}

	avx512buf = malloc(avx512len + 1);
	if (avx512buf == NULL) {
		perror("malloc(avx512buf)");
		free(refbuf);
		return (1);
	}

	avx512ibuf = malloc(avx512ilen + 1);
	if (avx512ibuf == NULL) {
		perror("malloc(avx512ibuf)");
		free(avx512buf);
		free(refbuf);
		return (1);
	}

	/* write bullshit to buffers so it's easy to catch uninitialised memory */
	memset(refbuf, 0xff, reflen);
	memset(avx512buf, 0xff, avx512len);
	memset(avx512ibuf, 0xff, avx512ilen);

	/* add sentinels to catch buffer overruns */
	refbuf[reflen] = 0xfe;
	avx512buf[avx512len] = 0xfe;
	avx512ibuf[avx512ilen] = 0xfe;

	refxlat = utf16le_to_utf8_ref(refbuf, vector, inlen, &refout);
	avx512xlat = utf16le_to_utf8_avx512(avx512buf, vector, inlen, &avx512out);
	avx512ixlat = utf16le_to_utf8_avx512i(avx512ibuf, vector, inlen, &avx512iout);

	if (refxlat != avx512xlat || refout != avx512out
	    || refxlat != avx512ixlat || refout != avx512iout) {
		print_vector(i, vector);
		printf("length mismatch:\n");
		printf("	converted: %zu (ref) vs %zu (avx512) vs %zu (avx512i)\n", refxlat, avx512xlat, avx512ixlat);
		printf("	output length: %zu (ref) vs %zu (avx512) vs %zu (avx512i)\n", refout, avx512out, avx512iout);

		goto failed;
	}

	if (refout > reflen) {
		print_vector(i, vector);
		printf("implausible output length %zu (ref)\n", refout);
		goto failed;
	}

	if (memcmp(refbuf, avx512buf, refout) != 0 || memcmp(refbuf, avx512ibuf, refout) != 0) {
		print_vector(i, vector);
		printf("encoding mismatch\n");

	failed:	if (refout > reflen)
			refout = reflen;

		if (avx512out > avx512len)
			avx512out = avx512len;

		if (avx512iout > avx512ilen)
			avx512iout = avx512ilen;

		printf("OUTPUT %d/ref (%zu)\n", i, refout);
		print_utf8(refbuf, refout);
		printf("OUTPUT %d/avx512 (%zu)\n", i, avx512out);
		print_utf8(avx512buf, avx512out);
		printf("OUTPUT %d/avx512i (%zu)\n", i, avx512iout);
		print_utf8(avx512ibuf, avx512iout);

		result = 1;
	}

	if (refbuf[reflen] != 0xfe || avx512buf[avx512len] != 0xfe || avx512ibuf[avx512ilen] != 0xfe) {
		printf("BUFFER OVERRUN (%d)\n", i);
		abort();
	}

	free(refbuf);
	free(avx512buf);
	free(avx512ibuf);

	return (result);
}

int main() {
	int i;
	int res = 0;

	for (i = 0; vectors[i] != NULL; i++)
		res |= test(i, vectors[i]);

	return (res != 0);
}
