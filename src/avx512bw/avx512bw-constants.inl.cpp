// file included directly

const __m512i v_3f3f_3f7f = _mm512_set1_epi32(0x3f3f3f7f);
const __m512i v_3f3f_3f00 = _mm512_set1_epi32(0x3f3f3f00);
const __m512i v_0140_0140 = _mm512_set1_epi32(0x01400140);
const __m512i v_0001_0000 = _mm512_set1_epi32(0x00010000);
const __m512i v_0001_1000 = _mm512_set1_epi32(0x00011000);
const __m512i v_0010_0000 = _mm512_set1_epi32(0x00100000);
const __m512i v_0000_ffff = _mm512_set1_epi32(0x0000ffff);
const __m512i v_0010_ffff = _mm512_set1_epi32(0x0010ffff);
const __m512i v_ffff_0000 = _mm512_set1_epi32(0xffff0000);
const __m512i v_ffff_f800 = _mm512_set1_epi32(0xfffff800);
const __m512i v_0000_d800 = _mm512_set1_epi32(0xd800);
const __m512i v_0000_000f = _mm512_set1_epi32(0x0f);
const __m512i v_0000_00ff = _mm512_set1_epi32(0xff);
const __m512i v_8080_8000 = _mm512_set1_epi32(0x80808000);
const __m512i v_0000_00c0 = _mm512_set1_epi32(0xc0);
const __m512i v_0000_0080 = _mm512_set1_epi32(0x80);
const __m512i v_fc00_fc00 = _mm512_set1_epi32(0xfc00fc00);
const __m512i v_d800_dc00 = _mm512_set1_epi32(0xd800dc00);
const __m512i v_0f = _mm512_set1_epi8(0x0f);
const __m512i v_10 = _mm512_set1_epi8(0x10);
const __m512i v_40 = _mm512_set1_epi8(0x40);
const __m512i v_7f = _mm512_set1_epi8(char(0x7f));
const __m512i v_80 = _mm512_set1_epi8(char(0x80));
const __m512i v_1f = _mm512_set1_epi8(0x1f);
const __m512i v_3f = _mm512_set1_epi8(0x3f);
const __m512i v_c0 = _mm512_set1_epi8(char(0xc0));
const __m512i v_c2 = _mm512_set1_epi8(char(0xc2));
const __m512i v_e0 = _mm512_set1_epi8(char(0xe0));
const __m512i v_f0 = _mm512_set1_epi8(char(0xf0));
const __m512i v_f8 = _mm512_set1_epi8(char(0xf8));
const __m512i v_neg65 = _mm512_set1_epi8(-65);

const __m512i broadcast_0th_lane = _mm512_setr_epi32(
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3
);
