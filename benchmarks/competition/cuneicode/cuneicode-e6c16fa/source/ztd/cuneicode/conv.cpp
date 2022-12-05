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

#include <ztd/cuneicode/conv.h>
#include <ztd/cuneicode/mcchar.h>
#include <ztd/cuneicode/max_output.h>
#include <ztd/cuneicode/pivot_info.h>
#include <ztd/cuneicode/mcchar_named.h>

#include <ztd/cuneicode/detail/conv_id.hpp>
#include <ztd/cuneicode/detail/align.hpp>
#include <ztd/cuneicode/detail/err.hpp>
#include <ztd/cuneicode/detail/heap_allocator.hpp>
#include <ztd/cuneicode/detail/conversion.hpp>
#include <ztd/cuneicode/detail/registry.hpp>
#include <ztd/cuneicode/detail/buffer_size.h>

#include <ztd/idk/hash.hpp>
#include <ztd/idk/size.hpp>
#include <ztd/idk/encoding_name.hpp>

#include <string_view>
#include <unordered_map>
#include <vector>
#include <new>
#include <memory>
#include <cstring>

namespace {
	template <typename _State>
	static inline _State* __basic_state(unsigned char* __state,
	     size_t __available_space = sizeof(_State) + (sizeof(_State) - 1)) {
		void* __extra_start   = static_cast<void*>(__state);
		void* __aligned_extra = ::cnc::__cnc_detail::__align(
		     alignof(_State), sizeof(_State), __extra_start, __available_space);
		if (__aligned_extra == nullptr) {
			return nullptr;
		}
		return static_cast<_State*>(static_cast<void*>(__aligned_extra));
	}

	template <typename _State>
	static inline _State* __basic_state(void* __state,
	     size_t __available_space = sizeof(_State) + (sizeof(_State) - 1)) noexcept {
		return __basic_state<_State>(static_cast<unsigned char*>(__state), __available_space);
	}

	template <typename _State>
	static inline void __basic_close_function(void* __state) noexcept {
		_State* __basic = __basic_state<_State>(__state);
		__basic->~_State();
	}

	template <typename _State, typename _CompleteFunction,
	     _CompleteFunction __state_is_complete_function>
	static inline bool __basic_state_is_complete_function(
	     cnc_conversion*, void* __state) noexcept {
		_State* __basic = __basic_state<_State>(__state);
		return __state_is_complete_function(__basic);
	}

	template <typename _State>
	static inline ::std::pair<cnc_open_error, _State*> __core_open_function(
	     cnc_conversion_registry*, cnc_conversion* __conversion, size_t* __p_available_space,
	     size_t* __p_max_alignment, void** __p_space) noexcept {
		const bool __is_counting = __conversion == nullptr;
		*__p_max_alignment = ::std::max(*__p_max_alignment, static_cast<size_t>(alignof(_State)));
		//[[maybe_unused]] const size_t __starting_available_space = *__p_available_space;
		void* const __starting_p_space = __p_space == nullptr ? nullptr : *__p_space;
		_State* __basic_aligned = __basic_state<_State>(__starting_p_space, *__p_available_space);
		if (__basic_aligned == nullptr) {
			// Ffffffffflubberbuckets.
			return { CNC_OPEN_ERROR_ALLOCATION_FAILURE, nullptr };
		}
		unsigned char* const __aligned_space
		     = static_cast<unsigned char*>(static_cast<void*>(__basic_aligned + 1));
		const size_t __used_space
		     = (__aligned_space - static_cast<unsigned char*>(__starting_p_space));
		if (__is_counting) {
			*__p_available_space -= (__used_space);
			return { CNC_OPEN_ERROR_OK, nullptr };
		}
		_State* __basic = new (static_cast<void*>(__basic_aligned)) _State();
		*__p_available_space -= __used_space;
		*__p_space = __aligned_space;
		return { CNC_OPEN_ERROR_OK, __basic };
	}

	template <typename _State>
	static inline cnc_open_error __basic_open_function(cnc_conversion_registry* __registry,
	     cnc_conversion* __conversion, size_t* __p_available_space, size_t* __p_max_alignment,
	     void** __p_space) noexcept {
		auto __code_and_ptr = __core_open_function<_State>(
		     __registry, __conversion, __p_available_space, __p_max_alignment, __p_space);
		return __code_and_ptr.first;
	}

	struct __typical_cnc_conversion {
		cnc_mcstate_t __state;
	};

	struct __intermediary_cnc_conversion {
		cnc_conversion __link0;
		cnc_conversion __link1;
		// from the end of this struct, AKA from "this + 1"!
		size_t __link1_state_byte_offset;
	};

	struct __intermediary_states {
		__intermediary_cnc_conversion* __intermediary_state;
		void* __link0_state;
		void* __link1_state;
	};

	static inline __typical_cnc_conversion* __typical_state(void* __state,
	     size_t __available_space
	     = sizeof(__typical_cnc_conversion) + (sizeof(__typical_cnc_conversion) - 1)) noexcept {
		return __basic_state<__typical_cnc_conversion>(__state, __available_space);
	}

	static inline void __typical_close_function(void* __state) noexcept {
		__basic_close_function<__typical_cnc_conversion>(__state);
	}

	static inline cnc_open_error __typical_open_function(cnc_conversion_registry* __registry,
	     cnc_conversion* __conversion, size_t* __p_available_space, size_t* __p_max_alignment,
	     void** __p_space) noexcept {
		auto __code_and_ptr = __core_open_function<__typical_cnc_conversion>(
		     __registry, __conversion, __p_available_space, __p_max_alignment, __p_space);
		if (__code_and_ptr.second) {
			cnc_mcstate_t* __p_state = &(__code_and_ptr.second->__state);
			*__p_state               = cnc_mcstate_t {};
		}
		return __code_and_ptr.first;
	}

	static inline bool __typical_state_is_complete_function(
	     cnc_conversion* __conversion, void* __state) noexcept {
		(void)__conversion;
		auto __conversion_state = __typical_state(__state);
		return cnc_mcstate_is_complete(&__conversion_state->__state);
	}

	template <typename _SourceChar, typename _DestChar, typename _Func, _Func __func,
	     typename _State>
	static inline cnc_mcerror __basic_single_conversion(cnc_conversion*,
	     size_t* __p_bytes_out_count, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		const bool __using_provided_pivot_info = __p_pivot_info != nullptr;
		const bool __is_counting  = __p_bytes_out == nullptr || *__p_bytes_out == nullptr;
		const bool __is_unbounded = __p_bytes_out_count == nullptr;
		const size_t __initial_source_count = *__p_bytes_in_count / sizeof(_SourceChar);
		size_t __source_count               = __initial_source_count;
		const _SourceChar* __source_first   = reinterpret_cast<const _SourceChar*>(*__p_bytes_in);
		const _SourceChar* __source         = __source_first;
		[[maybe_unused]] const _SourceChar* __source_last = __source_first + __source_count;
		const size_t __initial_dest_count
		     = __is_unbounded ? SIZE_MAX : *__p_bytes_out_count / sizeof(_DestChar);
		size_t __dest_count = __initial_dest_count;
		_DestChar* __dest_first
		     = __is_counting ? nullptr : reinterpret_cast<_DestChar*>(*__p_bytes_out);
		_DestChar* __dest    = __dest_first;
		_State* __conversion = __basic_state<_State>(__state);
		cnc_mcerror __err;
		if constexpr (::std::is_same_v<__typical_cnc_conversion, _State>) {
			cnc_mcstate_t* __p_state = &__conversion->__state;
			__err = __func(__is_unbounded ? nullptr : &__dest_count, &__dest, &__source_count,
			     &__source, __p_state);
		}
		else {
			__err = __func(__is_unbounded ? nullptr : &__dest_count, &__dest, &__source_count,
			     &__source, __conversion);
		}
		// always update all relevant counts
		// and pointers, whether or not there is an actual error
		size_t __bytes_written = (__initial_dest_count - __dest_count) * sizeof(_DestChar);
		size_t __bytes_read    = (__initial_source_count - __source_count) * sizeof(_SourceChar);
		*__p_bytes_in          = reinterpret_cast<const unsigned char*>(__source);
		*__p_bytes_in_count -= static_cast<size_t>(__bytes_read);
		if (!__is_counting) {
			*__p_bytes_out = reinterpret_cast<unsigned char*>(__dest);
		}
		if (!__is_unbounded) {
			*__p_bytes_out_count -= static_cast<size_t>(__bytes_written);
		}
		if (__using_provided_pivot_info) {
			__p_pivot_info->error = CNC_MCERROR_OK;
		}
		return __err;
	}

	template <typename _SourceChar, typename _DestChar, typename _Func, _Func __func,
	     typename _State>
	static inline cnc_mcerror __basic_multi_conversion(cnc_conversion*,
	     size_t* __p_bytes_out_count, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		const bool __using_provided_pivot_info = __p_pivot_info != nullptr;
		const bool __is_counting  = __p_bytes_out == nullptr || *__p_bytes_out == nullptr;
		const bool __is_unbounded = __p_bytes_out_count == nullptr;
		const size_t __initial_source_count = *__p_bytes_in_count / sizeof(_SourceChar);
		size_t __source_count               = __initial_source_count;
		const _SourceChar* __source_first   = reinterpret_cast<const _SourceChar*>(*__p_bytes_in);
		const _SourceChar* __source         = __source_first;
		const size_t __initial_dest_count
		     = __is_unbounded ? SIZE_MAX : *__p_bytes_out_count / sizeof(_DestChar);
		size_t __dest_count = __initial_dest_count;
		_DestChar* __dest_first
		     = __is_counting ? nullptr : reinterpret_cast<_DestChar*>(*__p_bytes_out);
		_DestChar* __dest    = __dest_first;
		_State* __conversion = __basic_state<_State>(__state);
		cnc_mcerror __err;
		if constexpr (::std::is_same_v<__typical_cnc_conversion, _State>) {
			cnc_mcstate_t* __p_state = &__conversion->__state;
			__err = __func(__is_unbounded ? nullptr : &__dest_count, &__dest, &__source_count,
			     &__source, __p_state);
		}
		else {
			__err = __func(__is_unbounded ? nullptr : &__dest_count, &__dest, &__source_count,
			     &__source, __conversion);
		}
		// always update all relevant counts
		// and pointers, whether or not there is an actual error
		size_t __bytes_written = (__initial_dest_count - __dest_count) * sizeof(_DestChar);
		size_t __bytes_read    = (__initial_source_count - __source_count) * sizeof(_SourceChar);
		*__p_bytes_in          = reinterpret_cast<const unsigned char*>(__source);
		*__p_bytes_in_count -= static_cast<size_t>(__bytes_read);
		if (!__is_counting) {
			*__p_bytes_out = reinterpret_cast<unsigned char*>(__dest);
		}
		if (!__is_unbounded) {
			*__p_bytes_out_count -= static_cast<size_t>(__bytes_written);
		}
		if (__using_provided_pivot_info) {
			__p_pivot_info->error = CNC_MCERROR_OK;
		}
		return __err;
	}

	template <typename _SourceChar, typename _DestChar, typename _Func, _Func __func>
	static inline cnc_mcerror __typical_multi_conversion(cnc_conversion* __base_conversion,
	     size_t* __p_bytes_out_count, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		return __basic_multi_conversion<_SourceChar, _DestChar, _Func, __func,
		     __typical_cnc_conversion>(__base_conversion, __p_bytes_out_count, __p_bytes_out,
		     __p_bytes_in_count, __p_bytes_in, __p_pivot_info, __state);
	}

	template <typename _SourceChar, typename _DestChar, typename _Func, _Func __func>
	static inline cnc_mcerror __typical_single_conversion(
	     [[maybe_unused]] cnc_conversion* __base_conversion, size_t* __p_bytes_out_count,
	     unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		const bool __using_provided_pivot_info = __p_pivot_info != nullptr;
		const bool __is_counting  = __p_bytes_out == nullptr || *__p_bytes_out == nullptr;
		const bool __is_unbounded = __p_bytes_out_count == nullptr;
		const size_t __initial_source_count = *__p_bytes_in_count / sizeof(_SourceChar);
		size_t __source_count               = __initial_source_count;
		const _SourceChar* __source_first   = reinterpret_cast<const _SourceChar*>(*__p_bytes_in);
		const _SourceChar* __source         = __source_first;
		[[maybe_unused]] const _SourceChar* __source_last = __source_first + __source_count;
		const size_t __initial_dest_count
		     = __is_unbounded ? SIZE_MAX : *__p_bytes_out_count / sizeof(_DestChar);
		size_t __dest_count = __initial_dest_count;
		_DestChar* __dest_first
		     = __is_counting ? nullptr : reinterpret_cast<_DestChar*>(*__p_bytes_out);
		_DestChar* __dest                      = __dest_first;
		__typical_cnc_conversion* __conversion = __typical_state(__state);
		cnc_mcstate_t* __p_state               = &__conversion->__state;
		cnc_mcerror __err = __func(__is_unbounded ? nullptr : &__dest_count, &__dest,
		     &__source_count, &__source, __p_state);
		// always update all relevant counts
		// and pointers, whether or not there is an actual error
		size_t __bytes_written = (__initial_dest_count - __dest_count) * sizeof(_DestChar);
		size_t __bytes_read    = (__initial_source_count - __source_count) * sizeof(_SourceChar);
		*__p_bytes_in          = reinterpret_cast<const unsigned char*>(__source);
		*__p_bytes_in_count -= static_cast<size_t>(__bytes_read);
		if (!__is_counting) {
			*__p_bytes_out = reinterpret_cast<unsigned char*>(__dest);
		}
		if (!__is_unbounded) {
			*__p_bytes_out_count -= static_cast<size_t>(__bytes_written);
		}
		if (__using_provided_pivot_info) {
			__p_pivot_info->error = CNC_MCERROR_OK;
		}
		return __err;
	}

	static inline __intermediary_cnc_conversion* __intermediary_align(
	     void* __state, [[maybe_unused]] size_t __available_space) {
		size_t __intermediary_extra_space = sizeof(__intermediary_cnc_conversion);
		void* __intermediary_extra_start  = __state;
		void* __intermediary_aligned      = ::cnc::__cnc_detail::__align(
		          alignof(__intermediary_cnc_conversion), sizeof(__intermediary_cnc_conversion),
		          __intermediary_extra_start, __intermediary_extra_space);
		if (__intermediary_aligned == nullptr) {
			return nullptr;
		}
		__intermediary_cnc_conversion* __intermediate_state
		     = reinterpret_cast<__intermediary_cnc_conversion*>(__intermediary_aligned);
		return __intermediate_state;
	}

	static inline __intermediary_states __intermediary_state(unsigned char* __state,
	     size_t __available_space = sizeof(__intermediary_cnc_conversion)
	          + (sizeof(__intermediary_cnc_conversion) - 1)) noexcept {
		__intermediary_cnc_conversion* __intermediate_state
		     = __intermediary_align(__state, __available_space);
		if (__intermediate_state == nullptr) {
			return { nullptr, nullptr, nullptr };
		}
		unsigned char* __link0_byte_data = reinterpret_cast<unsigned char*>(__intermediate_state)
		     + sizeof(__intermediary_cnc_conversion);
		unsigned char* __link1_byte_data
		     = __link0_byte_data + __intermediate_state->__link1_state_byte_offset;
		return { __intermediate_state, static_cast<void*>(__link0_byte_data),
			static_cast<void*>(__link1_byte_data) };
	}

	static inline __intermediary_states __intermediary_state(void* __state,
	     size_t __available_space = sizeof(__intermediary_cnc_conversion)
	          + (sizeof(__intermediary_cnc_conversion) - 1)) noexcept {
		return __intermediary_state(static_cast<unsigned char*>(__state), __available_space);
	}

	static inline void __intermediary_close_function(void* __state) noexcept {
		__intermediary_states __intermediary = __intermediary_state(__state);
		__intermediary.__intermediary_state->__link0.__close_function(
		     &__intermediary.__intermediary_state->__link0);
		__intermediary.__intermediary_state->__link1.__close_function(
		     &__intermediary.__intermediary_state->__link1);
		__intermediary.__intermediary_state->~__intermediary_cnc_conversion();
	}

	static inline cnc_open_error __intermediary_open_function(cnc_conversion_registry* __registry,
	     cnc_conversion* __conversion, const __cnc_registry_entry* __from,
	     const __cnc_registry_entry* __to, size_t* __p_available_space, size_t* __p_max_alignment,
	     void** __p_space) noexcept {
		if (__p_space == nullptr || *__p_space == nullptr) {
			return CNC_OPEN_ERROR_INVALID_PARAMETER;
		}
		const bool __is_counting = __conversion == nullptr;
		*__p_max_alignment       = ::std::max(
		           *__p_max_alignment, static_cast<size_t>(alignof(__intermediary_cnc_conversion)));
		const size_t __starting_available_space = *__p_available_space;
		void* const __starting_space            = *__p_space;
		__intermediary_cnc_conversion* __intermediary_aligned
		     = __intermediary_align(*__p_space, *__p_available_space);
		const size_t __aligned_up_size
		     = (static_cast<unsigned char*>(__starting_space)
		            - reinterpret_cast<unsigned char*>(__intermediary_aligned))
		     + sizeof(__intermediary_cnc_conversion);
		if (__is_counting) {
			*__p_space = static_cast<void*>(
			     reinterpret_cast<unsigned char*>(*__p_space) + __aligned_up_size);
			*__p_available_space -= __aligned_up_size;
			cnc_open_error __link0_err = __from->__open_function(
			     __registry, nullptr, __p_available_space, __p_max_alignment, __p_space);
			if (__link0_err != CNC_OPEN_ERROR_OK) {
				*__p_available_space = __starting_available_space;
				*__p_space           = __starting_space;
				return __link0_err;
			}
			cnc_open_error __link1_err = __to->__open_function(
			     __registry, nullptr, __p_available_space, __p_max_alignment, __p_space);
			if (__link1_err != CNC_OPEN_ERROR_OK) {
				return __link1_err;
			}
			return CNC_OPEN_ERROR_OK;
		}
		if (__intermediary_aligned == nullptr) {
			// Ffffffffflubberbuckets.
			return CNC_OPEN_ERROR_ALLOCATION_FAILURE;
		}
		__intermediary_cnc_conversion* __intermediary
		     = new (__intermediary_aligned) __intermediary_cnc_conversion();
		__intermediary->__link1_state_byte_offset = 0;
		*__p_available_space -= sizeof(__intermediary_cnc_conversion);
		*__p_space          = static_cast<void*>(reinterpret_cast<unsigned char*>(__intermediary)
               + sizeof(__intermediary_cnc_conversion));
		void* __link0_space = *__p_space;
		size_t __space_before_link0 = *__p_available_space;
		cnc_open_error __link0_err  = __from->__open_function(
		      __registry, __conversion, __p_available_space, __p_max_alignment, &__link0_space);
		if (__link0_err != CNC_OPEN_ERROR_OK) {
			__intermediary->~__intermediary_cnc_conversion();
			*__p_available_space = __starting_available_space;
			*__p_space           = __starting_space;
			return __link0_err;
		}
		const size_t __link0_size = static_cast<unsigned char*>(__link0_space)
		     - static_cast<unsigned char*>(*__p_space);
		*__p_space                 = __link0_space;
		size_t __link1_byte_offset = (__space_before_link0 - *__p_available_space);
		void* __link1_space        = static_cast<unsigned char*>(*__p_space)
		     + __intermediary->__link1_state_byte_offset;
		cnc_open_error __link1_err = __to->__open_function(
		     __registry, __conversion, __p_available_space, __p_max_alignment, &__link1_space);
		if (__link1_err != CNC_OPEN_ERROR_OK) {
			__intermediary->~__intermediary_cnc_conversion();
			__from->__close_function(__link0_space);
			*__p_available_space = __starting_available_space;
			*__p_space           = __starting_space;
			return __link1_err;
		}
		const size_t __link1_size = static_cast<unsigned char*>(__link1_space)
		     - static_cast<unsigned char*>(*__p_space);

		*__p_space                                          = __link1_space;
		__intermediary->__link0.__registry                  = __registry;
		__intermediary->__link0.__multi_conversion_function = __from->__multi_conversion_function;
		__intermediary->__link0.__single_conversion_function
		     = __from->__single_conversion_function;
		__intermediary->__link0.__state_is_complete_function
		     = __from->__state_is_complete_function;
		__intermediary->__link0.__close_function             = __from->__close_function;
		__intermediary->__link0.__size                       = __link0_size;
		__intermediary->__link0.__properties                 = CNC_CONV_PROPS_NONE;
		__intermediary->__link1.__registry                   = __registry;
		__intermediary->__link1.__multi_conversion_function  = __to->__multi_conversion_function;
		__intermediary->__link1.__single_conversion_function = __to->__single_conversion_function;
		__intermediary->__link1.__state_is_complete_function = __to->__state_is_complete_function;
		__intermediary->__link1.__close_function             = __to->__close_function;
		__intermediary->__link1_state_byte_offset            = __link1_byte_offset;
		__intermediary->__link1.__size                       = __link1_size;
		return CNC_OPEN_ERROR_OK;
	}

	static inline cnc_mcerror __intermediary_multi_conversion(cnc_conversion*,
	     size_t* __p_bytes_out_count, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		const bool __using_provided_pivot_info = __p_pivot_info != nullptr;
		constexpr const size_t __intermediate_pivot_buffer_max
		     = CNC_DEFAULT_CONVERSION_INTERMEDIATE_BUFFER_SIZE;
		unsigned char __intermediate_pivot_buffer[__intermediate_pivot_buffer_max] {};
		cnc_pivot_info __backup_pivot_info
		     = { __intermediate_pivot_buffer_max, __intermediate_pivot_buffer, CNC_MCERROR_OK };
		cnc_pivot_info* __target_p_pivot_info
		     = !__using_provided_pivot_info || __p_pivot_info->bytes == nullptr
		     ? &__backup_pivot_info
		     : __p_pivot_info;
		cnc_pivot_info __empty_pivot_info = { 0, nullptr, CNC_MCERROR_OK };
		__intermediary_states __states    = __intermediary_state(__state);
		for (;;) {
			size_t __start_bytes_in_count         = *__p_bytes_in_count;
			size_t __intermediate_bytes_out_count = __target_p_pivot_info->bytes_size;
			if (__start_bytes_in_count == 0) {
				const bool __is_link0_state_complete
				     = __states.__intermediary_state->__link0.__state_is_complete_function(
				          &__states.__intermediary_state->__link0, __states.__link0_state);
				if (__is_link0_state_complete) {
					const bool __is_link1_state_complete
					     = __states.__intermediary_state->__link1.__state_is_complete_function(
					          &__states.__intermediary_state->__link1, __states.__link1_state);
					if (__is_link1_state_complete) {
						break;
					}
					else {
						// we're only halfway done: skip to the second half of the loop!
					}
				}
				// otherwise, we are not completely done, so go through the whole setup again.
			}
			const unsigned char* __start_bytes_in   = *__p_bytes_in;
			unsigned char* __intermediate_bytes_out = __target_p_pivot_info->bytes;
			cnc_mcerror __link0res
			     = __states.__intermediary_state->__link0.__multi_conversion_function(
			          &__states.__intermediary_state->__link0, &__intermediate_bytes_out_count,
			          &__intermediate_bytes_out, __p_bytes_in_count, __p_bytes_in,
			          &__empty_pivot_info, __states.__link0_state);
			if (__link0res != CNC_MCERROR_OK && __link0res != CNC_MCERROR_INSUFFICIENT_OUTPUT) {
				// something bad happened: revert potential writes to the
				// progress variables
				*__p_bytes_in_count = __start_bytes_in_count;
				*__p_bytes_in       = __start_bytes_in;
				if (__using_provided_pivot_info) {
					__p_pivot_info->error = __link0res;
				}
				return __link0res;
			}
			size_t __intermediate_bytes_in_count
			     = __target_p_pivot_info->bytes_size - __intermediate_bytes_out_count;
			const unsigned char* __intermediate_bytes_in = __target_p_pivot_info->bytes;

			cnc_mcerror __link1res
			     = __states.__intermediary_state->__link1.__multi_conversion_function(
			          &__states.__intermediary_state->__link1, __p_bytes_out_count,
			          __p_bytes_out, &__intermediate_bytes_in_count, &__intermediate_bytes_in,
			          &__empty_pivot_info, __states.__link1_state);
			if (__link1res != CNC_MCERROR_OK) {
				if (__using_provided_pivot_info) {
					__p_pivot_info->error = __link1res;
				}
				return __link1res;
			}
		}

		if (__using_provided_pivot_info) {
			__p_pivot_info->error = CNC_MCERROR_OK;
		}
		return CNC_MCERROR_OK;
	}

	static inline cnc_mcerror __intermediary_single_conversion(cnc_conversion*,
	     size_t* __p_bytes_out_count, unsigned char** __p_bytes_out, size_t* __p_bytes_in_count,
	     const unsigned char** __p_bytes_in, cnc_pivot_info* __p_pivot_info,
	     void* __state) noexcept {
		const bool __using_provided_pivot_info = __p_pivot_info != nullptr;
		const size_t __intermediate_pivot_buffer_max
		     = CNC_DEFAULT_CONVERSION_INTERMEDIATE_BUFFER_SIZE;
		unsigned char __intermediate_pivot_buffer[__intermediate_pivot_buffer_max] {};
		cnc_pivot_info __backup_pivot_info
		     = { __intermediate_pivot_buffer_max, __intermediate_pivot_buffer, CNC_MCERROR_OK };
		cnc_pivot_info* __target_p_pivot_info
		     = !__using_provided_pivot_info || __p_pivot_info->bytes == nullptr
		     ? &__backup_pivot_info
		     : __p_pivot_info;
		cnc_pivot_info __empty_pivot_info                      = { 0, nullptr, CNC_MCERROR_OK };
		__intermediary_states __states                         = __intermediary_state(__state);
		[[maybe_unused]] size_t __start_bytes_in_count         = *__p_bytes_in_count;
		[[maybe_unused]] const unsigned char* __start_bytes_in = *__p_bytes_in;
		size_t __intermediate_bytes_out_count   = __target_p_pivot_info->bytes_size;
		unsigned char* __intermediate_bytes_out = __target_p_pivot_info->bytes;
		cnc_mcerror __link0res
		     = __states.__intermediary_state->__link0.__single_conversion_function(
		          &__states.__intermediary_state->__link0, &__intermediate_bytes_out_count,
		          &__intermediate_bytes_out, __p_bytes_in_count, __p_bytes_in,
		          &__empty_pivot_info, __states.__link0_state);
		if (__link0res != CNC_MCERROR_OK) {
			if (__using_provided_pivot_info) {
				__p_pivot_info->error = __link0res;
			}
			return __link0res;
		}
		if (__using_provided_pivot_info) {
			__p_pivot_info->error = CNC_MCERROR_OK;
		}
		size_t __intermediate_bytes_in_count
		     = __target_p_pivot_info->bytes_size - __intermediate_bytes_out_count;
		const unsigned char* __intermediate_bytes_in = __target_p_pivot_info->bytes;
		cnc_mcerror __link1res
		     = __states.__intermediary_state->__link1.__single_conversion_function(
		          &__states.__intermediary_state->__link1, __p_bytes_out_count, __p_bytes_out,
		          &__intermediate_bytes_in_count, &__intermediate_bytes_in, &__empty_pivot_info,
		          __states.__link1_state);
		return __link1res;
	}

	static inline cnc_open_error __cnc_open_with(cnc_conversion_registry* __registry,
	     const __cnc_registry_entry* __entry, cnc_conversion** __out_p_conversion,
	     size_t* __p_available_space, void** __p_space) {
		const size_t __starting_available_space = *__p_available_space;
		void* __target                          = *__p_space;
		void* __aligned_target                  = ::cnc::__cnc_detail::__align(
		                      alignof(cnc_conversion), sizeof(cnc_conversion), __target, *__p_available_space);
		if (__aligned_target == nullptr) {
			*__p_available_space = __starting_available_space;
			return CNC_OPEN_ERROR_INSUFFICIENT_OUTPUT;
		}
		cnc_conversion* __base_conv               = new (__aligned_target) cnc_conversion();
		__base_conv->__registry                   = __registry;
		__base_conv->__size                       = __starting_available_space;
		__base_conv->__properties                 = CNC_CONV_PROPS_NONE;
		__base_conv->__multi_conversion_function  = __entry->__multi_conversion_function;
		__base_conv->__single_conversion_function = __entry->__single_conversion_function;
		__base_conv->__close_function             = __entry->__close_function;
		*__out_p_conversion                       = __base_conv;
		*__p_space = static_cast<unsigned char*>(static_cast<void*>(__base_conv + 1));
		*__p_available_space -= sizeof(cnc_conversion);
		size_t __max_alignment = alignof(cnc_conversion);
		cnc_open_error __err   = __entry->__open_function(
		       __registry, __base_conv, __p_available_space, &__max_alignment, __p_space);
		if (__err != CNC_OPEN_ERROR_OK) {
			__base_conv->~cnc_conversion();
			*__out_p_conversion  = nullptr;
			*__p_available_space = __starting_available_space;
			return __err;
		}
		return __err;
	}

	static inline cnc_open_error __cnc_open_intermediary_with(cnc_conversion_registry* __registry,
	     const __cnc_registry_entry* __from, const __cnc_registry_entry* __to,
	     cnc_conversion** __out_p_conversion, size_t* __p_available_space, void** __p_space) {
		const size_t __starting_available_space = *__p_available_space;
		void* __target                          = *__p_space;
		void* __aligned_target                  = ::cnc::__cnc_detail::__align(
		                      alignof(cnc_conversion), sizeof(cnc_conversion), __target, *__p_available_space);
		if (__aligned_target == nullptr) {
			*__p_available_space = __starting_available_space;
			return CNC_OPEN_ERROR_INSUFFICIENT_OUTPUT;
		}
		size_t __max_alignment                    = alignof(cnc_conversion);
		cnc_conversion* __base_conv               = new (__aligned_target) cnc_conversion();
		__base_conv->__registry                   = __registry;
		__base_conv->__size                       = __starting_available_space;
		__base_conv->__properties                 = CNC_CONV_PROPS_INDIRECT;
		__base_conv->__multi_conversion_function  = &__intermediary_multi_conversion;
		__base_conv->__single_conversion_function = &__intermediary_single_conversion;
		__base_conv->__close_function             = &__intermediary_close_function;
		*__p_available_space -= sizeof(cnc_conversion);
		*__p_space
		     = static_cast<void*>(static_cast<unsigned char*>(static_cast<void*>(__base_conv))
		          + sizeof(cnc_conversion));
		*__out_p_conversion  = __base_conv;
		cnc_open_error __err = ::__intermediary_open_function(__registry, __base_conv, __from,
		     __to, __p_available_space, &__max_alignment, __p_space);
		if (__err != CNC_OPEN_ERROR_OK) {
			__base_conv->~cnc_conversion();
			*__out_p_conversion  = nullptr;
			*__p_available_space = __starting_available_space;
			return __err;
		}

		return __err;
	}
} // namespace

//////
/// @brief This function must be marked extern because it must have a stable external symbol w.r.t
/// this library.
extern cnc_mcerror __cnc_multi_from_single_conversion(cnc_conversion* __conversion,
     size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info,
     void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_input_bytes_size == nullptr || __p_input_bytes == nullptr) {
		return CNC_MCERROR_OK;
	}
	const unsigned char*& __input_bytes = *__p_input_bytes;
	size_t& __input_bytes_size          = *__p_input_bytes_size;
	if (__input_bytes == nullptr || __input_bytes_size == 0) {
		return CNC_MCERROR_OK;
	}
	for (;;) {
		cnc_mcerror __err = __conversion->__single_conversion_function(__conversion,
		     __out_pput_bytes_size, __out_pput_bytes, __p_input_bytes_size, __p_input_bytes,
		     __p_pivot_info, __user_data);
		switch (__err) {
		case CNC_MCERROR_OK:
			if (__input_bytes_size > 0) {
				// loop around!
				continue;
			}
			if (__conversion->__state_is_complete_function(__conversion, __user_data)) {
				// we're done!
				return CNC_MCERROR_OK;
			}
		default:
			break;
		}
		return __err;
	}
}

//////
/// @brief This function must be marked extern because it must have a stable external symbol w.r.t
/// this library.
extern cnc_mcerror __cnc_single_from_multi_conversion(cnc_conversion* __conversion,
     size_t* __out_pput_bytes_size, unsigned char** __out_pput_bytes, size_t* __p_input_bytes_size,
     const unsigned char** __p_input_bytes, cnc_pivot_info* __p_pivot_info,
     void* __user_data) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_input_bytes_size == nullptr || __p_input_bytes == nullptr) {
		return CNC_MCERROR_OK;
	}
	const unsigned char* __input_bytes = *__p_input_bytes;
	size_t __input_bytes_size          = *__p_input_bytes_size;
	if (__input_bytes == nullptr || __input_bytes_size == 0) {
		return CNC_MCERROR_OK;
	}
	for (size_t __len = 1; __len <= __input_bytes_size; ++__len) {
		cnc_mcerror __err
		     = __conversion->__multi_conversion_function(__conversion, __out_pput_bytes_size,
		          __out_pput_bytes, &__len, &__input_bytes, __p_pivot_info, __user_data);
		switch (__err) {
		case CNC_MCERROR_INCOMPLETE_INPUT:
			// alright, so we just need more input:
			// we go A G A N E !
			continue;
		default:
			break;
		}
		return __err;
	}
	// if we reach here, we just simply do not have enough input: bail out
	return CNC_MCERROR_INCOMPLETE_INPUT;
}

extern cnc_open_error __cnc_add_default_registry_entries(
     cnc_conversion_registry* __registry) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_open_error __err;
