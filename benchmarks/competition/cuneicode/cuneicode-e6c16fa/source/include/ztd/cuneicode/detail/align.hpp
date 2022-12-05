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

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_ALIGN_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_ALIGN_HPP

#include <ztd/cuneicode/version.h>

#include <cstddef>
#include <cstdint>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __cnc_detail {

		inline void* __align(::std::size_t alignment, ::std::size_t size, void*& ptr,
		     ::std::size_t& space, ::std::size_t& required_space) {
			::std::uintptr_t initial = reinterpret_cast<::std::uintptr_t>(ptr);
			::std::uintptr_t offby   = static_cast<::std::uintptr_t>(initial % alignment);
			::std::uintptr_t padding = (alignment - offby) % alignment;
			required_space += size + padding;
			if (space < required_space) {
				return nullptr;
			}
			ptr = static_cast<void*>(static_cast<char*>(ptr) + padding);
			space -= padding;
			return ptr;
		}

		inline void* __align(
		     ::std::size_t alignment, ::std::size_t size, void*& ptr, ::std::size_t& space) {
			::std::size_t required_space = 0;
			return __align(alignment, size, ptr, space, required_space);
		}

	} // namespace __cnc_detail
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif
