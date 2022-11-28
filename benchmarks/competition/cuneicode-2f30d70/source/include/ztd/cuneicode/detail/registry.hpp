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

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_REGISTRY_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_REGISTRY_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/registry.h>
#include <ztd/cuneicode/mcchar.h>
#include <ztd/cuneicode/detail/heap_allocator.hpp>

#include <ztd/idk/hash.hpp>
#include <ztd/idk/size.hpp>
#include <ztd/idk/charN_t.hpp>
#include <ztd/idk/encoding_name.hpp>

#include <string_view>
#include <unordered_map>
#include <vector>

inline constexpr const ::std::size_t __cnc_detail_inline_size_guess
     = sizeof(size_t) > 1 ? sizeof(size_t) - 1 : sizeof(size_t);

inline bool __cnc_detail_encoding_name_code_unit_predicate(ztd_char8_t __value) noexcept {
	return ::ztd::__idk_detail::__readable_characters_v<char>.find(static_cast<char>(__value))
	     != ::std::string_view::npos;
}

inline bool __cnc_detail_encoding_name_code_unit_hash(ztd_char8_t __value) noexcept {
	::std::hash<char> __char_hasher {};
	if (__value > 'A' && __value < 'Z') {
		__value += +('a' - 'A');
	}
	return __char_hasher(static_cast<char>(__value));
}

inline constexpr bool __cnc_detail_select_everything_okay(
     size_t, const ztd_char8_t*, size_t, const ztd_char8_t*, size_t, const ztd_char8_t*) noexcept {
	return true;
}

struct __cnc_registry_entry {
	cnc_conversion_function* __multi_conversion_function;
	cnc_conversion_function* __single_conversion_function;
	cnc_state_is_complete_function* __state_is_complete_function;
	cnc_open_function* __open_function;
	cnc_close_function* __close_function;
};

struct __cnc_registry_entry_key {
	::std::basic_string_view<ztd_char8_t> __from;
	::std::basic_string_view<ztd_char8_t> __to;
};

struct __cnc_registry_entry_key_hash {
	::std::size_t operator()(const __cnc_registry_entry_key& __value) const noexcept {
		::std::size_t __from_hash = ::ztd::fnv1a_offset_basis;
		__from_hash               = ::ztd::fnv1a_hash_if(__from_hash, __value.__from,
		                   &__cnc_detail_encoding_name_code_unit_predicate,
		                   &__cnc_detail_encoding_name_code_unit_hash);
		return ::ztd::fnv1a_hash_if(__from_hash, __value.__to,
		     &__cnc_detail_encoding_name_code_unit_predicate,
		     &__cnc_detail_encoding_name_code_unit_hash);
	}
};

struct __cnc_registry_entry_key_equals {
	bool operator()(const __cnc_registry_entry_key& __left,
	     const __cnc_registry_entry_key& __right) const noexcept {
		return ::ztd::is_encoding_name_equal(__left.__from, __right.__from)
		     && ::ztd::is_encoding_name_equal(__left.__to, __right.__to);
	}
};

struct __path_key_deleter {
	cnc_conversion_heap* __heap;
	size_t __key_size;

	void operator()(ztd_char8_t* __ptr) const {
		constexpr size_t alignment = static_cast<size_t>(
		     ::cnc::__cnc_detail::__heap_allocator<ztd_char8_t>::alignment());
		unsigned char* untyped_ptr = static_cast<unsigned char*>(static_cast<void*>(__ptr));
		__heap->deallocate(untyped_ptr, __key_size, alignment, __heap->user_data);
	}
};

using __path_allocator = ::cnc::__cnc_detail::__heap_allocator<
     ::std::pair<const __cnc_registry_entry_key, __cnc_registry_entry>>;
using __path_key_allocator  = ::cnc::__cnc_detail::__heap_allocator<ztd_char8_t>;
using __path_key            = ::std::vector<ztd_char8_t, __path_key_allocator>;
using __path_keys_allocator = ::cnc::__cnc_detail::__heap_allocator<__path_key>;

struct cnc_conversion_registry {
	cnc_conversion_heap __heap;
	::std::unordered_map<__cnc_registry_entry_key, __cnc_registry_entry,
	     __cnc_registry_entry_key_hash, __cnc_registry_entry_key_equals, __path_allocator>
	     __paths;
	::std::vector<__path_key, __path_keys_allocator> __path_keys;

	cnc_conversion_registry(const cnc_conversion_heap& __heap_)
	: __heap(__heap_)
	, __paths(__path_allocator(&this->__heap))
	, __path_keys(static_cast<::std::size_t>(2),
	       __path_key { __cnc_detail_inline_size_guess, __path_key_allocator(&this->__heap) },
	       __path_keys_allocator(&this->__heap)) {
	}
};

extern cnc_open_error __cnc_add_default_registry_entries(
     cnc_conversion_registry* __registry) ZTD_NOEXCEPT_IF_CXX_I_;

extern cnc_open_error __cnc_find_entry(cnc_conversion_registry* __registry,
     ::std::basic_string_view<ztd_char8_t> __from_view,
     ::std::basic_string_view<ztd_char8_t> __to_view,
     cnc_indirect_selection_c8_function* __selection, const __cnc_registry_entry** __p_from_entry,
     const __cnc_registry_entry** __p_to_entry,
     cnc_conversion_info* __p_info) ZTD_NOEXCEPT_IF_CXX_I_;

#endif // ZTD_CUNEICODE_SOURCE_DETAIL_REGISTRY_HPP
