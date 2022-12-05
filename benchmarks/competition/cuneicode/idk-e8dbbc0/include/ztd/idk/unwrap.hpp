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

#ifndef ZTD_IDK_UNWRAP_HPP
#define ZTD_IDK_UNWRAP_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/to_address.hpp>
#include <ztd/idk/contiguous_iterator_tag.hpp>
#include <ztd/idk/hijack.hpp>

#include <utility>
#include <iterator>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		template <typename _Type>
		using __is_unwrappable_value_test = decltype(unwrap(::std::declval<_Type>()));

		template <typename _Type>
		using __is_unwrappable_iter_value_test = decltype(unwrap_iterator(::std::declval<_Type>()));
	} // namespace __idk_detail

	//////
	/// @brief Test whether a type can have `unwrap(...)` called on it.
	template <typename _Type>
	inline constexpr bool is_unwrappable_value_v
	     = ztd::is_detected_v<__idk_detail::__is_unwrappable_value_test, _Type>;

	//////
	/// @brief Test whether a type can have `unwrap_iterator(...)` called on it.
	template <typename _Type>
	inline constexpr bool is_unwrappable_iterator_value_v
	     = ztd::is_detected_v<__idk_detail::__is_unwrappable_iter_value_test, _Type>;

	namespace __idk_detail {
		class __unwrap_fn : public ::ztd::hijack::token<__unwrap_fn>, public ::ztd_hijack_global_token<__unwrap_fn> {
		public:
			template <typename _Type>
			constexpr decltype(auto) operator()(_Type&& __value) const noexcept {
				if constexpr (is_specialization_of_v<remove_cvref_t<_Type>, ::std::reference_wrapper>) {
					return __value.get();
				}
				else if constexpr (is_unwrappable_value_v<_Type>) {
					return unwrap(::std::forward<_Type>(__value));
				}
				else {
					return ::std::forward<_Type>(__value);
				}
			}
		};
	} // namespace __idk_detail

	inline namespace __fn {
		//////
		/// @brief Unwraps a value, if possible. Otherwise, simply forwards the input value through.
		///
		/// @returns The unwrapped value.
		inline constexpr __idk_detail::__unwrap_fn unwrap = {};
	} // namespace __fn


	namespace __idk_detail {
		class __unwrap_iterator_fn : public ::ztd::hijack::token<__unwrap_iterator_fn>,
		                             public ::ztd_hijack_global_token<__unwrap_iterator_fn> {
		public:
			template <typename _Type>
			constexpr decltype(auto) operator()(_Type&& __value) const noexcept {
				if constexpr (is_unwrappable_iterator_value_v<_Type>) {
					return unwrap_iterator(::std::forward<_Type>(__value));
				}
				else {
					return ::ztd::unwrap(*::std::forward<_Type>(__value));
				}
			}
		};
	} // namespace __idk_detail

	inline namespace __fn {
		//////
		/// @brief Unwraps either an iterator, or unwraps the value and returns its address, or forwards the input
		/// value through.
		///
		/// @returns The iterator's unwrapped value.
		inline constexpr __idk_detail::__unwrap_iterator_fn unwrap_iterator = {};
	} // namespace __fn

	//////
	/// @brief Retrives the unwrapped type if the object were put through a call to ztd::unwrap.
	///
	/// @remarks Typically used to get the type underlying a `std::reference_wrapper` or similar.
	template <typename _Type>
	using unwrap_t = decltype(::ztd::unwrap(::std::declval<_Type>()));

	//////
	/// @brief Retrives the unwrapped type if the object were put through a call to ztd::unwrap.
	///
	/// @remarks Typically used to get the type underlying a `std::reference_wrapper` or similar.
	template <typename _Type>
	using unwrap_iterator_t = decltype(::ztd::unwrap_iterator(::std::declval<_Type>()));

	//////
	/// @brief Retrives the unwrapped type if the object were put through a call to ztd::unwrap.
	///
	/// @remarks Typically used to get the type underlying a `std::reference_wrapper` or similar.
	template <typename _Type>
	using unwrap_remove_cvref_t = remove_cvref_t<unwrap_t<_Type>>;

	//////
	/// @brief Retrives the unwrapped type if the object were put through a call to ztd::unwrap.
	///
	/// @remarks Typically used to get the type underlying a `std::reference_wrapper` or similar.
	template <typename _Type>
	using unwrap_remove_reference_t = ::std::remove_reference_t<unwrap_t<_Type>>;

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
