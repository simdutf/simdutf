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

#include <ztd/idk/version.h>
#include <ztd/idk/assert.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#include <cstring>
#else
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#endif

// clang-format off
#if !defined(ZTD_IDK_C_SPAN_TYPE)
	#error "A type must be specified using ZTD_IDK_C_SPAN_TYPE when including the generation header."
#endif

#if defined(ZTD_IDK_C_SPAN_TYPE_IS_CONST) && (ZTD_IDK_C_SPAN_TYPE_IS_CONST != 0)
	#define ZTD_IDK_C_SPAN_TYPE_IS_CONST_I_ ZTD_ON
	#define ZTD_IDK_C_SPAN_TYPE_I_ ZTD_IDK_C_SPAN_TYPE
	#define ZTD_IDK_C_SPAN_CONST_TYPE_I_ ZTD_IDK_C_SPAN_TYPE const
#else
	#define ZTD_IDK_C_SPAN_TYPE_IS_CONST_I_ ZTD_OFF
	#define ZTD_IDK_C_SPAN_TYPE_I_ ZTD_IDK_C_SPAN_TYPE
	#define ZTD_IDK_C_SPAN_CONST_TYPE_I_ ZTD_IDK_C_SPAN_TYPE
#endif

#if defined(ZTD_IDK_C_SPAN_SIZE_TYPE)
	#define ZTD_IDK_C_SPAN_SIZE_TYPE_I_ ZTD_IDK_C_SPAN_SIZE_TYPE
#else
	#define ZTD_IDK_C_SPAN_SIZE_TYPE_I_ size_t
#endif

#if defined(ZTD_IDK_C_SPAN_TYPE_NAME)
	#define ZTD_IDK_C_SPAN_TYPE_NAME_I_ ZTD_IDK_C_SPAN_TYPE_NAME
#else
	#define ZTD_IDK_C_SPAN_TYPE_NAME_I_ ZTD_IDK_C_SPAN_TYPE_I_
#endif

#if defined(ZTD_IDK_C_SPAN_SIZE_TYPE_NAME)
	#define ZTD_IDK_C_SPAN_SIZE_TYPE_NAME_I_ ZTD_IDK_C_SPAN_SIZE_TYPE_NAME
#else
	#define ZTD_IDK_C_SPAN_SIZE_TYPE_NAME_I_ ZTD_IDK_C_SPAN_SIZE_TYPE_I_
#endif

#if defined(ZTD_IDK_C_SPAN_TYPE_NAME)
	#define ZTD_IDK_C_SPAN_NAME_SUFFIX_I_ ZTD_IDK_C_SPAN_TYPE_NAME_I_
#else
	#if defined(ZTD_IDK_C_SPAN_SIZE_TYPE)
		#define ZTD_IDK_C_SPAN_NAME_SUFFIX_I_ ZTD_CONCAT_TOKENS_I_(ZTD_IDK_C_SPAN_TYPE_NAME_I_, ZTD_IDK_C_SPAN_SIZE_TYPE_NAME_I_)
	#else
		#define ZTD_IDK_C_SPAN_NAME_SUFFIX_I_ ZTD_IDK_C_SPAN_TYPE_NAME_I_
	#endif
#endif

#if defined(ZTD_IDK_C_SPAN_NAME)
	#define ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_NAME
#else
	#define ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_CONCAT_TOKENS_I_(c_span_, ZTD_IDK_C_SPAN_NAME_SUFFIX_I_)
#endif

#define ZTD_IDK_C_SPAN_PREFIX_I_(_PREFIX) ZTD_CONCAT_TOKENS_I_(_PREFIX, ZTD_IDK_C_SPAN_FULL_NAME_I_)
#define ZTD_IDK_C_SPAN_SUFFIX_I_(_SUFFIX) ZTD_CONCAT_TOKENS_I_(ZTD_IDK_C_SPAN_FULL_NAME_I_, _SUFFIX)

