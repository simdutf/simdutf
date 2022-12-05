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

#ifndef ZTD_IDK_DETAIL_BIT_INTRINSIC_IMPL_H
#define ZTD_IDK_DETAIL_BIT_INTRINSIC_IMPL_H

#include <ztd/idk/version.h>

#if ZTD_IS_ON(ZTD_C)
// clang-format off
	#if ZTD_IS_ON(ZTD_BUILTIN_POPCOUNT)
		#define _ZTDC_COUNT_ONES_GENERIC_I_(...)             \
			_Generic((__VA_ARGS__), char                                   \
			                : __builtin_popcount, unsigned char       \
			                : __builtin_popcount, unsigned short      \
			                : __builtin_popcount, unsigned int        \
			                : __builtin_popcount, unsigned long       \
			                : __builtin_popcountl, unsigned long long \
			                : __builtin_popcountll)(__VA_ARGS__)
	#endif
// clang-format on
#endif

#if ZTD_IS_ON(ZTD_CXX)
#include <ztd/idk/type_traits.hpp>

#include <limits>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __idk_detail {
		template <typename _Value>
		int __count_ones(_Value __value) noexcept {
#if ZTD_IS_ON(ZTD_BUILTIN_POPCOUNT)
			if constexpr (::std::is_unsigned_v<_Value>) {
				if constexpr (::std::numeric_limits<_Value>::digits <= (sizeof(unsigned int) * CHAR_BIT)) {
					return __builtin_popcount(__value);
				}
				else if constexpr (::std::numeric_limits<_Value>::digits <= (sizeof(unsigned long) * CHAR_BIT)) {
					return __builtin_popcountl(__value);
				}
				else if constexpr (::std::numeric_limits<_Value>::digits
				     <= (sizeof(unsigned long long) * CHAR_BIT)) {
					return __builtin_popcountll(__value);
				}
				else {
					static_assert(::ztd::always_false_v<_Value>,
					     "The given unsigned integer type is too large to be used with the given intrinsics and "
					     "an implementation has not yet been provided. File an issue!");
				}
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_ones");
			}
#else
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_count_onesuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_count_onesus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_count_onesui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_count_onesul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_count_onesull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_ones");
			}
#endif
		}

		template <typename _Value>
		int __count_zeros(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char> ||      // cf
			     ::std::is_same_v<_Value, unsigned short> || ::std::is_same_v<_Value, unsigned int> ||      // cf
			     ::std::is_same_v<_Value, unsigned long> || ::std::is_same_v<_Value, unsigned long long>) { // cf
				return ((sizeof(_Value) * CHAR_BIT) - __count_ones(__value));
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_zeros");
			}
		}

		template <typename _Value>
		int __count_leading_zeros(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_count_leading_zerosuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_count_leading_zerosus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_count_leading_zerosui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_count_leading_zerosul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_count_leading_zerosull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_leading_zeros");
			}
		}

		template <typename _Value>
		int __count_trailing_zeros(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_count_trailing_zerosuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_count_trailing_zerosus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_count_trailing_zerosui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_count_trailing_zerosul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_count_trailing_zerosull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_trailing_zeros");
			}
		}

		template <typename _Value>
		int __count_leading_ones(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_count_leading_onesuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_count_leading_onesus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_count_leading_onesui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_count_leading_onesul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_count_leading_onesull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_leading_ones");
			}
		}

		template <typename _Value>
		int __count_trailing_ones(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_count_trailing_onesuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_count_trailing_onesus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_count_trailing_onesui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_count_trailing_onesul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_count_trailing_onesull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_count_trailing_ones");
			}
		}

		template <typename _Value>
		int __first_leading_zero(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_first_leading_zerouc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_first_leading_zerous(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_first_leading_zeroui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_first_leading_zeroul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_first_leading_zeroull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_first_leading_zero");
			}
		}

		template <typename _Value>
		int __first_trailing_zero(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_first_trailing_zerouc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_first_trailing_zerous(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_first_trailing_zeroui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_first_trailing_zeroul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_first_trailing_zeroull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_first_trailing_zero");
			}
		}

		template <typename _Value>
		int __first_leading_one(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_first_leading_oneuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_first_leading_oneus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_first_leading_oneui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_first_leading_oneul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_first_leading_oneull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_first_leading_one");
			}
		}

		template <typename _Value>
		int __first_trailing_one(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_first_trailing_oneuc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_first_trailing_oneus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_first_trailing_oneui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_first_trailing_oneul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_first_trailing_oneull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_first_trailing_one");
			}
		}

		template <typename _Value>
		int __rotate_left(_Value __value, int __rotation) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_rotate_leftuc(static_cast<unsigned char>(__value), __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_rotate_leftus(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_rotate_leftui(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_rotate_leftul(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_rotate_leftull(__value, __rotation);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_rotate_left as the value");
			}
		}

		template <typename _Value>
		int __rotate_right(_Value __value, int __rotation) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_rotate_rightuc(static_cast<unsigned char>(__value), __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_rotate_rightus(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_rotate_rightui(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_rotate_rightul(__value, __rotation);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_rotate_rightull(__value, __rotation);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_rotate_right as the value");
			}
		}

		template <typename _Value>
		int __has_single_bit(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_has_single_bituc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_has_single_bitus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_has_single_bitui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_has_single_bitul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_has_single_bitull(__value);
			}
			else {
				static_assert(::ztd::always_false_v<_Value>,
				     "Only an unsigned integer type can be given to ztdc_has_single_bit");
			}
		}

		template <typename _Value>
		int __bit_width(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char> ||      // cf
			     ::std::is_same_v<_Value, unsigned short> || ::std::is_same_v<_Value, unsigned int> ||      // cf
			     ::std::is_same_v<_Value, unsigned long> || ::std::is_same_v<_Value, unsigned long long>) { // cf
				return ((sizeof(__value) * CHAR_BIT) - ztdc_count_leading_zeros(__value));
			}
			else {
				static_assert(
				     ::ztd::always_false_v<_Value>, "Only an unsigned integer type can be given to ztdc_bit_width");
			}
		}

		template <typename _Value>
		int __bit_ceil(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_bit_ceiluc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_bit_ceilus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_bit_ceilui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_bit_ceilul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_bit_ceilull(__value);
			}
			else {
				static_assert(
				     ::ztd::always_false_v<_Value>, "Only an unsigned integer type can be given to ztdc_bit_ceil");
			}
		}

		template <typename _Value>
		int __bit_floor(_Value __value) noexcept {
			if constexpr (::std::is_same_v<_Value, char> || ::std::is_same_v<_Value, unsigned char>) {
				return ztdc_bit_flooruc(static_cast<unsigned char>(__value));
			}
			else if constexpr (::std::is_same_v<_Value, unsigned short>) {
				return ztdc_bit_floorus(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned int>) {
				return ztdc_bit_floorui(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long>) {
				return ztdc_bit_floorul(__value);
			}
			else if constexpr (::std::is_same_v<_Value, unsigned long long>) {
				return ztdc_bit_floorull(__value);
			}
			else {
				static_assert(
				     ::ztd::always_false_v<_Value>, "Only an unsigned integer type can be given to ztdc_bit_floor");
			}
		}
	} // namespace __idk_detail
	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd
#endif

// clang-format on

#endif
