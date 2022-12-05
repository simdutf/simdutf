
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

#pragma once

#ifndef ZTD_IDK_DETAIL_BIT_INTRINSIC_GENERIC_H
#define ZTD_IDK_DETAIL_BIT_INTRINSIC_GENERIC_H

#include <ztd/idk/version.h>

#include <ztd/idk/detail/bit.intrinsic.h>
#include <ztd/idk/detail/bit.intrinsic.impl.h>

#if ZTD_IS_ON(ZTD_C)

#if defined(_ZTDC_COUNT_ONES_GENERIC_I_)
#define _ZTDC_COUNT_ONES_I_(...) _ZTDC_COUNT_ONES_GENERIC_I_(__VA_ARGS__)
#else
#define _ZTDC_COUNT_ONES_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                              \
	          : ztdc_count_onesuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_count_onesuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_count_onesus(__VA_ARGS__), unsigned int                   \
	          : ztdc_count_onesui(__VA_ARGS__), unsigned long                  \
	          : ztdc_count_onesul(__VA_ARGS__), unsigned long long             \
	          : ztdc_count_onesull(__VA_ARGS__)))
#endif

#define _ZTDC_COUNT_ZEROS_I_(...) ((sizeof(__VA_ARGS__) * CHAR_BIT) - ztdc_count_ones(__VA_ARGS__))

#define _ZTDC_COUNT_LEADING_ZEROS_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                       \
	          : ztdc_count_leading_zerosuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_count_leading_zerosuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_count_leading_zerosus(__VA_ARGS__), unsigned int                   \
	          : ztdc_count_leading_zerosui(__VA_ARGS__), unsigned long                  \
	          : ztdc_count_leading_zerosul(__VA_ARGS__), unsigned long long             \
	          : ztdc_count_leading_zerosull(__VA_ARGS__)))

#define _ZTDC_COUNT_TRAILING_ZEROS_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                        \
	          : ztdc_count_trailing_zerosuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_count_trailing_zerosuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_count_trailing_zerosus(__VA_ARGS__), unsigned int                   \
	          : ztdc_count_trailing_zerosui(__VA_ARGS__), unsigned long                  \
	          : ztdc_count_trailing_zerosul(__VA_ARGS__), unsigned long long             \
	          : ztdc_count_trailing_zerosull(__VA_ARGS__)))

#define _ZTDC_COUNT_LEADING_ONES_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                      \
	          : ztdc_count_leading_onesuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_count_leading_onesuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_count_leading_onesus(__VA_ARGS__), unsigned int                   \
	          : ztdc_count_leading_onesui(__VA_ARGS__), unsigned long                  \
	          : ztdc_count_leading_onesul(__VA_ARGS__), unsigned long long             \
	          : ztdc_count_leading_onesull(__VA_ARGS__)))

#define _ZTDC_COUNT_TRAILING_ONES_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                       \
	          : ztdc_count_trailing_onesuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_count_trailing_onesuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_count_trailing_onesus(__VA_ARGS__), unsigned int                   \
	          : ztdc_count_trailing_onesui(__VA_ARGS__), unsigned long                  \
	          : ztdc_count_trailing_onesul(__VA_ARGS__), unsigned long long             \
	          : ztdc_count_trailing_onesull(__VA_ARGS__)))

#define _ZTDC_FIRST_LEADING_ZERO_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                      \
	          : ztdc_first_leading_zerouc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_first_leading_zerouc(__VA_ARGS__), unsigned short                 \
	          : ztdc_first_leading_zerous(__VA_ARGS__), unsigned int                   \
	          : ztdc_first_leading_zeroui(__VA_ARGS__), unsigned long                  \
	          : ztdc_first_leading_zeroul(__VA_ARGS__), unsigned long long             \
	          : ztdc_first_leading_zeroull(__VA_ARGS__)))

