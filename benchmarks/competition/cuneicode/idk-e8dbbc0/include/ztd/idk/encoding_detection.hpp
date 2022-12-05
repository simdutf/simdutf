// =============================================================================
//
// ztd.cuneicode
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
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
// ============================================================================

#ifndef ZTD_IDK_ENCODING_DETECTION_HPP
#define ZTD_IDK_ENCODING_DETECTION_HPP

#pragma once

#include <ztd/idk/version.hpp>

#include <string_view>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	ZTD_IDK_API_LINKAGE_I_ bool is_execution_encoding_utf8(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ bool is_execution_encoding_unicode(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_unicode(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf8(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf16(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf32(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ std::string_view execution_encoding_name(void) noexcept;
	ZTD_IDK_API_LINKAGE_I_ std::string_view wide_execution_encoding_name(void) noexcept;

	inline constexpr std::string_view literal_encoding_name(void) noexcept {
		return ZTD_CXX_COMPILE_TIME_ENCODING_NAME_GET_I_();
	}

	inline constexpr std::string_view wide_literal_encoding_name(void) noexcept {
		return ZTD_CXX_COMPILE_TIME_WIDE_ENCODING_NAME_GET_I_();
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
