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

#ifndef ZTD_IDK_REFERENCE_WRAPPER
#define ZTD_IDK_REFERENCE_WRAPPER

#include <ztd/idk/version.hpp>

#include <functional>
#include <utility>
#include <type_traits>
#include <memory>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {

		template <typename _Ty>
		class __reference_wrapper {
		private:
			using _Ref = ::std::add_lvalue_reference_t<_Ty>;
			using _Ptr = ::std::add_pointer_t<_Ty>;
			_Ptr __ptr;

		public:
			constexpr __reference_wrapper(_Ref __ref) noexcept : __ptr(::std::addressof(__ref)) {
			}

			constexpr operator _Ref() const noexcept {
				return *__ptr;
			}

			constexpr _Ref get() const noexcept {
				return *__ptr;
			}

			template <typename... _Args, ::std::enable_if_t<::std::is_invocable_v<_Ty, _Args...>>* = nullptr>
			constexpr decltype(auto) operator()(_Args&&... __args) const
			     noexcept(::std::is_invocable_v<_Ty, _Args...>) {
				return ::std::invoke(_FWD(__args)...);
			}
		};

		template <typename _Ty>
		__reference_wrapper(_Ty&) -> __reference_wrapper<_Ty>;

	} // namespace __idk_detail

	//////
	/// @brief A subsitute for C++20's reference wrapper if the current @c std::reference_wrapper provided by the
	/// standard library is not @c constexpr since it was only done then.
	///
	template <typename _Ty>
	using reference_wrapper =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_CONSTEXPR_UTILITIES)
	     ::std::reference_wrapper<_Ty>
#else
	     ::ztd::__idk_detail::__reference_wrapper<_Ty>
#endif
	     ;

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif // ZTD_IDK_REFERENCE_WRAPPER
