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

#ifndef ZTD_RANGES_DETAIL_INSERT_BULK_HPP
#define ZTD_RANGES_DETAIL_INSERT_BULK_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/range.hpp>
#include <ztd/ranges/iterator.hpp>

#include <ztd/idk/type_traits.hpp>

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {
		template <typename _OutputContainer, typename _Insertion>
		constexpr void __container_insert_bulk(_OutputContainer& __output, _Insertion&& __insertion) noexcept {
			using _Iterator = ranges::range_iterator_t<remove_cvref_t<_Insertion>>;
			if constexpr (is_detected_v<detect_insert_bulk, _OutputContainer, _Iterator, _Iterator>) {
				// inserting in bulk
				// can be faster, more performant,
				// save us some coding too
				__output.insert(__output.cend(), __insertion.begin(), __insertion.end());
			}
			else {
				// O O F! we have to insert one at a time.
				for (auto&& __value : __insertion) {
					if constexpr (is_detected_v<ranges::detect_push_back, _OutputContainer,
						              ranges::iterator_reference_t<_Iterator>>) {
						__output.push_back(::std::forward<decltype(__value)>(__value));
					}
					else {
						__output.insert(__output.cend(), ::std::forward<decltype(__value)>(__value));
					}
				}
			}
		}
	} // namespace __rng_detail

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