#if defined(ZTD_IDK_C_SPAN_SIZE_FIRST)
	#if (ZTD_IDK_C_SPAN_SIZE_FIRST != 0)
		#define ZTD_IDK_C_SPAN_SIZE_FIRST_I_ ZTD_ON
	#else
		#define ZTD_IDK_C_SPAN_SIZE_FIRST_I_ ZTD_OFF
	#endif
#else
	#define ZTD_IDK_C_SPAN_SIZE_FIRST_I_ ZTD_DEFAULT_OFF
#endif
// clang-format on

ZTD_C_STRUCT_LINKAGE_I_ typedef struct ZTD_IDK_C_SPAN_FULL_NAME_I_ {
#if ZTD_IS_ON(ZTD_IDK_C_SPAN_SIZE_FIRST)
	ZTD_IDK_C_SPAN_SIZE_TYPE_I_ ZTD_CONST_IF_NOT_BROKEN_CXX_I_ size;
	ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_CONST_IF_NOT_BROKEN_CXX_I_ data;
#else
	ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_CONST_IF_NOT_BROKEN_CXX_I_ data;
	ZTD_IDK_C_SPAN_SIZE_TYPE_I_ ZTD_CONST_IF_NOT_BROKEN_CXX_I_ size;
#endif
} ZTD_IDK_C_SPAN_FULL_NAME_I_;

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ void ZTD_IDK_C_SPAN_PREFIX_I_(copy_)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_* __destination, ZTD_IDK_C_SPAN_FULL_NAME_I_ __source) {
	ZTD_ASSERT_MESSAGE_I_("the destination parameter must not be null", __destination != NULL);
	(*(ZTD_IDK_C_SPAN_CONST_TYPE_I_**)&__destination->data) = __source.data;
	(*(ZTD_IDK_C_SPAN_SIZE_TYPE_I_*)&__destination->size)   = __source.size;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_PREFIX_I_(make_)(
     ZTD_IDK_C_SPAN_CONST_TYPE_I_* __first, ZTD_IDK_C_SPAN_CONST_TYPE_I_* __last) {
	ZTD_IDK_C_SPAN_FULL_NAME_I_ __result =
#if ZTD_IS_ON(ZTD_IDK_C_SPAN_SIZE_FIRST)
	     { (ZTD_IDK_C_SPAN_SIZE_TYPE_I_)(__last - __first), __first }
#else
	     { __first, (ZTD_IDK_C_SPAN_SIZE_TYPE_I_)(__last - __first) }
#endif
	;
	return __result;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_PREFIX_I_(make_sized_)(
     ZTD_IDK_C_SPAN_CONST_TYPE_I_* __first, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __size) {
	ZTD_IDK_C_SPAN_FULL_NAME_I_ __result =
#if ZTD_IS_ON(ZTD_IDK_C_SPAN_SIZE_FIRST)
	     { __size, __first }
#else
	     { __first, __size }
#endif
	;

	return __result;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_IDK_C_SPAN_SUFFIX_I_(_data)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.data;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_SIZE_TYPE_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_size)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.size;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ bool ZTD_IDK_C_SPAN_SUFFIX_I_(_empty)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.size == 0;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_TYPE_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_front)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	ZTD_ASSERT_MESSAGE_I_("c_span's size must be greater than zero", __span.size > 0);
	return *__span.data;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_TYPE_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_back)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	ZTD_ASSERT_MESSAGE_I_("c_span's size must be greater than zero", __span.size > 0);
	return *(__span.data + __span.size - 1);
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_TYPE_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_at)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __index) {
	ZTD_ASSERT_MESSAGE_I_("index must be within c_span's boundaries", __span.size > __index);
	return __span.data[__index];
}

