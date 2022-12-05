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

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_BASIC_CONV_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_BASIC_CONV_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode.h>
#include <ztd/cuneicode/detail/err.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <climits>
#include <cstring>
#include <cstddef>

#if ZTD_IS_ON(ZTD_LOCALE_DEPENDENT_WIDE_EXECUTION)
#include <langinfo.h>
#endif // nl_langinfo

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __cnc_detail {

		inline constexpr size_t __wchar_t_pointer_size = (sizeof(wchar_t*) * CHAR_BIT);
		inline constexpr bool __least64_bit_pointers   = __wchar_t_pointer_size >= 64;
		inline constexpr bool __least32_bit_pointers   = __wchar_t_pointer_size >= 32;

		struct alignas(mbstate_t) __c32_state {
			ztd_char32_t __accumulation : 32;
			unsigned char __accumulation_count : 8;
			unsigned char __expected_count : 8;
			unsigned char __locality : 8;
			unsigned char __phase : 8;
		};
	} // namespace __cnc_detail
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif
