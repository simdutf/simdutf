// =============================================================================
//
// ztd.idk
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.idk licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================ //

#include <ztd/idk/version.h>

#include <ztd/idk/bit.h>

#include <ztd/idk/endian.h>
#include <ztd/idk/static_assert.h>
#include <ztd/idk/assume_aligned.hpp>
#include <ztd/idk/detail/bit.intrinsic.impl.h>

#if ZTD_IS_ON(ZTD_C)
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#else
#include <cstring>
#include <cstddef>
#include <cstdint>
#endif

#if ZTD_IS_ON(ZTD_BUILTIN_POPCOUNT)
#define _ZTDC_COUNT_ONES_BODY_I_(_TYPE, _VALUE) return _ZTDC_COUNT_ONES_GENERIC_I_(_VALUE)
#else
#define _ZTDC_COUNT_ONES_BODY_I_(_TYPE, _VALUE)                                             \
	int __num = 0;                                                                         \
	for (size_t __bit_index = (sizeof((_VALUE)) * CHAR_BIT); __bit_index-- > 0;) {         \
		_Bool __is_set = ((_VALUE & (_TYPE)((_TYPE)(1) << __bit_index)) != ((_TYPE)(0))); \
		if (__is_set) {                                                                   \
			__num += 1;                                                                  \
		}                                                                                 \
	}                                                                                      \
	return __num
#endif

int ztdc_count_onesuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_ONES_BODY_I_(unsigned long long, __value);
}
int ztdc_count_onesus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_ONES_BODY_I_(unsigned long long, __value);
}
int ztdc_count_onesui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_ONES_BODY_I_(unsigned long long, __value);
}
int ztdc_count_onesul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_ONES_BODY_I_(unsigned long long, __value);
}
int ztdc_count_onesull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_ONES_BODY_I_(unsigned long long, __value);
}

#undef _ZTDC_COUNT_ONES_BODY_I_

int ztdc_count_zerosuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_zeros(__value);
}
int ztdc_count_zerosus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_zeros(__value);
}
int ztdc_count_zerosui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_zeros(__value);
}
int ztdc_count_zerosul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_zeros(__value);
}
int ztdc_count_zerosull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_zeros(__value);
}

bool ztdc_has_single_bituc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_ones(__value) == 1;
}
bool ztdc_has_single_bitus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_ones(__value) == 1;
}
bool ztdc_has_single_bitui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_ones(__value) == 1;
}
bool ztdc_has_single_bitul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_ones(__value) == 1;
}
bool ztdc_has_single_bitull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_ones(__value) == 1;
}

#if ZTD_IS_ON(ZTD_BUILTIN_CLZ)
#define _ZTDC_COUNT_LEADING_ZEROES_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	if (_VALUE == 0)                                                                           \
		return sizeof(_VALUE) * CHAR_BIT;                                                     \
	return __builtin_clz##_BUILTIN_SUFFIX(_VALUE) - _BUILTIN_ADJUSTMENT
#else
#define _ZTDC_COUNT_LEADING_ZEROES_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	int __num = 0;                                                                             \
	for (size_t __bit_index = (sizeof((_VALUE)) * CHAR_BIT); __bit_index-- > 0;) {             \
		bool __not_is_set = (_VALUE & (_TYPE)((_TYPE)(1) << __bit_index)) == (_TYPE)(0);      \
		if (__not_is_set) {                                                                   \
			__num += 1;                                                                      \
		}                                                                                     \
		else {                                                                                \
			break;                                                                           \
		}                                                                                     \
	}                                                                                          \
	return __num
#endif

#if ZTD_IS_ON(ZTD_BUILTIN_CTZ)
#define _ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	if (_VALUE == 0)                                                                            \
		return sizeof(_VALUE) * CHAR_BIT;                                                      \
	return __builtin_ctz##_BUILTIN_SUFFIX(_VALUE) - _BUILTIN_ADJUSTMENT
#else
#define _ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	int __num = 0;                                                                              \
	for (size_t __bit_index = 0; __bit_index < (sizeof((_VALUE)) * CHAR_BIT); ++__bit_index) {  \
		bool __not_is_set = (_VALUE & (_TYPE)((_TYPE)(1) << __bit_index)) == (_TYPE)(0);       \
		if (__not_is_set) {                                                                    \
			__num += 1;                                                                       \
		}                                                                                      \
		else {                                                                                 \
			break;                                                                            \
		}                                                                                      \
	}                                                                                           \
	return __num