#define _CHECK_ERR_AND_RETURN(...)                     \
	__err = __VA_ARGS__;                              \
	if (__err != cnc_open_error::CNC_OPEN_ERROR_OK) { \
		return __err;                                \
	}                                                 \
	static_assert(true, "")

	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__exec_name(),
	     ::cnc::__cnc_detail::__exec_name(),
	     &__typical_multi_conversion<char, char, decltype(&::cnc_mcsnrtomcsn), &::cnc_mcsnrtomcsn>,
	     &__typical_single_conversion<char, char, decltype(&::cnc_mcnrtomcn), &::cnc_mcnrtomcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__exec_name(),
	     ::cnc::__cnc_detail::__wide_name(),
	     &__typical_multi_conversion<char, wchar_t, decltype(&::cnc_mcsnrtomwcsn),
	          &::cnc_mcsnrtomwcsn>,
	     &__typical_single_conversion<char, wchar_t, decltype(&::cnc_mcnrtomwcn),
	          &::cnc_mcnrtomwcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__exec_name(),
	     ::cnc::__cnc_detail::__utf8_name(),
	     &__typical_multi_conversion<char, ztd_char8_t, decltype(&::cnc_mcsnrtoc8sn),
	          &::cnc_mcsnrtoc8sn>,
	     &__typical_single_conversion<char, ztd_char8_t, decltype(&::cnc_mcnrtoc8n),
	          &::cnc_mcnrtoc8n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__exec_name(),
	     ::cnc::__cnc_detail::__utf16_name(),
	     &__typical_multi_conversion<char, ztd_char16_t, decltype(&::cnc_mcsnrtoc16sn),
	          &::cnc_mcsnrtoc16sn>,
	     &__typical_single_conversion<char, ztd_char16_t, decltype(&::cnc_mcnrtoc16n),
	          &::cnc_mcnrtoc16n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__exec_name(),
	     ::cnc::__cnc_detail::__utf32_name(),
	     &__typical_multi_conversion<char, ztd_char32_t, decltype(&::cnc_mcsnrtoc32sn),
	          &::cnc_mcsnrtoc32sn>,
	     &__typical_single_conversion<char, ztd_char32_t, decltype(&::cnc_mcnrtoc32n),
	          &::cnc_mcnrtoc32n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));

	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__wide_name(),
	     ::cnc::__cnc_detail::__exec_name(),
	     &__typical_multi_conversion<wchar_t, char, decltype(&::cnc_mwcsnrtomcsn),
	          &::cnc_mwcsnrtomcsn>,
	     &__typical_single_conversion<wchar_t, char, decltype(&::cnc_mwcnrtomcn),
	          &::cnc_mwcnrtomcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__wide_name(),
	     ::cnc::__cnc_detail::__wide_name(),
	     &__typical_multi_conversion<wchar_t, wchar_t, decltype(&::cnc_mwcsnrtomwcsn),
	          &::cnc_mwcsnrtomwcsn>,
	     &__typical_single_conversion<wchar_t, wchar_t, decltype(&::cnc_mwcnrtomwcn),
	          &::cnc_mwcnrtomwcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__wide_name(),
	     ::cnc::__cnc_detail::__utf8_name(),
	     &__typical_multi_conversion<wchar_t, ztd_char8_t, decltype(&::cnc_mwcsnrtoc8sn),
	          &::cnc_mwcsnrtoc8sn>,
	     &__typical_single_conversion<wchar_t, ztd_char8_t, decltype(&::cnc_mwcnrtoc8n),
	          &::cnc_mwcnrtoc8n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__wide_name(),
	     ::cnc::__cnc_detail::__utf16_name(),
	     &__typical_multi_conversion<wchar_t, ztd_char16_t, decltype(&::cnc_mwcsnrtoc16sn),
	          &::cnc_mwcsnrtoc16sn>,
	     &__typical_single_conversion<wchar_t, ztd_char16_t, decltype(&::cnc_mwcnrtoc16n),
	          &::cnc_mwcnrtoc16n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__wide_name(),
	     ::cnc::__cnc_detail::__utf32_name(),
	     &__typical_multi_conversion<wchar_t, ztd_char32_t, decltype(&::cnc_mwcsnrtoc32sn),
	          &::cnc_mwcsnrtoc32sn>,
	     &__typical_single_conversion<wchar_t, ztd_char32_t, decltype(&::cnc_mwcnrtoc32n),
	          &::cnc_mwcnrtoc32n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));

	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf8_name(),
	     ::cnc::__cnc_detail::__exec_name(),
	     &__typical_multi_conversion<ztd_char8_t, char, decltype(&::cnc_c8snrtomcsn),
	          &::cnc_c8snrtomcsn>,
	     &__typical_single_conversion<ztd_char8_t, char, decltype(&::cnc_c8nrtomcn),
	          &::cnc_c8nrtomcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf8_name(),
	     ::cnc::__cnc_detail::__wide_name(),
	     &__typical_multi_conversion<ztd_char8_t, wchar_t, decltype(&::cnc_c8snrtomwcsn),
	          &::cnc_c8snrtomwcsn>,
	     &__typical_single_conversion<ztd_char8_t, wchar_t, decltype(&::cnc_c8nrtomwcn),
	          &::cnc_c8nrtomwcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf8_name(),
	     ::cnc::__cnc_detail::__utf8_name(),
	     &__typical_multi_conversion<ztd_char8_t, ztd_char8_t, decltype(&::cnc_c8snrtoc8sn),
	          &::cnc_c8snrtoc8sn>,
	     &__typical_single_conversion<ztd_char8_t, ztd_char8_t, decltype(&::cnc_c8nrtoc8n),
	          &::cnc_c8nrtoc8n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf8_name(),
	     ::cnc::__cnc_detail::__utf16_name(),
	     &__typical_multi_conversion<ztd_char8_t, ztd_char16_t, decltype(&::cnc_c8snrtoc16sn),
	          &::cnc_c8snrtoc16sn>,
	     &__typical_single_conversion<ztd_char8_t, ztd_char16_t, decltype(&::cnc_c8nrtoc16n),
	          &::cnc_c8nrtoc16n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf8_name(),
	     ::cnc::__cnc_detail::__utf32_name(),
	     &__typical_multi_conversion<ztd_char8_t, ztd_char32_t, decltype(&::cnc_c8snrtoc32sn),
	          &::cnc_c8snrtoc32sn>,
	     &__typical_single_conversion<ztd_char8_t, ztd_char32_t, decltype(&::cnc_c8nrtoc32n),
	          &::cnc_c8nrtoc32n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));

	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf16_name(),
	     ::cnc::__cnc_detail::__exec_name(),
	     &__typical_multi_conversion<ztd_char16_t, char, decltype(&::cnc_c16snrtomcsn),
	          &::cnc_c16snrtomcsn>,
	     &__typical_single_conversion<ztd_char16_t, char, decltype(&::cnc_c16nrtomcn),
	          &::cnc_c16nrtomcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf16_name(),
	     ::cnc::__cnc_detail::__wide_name(),
	     &__typical_multi_conversion<ztd_char16_t, wchar_t, decltype(&::cnc_c16snrtomwcsn),
	          &::cnc_c16snrtomwcsn>,
	     &__typical_single_conversion<ztd_char16_t, wchar_t, decltype(&::cnc_c16nrtomwcn),
	          &::cnc_c16nrtomwcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf16_name(),
	     ::cnc::__cnc_detail::__utf8_name(),
	     &__typical_multi_conversion<ztd_char16_t, ztd_char8_t, decltype(&::cnc_c16snrtoc8sn),
	          &::cnc_c16snrtoc8sn>,
	     &__typical_single_conversion<ztd_char16_t, ztd_char8_t, decltype(&::cnc_c16nrtoc8n),
	          &::cnc_c16nrtoc8n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf16_name(),
	     ::cnc::__cnc_detail::__utf16_name(),
	     &__typical_multi_conversion<ztd_char16_t, ztd_char16_t, decltype(&::cnc_c16snrtoc16sn),
	          &::cnc_c16snrtoc16sn>,
	     &__typical_single_conversion<ztd_char16_t, ztd_char16_t, decltype(&::cnc_c16nrtoc16n),
	          &::cnc_c16nrtoc16n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf16_name(),
	     ::cnc::__cnc_detail::__utf32_name(),
	     &__typical_multi_conversion<ztd_char16_t, ztd_char32_t, decltype(&::cnc_c16snrtoc32sn),
	          &::cnc_c16snrtoc32sn>,
	     &__typical_single_conversion<ztd_char16_t, ztd_char32_t, decltype(&::cnc_c16nrtoc32n),
	          &::cnc_c16nrtoc32n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));

	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),
	     ::cnc::__cnc_detail::__exec_name(),
	     &__typical_multi_conversion<ztd_char32_t, char, decltype(&::cnc_c32snrtomcsn),
	          &::cnc_c32snrtomcsn>,
	     &__typical_single_conversion<ztd_char32_t, char, decltype(&::cnc_c32nrtomcn),
	          &::cnc_c32nrtomcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),
	     ::cnc::__cnc_detail::__wide_name(),
	     &__typical_multi_conversion<ztd_char32_t, wchar_t, decltype(&::cnc_c32snrtomwcsn),
	          &::cnc_c32snrtomwcsn>,
	     &__typical_single_conversion<ztd_char32_t, wchar_t, decltype(&::cnc_c32nrtomwcn),
	          &::cnc_c32nrtomwcn>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),
	     ::cnc::__cnc_detail::__utf8_name(),
	     &__typical_multi_conversion<ztd_char32_t, ztd_char8_t, decltype(&::cnc_c32snrtoc8sn),
	          &::cnc_c32snrtoc8sn>,
	     &__typical_single_conversion<ztd_char32_t, ztd_char8_t, decltype(&::cnc_c32nrtoc8n),
	          &::cnc_c32nrtoc8n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),
	     ::cnc::__cnc_detail::__utf16_name(),
	     &__typical_multi_conversion<ztd_char32_t, ztd_char16_t, decltype(&::cnc_c32snrtoc16sn),
	          &::cnc_c32snrtoc16sn>,
	     &__typical_single_conversion<ztd_char32_t, ztd_char16_t, decltype(&::cnc_c32nrtoc16n),
	          &::cnc_c32nrtoc16n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),
	     ::cnc::__cnc_detail::__utf32_name(),
	     &__typical_multi_conversion<ztd_char32_t, ztd_char32_t, decltype(&::cnc_c32snrtoc32sn),
	          &::cnc_c32snrtoc32sn>,
	     &__typical_single_conversion<ztd_char32_t, ztd_char32_t, decltype(&::cnc_c32nrtoc32n),
	          &::cnc_c32nrtoc32n>,
	     &::__typical_state_is_complete_function, &::__typical_open_function,
	     &::__typical_close_function));

