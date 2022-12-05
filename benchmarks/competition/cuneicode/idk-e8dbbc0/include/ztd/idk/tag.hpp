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

#ifndef ZTD_IDK_TAG_HPP
#define ZTD_IDK_TAG_HPP

#include <ztd/idk/version.hpp>

#include <ztd/prologue.hpp>

//////
/// @addtogroup ztd_idk_tags Tags
/// @{
//////

namespace ztd {

	//////
	/// @brief A tag type which can hold arbitrarily many types. Useful as an anchor for overload resolution on
	/// specific entities, extension point anchors, and more. Not related to the ztd::tag_invoke infrastructure.
	template <typename...>
	class tag { };

} // namespace ztd

/// @}
//////


#include <ztd/epilogue.hpp>

#endif
