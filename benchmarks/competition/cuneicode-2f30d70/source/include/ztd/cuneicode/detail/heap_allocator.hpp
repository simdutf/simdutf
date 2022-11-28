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

#pragma once

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_HEAP_ALLOCATOR_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_HEAP_ALLOCATOR_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/heap.h>

#include <memory>
#include <type_traits>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __cnc_detail {

		template <typename _Ty>
		class __heap_allocator {
		private:
			template <typename>
			friend class __heap_allocator;

			using __underlying_pointer = void*;

		public:
			using value_type         = _Ty;
			using pointer            = value_type*;
			using const_void_pointer = const void*;
			using size_type          = ::std::size_t;

			using propagate_on_container_copy_assignment = ::std::true_type;
			using propagate_on_container_move_assignment = ::std::true_type;
			using propagate_on_container_swap            = ::std::true_type;
			using is_always_equal                        = ::std::false_type;

			__heap_allocator(cnc_conversion_heap& __heap_) noexcept
			: __heap_allocator(::std::addressof(__heap_)) {
			}
			__heap_allocator(cnc_conversion_heap* __p_heap_) noexcept : __p_heap(__p_heap_) {
			}
			__heap_allocator(const __heap_allocator&) noexcept            = default;
			__heap_allocator(__heap_allocator&&) noexcept                 = default;
			__heap_allocator& operator=(const __heap_allocator&) noexcept = default;
			__heap_allocator& operator=(__heap_allocator&&) noexcept      = default;
			template <typename _RightTy>
			__heap_allocator(const __heap_allocator<_RightTy>& __right) noexcept
			: __p_heap(__right.__p_heap) {
			}

			static constexpr ::std::align_val_t alignment() noexcept {
				if constexpr (alignof(_Ty) < alignof(::std::max_align_t)) {
					return static_cast<::std::align_val_t>(alignof(::std::max_align_t));
				}
				else {
					return static_cast<::std::align_val_t>(alignof(_Ty));
				}
			}

			pointer allocate(size_type __requested_number_of_elements,
			     [[maybe_unused]] const_void_pointer hint = nullptr) {
				size_type __requested_number_of_bytes
				     = __requested_number_of_elements * sizeof(value_type);
				size_type __actual_number_of_bytes;
				__underlying_pointer __ptr = this->__p_heap->allocate(
				     __requested_number_of_bytes, static_cast<size_type>(this->alignment()),
				     &__actual_number_of_bytes, this->__p_heap->user_data);
				if (__ptr == nullptr) {
					throw std::bad_alloc();
				}
				return pointer(__ptr);
			}

			void deallocate(pointer __original, size_type __original_number_of_elements) {
				size_type __original_number_of_bytes
				     = __original_number_of_elements * sizeof(value_type);
				__p_heap->deallocate(__underlying_pointer(__original),
				     __original_number_of_bytes, static_cast<size_type>(this->alignment()),
				     this->__p_heap->user_data);
			}

			friend bool operator==(
			     const __heap_allocator& __left, const __heap_allocator& __right) noexcept {
				return __left.__p_heap == __right.__p_heap
				     && cnc_conversion_heap_equals(__left.__p_heap, __right.__p_heap);
			}

			friend bool operator!=(
			     const __heap_allocator& __left, const __heap_allocator& __right) noexcept {
				return __left.__p_heap != __right.__p_heap
				     || cnc_conversion_heap_not_equals(__left.__p_heap, __right.__p_heap);
			}

		private:
			cnc_conversion_heap* __p_heap;
		};

	} // namespace __cnc_detail
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif // ZTD_CUNEICODE_SOURCE_DETAIL_HEAP_ALLOCATOR_HPP
