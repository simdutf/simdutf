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

#include <ztd/idk/encoding_detection.hpp>

#include <ztd/idk/encoding_detection.h>

#include <string_view>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	ZTD_IDK_API_LINKAGE_I_ bool is_execution_encoding_utf8(void) noexcept {
		return ztdc_is_execution_encoding_utf8();
	}

	ZTD_IDK_API_LINKAGE_I_ bool is_execution_encoding_unicode(void) noexcept {
		return ztdc_is_execution_encoding_unicode();
	}

	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_unicode(void) noexcept {
		return ztdc_is_wide_execution_encoding_unicode();
	}

	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf8(void) noexcept {
		return ztdc_is_wide_execution_encoding_utf8();
	}

	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf16(void) noexcept {
		return ztdc_is_wide_execution_encoding_utf16();
	}

	ZTD_IDK_API_LINKAGE_I_ bool is_wide_execution_encoding_utf32(void) noexcept {
		return ztdc_is_wide_execution_encoding_utf32();
	}

	ZTD_IDK_API_LINKAGE_I_ std::string_view execution_encoding_name(void) noexcept {
		return ztdc_execution_encoding_name();
	}

	ZTD_IDK_API_LINKAGE_I_ std::string_view wide_execution_encoding_name(void) noexcept {
		return ztdc_wide_execution_encoding_name();
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd
