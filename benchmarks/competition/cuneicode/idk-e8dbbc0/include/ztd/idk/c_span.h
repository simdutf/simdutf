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

#ifndef ZTD_IDK_C_SPAN_H
#define ZTD_IDK_C_SPAN_H

#include <ztd/idk/version.h>

#include <ztd/idk/charN_t.h>

//////
/// @addtogroup ztd_idk_c_c_span_defines ztd.idk c_span defines
///
/// @{
//////

//////
/// @brief The type used to create a new c_span type.
///
/// @remarks This definition is required. If a type is not provided and the generation header is included, then an error
/// will be produced.
//////
#define ZTD_IDK_C_SPAN_TYPE

//////
/// @brief The name to use when generating the function and structure names.
///
/// @remarks This definition is optional. The default is whatever ZTD_IDK_C_SPAN_TYPE is. However, that may be bad since
/// sometimes type names can have spaces in them (such as `unsigned char`). Therefore, one can se names to make it all
/// better, like `uchar` to represent `unsigned char`.
//////
#define ZTD_IDK_C_SPAN_TYPE_NAME
//////
/// @brief The size type used to create a new c_span type.
///
/// @remarks This definition is optional. The default is `size_t`. In certain cases, a more compact size type may be
/// beneficial than the original size_type. Some may also want to provide a signed type rather than an unsigned type.
/// Note that contract checks will still check for things such as `> 0` or `< size`, even if what is provided is a
/// signed size type (span will not allow negative indexing, where viable).
//////
#define ZTD_IDK_C_SPAN_SIZE_TYPE
//////
/// @brief The name to use when generating the function and structure names.
///
/// @remarks This definition is optional. Normally, it would be defaulted to whatever ZTD_IDK_C_SPAN_SIZE_TYPE is.
/// However, that may be bad since sometimes type names can have spaces in them (such as `long long`). Therefore, one
/// can se names to make it all better, like `uchar` to represent `unsigned char`.
//////
#define ZTD_IDK_C_SPAN_SIZE_TYPE_NAME
//////
/// @brief The whole name of the generated type.
///
/// @remarks This definition is optional. When not provided, a sequence of checks are gone through to define a hopefully
/// unique name for the newly generated c_span. The first generated attempt is just using `c_span{type name}{size
/// type}`, where the size type is only used if ZTD_IDK_C_SPAN_SIZE_TYPE is also defined by you. Otherwise, it defaults
/// to just `c_span{type name}` (without the brackets and with the names substituted in).
//////
#define ZTD_IDK_C_SPAN_NAME
//////
/// @brief Whether or not the size type comes before the pointer.
///
/// @remarks This definition is optional. When not provided, the default layout is `{ pointer_type , size_type }`. If
/// this is defined and its value is 1, the layout is `{ size_type, pointer_type }`. This can aid when generating
/// certain types that are meant to be compatible with other kinds of buffers, e.g. with POSIX's `iovec`.
//////
#define ZTD_IDK_C_SPAN_SIZE_FIRST

#undef ZTD_IDK_C_SPAN_TYPE
#undef ZTD_IDK_C_SPAN_TYPE_NAME
#undef ZTD_IDK_C_SPAN_SIZE_TYPE
#undef ZTD_IDK_C_SPAN_SIZE_TYPE_NAME
#undef ZTD_IDK_C_SPAN_SIZE_FIRST
#undef ZTD_IDK_C_SPAN_NAME

//////
/// @}
//////


//////
/// @cond ZTD_DOXYGEN_DO_NOT_DOCUMENT
//////
#define ZTD_IDK_C_SPAN_TYPE char
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE unsigned char
#define ZTD_IDK_C_SPAN_TYPE_NAME uchar
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_wchar_t
#define ZTD_IDK_C_SPAN_TYPE_NAME wchar_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char8_t
#define ZTD_IDK_C_SPAN_TYPE_NAME char8_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char16_t
#define ZTD_IDK_C_SPAN_TYPE_NAME char16_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char32_t
#define ZTD_IDK_C_SPAN_TYPE_NAME char32_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE char
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_char
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE unsigned char
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_uchar
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_wchar_t
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_wchar_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char8_t
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_char8_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char16_t
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_char16_t
#include <ztd/idk/c_span.g.h>

#define ZTD_IDK_C_SPAN_TYPE ztd_char32_t
#define ZTD_IDK_C_SPAN_TYPE_IS_CONST 1
#define ZTD_IDK_C_SPAN_TYPE_NAME const_char32_t
#include <ztd/idk/c_span.g.h>
//////
/// @endcond
//////


#if defined(ZTD_DOXYGEN_PREPROCESSING) && (ZTD_DOXYGEN_PREPROCESSING != 0)

#include <ztd/text/detail/doxygen_type.h>

//////
/// @addtogroup ztd_idk_c_c_span ztd.idk c_span
///
/// @{
//////

//////
/// @brief A structured pointer which keeps its size type, which represents a non-owning view into a buffer.
///
/// @remarks This type is meant to be "immutable", which is why the members are marked const. This can present some
/// issues when dealing with, for example, trying to fill out members manually in structures that are heap-allocated.
/// Instead, copy it using `memcpy`, like `memcpy(my_span_ptr, &some_span, sizeof(some_span));` rather than
/// `my_span_ptr->data = some_ptr; my_span_ptr->size = some_size;`.
///
/// This type can be initialized with designated initializers.
//////
typedef struct c_span {
	//////
	/// @brief A pointer to the region of data.
	ztd_generic_type* const data;
	//////
	/// @brief The size of the region of data (in number of elements).
	size_t const size;
} c_span;



//////
/// @brief Copies on c_span into the memory of another.
///
/// @param[in] __destination Pointer to the destination.
/// @param[in] __source The source span to copy.
///
/// @remarks
/// @preconditions
/// - `__destination != NULL`
//////
void copy_c_span(c_span* __destination, c_span __source);

//////
/// @brief Create a c_span from two pointers which denote a region of memory.
///
/// @param[in] __first The start of the region of memory, inclusive.
/// @param[in] __last The end of the region of memory, non-inclusive.
///
/// @remarks
/// @preconditions
/// - `__first` <  `__last` (`__first` is reachable from `__last`).
/// - `__first` and `__last` are part of the same storage and form a valid range.
//////
c_span make_c_span(ztd_generic_type* __first, ztd_generic_type* __last);

//////
/// @brief Create a c_span from two pointers which denote a region of memory.
///
/// @param[in] __first The start of the region of memory, inclusive.
/// @param[in] __size The number of elements of the region of memory.
///
/// @remarks
/// @preconditions
/// - `__first` and `__first + __size` are part of the same storage and form a valid range.
//////
c_span make_sized_c_span(ztd_generic_type* __first, size_t __size);

//////
/// @brief Retrieves a pointer to the start of this span of memory.
///
/// @param[in] __span The "self" object.
//////
ztd_generic_type* c_span_data(c_span __span);

//////
/// @brief Retrieves the size of this span of memory, in number of elements.
///
/// @param[in] __span The "self" object.
//////
size_t c_span_size(c_span __span);

//////
/// @brief Returns whether or not this span is empty.
///
/// @param[in] __span The "self" object.
//////
bool c_span_empty(c_span __span);

//////
/// @brief Retrieves the first element of this span of elements.
///
/// @param[in] __span The "self" object.
///
/// @remarks Preconditions:
/// - `__span.size > 0`.
//////
ztd_generic_type c_span_front(c_span __span);

//////
/// @brief Retrieves the last element of this span of elements.
///
/// @remarks Preconditions:
/// - `__span.size > 0`.
//////
ztd_generic_type c_span_back(c_span __span);

//////
/// @brief Retrieves the the element at the provided index.
///
/// @param[in] __span The "self" object.
/// @param[in] __index The offset into the span of elements to access.
///
/// @remarks Preconditions:
/// - `__span.size > __index`.
//////
ztd_generic_type c_span_at(c_span __span, size_t __index);

//////
/// @brief Retrieves the the element at the provided index.
///
/// @param[in] __span The "self" object.
/// @param[in] __index The offset into the span of elements to access.
/// @param[in] __value The value to insert.
///
/// @remarks Preconditions:
/// - `__span.size > __index`.
//////
void c_span_set(c_span __span, size_t __index, ztd_generic_type __value);

//////
/// @brief Retrieves the the element at the provided index.
///
/// @param[in] __span The "self" object.
/// @param[in] __index The offset into the span of elements to access.
///
/// @remarks Preconditions:
/// - `__span.size > __index`.
//////
ztd_generic_type* c_span_ptr_at(c_span __span, size_t __index);

//////
/// @brief Retrieves the the element at the provided index.
///
/// @param[in] __span The "self" object.
/// @param[in] __index The offset into the span of elements to access.
///
/// @remarks This function checks size so there are no preconditions.
//////
ztd_generic_type* c_span_maybe_ptr_at(c_span __span, size_t __index);

//////
/// @brief Retrieves the size of this span of memory, in number of `unsigned char`s.
///
/// @param[in] __span The "self" object.
//////
size_t c_span_byte_size(c_span __span);

//////
/// @brief An iterator to the beginning of the span of elements.
///
/// @param[in] __span The "self" object.
//////
c_span c_span_begin(c_span __span);

//////
/// @brief An iterator to the end of the span of elements.
///
/// @param[in] __span The "self" object.
//////
ztd_generic_type* c_span_end(c_span __span);

//////
/// @brief Creates a smaller span from this span, using the given offset into the span and the desired size.
///
/// @param[in] __span The "self" object.
/// @param[in] __offset_index The offset into the span.
/// @param[in] __size The size of the resulting span.
///
/// @remarks
/// @preconditions
/// - `__span.size >= (__offset_index + __size)`.
//////
c_span c_span_subspan(c_span __span, size_t __offset_index, size_t __size);

//////
/// @brief Creates a smaller span from this span, from the given offset. The resulting size is the offset minus the
/// `__span`'s current size.
///
/// @param[in] __span The "self" object.
/// @param[in] __offset_index The offset into the span.
///
/// @remarks
/// @preconditions
/// - `__span.size >= __offset_index`.
//////
c_span c_span_subspan_at(c_span __span, size_t __offset_index);

//////
/// @brief Creates a smaller span from this span, from the given size. The resulting offset is from 0 and has the given
/// size.
///
/// @param[in] __span The "self" object.
/// @param[in] __size The size of the span, from the beginning.
///
/// @remarks
/// @preconditions
/// - `__span.size >= __size`.
//////
c_span c_span_subspan_prefix(c_span __span, size_t __size);

//////
/// @brief Creates a smaller span from this span, from the given size.
///
/// @param[in] __span The "self" object.
/// @param[in] __size The size of the span, from the beginning.
///
/// @remarks The resulting offset is from the current span's size minus the desired size, and has the given `__size`.
/// @preconditions
/// - `__span.size >= __size`.
//////
c_span c_span_subspan_suffix(c_span __span, size_t __size);
#endif

//////
/// @}
//////

#endif
