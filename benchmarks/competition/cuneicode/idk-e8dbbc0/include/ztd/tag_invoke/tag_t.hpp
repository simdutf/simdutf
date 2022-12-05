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

#ifndef ZTD_TAG_INVOKE_TAG_T_HPP
#define ZTD_TAG_INVOKE_TAG_T_HPP

#include <ztd/tag_invoke/version.hpp>

#include <type_traits>

namespace ztd {
	ZTD_TAG_INVOKE_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief The tag_t alias produces the type of the given reconstruction point. It's used to directly hook into the
	/// tag_invoke infrastructure.
	template <auto& _Name>
	using tag_t = ::std::remove_cv_t<::std::remove_reference_t<decltype(_Name)>>;

	ZTD_TAG_INVOKE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
