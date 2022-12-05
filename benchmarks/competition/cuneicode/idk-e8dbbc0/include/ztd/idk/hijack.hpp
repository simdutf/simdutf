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

#ifndef ZTD_IDK_HIJACK_HPP
#define ZTD_IDK_HIJACK_HPP

#include <ztd/idk/version.hpp>

#include <ztd/prologue.hpp>

//////
/// @addtogroup ztd_idk_hijack Hijacking Tokens (CRTP)
/// @{
//////

//////
/// @brief A token to derive from, which in some cases allows external members to place customization points and
/// extension functions in the global namespace. This is useful for enabling functionality against C-like types.
//////
template <typename...>
class ztd_hijack_global_token { };

namespace ztd { namespace hijack {

	//////
	/// @brief A token to derive from, which in some cases allows external members to place customization points and
	/// extension functions in the hijack namespace. Extension points would be defined in the "namespace ztd {
	/// namespace hijack { /* here */ }}" area.
	template <typename...>
	class token { };

}} // namespace ztd::hijack

/// @}
//////


#include <ztd/epilogue.hpp>

#endif
