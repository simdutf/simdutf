// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#pragma once

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_CONV_ID_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_CONV_ID_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/idk/charN_t.hpp>
#include <ztd/idk/char_traits.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <string_view>
#include <vector>
#include <cstddef>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __cnc_detail {

		inline const ztd_char8_t* __exec_name() noexcept {
			static const auto* __ptr = u8"execution";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __wide_name() noexcept {
			static const auto* __ptr = u8"wide_execution";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __exec_alias() noexcept {
			static const auto* __ptr = u8"char";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __wide_alias() noexcept {
			static const auto* __ptr = u8"wchar_t";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf8_name() noexcept {
			static const auto* __ptr = u8"utf8";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf8_unchecked_name() noexcept {
			static const auto* __ptr = u8"utf8-unchecked";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf16_name() noexcept {
			static const auto* __ptr = u8"utf16";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf16_unchecked_name() noexcept {
			static const auto* __ptr = u8"utf16-unchecked";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf32_name() noexcept {
			static const auto* __ptr = u8"utf32";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		inline const ztd_char8_t* __utf32_unchecked_name() noexcept {
			static const auto* __ptr = u8"utf32-unchecked";
			return reinterpret_cast<const ztd_char8_t*>(__ptr);
		}

		enum class __cnc_defined_slot {
			__utf8,
			__utf8_unchecked,
			__utf16,
			__utf16_unchecked,
			__utf32,
			__utf32_unchecked
		};

		inline static const int __cnc_defined_slot_max = 6;

		inline const ::std::basic_string_view<ztd_char8_t>& _TO_Name(__cnc_defined_slot __slot) {
			static const ::std::basic_string_view<ztd_char8_t>
			     __identifiers[__cnc_defined_slot_max] = {
				     __utf8_name(),
				     __utf8_unchecked_name(),
				     __utf16_name(),
				     __utf16_unchecked_name(),
				     __utf32_name(),
				     __utf32_unchecked_name(),
			     };
			return __identifiers[static_cast<size_t>(__slot)];
		}

		inline bool __is_unicode_identifier(
		     const ::std::basic_string_view<ztd_char8_t>& __id, __cnc_defined_slot& __out_slot) {
			for (size_t __slot_index = 0; __slot_index < __cnc_defined_slot_max;
			     ++__slot_index) {
				__cnc_defined_slot __slot = static_cast<__cnc_defined_slot>(__slot_index);
				if (_TO_Name(__slot) == __id) {
					__out_slot = __slot;
					return true;
				}
			}
			return false;
		}
	} // namespace __cnc_detail
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif
