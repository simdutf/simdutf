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

#ifndef ZTD_IDK_UNINIT_HPP
#define ZTD_IDK_UNINIT_HPP

#include <ztd/idk/version.hpp>
#include <ztd/idk/unwrap.hpp>

#include <utility>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief A class for holding a value inside of an unnamed union which is composed of two objects, one of `char`
	/// and one of `_Type`.
	template <typename _Type>
	class alignas(_Type) uninit {
	public:
		//////
		/// @brief Constructs an empty placeholder.
		constexpr uninit() : placeholder() {
		}

		//////
		/// @brief Constructs the `value` from the given arguments
		///
		/// @param[in] __args The arguments to construct `value` with.
		template <typename... _Args>
		constexpr uninit(::std::in_place_t, _Args&&... __args) : value(::std::forward<_Args>(__args)...) {
		}

		//////
		/// @brief Extension point for returning the value inside of this uninitialized type.
		friend _Type& unwrap(uninit& __wrapped_value) noexcept {
			return __wrapped_value.value;
		}

		//////
		/// @brief Extension point for returning the value inside of this uninitialized type.
		friend const _Type& unwrap(const uninit& __wrapped_value) noexcept {
			return __wrapped_value.value;
		}

		//////
		/// @brief Extension point for returning the value inside of this uninitialized type.
		friend _Type&& unwrap(uninit&& __wrapped_value) noexcept {
			return ::std::move(__wrapped_value.value);
		}

		union {
			//////
			/// @brief Placeholder empty value for default / empty  initialization, esp. with arrays.
			char placeholder;
			//////
			/// @brief Actual value.
			_Type value;
		};

		//////
		/// @brief An empty destructor. Required, as there is a union object present.
		~uninit() {
			// No action.
		}
	};


	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