#endif

int ztdc_count_leading_zerosuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_LEADING_ZEROES_BODY_I_(
	     unsigned char, __value, , ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(unsigned char) * CHAR_BIT)));
}
int ztdc_count_leading_zerosus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_LEADING_ZEROES_BODY_I_(
	     unsigned short, __value, , ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(unsigned short) * CHAR_BIT)));
}
int ztdc_count_leading_zerosui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_LEADING_ZEROES_BODY_I_(unsigned int, __value, , 0);
}
int ztdc_count_leading_zerosul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_LEADING_ZEROES_BODY_I_(unsigned long, __value, l, 0);
}
int ztdc_count_leading_zerosull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_LEADING_ZEROES_BODY_I_(unsigned long long, __value, ll, 0);
}

int ztdc_count_trailing_zerosuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(unsigned char, __value, , 0);
}
int ztdc_count_trailing_zerosus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(unsigned short, __value, , 0);
}
int ztdc_count_trailing_zerosui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(unsigned int, __value, , 0);
}
int ztdc_count_trailing_zerosul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(unsigned long, __value, l, 0);
}
int ztdc_count_trailing_zerosull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_COUNT_TRAILING_ZEROES_BODY_I_(unsigned long long, __value, ll, 0);
}

#undef _ZTDC_COUNT_LEADING_ZEROES_BODY_I_
#undef _ZTDC_COUNT_TRAILING_ZEROES_BODY_I_

int ztdc_count_leading_onesuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_leading_zeros((unsigned char)~__value);
}
int ztdc_count_leading_onesus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_leading_zeros((unsigned short)~__value);
}
int ztdc_count_leading_onesui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_leading_zeros((unsigned int)~__value);
}
int ztdc_count_leading_onesul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_leading_zeros((unsigned long)~__value);
}
int ztdc_count_leading_onesull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_leading_zeros((unsigned long long)~__value);
}

int ztdc_count_trailing_onesuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_trailing_zerosuc((unsigned char)~__value);
}
int ztdc_count_trailing_onesus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_trailing_zerosus((unsigned short)~__value);
}
int ztdc_count_trailing_onesui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_trailing_zerosui((unsigned int)~__value);
}
int ztdc_count_trailing_onesul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_trailing_zerosul((unsigned long)~__value);
}
int ztdc_count_trailing_onesull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_count_trailing_zerosull((unsigned long long)~__value);
}

#define _ZTDC_ROTATE_LEFT_BODY_I_()                              \
	if (__count == 0) {                                         \
		return __value;                                        \
	}                                                           \
	const unsigned int __width    = sizeof(__value) * CHAR_BIT; \
	const unsigned int __rotation = __count % __width;          \
	return (__value << __rotation) | (__value >> (__width - __rotation))

#define _ZTDC_ROTATE_RIGHT_BODY_I_()                             \
	if (__count == 0) {                                         \
		return __value;                                        \
	}                                                           \
	const unsigned int __width    = sizeof(__value) * CHAR_BIT; \
	const unsigned int __rotation = __count % __width;          \
	return (__value >> __rotation) | (__value << (__width - __rotation))

unsigned char ztdc_rotate_leftuc(unsigned char __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_LEFT_BODY_I_();
}
unsigned short ztdc_rotate_leftus(unsigned short __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_LEFT_BODY_I_();
}
unsigned int ztdc_rotate_leftui(unsigned int __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_LEFT_BODY_I_();
}
unsigned long ztdc_rotate_leftul(unsigned long __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_LEFT_BODY_I_();
}
unsigned long long ztdc_rotate_leftull(unsigned long long __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_LEFT_BODY_I_();
}

