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

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/heap.h>

#include <cstdlib>
#include <cstring>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void* __cnc_default_allocate(
     size_t __requested_size, size_t __alignment, size_t* __p_actual_size,
     void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__user_data;
	(void)__alignment;
	unsigned char* __ptr = static_cast<unsigned char*>(malloc(__requested_size));
	if (__ptr == nullptr) {
		return nullptr;
	}
	*__p_actual_size = __requested_size;
	return __ptr;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void* __cnc_default_reallocate(
     void* __original, size_t __requested_size, size_t __alignment, size_t* __p_actual_size,
     void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__user_data;
	(void)__alignment;
	unsigned char* __ptr = static_cast<unsigned char*>(realloc(__original, __requested_size));
	if (__ptr == nullptr) {
		*__p_actual_size = 0;
		return nullptr;
	}
	*__p_actual_size = __requested_size;
	return __ptr;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void* __cnc_default_expand_allocation(
     void* __original, size_t __original_size, size_t __alignment, size_t __expand_left,
     size_t __expand_right, size_t* __p_actual_size, void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__original;
	(void)__original_size;
	(void)__alignment;
	(void)__expand_left;
	(void)__expand_right;
	(void)__user_data;
	*__p_actual_size = 0;
	return nullptr;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void* __cnc_default_shrink_allocation(
     void* __original, size_t __original_size, size_t __alignment, size_t __reduce_left,
     size_t __reduce_right, size_t* __p_actual_size, void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__original;
	(void)__original_size;
	(void)__alignment;
	(void)__reduce_left;
	(void)__reduce_right;
	(void)__user_data;
	*__p_actual_size = 0;
	return nullptr;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void __cnc_default_deallocate(
     void* __ptr, size_t __ptr_size, size_t __alignment, void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__ptr_size;
	(void)__user_data;
	(void)__alignment;
	free(__ptr);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_conversion_heap cnc_create_default_heap(
     void) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_conversion_heap __heap { nullptr, __cnc_default_allocate, __cnc_default_reallocate,
		__cnc_default_expand_allocation, __cnc_default_shrink_allocation,
		__cnc_default_deallocate };
	return __heap;
}


ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conversion_heap_equals(
     const cnc_conversion_heap* __left, const cnc_conversion_heap* __right) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __left->allocate == __right->allocate && __left->reallocate == __right->reallocate
	     && __left->shrink == __right->shrink && __left->expand == __right->expand
	     && __left->deallocate == __right->deallocate && __left->user_data == __right->user_data;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_conversion_heap_not_equals(
     const cnc_conversion_heap* __left, const cnc_conversion_heap* __right) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __left->allocate != __right->allocate || __left->reallocate != __right->reallocate
	     || __left->shrink != __right->shrink || __left->expand != __right->expand
	     || __left->deallocate != __right->deallocate || __left->user_data != __right->user_data;
}
