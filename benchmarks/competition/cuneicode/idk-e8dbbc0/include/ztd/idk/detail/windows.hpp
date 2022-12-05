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

#ifndef ZTD_IDK_DETAIL_WINDOWS_HPP
#define ZTD_IDK_DETAIL_WINDOWS_HPP

#include <ztd/idk/version.hpp>

#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)

#if ZTD_IS_ON(ZTD_COMPILER_VCXX) || ZTD_IS_ON(ZTD_COMPILER_GCC)
#pragma push_macro("NOMINMAX")
#pragma push_macro("WIN32_LEAN_AND_MEAN")
#pragma push_macro("VC_EXTRALEAN")
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef VC_EXTRALEAN
#endif

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN 1

#ifdef _MSC_VER
#include <cstddef>
#include <ciso646>
#include <cwchar>
#include <locale>
#endif

ZTD_EXTERN_C_OPEN_I_
#include <Windows.h>
ZTD_EXTERN_C_CLOSE_I_
#include <winapifamily.h>

#include <ztd/prologue.hpp>

#if !defined(_KERNELX) && !defined(_ONECORE)
#if defined(WINAPI_FAMILY) || defined(WINAPI_FAMILY_APP)
#define ZTD_FILEAPISAREANSI_I_ ZTD_OFF
#else
#define ZTD_FILEAPISAREANSI_I_ ZTD_ON
#endif
#else
#define ZTD_FILEAPISAREANSI_I_ ZTD_OFF
#endif

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __idk_detail { namespace __windows {

		inline int __determine_active_code_page() noexcept {
#if defined(_STL_LANG) || defined(_YVALS_CORE_H) || defined(_STDEXT)
			// Removed in later versions of VC++
			if (___lc_codepage_func() == CP_UTF8) {
				return CP_UTF8;
			}
#endif // VC++ stuff

#if ZTD_IS_ON(ZTD_FILEAPISAREANSI)
			if (!::AreFileApisANSI()) {
				return CP_OEMCP;
			}
#endif // !defined(_KERNELX) && !defined(_ONECORE)

			return CP_ACP;
		}

		inline bool __is_unicode_code_page(int __codepage_id) {
			switch (__codepage_id) {
			case CP_UTF7:
			case CP_UTF8:
			case 1200:  // UTF-16, Little Endian ("utf-16")
			case 1201:  // UTF-16, Big Endian ("unicodeFFFE")
			case 12000: // UTF-16, Little Endian ("utf-32")
			case 12001: // UTF-16, Big Endian ("utf-32BE")
			case 54936: // GB18030, 4 bytes long
				return true;
			default:
				return false;
			}
		}

	}} // namespace __idk_detail::__windows
	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#if ZTD_IS_ON(ZTD_COMPILER_VCXX)
#pragma pop_macro("VC_EXTRALEAN")
#pragma pop_macro("WIN32_LEAN_AND_MEAN")
#pragma pop_macro("NOMINMAX")
#endif

#endif // Windows nightmare

#include <ztd/epilogue.hpp>

#endif
