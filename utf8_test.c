#include <stdio.h>
#include <uchar.h>
#include <stdlib.h>
#include <string.h>

extern size_t utf8_to_utf16le_ref(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);
#ifdef __amd64__
extern size_t utf8_to_utf16le_avx512(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);
extern size_t utf8_to_utf16le_avx512i(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);
#endif
#ifdef __aarch64__
extern size_t utf8_to_utf16le_neon(char16_t out[restrict], const unsigned char in[restrict], size_t len, size_t *outlen);
#endif

extern size_t utf8_to_utf16le_buflen_ref(size_t);
#ifdef __amd64__
extern size_t utf8_to_utf16le_buflen_avx512(size_t);
extern size_t utf8_to_utf16le_buflen_avx512i(size_t);
#endif
#ifdef __aarch64__
extern size_t utf8_to_utf16le_buflen_neon(size_t);
#endif

/* all test vectors end in FF to allow embedded NUL characters */
const char *vectors[] = {
	"\xff", /* empty string */
	"Sphinx of black quartz, judge my vows!\n\xff"
	"#include <stdio.h>\n\nint main(void)\n{\n\tputs(\"hello world\");\n}\n\xff"
	"3.14159265358979323846264338327950\xff", /* ASCII */
	"Fix Schwyz quäkt Jürgen blöd vom Paß.\xff", /* ISO-8859-1 */
	"Falsches Üben von Xylophonmusik quält jeden größeren Zwerg.  Voyez le brick géant que j’examine près du wharf.\xff",
	"すべての人間は、生れながらにして自由であり、かつ、尊厳と権利とについて平等である。\xff", /* Japanese mixed script */
	"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق.\xff", /* Arabic */
	"國之語音，異乎中國，與文字不相流通，故愚民有所欲言，而終不得伸其情者多矣。予為此憫然，新制二十八字，欲使人人易習便日用耳。\xff", /* Chinese */
	"모든 인간은 태어날 때부터 자유로우며 그 존엄과 권리에 있어 동등하다. 인간은 천부적으로 이성과 양심을 부여받았으며 서로 형제애의 정신으로 행동하여야 한다.\xff", /* Korean */

	"\0\xff", /* NUL string */
	"A small step for man\0a large step for mankind\0\xff", /* NUL embedded into ASCII */
	"Université\0TÉLUQ\xff", /* NUL embedded into two-byte characters */
	"Germany\0דייטשלאנד\0آلمان\0Германия\xff", /* NUL embedded into three byte characters */
	"ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄA\xff", /* all two byte */
	"AÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\xff", /* two byte on odd positions */

	"𑀤𑁂𑀯𑀸𑀦𑀁𑀧𑀺𑀬𑁂𑀦 𑀧𑀺𑀬𑀤𑀲𑀺𑀦 𑀮𑀸𑀚𑀺𑀦𑀯𑀻𑀲𑀢𑀺𑀯𑀲𑀸𑀪𑀺𑀲𑀺𑀢𑁂𑀦\xff", /* Brahmi script, all surrogates */
	"x😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏\xff", /* Emoji w/ surrogates in odd positions */
	"😐😑😒😓😔😕😖😗😘😙😚😛😜😝😞😟x\xff", /* Emoji at even positions */
	"🚀🚁🚂🚃🚄🚅🚆🚇🚈🚉🚊🚋🚌🚍🚎🚏🚐🚑🚒🚓🚔🚕🚖🚗🚘🚙🚚🚛🚜🚝🚞🚟\xff" /* map symbols (x32) */
	"no bikes: 🚳, no drinking: 🚱, no littering: 🚯\xff", /* map symbols intermixed with ASCII */

	/* test cases with encoding errors */
	"foo\xc0\x80""bar\xff", /* overlong two byte */
	"bump\xc1\x81this\xff",
	"this sucks\xe0\x80\x80\xff", /* overlong three byte */
	"and don't get me started\xe0\x81\xbf\xff",
	"no surrogates for you!\xf0\x80\x80\x80\xff", /* overlong four byte */
	"even messier\xf0\x8f\xbf\xbf\xff",
	"that sounds crazy\xf4\x90\x80\x80\xff", /* character out of range */
	"Röck döts\xf7\xbf\xbf\xbf\xff",
	"NEEDS MOAR!!!\xf8\x80\x80\x80\x80\xff", /* overlong sequences */
	"I SAID MÖAR!\xfb\xbf\xbf\xbf\xbf\xff",
	"素晴らしいですね\xfc\x80\x80\x80\x80\x80\xff",
	"真香！\xfd\xbf\xbf\xbf\xbf\xbf\xff",
	"\xfe\xff", /* illegal bytes */
	"尽二秀才\xfe\xff", /* illegal bytes */
	"\x80""Glaub mir!  Das Pferd frisst keinen Gurkensalat!\xff", /* lone follow byte */
	"really\x80, you gotta believe me with this one!\xff",
	"reaa\x80\x80lly\xff",
	"間違い\x80ない\xff",
	"hicup\xc2\x80\x80\xff", /* too many follow bytes */
	"sneeze\xe4\x81\x82\x83\xff",
	"snooze\xf1\x90\x91\x92\x93\xff",
	"This text\xc7is all wonky!  I wonder why that is...\xff", /* not enough follow bytes */
	"I have too many\xc5€€€\xff",
	"吾輩は猫である。\xe3\x81名前はまだ無い。\xff",

	/* checks for implementation details */
	"01§456789abcdef0123456789abcdef0123456789abcdef0123456789abcde字\xff", /* check for correct wrap around of third-last bytes */
	"0123456789abcdef0123456789abcdef°23456789ABCDEF0123456789abcdef0123456789ABCdef0123456789abcDEF\xff",
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
		printf("%s%02x", j % 32 == 0 ? "\n\t" : " ", (unsigned)(unsigned char)vector[j]);

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
	char16_t *refbuf;
	size_t reflen, inlen, refvalid, refxlat, refout = -1;
#ifdef __amd64__
	char16_t *avx512buf, *avx512ibuf;
	size_t avx512len, avx512ilen, avx512valid, avx512xlat, avx512ixlat;
	size_t avx512out = -1, avx512iout = -1;
#endif
#ifdef __aarch64__
	char16_t *neonbuf;
	size_t neonlen, neonvalid, neonxlat, neonout = -1;
#endif
	int result = 0;

	printf("TESTCASE %d\n", i);

	inlen = strlen_ff(vector);
	reflen = utf8_to_utf16le_buflen_ref(inlen);
#ifdef __amd64__
	avx512len = utf8_to_utf16le_buflen_avx512(inlen);
	avx512ilen = utf8_to_utf16le_buflen_avx512i(inlen);
#endif
#ifdef __aarch64__
	neonlen = utf8_to_utf16le_buflen_neon(inlen);
#endif

	refbuf = malloc((reflen + 1) * sizeof *refbuf);
	if (refbuf == NULL) {
		perror("malloc(refbuf)");
		return (1);
	}

#ifdef __amd64__
	avx512buf = malloc((avx512len + 1) * sizeof *refbuf);
	if (avx512buf == NULL) {
		perror("malloc(avx512buf)");
		free(avx512buf);
		return (1);
	}

	avx512ibuf = malloc((avx512ilen + 1) * sizeof *refbuf);
	if (avx512ibuf == NULL) {
		perror("malloc(avx512ibuf)");
		free(avx512ibuf);
		return (1);
	}
#endif
#ifdef __aarch64__
	neonbuf = malloc((neonlen + 1) * sizeof *refbuf);
	if (neonbuf == NULL) {
		perror("malloc(neonbuf)");
		free(neonbuf);
		return (1);
	}
#endif

	/* write bullshit to buffers so it's easy to catch uninitialised memory */
	memset(refbuf, 0xff, reflen * sizeof *refbuf);
#ifdef __amd64__
	memset(avx512buf, 0xff, avx512len * sizeof *avx512buf);
	memset(avx512ibuf, 0xff, avx512ilen * sizeof *avx512ibuf);
#endif
#ifdef __aarch64__
	memset(neonbuf, 0xff, neonlen * sizeof *neonbuf);
#endif

	/* add sentinels to catch buffer overruns */
	refbuf[reflen] = 0xfffe;
#ifdef __amd64__
	avx512buf[avx512len] = 0xfffe;
	avx512ibuf[avx512ilen] = 0xfffe;
#endif
#ifdef __aarch64__
	neonbuf[neonlen] = 0xfffe;
#endif

	refxlat = utf8_to_utf16le_ref(refbuf, vector, inlen, &refout);
#ifdef __amd64__
	avx512xlat = utf8_to_utf16le_avx512(avx512buf, vector, inlen, &avx512out);
	avx512ixlat = utf8_to_utf16le_avx512i(avx512ibuf, vector, inlen, &avx512iout);

	if (refxlat != avx512xlat || refout != avx512out || refxlat != avx512ixlat || refout != avx512iout) {
		print_vector(i, vector);
		printf("length mismatch:\n");
		printf("	converted: %zu (ref) vs %zu (avx512) vs %zu (avx512i)\n", refxlat, avx512xlat, avx512ixlat);
		printf("	output length: %zu (ref) vs %zu (avx512) vs %zu (avx512i)\n", refout, avx512out, avx512iout);

		goto failed;
	}
#endif
#ifdef __aaarch64__
	neonxlat = utf8_to_utf16le_neon(neonbuf, vector, inlen, &neonout);

	if (refxlat != neonxlat || refout != neonout) {
		print_vector(i, vector);
		printf("length mismatch:\n");
		printf("	converted: %zu (ref) vs %zu (neon)\n", refxlat, neonxlat);
		printf("	output length: %zu (ref) vs %zu (neon\n", refout, neonout);

		goto failed;
	}
#endif


	if (refout > reflen) {
		print_vector(i, vector);
		printf("implausible output length %zu (ref)\n", refout);
		goto failed;
	}

#ifdef __amd64__
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
		print_utf16(refbuf, refout);
		printf("OUTPUT %d/avx512 (%zu)\n", i, avx512out);
		print_utf16(avx512buf, avx512out);
		printf("OUTPUT %d/avx512i (%zu)\n", i, avx512iout);
		print_utf16(avx512ibuf, avx512iout);

		result = 1;
	}
