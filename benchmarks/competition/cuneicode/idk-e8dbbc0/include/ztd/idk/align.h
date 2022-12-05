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

#ifndef ZTD_IDK_ALIGN_H
#define ZTD_IDK_ALIGN_H

#include <ztd/idk/version.h>

#if ZTD_IS_ON(ZTD_C)
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#endif

//////
/// @addtogroup ztd_idk_align Align
/// @{

//////
/// @brief A structure containing a pointer, the size required by the object at the givne pointer offset, and the amount
/// of space leftover from a ztdc_align call.
typedef struct ztdc_aligned_const_pointer {
	//////
	/// @brief The pointer that was aligned. If the ztdc_aligned call failed, this member will be a null pointer
	/// constant.
	const void* ptr;
	//////
	/// @brief The amount of space required at the offset of the passed-in pointer.
	size_t required_space;
	//////
	/// @brief The amount of space leftover from the space after adjusting the pointer to its new offset, if ztdc_align
	/// succeeded.
	size_t leftover_space;
} ztdc_aligned_const_pointer;

//////
/// @brief A structure containing a pointer, the size required by the object at the givne pointer offset, and the amount
/// of space leftover from a ztdc_align call.
typedef struct ztdc_aligned_mutable_pointer {
	//////
	/// @brief The pointer that was aligned. If the ztdc_aligned call failed, this member will be a null pointer
	/// constant.
	void* ptr;
	//////
	/// @brief The amount of space required at the offset of the passed-in pointer.
	size_t required_space;
	//////
	/// @brief The amount of space leftover from the space after adjusting the pointer to its new offset, if ztdc_align
	/// succeeded.
	size_t leftover_space;
} ztdc_aligned_mutable_pointer;

//////
/// @brief Aligns a pointer according to the given `alignment` and `size`, within the available `space` bytes.
///
/// @param alignment The desired alignment for object that will be put at the (new aligned) pointer's location.
/// @param size The size of the object that will be put at the (newly aligned) pointer's location, in bytes.
/// @param ptr The pointer to align.
/// @param space The amount of available space within which this alignment pay be performed, in bytes.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ inline ztdc_aligned_const_pointer ztdc_align_const(
     size_t alignment, size_t size, const void* ptr, size_t space) ZTD_NOEXCEPT_IF_CXX_I_ {
	const uintptr_t initial     = (uintptr_t)(ptr);
	const uintptr_t offby       = (uintptr_t)(initial % alignment);
	const uintptr_t padding     = (alignment - offby) % alignment;
	const size_t required_space = size + padding;
	if (space < required_space) {
		return { nullptr, required_space, space };
	}
	const void* const aligned_ptr = (const void*)((const char*)(ptr) + padding);
	const size_t leftover_space   = space - padding;
	return { aligned_ptr, required_space, leftover_space };
}

//////
/// @brief Aligns a pointer according to the given `alignment` and `size`, within the available `space` bytes.
///
/// @param alignment The desired alignment for object that will be put at the (new aligned) pointer's location.
/// @param size The size of the object that will be put at the (newly aligned) pointer's location, in bytes.
/// @param ptr The pointer to align.
/// @param space The amount of available space within which this alignment pay be performed, in bytes.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ inline ztdc_aligned_mutable_pointer ztdc_align_mutable(
     size_t alignment, size_t size, void* ptr, size_t space) ZTD_NOEXCEPT_IF_CXX_I_ {
	const uintptr_t initial     = (uintptr_t)(ptr);
	const uintptr_t offby       = (uintptr_t)(initial % alignment);
	const uintptr_t padding     = (alignment - offby) % alignment;
	const size_t required_space = size + padding;
	if (space < required_space) {
		return { nullptr, required_space, space };
	}
	void* const aligned_ptr     = (void*)((char*)(ptr) + padding);
	const size_t leftover_space = space - padding;
	return { aligned_ptr, required_space, leftover_space };
}

#if ZTD_IS_ON(ZTD_C)
//////
/// @brief Aligns a pointer according to the given `alignment` and `size`, within the available `space` bytes.
///
/// @param _ALIGN The desired alignment for object that will be put at the (new aligned) pointer's location.
/// @param _SIZE The size of the object that will be put at the (newly aligned) pointer's location, in bytes.
/// @param _PTR The pointer to align.
/// @param _SPACE The amount of available space within which this alignment pay be performed, in bytes.
#define ztdc_align(_ALIGN, _SIZE, _PTR, _SPACE) \
	_Generic(_PTR, const void* : ztdc_align_const, void* : ztdc_align_mutable)(_ALIGN, _SIZE, _PTR, _SPACE)
#endif


//////
/// @}

#endif