#if ZTD_IS_OFF(ZTD_IDK_C_SPAN_TYPE_IS_CONST)
// need: https://thephd.dev/_vendor/future_cxx/papers/C%20-%20typeof.html
// to make it non-conditional and instead assert/toss when the function is called
ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ void ZTD_IDK_C_SPAN_SUFFIX_I_(_set)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __index, ZTD_IDK_C_SPAN_TYPE_I_ __value) {
	ZTD_ASSERT_MESSAGE_I_("index must be within c_span's boundaries", __span.size > __index);
	__span.data[__index] = __value;
}
#endif

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_IDK_C_SPAN_SUFFIX_I_(_ptr_at)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __index) {
	ZTD_ASSERT_MESSAGE_I_("index must be within c_span's boundaries", __index < __span.size);
	return __span.data + __index;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_IDK_C_SPAN_SUFFIX_I_(
     _maybe_ptr_at)(ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __index) {
	if (__index < __span.size)
		return __span.data + __index;
	return NULL;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_SIZE_TYPE_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_byte_size)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.size * sizeof(char);
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_IDK_C_SPAN_SUFFIX_I_(_begin)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.data;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_CONST_TYPE_I_* ZTD_IDK_C_SPAN_SUFFIX_I_(_end)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span) {
	return __span.data + __span.size;
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_subspan)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __offset_index,
     ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __size) {
	ZTD_ASSERT_MESSAGE_I_("c_span's size must be greater than or equal to the offset and size",
	     ((__span.size >= __offset_index) ? ((__span.size - __offset_index) >= __size) : false));
	return ZTD_IDK_C_SPAN_PREFIX_I_(make_sized_)(__span.data + __offset_index, __size);
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(_subspan_at)(
     ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __offset_index) {
	ZTD_ASSERT_MESSAGE_I_(
	     "c_span's size must be greater than or equal to the offset and size", __span.size >= (__offset_index));
	return ZTD_IDK_C_SPAN_SUFFIX_I_(_subspan)(__span, __offset_index, __span.size - __offset_index);
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(
     _subspan_prefix)(ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __size) {
	ZTD_ASSERT_MESSAGE_I_("c_span's size must be greater than or equal to the size", __span.size >= (__size));
	return ZTD_IDK_C_SPAN_SUFFIX_I_(_subspan)(__span, 0, __span.size - __size);
}

ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_INLINE_I_ ZTD_IDK_C_SPAN_FULL_NAME_I_ ZTD_IDK_C_SPAN_SUFFIX_I_(
     _subspan_suffix)(ZTD_IDK_C_SPAN_FULL_NAME_I_ __span, ZTD_IDK_C_SPAN_SIZE_TYPE_I_ __size) {
	ZTD_ASSERT_MESSAGE_I_("c_span's size must be greater than or equal to the size", __span.size >= (__size));
	return ZTD_IDK_C_SPAN_SUFFIX_I_(_subspan)(__span, __span.size - __size, __size);
}

// clang-format off
#undef ZTD_IDK_C_SPAN_TYPE
#if defined(ZTD_IDK_C_SPAN_TYPE_IS_CONST)
	#undef ZTD_IDK_C_SPAN_TYPE_IS_CONST
#endif
#if defined(ZTD_IDK_C_SPAN_TYPE_NAME)
	#undef ZTD_IDK_C_SPAN_TYPE_NAME
#endif
#if defined(ZTD_IDK_C_SPAN_SIZE_TYPE)
	#undef ZTD_IDK_C_SPAN_SIZE_TYPE
#endif
#if defined(ZTD_IDK_C_SPAN_SIZE_TYPE_NAME)
	#undef ZTD_IDK_C_SPAN_SIZE_TYPE
#endif
#if defined(ZTD_IDK_C_SPAN_NAME)
	#undef ZTD_IDK_C_SPAN_NAME
#endif

#undef ZTD_IDK_C_SPAN_TYPE_IS_CONST_I_
#undef ZTD_IDK_C_SPAN_TYPE_I_
#undef ZTD_IDK_C_SPAN_CONST_TYPE_I_
#undef ZTD_IDK_C_SPAN_TYPE_NAME_I_
#undef ZTD_IDK_C_SPAN_SIZE_TYPE_I_
#undef ZTD_IDK_C_SPAN_SIZE_TYPE_NAME_I_
#undef ZTD_IDK_C_SPAN_NAME_SUFFIX_I_
#undef ZTD_IDK_C_SPAN_FULL_NAME_I_
#undef ZTD_IDK_C_SPAN_PREFIX_I_
#undef ZTD_IDK_C_SPAN_SUFFIX_I_
// clang-format on