#endif
#ifdef __aarch64__
	if (memcmp(refbuf, neonbuf, refout) != 0) {
		print_vector(i, vector);
		printf("encoding mismatch\n");

	failed:	if (refout > reflen)
			refout = reflen;

		if (neonout > neonlen)
			neonout = neonlen;

		printf("OUTPUT %d/ref (%zu)\n", i, refout);
		print_utf16(refbuf, refout);
		printf("OUTPUT %d/neon (%zu)\n", i, neonout);
		print_utf16(neonbuf, neonout);

		result = 1;
	}
#endif

#ifdef __amd64__
	if (refbuf[reflen] != 0xfffe || avx512buf[avx512len] != 0xfffe || avx512ibuf[avx512ilen] != 0xfffe) {
		printf("BUFFER OVERRUN (%d)\n", i);
		abort();
	}
#endif
#ifdef __aarch64__
	if (refbuf[reflen] != 0xfffe || neonbuf[neonlen] != 0xfffe) {
		printf("BUFFER OVERRUN (%d)\n", i);
		abort();
	}
#endif

end:	free(refbuf);
#ifdef __amd64__
	free(avx512buf);
	free(avx512ibuf);
#endif
#ifdef __aarch64__
	free(neonbuf);
#endif

	return (result);
}

int main() {
	int i;
	int res = 0;

	for (i = 0; vectors[i] != NULL; i++)
		res |= test(i, vectors[i]);

	return (res != 0);
}

