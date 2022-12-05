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

#ifndef ZTD_IDK_CONSTRUCT_DESTROY_HPP
#define ZTD_IDK_CONSTRUCT_DESTROY_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>

#include <iterator>
#include <memory>
#include <utility>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		template <typename _Ty, typename... _Args>
		constexpr bool __construct_at_noexcept() noexcept {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_CONSTRUCT_AT)
			return noexcept(::std::construct_at(::std::declval<_Ty*>(), ::std::declval<_Args>()...));
#else
			return ::std::is_nothrow_constructible_v<_Ty, _Args...>;
#endif
		}

		template <typename _Ty>
		constexpr bool __destroy_at_noexcept() noexcept {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_DESTROY_AT)
			return noexcept(::std::destroy_at(::std::declval<_Ty*>()));
#else
			if constexpr (::std::is_array_v<_Ty>) {
				return ::ztd::__idk_detail::__destroy_at_noexcept<::std::remove_extent_t<_Ty>>();
			}
			else {
				return ::std::is_nothrow_destructible_v<_Ty>;
			}
#endif
		}
	} // namespace __idk_detail

	//////
	/// @brief Constructs an element of `_Ty` and the given `__ptr` location, using forwarded `__args...`.
	///
	/// @tparam _Ty The type of the pointer to destroy.
	/// @tparam _Args The argument types, if any, to use to construct the pointed-to type.
	///
	/// @param[in] __ptr Location for the value to be constructed.
	/// @param[in] __args The arguments, if any, to use to cosntruct the pointed-to type.
	///
	/// @remarks There is currently no way to specify default-init with this paradigm, potentially resulting in lost
	/// performace for niche use cases (such as indeterminate initialization and partial setting for integral types
	/// used for bit vector implementations or similar constructs.)
	template <typename _Ty, typename... _Args>
	constexpr _Ty* construct_at(_Ty* __ptr, _Args&&... __args) noexcept(
	     __idk_detail::__construct_at_noexcept<_Ty, _Args...>()) {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_CONSTRUCT_AT)
		return ::std::construct_at(__ptr, ::std::forward<_Args>(__args)...);
#else
		if constexpr (::std::is_trivial_v<_Ty>) {
			// cheat attempt: just set it equals and let the rules for C++17-and-beyond constexpr kick in
			*__ptr = _Ty(::std::forward<_Args>(__args)...);
			return __ptr;
		}
		else {
			return ::new (const_cast<void*>(static_cast<const volatile void*>(__ptr)))
			     _Ty(::std::forward<_Args>(__args)...);
		}
#endif
	}

	//////
	/// @brief Destroys an object of type `_Ty` at the given location `_ptr`.
	///
	/// @tparam _Ty The type of the pointer to destroy.
	///
	/// @param[in] __ptr Location for the value to be destroyed.
	///
	/// @remarks For arrays, each element will be destroyed, including recursively into other C-array types.
	template <typename _Ty>
	constexpr void destroy_at(_Ty* __ptr) noexcept(__idk_detail::__destroy_at_noexcept<_Ty>()) {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_DESTROY_AT)
		return ::std::destroy_at(__ptr);
#else
		if constexpr (::std::is_array_v<_Ty>) {
			for (auto& __element : *__ptr) {
				::ztd::destroy_at(::std::addressof(__element));
			}
		}
		else {
			__ptr->~_Ty();
		}
#endif
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