#define _ADD_MCN_NAMED_ENCODING_BASIC(                                                             \
     _NAME, _SUFFIX, _DECODE_STATE, _DECODE_COMPLETE_FN, _ENCODE_STATE, _ENCODE_COMPLETE_FN)       \
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, (const ztd_char8_t*)(_NAME),            \
	     ::cnc::__cnc_detail::__utf32_name(),                                                     \
	     &__basic_multi_conversion<ztd_char_t, ztd_char32_t,                                      \
	          decltype(&::cnc_mcsnrtoc32sn_##_SUFFIX), &::cnc_mcsnrtoc32sn_##_SUFFIX,             \
	          _DECODE_STATE>,                                                                     \
	     &__basic_single_conversion<ztd_char_t, ztd_char32_t,                                     \
	          decltype(&::cnc_mcnrtoc32n_##_SUFFIX), &::cnc_mcnrtoc32n_##_SUFFIX, _DECODE_STATE>, \
	     &::__basic_state_is_complete_function<_DECODE_STATE, decltype(_DECODE_COMPLETE_FN),      \
	          _DECODE_COMPLETE_FN>,                                                               \
	     &::__basic_open_function<_DECODE_STATE>, &::__basic_close_function<_DECODE_STATE>));     \
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(),    \
	     (const ztd_char8_t*)(_NAME),                                                             \
	     &__basic_multi_conversion<ztd_char32_t, ztd_char_t,                                      \
	          decltype(&::cnc_c32snrtomcsn_##_SUFFIX), &::cnc_c32snrtomcsn_##_SUFFIX,             \
	          _ENCODE_STATE>,                                                                     \
	     &__basic_single_conversion<ztd_char32_t, ztd_char_t,                                     \
	          decltype(&::cnc_c32nrtomcn_##_SUFFIX), &::cnc_c32nrtomcn_##_SUFFIX, _ENCODE_STATE>, \
	     &::__basic_state_is_complete_function<_ENCODE_STATE, decltype(_ENCODE_COMPLETE_FN),      \
	          _ENCODE_COMPLETE_FN>,                                                               \
	     &::__basic_open_function<_ENCODE_STATE>, &::__basic_close_function<_ENCODE_STATE>))