#define _ZTDC_FIRST_TRAILING_ZERO_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                       \
	          : ztdc_first_trailing_zerouc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_first_trailing_zerouc(__VA_ARGS__), unsigned short                 \
	          : ztdc_first_trailing_zerous(__VA_ARGS__), unsigned int                   \
	          : ztdc_first_trailing_zeroui(__VA_ARGS__), unsigned long                  \
	          : ztdc_first_trailing_zeroul(__VA_ARGS__), unsigned long long             \
	          : ztdc_first_trailing_zeroull(__VA_ARGS__)))

#define _ZTDC_FIRST_LEADING_ONE_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                     \
	          : ztdc_first_leading_oneuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_first_leading_oneuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_first_leading_oneus(__VA_ARGS__), unsigned int                   \
	          : ztdc_first_leading_oneui(__VA_ARGS__), unsigned long                  \
	          : ztdc_first_leading_oneul(__VA_ARGS__), unsigned long long             \
	          : ztdc_first_leading_oneull(__VA_ARGS__)))

#define _ZTDC_FIRST_TRAILING_ONE_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                      \
	          : ztdc_first_trailing_oneuc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_first_trailing_oneuc(__VA_ARGS__), unsigned short                 \
	          : ztdc_first_trailing_oneus(__VA_ARGS__), unsigned int                   \
	          : ztdc_first_trailing_oneui(__VA_ARGS__), unsigned long                  \
	          : ztdc_first_trailing_oneul(__VA_ARGS__), unsigned long long             \
	          : ztdc_first_trailing_oneull(__VA_ARGS__)))

#define _ZTDC_ROTATE_LEFT_I_(_VALUE, ...)                                                \
	(_Generic((_VALUE), char                                                            \
	          : ztdc_rotate_leftuc((unsigned char)(_VALUE), __VA_ARGS__), unsigned char \
	          : ztdc_rotate_leftuc(_VALUE, __VA_ARGS__), unsigned short                 \
	          : ztdc_rotate_leftus(_VALUE, __VA_ARGS__), unsigned int                   \
	          : ztdc_rotate_leftui(_VALUE, __VA_ARGS__), unsigned long                  \
	          : ztdc_rotate_leftul(_VALUE, __VA_ARGS__), unsigned long long             \
	          : ztdc_rotate_leftull(_VALUE, __VA_ARGS__)))

#define _ZTDC_ROTATE_RIGHT_I_(_VALUE, ...)                                                \
	(_Generic((_VALUE), char                                                             \
	          : ztdc_rotate_rightuc((unsigned char)(_VALUE), __VA_ARGS__), unsigned char \
	          : ztdc_rotate_rightuc(_VALUE, __VA_ARGS__), unsigned short                 \
	          : ztdc_rotate_rightus(_VALUE, __VA_ARGS__), unsigned int                   \
	          : ztdc_rotate_rightui(_VALUE, __VA_ARGS__), unsigned long                  \
	          : ztdc_rotate_rightul(_VALUE, __VA_ARGS__), unsigned long long             \
	          : ztdc_rotate_rightull(_VALUE, __VA_ARGS__)))

#define _ZTDC_HAS_SINGLE_BIT_I_(...)                                                \
	(_Generic((__VA_ARGS__), char                                                  \
	          : ztdc_has_single_bituc((unsigned char)(__VA_ARGS__)), unsigned char \
	          : ztdc_has_single_bituc(__VA_ARGS__), unsigned short                 \
	          : ztdc_has_single_bitus(__VA_ARGS__), unsigned int                   \
	          : ztdc_has_single_bitui(__VA_ARGS__), unsigned long                  \
	          : ztdc_has_single_bitul(__VA_ARGS__), unsigned long long             \
	          : ztdc_has_single_bitull(__VA_ARGS__)))

#define _ZTDC_BIT_WIDTH_I_(...)                                                                                    \
	_Generic((__VA_ARGS__), char                                                                                  \
	         : ((sizeof(unsigned char) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))), unsigned char      \
	         : ((sizeof(unsigned char) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))), unsigned short     \
	         : ((sizeof(unsigned short) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))), unsigned int      \
	         : ((sizeof(unsigned int) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))), unsigned long       \
	         : ((sizeof(unsigned long) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))), unsigned long long \
	         : ((sizeof(unsigned long long) * CHAR_BIT) - ztdc_count_leading_zeros((__VA_ARGS__))))