unsigned char ztdc_rotate_rightuc(unsigned char __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_RIGHT_BODY_I_();
}
unsigned short ztdc_rotate_rightus(unsigned short __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_RIGHT_BODY_I_();
}
unsigned int ztdc_rotate_rightui(unsigned int __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_RIGHT_BODY_I_();
}
unsigned long ztdc_rotate_rightul(unsigned long __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_RIGHT_BODY_I_();
}
unsigned long long ztdc_rotate_rightull(unsigned long long __value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_ {
	_ZTDC_ROTATE_RIGHT_BODY_I_();
}

#undef _ZTDC_ROTATE_LEFT_BODY_I_
#undef _ZTDC_ROTATE_RIGHT_BODY_I_

#define _ZTDC_BIT_FLOOR_BODY_I_(_TYPE, ...) \
	((__VA_ARGS__ == (_TYPE)0) ? (_TYPE)0 : (((_TYPE)1) << (ztdc_bit_width(__VA_ARGS__) - 1)));

unsigned char ztdc_bit_flooruc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_FLOOR_BODY_I_(unsigned char, __value);
}
unsigned short ztdc_bit_floorus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_FLOOR_BODY_I_(unsigned short, __value);
}
unsigned int ztdc_bit_floorui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_FLOOR_BODY_I_(unsigned int, __value);
}
unsigned long ztdc_bit_floorul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_FLOOR_BODY_I_(unsigned long, __value);
}
unsigned long long ztdc_bit_floorull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_FLOOR_BODY_I_(unsigned long long, __value);
}

#undef _ZTDC_BIT_FLOOR_BODY_I_

#if ZTD_IS_ON(ZTD_COMPILER_GCC) || ZTD_IS_ON(ZTD_COMPILER_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#elif ZTD_IS_ON(ZTD_COMPILER_VCXX)
#pragma warning(push)
#pragma warning(disable : 4333 4293)
#endif

#define _ZTDC_BIT_CEIL_BODY_I_(_TYPE, _Value) \
	((_Value <= (_TYPE)1) ? ((_TYPE)1) : ((_TYPE)(1u << (ztdc_bit_width((_TYPE)((_Value)-1))))))

#define _ZTDC_BIT_CEIL_BODY_PROMOTED_I_(_TYPE, _Value)                                                          \
	((_Value <= (_TYPE)1) ? (_TYPE)1                                                                           \
	                      : ((_TYPE)(1u << (ztdc_bit_width((_TYPE)((_Value)-1))                                \
	                                      + ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(_TYPE) * CHAR_BIT)))) \
	                           >> ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(_TYPE) * CHAR_BIT))))

// integer promotion rules means we need to
// precisely calculate the bit ceiling here >___>
#define _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(_TYPE, ...)     \
	_Generic((+(_TYPE)0), _TYPE                                    \
	         : _ZTDC_BIT_CEIL_BODY_I_(_TYPE, __VA_ARGS__), default \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTED_I_(_TYPE, __VA_ARGS__))

#define _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(...)                                                           \
	_Generic((__VA_ARGS__), char                                                                           \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned char, __VA_ARGS__), unsigned char      \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned char, __VA_ARGS__), unsigned short     \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned short, __VA_ARGS__), unsigned int      \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned int, __VA_ARGS__), unsigned long       \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned long, __VA_ARGS__), unsigned long long \
	         : _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_(unsigned long long, __VA_ARGS__))

unsigned char ztdc_bit_ceiluc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(__value);
}
unsigned short ztdc_bit_ceilus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(__value);
}
unsigned int ztdc_bit_ceilui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(__value);
}
unsigned long ztdc_bit_ceilul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(__value);
}
unsigned long long ztdc_bit_ceilull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_(__value);
}

#undef _ZTDC_BIT_CEIL_BODY_GENERIC_SUCKS_I_
#undef _ZTDC_BIT_CEIL_BODY_PROMOTION_PROTECTION_I_
#undef _ZTDC_BIT_CEIL_BODY_PROMOTED_I_
#undef _ZTDC_BIT_CEIL_BODY_I_

#if ZTD_IS_ON(ZTD_COMPILER_GCC) || ZTD_IS_ON(ZTD_COMPILER_CLANG)
#pragma GCC diagnostic pop
#elif ZTD_IS_ON(ZTD_COMPILER_VCXX)
#pragma warning(pop)
#endif

unsigned char ztdc_bit_widthuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_bit_width(__value);
}
unsigned short ztdc_bit_widthus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_bit_width(__value);
}
unsigned int ztdc_bit_widthui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_bit_width(__value);
}
unsigned long ztdc_bit_widthul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_bit_width(__value);
}
unsigned long long ztdc_bit_widthull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_bit_width(__value);
}

