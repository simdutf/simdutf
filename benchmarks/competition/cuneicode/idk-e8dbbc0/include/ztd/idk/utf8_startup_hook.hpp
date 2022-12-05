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

#ifndef ZTD_IDK_UTF8_STARTUP_HOOK_HPP
#define ZTD_IDK_UTF8_STARTUP_HOOK_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/utf8_locale.h>

#include <iostream>

namespace ztd {
	//////
	/// @brief A hook which attempts to set the locale to a UTF-8 locale of some kind.
	struct utf8_startup_hook {
		//////
		/// @brief The result of the hook.
		int result;

		//////
		/// @brief A constructor which attempts to set the locale to UTF-8.
		utf8_startup_hook() noexcept : result(0) {
			result = ztd_idk_attempt_utf8_locale();
			if (result == 0) {
				std::cerr << "cannot set the locale-based encoding in non-Windows to UTF8" << std::endl;
			}
		}
	};
} // namespace ztd


#endif
