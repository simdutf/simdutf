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
// ============================================================================

#include <ztd/idk/utf8_locale.h>

#include <ztd/idk/size.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <clocale>
#else
#include <locale.h>
#include <stdbool.h>
#endif

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztd_idk_attempt_utf8_locale(void) {
	typedef struct category_locale_attempt {
		int category;
		const char* specifier;
	} category_locale_attempt;
	const category_locale_attempt __attempts[] = {
	// Specifiers from Windows Codepage stuff
#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)
		{ LC_ALL, ".65001" },
		{ LC_CTYPE, "65001" },
#endif
		{ LC_CTYPE, "utf8" },
		{ LC_CTYPE, "Utf8" },
		{ LC_CTYPE, "UTF8" },
		{ LC_CTYPE, "utf-8" },
		{ LC_CTYPE, "Utf-8" },
		{ LC_CTYPE, "UTF-8" },
		// okay, now we're just trying WHATEVER we possibly can. this can break downstream software, which is why this
		// is ONLY used in tests, and not beyond that.
		{ LC_ALL, "C.utf8" },
		{ LC_ALL, "C.Utf8" },
		{ LC_ALL, "C.UTF8" },
		{ LC_ALL, "C-utf8" },
		{ LC_ALL, "C-Utf8" },
		{ LC_ALL, "C-UTF8" },
		{ LC_ALL, "en_US.utf8" },
		{ LC_ALL, "en_US.Utf8" },
		{ LC_ALL, "en_US.UTF8" },
		{ LC_ALL, "en_US-utf8" },
		{ LC_ALL, "en_US-Utf8" },
		{ LC_ALL, "en_US-UTF8" },
		{ LC_ALL, "US.utf8" },
		{ LC_ALL, "US.Utf8" },
		{ LC_ALL, "US.UTF8" },
		{ LC_ALL, "US-utf8" },
		{ LC_ALL, "US-Utf8" },
		{ LC_ALL, "US-UTF8" },
	};
	for (std::size_t __i = 0; __i < ztd_c_array_size(__attempts); ++__i) {
		char* __result = std::setlocale(__attempts[__i].category, __attempts[__i].specifier);
		if (__result != nullptr) {
			return true;
		}
	}

	return false;
}