#define _ZTDC_BIT_CEIL_I_(...)                                   \
	_Generic((__VA_ARGS__), char                                \
	         : ztdc_bit_ceiluc(__VA_ARGS__), unsigned char      \
	         : ztdc_bit_ceiluc(__VA_ARGS__), unsigned short     \
	         : ztdc_bit_ceilus(__VA_ARGS__), unsigned int       \
	         : ztdc_bit_ceilui(__VA_ARGS__), unsigned long      \
	         : ztdc_bit_ceilul(__VA_ARGS__), unsigned long long \
	         : ztdc_bit_ceilull(__VA_ARGS__))

#define _ZTDC_BIT_FLOOR_I_(...)                                   \
	_Generic((__VA_ARGS__), char                                 \
	         : ztdc_bit_flooruc(__VA_ARGS__), unsigned char      \
	         : ztdc_bit_flooruc(__VA_ARGS__), unsigned short     \
	         : ztdc_bit_floorus(__VA_ARGS__), unsigned int       \
	         : ztdc_bit_floorui(__VA_ARGS__), unsigned long      \
	         : ztdc_bit_floorul(__VA_ARGS__), unsigned long long \
	         : ztdc_bit_floorull(__VA_ARGS__))

#else

#define _ZTDC_COUNT_ONES_I_(...) ::ztd::__idk_detail::__count_ones(__VA_ARGS__)

#define _ZTDC_COUNT_ZEROS_I_(...) ::ztd::__idk_detail::__count_zeros(__VA_ARGS__)

#define _ZTDC_COUNT_LEADING_ZEROS_I_(...) ::ztd::__idk_detail::__count_leading_zeros(__VA_ARGS__)

#define _ZTDC_COUNT_TRAILING_ZEROS_I_(...) ::ztd::__idk_detail::__count_trailing_zeros(__VA_ARGS__)

#define _ZTDC_COUNT_LEADING_ONES_I_(...) ::ztd::__idk_detail::__count_leading_ones(__VA_ARGS__)

#define _ZTDC_COUNT_TRAILING_ONES_I_(...) ::ztd::__idk_detail::__count_trailing_ones(__VA_ARGS__)

#define _ZTDC_FIRST_LEADING_ZERO_I_(...) ::ztd::__idk_detail::__first_leading_zero(__VA_ARGS__)

#define _ZTDC_FIRST_TRAILING_ZERO_I_(...) ::ztd::__idk_detail::__first_trailing_zero(__VA_ARGS__)

#define _ZTDC_FIRST_LEADING_ONE_I_(...) ::ztd::__idk_detail::__first_leading_one(__VA_ARGS__)

#define _ZTDC_FIRST_TRAILING_ONE_I_(...) ::ztd::__idk_detail::__first_trailing_one(__VA_ARGS__)

#define _ZTDC_ROTATE_LEFT_I_(...) ::ztd::__idk_detail::__rotate_left(__VA_ARGS__)

#define _ZTDC_ROTATE_RIGHT_I_(...) ::ztd::__idk_detail::__rotate_right(__VA_ARGS__)

#define _ZTDC_HAS_SINGLE_BIT_I_(...) ::ztd::__idk_detail::__single_bit(__VA_ARGS__)

#define _ZTDC_BIT_WIDTH_I_(...) ::ztd::__idk_detail::__bit_width(__VA_ARGS__)

#define _ZTDC_BIT_CEIL_I_(...) ::ztd::__idk_detail::__bit_ceil(__VA_ARGS__)

#define _ZTDC_BIT_FLOOR_I_(...) ::ztd::__idk_detail::__bit_floor(__VA_ARGS__)

#endif // C vs. C++ only

#endif