#if ZTD_IS_ON(ZTD_BUILTIN_CLZ)
#define _ZTD_FIRST_LEADING_ONE_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	if (_VALUE == 0) {                                                                     \
		return 0;                                                                         \
	}                                                                                      \
	return __builtin_clz##_BUILTIN_SUFFIX(_VALUE) + 1 - (_BUILTIN_ADJUSTMENT);
#else
#define _ZTD_FIRST_LEADING_ONE_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX, _BUILTIN_ADJUSTMENT) \
	for (size_t __bit_index = (sizeof(_VALUE) * CHAR_BIT); __bit_index-- > 0;) {           \
		bool __is_set = (_VALUE & (_TYPE)(((_TYPE)1) << __bit_index)) != ((_TYPE)0);      \
		if (__is_set) {                                                                   \
			return (int)((sizeof(_VALUE) * CHAR_BIT) - __bit_index);                     \
		}                                                                                 \
	}                                                                                      \
	return 0
#endif

int ztdc_first_leading_oneuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_LEADING_ONE_BODY_I_(
	     unsigned char, __value, , ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(unsigned char) * CHAR_BIT)));
}
int ztdc_first_leading_oneus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_LEADING_ONE_BODY_I_(
	     unsigned short, __value, , ((sizeof(unsigned int) * CHAR_BIT) - (sizeof(unsigned short) * CHAR_BIT)));
}
int ztdc_first_leading_oneui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_LEADING_ONE_BODY_I_(unsigned int, __value, , 0);
}
int ztdc_first_leading_oneul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_LEADING_ONE_BODY_I_(unsigned long, __value, l, 0);
}
int ztdc_first_leading_oneull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_LEADING_ONE_BODY_I_(unsigned long long, __value, ll, 0);
}

#undef _ZTD_FIRST_LEADING_ONE_BODY_I_

#if ZTD_IS_ON(ZTD_BUILTIN_FFS)
#define _ZTD_FIRST_TRAILING_ONE_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX) return __builtin_ffs##_BUILTIN_SUFFIX(_VALUE)
#else
#define _ZTD_FIRST_TRAILING_ONE_BODY_I_(_TYPE, _VALUE, _BUILTIN_SUFFIX)                       \
	for (size_t __bit_index = 0; __bit_index < (sizeof(_VALUE) * CHAR_BIT); ++__bit_index) { \
		bool __is_set = (_VALUE & (_TYPE)(((_TYPE)1) << __bit_index)) != ((_TYPE)0);        \
		if (__is_set) {                                                                     \
			return (int)(__bit_index + 1);                                                 \
		}                                                                                   \
	}                                                                                        \
	return 0
#endif

int ztdc_first_trailing_oneuc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_TRAILING_ONE_BODY_I_(unsigned char, __value, );
}
int ztdc_first_trailing_oneus(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_TRAILING_ONE_BODY_I_(unsigned short, __value, );
}
int ztdc_first_trailing_oneui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_TRAILING_ONE_BODY_I_(unsigned int, __value, );
}
int ztdc_first_trailing_oneul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_TRAILING_ONE_BODY_I_(unsigned long, __value, l);
}
int ztdc_first_trailing_oneull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	_ZTD_FIRST_TRAILING_ONE_BODY_I_(unsigned long long, __value, ll);
}

#undef _ZTD_FIRST_TRAILING_ONE_BODY_I_

int ztdc_first_leading_zerouc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_leading_oneuc((unsigned char)~__value);
}
int ztdc_first_leading_zerous(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_leading_oneus((unsigned short)~__value);
}
int ztdc_first_leading_zeroui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_leading_oneui((unsigned int)~__value);
}
int ztdc_first_leading_zeroul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_leading_oneul((unsigned long)~__value);
}
int ztdc_first_leading_zeroull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_leading_oneull((unsigned long long)~__value);
}

int ztdc_first_trailing_zerouc(unsigned char __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_trailing_oneuc((unsigned char)~__value);
}
int ztdc_first_trailing_zerous(unsigned short __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_trailing_oneus((unsigned short)~__value);
}
int ztdc_first_trailing_zeroui(unsigned int __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_trailing_oneui((unsigned int)~__value);
}
int ztdc_first_trailing_zeroul(unsigned long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_trailing_oneul((unsigned long)~__value);
}
int ztdc_first_trailing_zeroull(unsigned long long __value) ZTD_CXX_NOEXCEPT_I_ {
	return ztdc_first_trailing_oneull((unsigned long long)~__value);
}
