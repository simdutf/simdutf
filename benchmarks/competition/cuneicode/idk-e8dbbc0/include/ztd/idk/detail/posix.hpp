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

#ifndef ZTD_IDK_DETAIL_POSIX_HPP
#define ZTD_IDK_DETAIL_POSIX_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/encoding_name.hpp>

#if ZTD_IS_ON(ZTD_PLATFORM_UNIX) || ZTD_IS_ON(ZTD_PLATFORM_POSIX)

// clang-format off
#include <clocale>
#if ZTD_IS_ON(ZTD_LANGINFO)
	extern "C" {
		#include <langinfo.h>
	}
#elif ZTD_IS_ON(ZTD_NL_LANGINFO)
	// IBM-specific??
	extern "C" {
		#include <nl_langinfo.h>
	}
#endif

#if ZTD_IS_ON(ZTD_PLATFORM_POSIX) && ZTD_IS_ON(ZTD_DLFCN_H)
	#include <dlfcn.h>
#endif
// clang-format on

#include <ztd/prologue.hpp>

namespace ztd {

	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail { namespace __posix {

		inline text_encoding_id __determine_active_code_page() noexcept {
#if ZTD_IS_ON(ZTD_LANGINFO) || ZTD_IS_ON(ZTD_NL_LANGINFO)
			const char* __name = nl_langinfo(LC_CTYPE);
			return __idk_detail::__to_encoding_id(__name);
#else
			// fallback to stdlib I guess?
			const char* __ctype_name = setlocale(LC_CTYPE, nullptr);
			return __idk_detail::__to_encoding_id(__ctype_name);
#endif
		}

	}} // namespace __idk_detail::__posix

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_

} // namespace ztd

#endif // POSIX

#include <ztd/epilogue.hpp>

#endif
