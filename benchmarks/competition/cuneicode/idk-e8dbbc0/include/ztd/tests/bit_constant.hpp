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

#ifndef ZTD_IDK_TESTS_BIT_CONSTANT_HPP
#define ZTD_IDK_TESTS_BIT_CONSTANT_HPP

#include <ztd/version.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/size.hpp>

#include <memory>
#include <cstddef>
#include <cstring>

namespace ztd { namespace tests {

	namespace __tests_detail {
		inline constexpr const unsigned char __distinct_bit_constant_source_bytes[] = {
#if ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN
			0xFF, 0xDD, 0xBB, 0x99, 0x77, 0x55, 0x33, 0x11, 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x10
#else
			0x10, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB, 0xDD, 0xFF
#endif
		};
		inline constexpr const unsigned char __distinct_bit_constant_source_bytes_reverse[] = {
#if ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN
			0x10, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB, 0xDD, 0xFF
#else
			0xFF, 0xDD, 0xBB, 0x99, 0x77, 0x55, 0x33, 0x11, 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x10
#endif
		};
	} // namespace __tests_detail

	template <typename _Type>
	_Type get_distinct_bit_constant_positive() noexcept {
		constexpr std::size_t _Nbytes = sizeof(_Type) * CHAR_BIT;
		if constexpr (_Nbytes == 8) {
			return static_cast<_Type>(0x10);
		}
		else if constexpr (_Nbytes == 16) {
			return static_cast<_Type>(0x1023);
		}
		else if constexpr (_Nbytes == 24) {
			return static_cast<_Type>(0x102345);
		}
		else if constexpr (_Nbytes == 32) {
			return static_cast<_Type>(0x10234567);
		}
		else if constexpr (_Nbytes == 40) {
			return static_cast<_Type>(0x1023456789);
		}
		else if constexpr (_Nbytes == 48) {
			return static_cast<_Type>(0x1023456789AB);
		}
		else if constexpr (_Nbytes == 56) {
			return static_cast<_Type>(0x1023456789ABCD);
		}
		else if constexpr (_Nbytes == 64) {
			return static_cast<_Type>(0x1023456789ABCDEF);
		}
		else {
			constexpr const auto& __source_bytes = __tests_detail::__distinct_bit_constant_source_bytes;
			static_assert(ztd::always_true_v<_Type> && CHAR_BIT == 8,
				"this branch can only be used on CHAR_BIT == 8 machines");
			static_assert(
				_Nbytes <= (sizeof(__source_bytes) * CHAR_BIT), "unusable (too large) bit size for the given type");

			_Type __value;
			::std::memcpy(::std::addressof(__value), __source_bytes, ztd_c_array_size(__source_bytes));
			return __value;
		}
	}

	template <typename _Type>
	_Type get_distinct_bit_constant_negative() noexcept {
		constexpr std::size_t _Nbytes = sizeof(_Type) * CHAR_BIT;
		if constexpr (::std::is_signed_v<_Type>) {
			if constexpr (_Nbytes == 8) {
				return -static_cast<_Type>(0x10);
			}
			else if constexpr (_Nbytes == 16) {
				return -static_cast<_Type>(0x1023);
			}
			else if constexpr (_Nbytes == 24) {
				return -static_cast<_Type>(0x102345);
			}
			else if constexpr (_Nbytes == 32) {
				return -static_cast<_Type>(0x10234567);
			}
			else if constexpr (_Nbytes == 40) {
				return -static_cast<_Type>(0x1023456789);
			}
			else if constexpr (_Nbytes == 48) {
				return -static_cast<_Type>(0x1023456789AB);
			}
			else if constexpr (_Nbytes == 56) {
				return -static_cast<_Type>(0x1023456789ABCD);
			}
			else if constexpr (_Nbytes == 64) {
				return -static_cast<_Type>(0x1023456789ABCDEF);
			}
			else {
				constexpr const auto& __source_bytes = __tests_detail::__distinct_bit_constant_source_bytes;
				static_assert(ztd::always_true_v<_Type> && CHAR_BIT == 8,
					"this branch can only be used on CHAR_BIT == 8 machines");
				static_assert(_Nbytes <= (sizeof(__source_bytes) * CHAR_BIT),
					"unusable (too large) bit size for the given type");

				_Type __value;
				::std::memcpy(::std::addressof(__value), __source_bytes, ztd_c_array_size(__source_bytes));
				return -__value;
			}
		}
		else {
			if constexpr (_Nbytes == 8) {
				return static_cast<_Type>(0x10);
			}
			else if constexpr (_Nbytes == 16) {
				return static_cast<_Type>(0x1023);
			}
			else if constexpr (_Nbytes == 24) {
				return static_cast<_Type>(0x102345);
			}
			else if constexpr (_Nbytes == 32) {
				return static_cast<_Type>(0x10234567);
			}
			else if constexpr (_Nbytes == 40) {
				return static_cast<_Type>(0x1023456789);
			}
			else if constexpr (_Nbytes == 48) {
				return static_cast<_Type>(0x1023456789AB);
			}
			else if constexpr (_Nbytes == 56) {
				return static_cast<_Type>(0x1023456789ABCD);
			}
			else if constexpr (_Nbytes == 64) {
				return static_cast<_Type>(0x1023456789ABCDEF);
			}
			else {
				constexpr const auto& __source_bytes = __tests_detail::__distinct_bit_constant_source_bytes;
				static_assert(ztd::always_true_v<_Type> && CHAR_BIT == 8,
					"this branch can only be used on CHAR_BIT == 8 machines");
				static_assert(_Nbytes <= (sizeof(__source_bytes) * CHAR_BIT),
					"unusable (too large) bit size for the given type");

				_Type __value;
				::std::memcpy(::std::addressof(__value), __source_bytes, ztd_c_array_size(__source_bytes));
				return __value;
			}
		}
	}

	template <typename _Type>
	_Type get_distinct_bit_constant() noexcept {
		return get_distinct_bit_constant_negative<_Type>();
	}

	template <typename _Type>
	_Type get_distinct_bit_constant_reverse() noexcept {
		constexpr std::size_t _Nbytes = sizeof(_Type) * CHAR_BIT;
		if constexpr (_Nbytes == 8) {
			return static_cast<_Type>(0x10);
		}
		else if constexpr (_Nbytes == 16) {
			return static_cast<_Type>(0x2310);
		}
		else if constexpr (_Nbytes == 24) {
			return static_cast<_Type>(0x452310);
		}
		else if constexpr (_Nbytes == 32) {
			return static_cast<_Type>(0x67452310);
		}
		else if constexpr (_Nbytes == 40) {
			return static_cast<_Type>(0x8967452310);
		}
		else if constexpr (_Nbytes == 48) {
			return static_cast<_Type>(0xAB8967452310);
		}
		else if constexpr (_Nbytes == 56) {
			return static_cast<_Type>(0xCDAB8967452310);
		}
		else if constexpr (_Nbytes == 64) {
			return static_cast<_Type>(0xEFCDAB8967452310);
		}
		else {
			constexpr const auto& __source_bytes = __tests_detail::__distinct_bit_constant_source_bytes_reverse;
			static_assert(ztd::always_true_v<_Type> && CHAR_BIT == 8,
				"this branch can only be used on CHAR_BIT == 8 machines");
			static_assert(
				_Nbytes <= (sizeof(__source_bytes) * CHAR_BIT), "unusable (too large) bit size for the given type");

			_Type __value;
			::std::memcpy(::std::addressof(__value), __source_bytes, ztd_c_array_size(__source_bytes));
			return __value;
		}
	}

}} // namespace ztd::tests

#endif
