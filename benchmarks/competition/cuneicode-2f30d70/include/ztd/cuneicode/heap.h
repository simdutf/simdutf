// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#ifndef ZTD_CUNEICODE_HEAP_H
#define ZTD_CUNEICODE_HEAP_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/idk/charN_t.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#include <stdbool.h>
#endif

//////
/// @addtogroup ztd_cuneicode_heap Heap Functions
///
/// @{

//////
/// @brief An allocation function type.
typedef void*(cnc_allocate_function)(size_t __requested_size, size_t __alignment,
     size_t* __p_actual_size, void* __user_data);
//////
/// @brief A reallocation function type.
typedef void*(cnc_reallocate_function)(void* __original, size_t __requested_size,
     size_t __alignment, size_t* __p_actual_size, void* __user_data);
//////
/// @brief An allocation expanding function type.
typedef void*(cnc_allocation_expand_function)(void* __original, size_t __original_size,
     size_t __alignment, size_t __expand_left, size_t __expand_right, size_t* __p_actual_size,
     void* __user_data);
//////
/// @brief An allocation shrink function type.
typedef void*(cnc_allocation_shrink_function)(void* __original, size_t __original_size,
     size_t __alignment, size_t __reduce_left, size_t __reduce_right, size_t* __p_actual_size,
     void* __user_data);
//////
/// @brief The allocation deallocate function type.
typedef void(cnc_deallocate_function)(
     void* __ptr, size_t __ptr_size, size_t __alignment, void* __user_data);

//////
/// @brief The conversion heap through which all allocating and deallocating happens, as well as any
/// related actions that require dynamic allocation.
typedef struct cnc_conversion_heap {
	//////
	/// @brief The userdata to be passed alng to the heap functions.
	void* user_data;
	//////
	/// @brief The allocation function. It takes a userdata passed in when creating the heap, and
	/// writes out the actual size of the returned allocated pointer.
	cnc_allocate_function* allocate;
	//////
	/// @brief The reallocation function. It takes the original pointer and the requested size
	/// alongside a userdata passed in when creating the heap, and writes out the actual size of
	/// the returned allocated pointer. The original pointer cannot be used.
	cnc_reallocate_function* reallocate;
	//////
	/// @brief The allocation expanding function. It takes the original allocation, the amount to
	/// expand the allocation by to its left (descending, lowest memory order) and right
	/// (ascending, highest memory order), the original pointer, and the original size alongside a
	/// userdata point. It returns the new pointer and writes out the new size.
	cnc_allocation_expand_function* shrink;
	//////
	/// @brief The allocation shrink function. It takes the original allocation, the amount to
	/// shrink the allocation by to its left (descending, lowest memory order) and right
	/// (ascending, highest memory order), the original pointer, and the original size alongside a
	/// userdata point. It returns the new pointer and writes out the new size.
	cnc_allocation_shrink_function* expand;
	//////
	/// @brief The allocation deallocate function. It takes the original pointer, its size, its
	/// alignment, and a userdata passed in during heap creation.
	cnc_deallocate_function* deallocate;
} cnc_conversion_heap;

//////
/// @brief Creates a default heap to be used. Goes through the globally-available allocator (malloc,
/// free. etc. or similar known provided free stores).
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_conversion_heap cnc_create_default_heap(
     void) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Compares whether two cnc_conversion_heaps are the same.
///
/// @param[in] __left The first heap.
/// @param[in] __right The second heap.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conversion_heap_equals(
     const cnc_conversion_heap* __left, const cnc_conversion_heap* __right) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @brief Compares whether two cnc_conversion_heaps are not the same.
///
/// @param[in] __left The first heap.
/// @param[in] __right The second heap.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conversion_heap_not_equals(
     const cnc_conversion_heap* __left, const cnc_conversion_heap* __right) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @}

#endif // ZTD_CUNEICODE_HEAP_H