#define _ADD_MCN_NAMED_ENCODING(_NAME, _SUFFIX)                                                 \
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, (const ztd_char8_t*)(_NAME),         \
	     ::cnc::__cnc_detail::__utf32_name(),                                                  \
	     &__typical_multi_conversion<ztd_char_t, ztd_char32_t,                                 \
	          decltype(&::cnc_mcsnrtoc32sn_##_SUFFIX), &::cnc_mcsnrtoc32sn_##_SUFFIX>,         \
	     &__typical_single_conversion<ztd_char_t, ztd_char32_t,                                \
	          decltype(&::cnc_mcnrtoc32n_##_SUFFIX), &::cnc_mcnrtoc32n_##_SUFFIX>,             \
	     &::__typical_state_is_complete_function, &::__typical_open_function,                  \
	     &::__typical_close_function));                                                        \
	_CHECK_ERR_AND_RETURN(cnc_registry_add_c8(__registry, ::cnc::__cnc_detail::__utf32_name(), \
	     (const ztd_char8_t*)(_NAME),                                                          \
	     &__typical_multi_conversion<ztd_char32_t, ztd_char_t,                                 \
	          decltype(&::cnc_c32snrtomcsn_##_SUFFIX), &::cnc_c32snrtomcsn_##_SUFFIX>,         \
	     &__typical_single_conversion<ztd_char32_t, ztd_char_t,                                \
	          decltype(&::cnc_c32nrtomcn_##_SUFFIX), &::cnc_c32nrtomcn_##_SUFFIX>,             \
	     &::__typical_state_is_complete_function, &::__typical_open_function,                  \
	     &::__typical_close_function))

	_ADD_MCN_NAMED_ENCODING("ascii", ascii);
	_ADD_MCN_NAMED_ENCODING("shift-jis", shift_jis);
	_ADD_MCN_NAMED_ENCODING("gbk", gbk);
	_ADD_MCN_NAMED_ENCODING("big5-hkscs", big5_hkscs);
	_ADD_MCN_NAMED_ENCODING("gb18030", gb18030);

	_ADD_MCN_NAMED_ENCODING_BASIC("punycode", punycode, cnc_pny_decode_state_t,
	     &cnc_pny_decode_state_is_complete, cnc_pny_encode_state_t,
	     &cnc_pny_encode_state_is_complete);
	_ADD_MCN_NAMED_ENCODING_BASIC("punycode idna", punycode_idna, cnc_pny_decode_state_t,
	     &cnc_pny_decode_state_is_complete, cnc_pny_encode_state_t,
	     &cnc_pny_encode_state_is_complete);

#undef _ADD_MCN_NAMED_ENCODING
#undef _ADD_MCN_NAMED_ENCODING_BASIC
#undef _CHECK_ERR_AND_RETURN

	return CNC_OPEN_ERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_conversion** __out_p_conversion, cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__from == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__to == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	size_t __from_size = ::ztd::c_string_ptr_size(__from);
	size_t __to_size   = ::ztd::c_string_ptr_size(__to);
	return cnc_conv_new_c8n(
	     __registry, __from_size, __from, __to_size, __to, __out_p_conversion, __p_info);
}



ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new(
     cnc_conversion_registry* __registry, const char* __from, const char* __to,
     cnc_conversion** __out_p_conversion, cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_new_c8(__registry, reinterpret_cast<const ztd_char8_t*>(__from),
	     reinterpret_cast<const ztd_char8_t*>(__to), __out_p_conversion, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)],
     cnc_indirect_selection_c8_function __selection, cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__from_size == 0 || __from == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__to_size == 0 || __to == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__selection == nullptr) {
		__selection = reinterpret_cast<cnc_indirect_selection_c8_function*>(
		     &__cnc_detail_select_everything_okay);
	}
	::std::basic_string_view<ztd_char8_t> __from_view(__from, __from_size);
	::std::basic_string_view<ztd_char8_t> __to_view(__to, __to_size);
	const __cnc_registry_entry* __from_entry;
	const __cnc_registry_entry* __to_entry;
	cnc_open_error __err = ::__cnc_find_entry(
	     __registry, __from_view, __to_view, __selection, &__from_entry, &__to_entry, __p_info);
	if (__err != CNC_OPEN_ERROR_OK) {
		return __err;
	}
	constexpr const size_t __default_guess = (sizeof(cnc_conversion) + 128)
	     + ((sizeof(cnc_conversion) + 128) % alignof(cnc_conversion));
	constexpr const size_t __buffer_space
	     = (ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_ / 2) > __default_guess
	     ? (ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_ / 2)
	     : __default_guess;
	size_t __before_available_space = __buffer_space;
	size_t __after_available_space  = __buffer_space - sizeof(cnc_conversion);
	size_t __max_alignment          = alignof(cnc_conversion);
	alignas(cnc_conversion) unsigned char __faux_space_buffer[__buffer_space];
	void* __faux_space = static_cast<unsigned char*>(__faux_space_buffer) + sizeof(cnc_conversion);
	if (!__p_info->is_indirect) {
		cnc_open_error __counting_err = __from_entry->__open_function(
		     __registry, nullptr, &__after_available_space, &__max_alignment, &__faux_space);
		if (__counting_err != CNC_OPEN_ERROR_OK) {
			return __counting_err;
		}
	}
	else {
		cnc_open_error __counting_err = ::__intermediary_open_function(__registry, nullptr,
		     __from_entry, __to_entry, &__after_available_space, &__max_alignment, &__faux_space);
		if (__counting_err != CNC_OPEN_ERROR_OK) {
			return __counting_err;
		}
	}
	size_t __available_space = __before_available_space - __after_available_space;
	void* __space            = __registry->__heap.allocate(
	                __available_space, __max_alignment, &__available_space, __registry->__heap.user_data);
	if (__space == nullptr) {
		return CNC_OPEN_ERROR_ALLOCATION_FAILURE;
	}
	if (!__p_info->is_indirect) {
		return ::__cnc_open_with(
		     __registry, __from_entry, __out_p_conversion, &__available_space, &__space);
	}
	else {
		return ::__cnc_open_intermediary_with(__registry, __from_entry, __to_entry,
		     __out_p_conversion, &__available_space, &__space);
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_c8n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_new_c8n_select(
	     __registry, __from_size, __from, __to_size, __to, nullptr, __out_p_conversion, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_new_n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_new_c8n(__registry, __from_size, reinterpret_cast<const ztd_char8_t*>(__from),
	     __to_size, reinterpret_cast<const ztd_char8_t*>(__to), __out_p_conversion, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_conversion** __out_p_conversion, size_t* __p_available_space, unsigned char* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__from == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__to == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	size_t __from_size = ::ztd::c_string_ptr_size(__from);
	size_t __to_size   = ::ztd::c_string_ptr_size(__to);
	return ::cnc_conv_open_c8n(__registry, __from_size, __from, __to_size, __to,
	     __out_p_conversion, __p_available_space, __space, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8_select(
     cnc_conversion_registry* __registry, const ztd_char8_t* __from, const ztd_char8_t* __to,
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__from == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__to == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	size_t __from_size = ::ztd::c_string_ptr_size(__from);
	size_t __to_size   = ::ztd::c_string_ptr_size(__to);
	return ::cnc_conv_open_c8n_select(__registry, __from_size, __from, __to_size, __to,
	     __selection, __out_p_conversion, __p_available_space, __space, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)],
     cnc_indirect_selection_c8_function* __selection, cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__from == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__to == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	if (__p_available_space == nullptr || __space == nullptr || __p_info == nullptr) {
		return CNC_OPEN_ERROR_INVALID_PARAMETER;
	}
	::std::basic_string_view<ztd_char8_t> __from_view(__from, __from_size);
	::std::basic_string_view<ztd_char8_t> __to_view(__to, __to_size);
	if (__from_view.empty()) {
		__from_view = ::cnc::__cnc_detail::__exec_name();
	}
	else if (::ztd::is_encoding_name_equal(__from_view, ::cnc::__cnc_detail::__exec_alias())) {
		__from_view = ::cnc::__cnc_detail::__exec_name();
	}
	else if (::ztd::is_encoding_name_equal(__from_view, ::cnc::__cnc_detail::__wide_alias())) {
		__from_view = ::cnc::__cnc_detail::__wide_name();
	}
	if (__to_view.empty()) {
		__to_view = ::cnc::__cnc_detail::__utf8_name();
	}
	else if (::ztd::is_encoding_name_equal(__to_view, ::cnc::__cnc_detail::__exec_alias())) {
		__to_view = ::cnc::__cnc_detail::__exec_name();
	}
	else if (::ztd::is_encoding_name_equal(__to_view, ::cnc::__cnc_detail::__wide_alias())) {
		__to_view = ::cnc::__cnc_detail::__wide_name();
	}

	const __cnc_registry_entry* __from_entry;
	const __cnc_registry_entry* __to_entry;
	cnc_open_error err = ::__cnc_find_entry(
	     __registry, __from_view, __to_view, __selection, &__from_entry, &__to_entry, __p_info);
	if (err != CNC_OPEN_ERROR_OK) {
		return err;
	}
	if (!__p_info->is_indirect) {
		return ::__cnc_open_with(
		     __registry, __from_entry, __out_p_conversion, __p_available_space, &__space);
	}
	else {
		// we have matching entries: open an intermediary
		return ::__cnc_open_intermediary_with(__registry, __from_entry, __to_entry,
		     __out_p_conversion, __p_available_space, &__space);
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_n_select(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_indirect_selection_function* __selection,
     cnc_conversion** __out_p_conversion, size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_open_c8n_select(__registry, __from_size,
	     reinterpret_cast<const ztd_char8_t*>(__from), __to_size,
	     reinterpret_cast<const ztd_char8_t*>(__to),
	     reinterpret_cast<cnc_indirect_selection_c8_function*>(__selection), __out_p_conversion,
	     __p_available_space, __space, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_c8n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const ztd_char8_t __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const ztd_char8_t __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_open_c8n_select(__registry, __from_size, __from, __to_size, __to, nullptr,
	     __out_p_conversion, __p_available_space, __space, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_open_error cnc_conv_open_n(
     cnc_conversion_registry* __registry, size_t __from_size,
     const char __from[ZTD_PTR_EXTENT(__from_size)], size_t __to_size,
     const char __to[ZTD_PTR_EXTENT(__to_size)], cnc_conversion** __out_p_conversion,
     size_t* __p_available_space, void* __space,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_ {
	return cnc_conv_open_c8n(__registry, __from_size, reinterpret_cast<const ztd_char8_t*>(__from),
	     __to_size, reinterpret_cast<const ztd_char8_t*>(__to), __out_p_conversion,
	     __p_available_space, __space, __p_info);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void cnc_conv_close(
     cnc_conversion* __conversion) ZTD_NOEXCEPT_IF_CXX_I_ {
	__conversion->__close_function(__conversion + 1);
	__conversion->~cnc_conversion();
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ void cnc_conv_delete(
     cnc_conversion* __conversion) ZTD_NOEXCEPT_IF_CXX_I_ {
	::cnc_conv_close(__conversion);
	const cnc_conversion_heap& __heap = __conversion->__registry->__heap;
	__heap.deallocate(static_cast<unsigned char*>(static_cast<void*>(__conversion)),
	     __conversion->__size, alignof(cnc_conversion), __heap.user_data);
}
