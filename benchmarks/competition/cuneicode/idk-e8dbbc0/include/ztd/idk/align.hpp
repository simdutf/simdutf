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

#ifndef ZTD_IDK_ALIGN_HPP
#define ZTD_IDK_ALIGN_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/align.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @addtogroup ztd_idk_align Align
	///
	/// @{

	//////
	/// @brief Aligns a pointer according to the given `alignment` and `size`, within the available `space` bytes.
	///
	/// @param __alignment The desired alignment for object that will be put at the (new aligned) pointer's location.
	/// @param __size The size of the object that will be put at the (newly aligned) pointer's location, in bytes.
	/// @param __ptr The pointer to align.
	/// @param __space The amount of available space within which this alignment pay be performed, in bytes.
	template <typename _Type>
	auto align(::std::size_t __alignment, ::std::size_t __size, _Type* __ptr, ::std::size_t __space) noexcept {
		if constexpr (::std::is_const_v<_Type>) {
			return ztdc_align_const(__alignment, __size, __ptr, __space);
		}
		else {
			return ztdc_align_mutable(__alignment, __size, __ptr, __space);
		}
	}

	//////
	/// @}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

// redefine for C++
#undef ztdc_align
//////
/// @brief Aligns a pointer according to the given `alignment` and `size`, within the available `space` bytes.
///
/// @param _ALIGN The desired alignment for object that will be put at the (new aligned) pointer's location.
/// @param _SIZE The size of the object that will be put at the (newly aligned) pointer's location, in bytes.
/// @param _PTR The pointer to align.
/// @param _SPACE The amount of available space within which this alignment pay be performed, in bytes.
#define ztdc_align(_ALIGN, _SIZE, _PTR, _SPACE) ::ztd::align(_ALIGN, _SIZE, _PTR, _SPACE)

#endif
