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

#ifndef ZTD_TAG_INVOKE_TAG_INVOKE_HPP
#define ZTD_TAG_INVOKE_TAG_INVOKE_HPP

#include <ztd/tag_invoke/version.hpp>

#include <utility>
#include <type_traits>

namespace ztd {
	ZTD_TAG_INVOKE_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __tginv_detail {
		namespace __adl {

#if 0
			// poison pill
			// This is disabled in general becase the poison pill is actually creating serious problems on MSVC, GCC,
			// and under select conditions Clang. I can imagine that there's some special change made in the standard
			// between C++14/1 and C++20 and on so that this works better but I truthfully cannot be assed to figure
			// it out.
			constexpr void tag_invoke() = delete;
#endif

			template <typename _Tag, typename... _Args>
			constexpr auto __adl_tag_invoke(_Tag&& __tag, _Args&&... __args) noexcept(
			     noexcept(tag_invoke(::std::declval<_Tag>(), ::std::declval<_Args>()...)))
			     -> decltype(tag_invoke(::std::declval<_Tag>(), ::std::declval<_Args>()...)) {
				return tag_invoke(::std::forward<_Tag>(__tag), ::std::forward<_Args>(__args)...);
			}
		} // namespace __adl

		class tag_invoke_fn {
		public:
			template <typename _Tag, typename... _Args>
			constexpr auto operator()(_Tag&& __tag, _Args&&... __args) const
			     noexcept(noexcept(__adl::__adl_tag_invoke(::std::declval<_Tag>(), ::std::declval<_Args>()...)))
			          -> decltype(__adl::__adl_tag_invoke(::std::declval<_Tag>(), ::std::declval<_Args>()...)) {
				return __adl::__adl_tag_invoke(::std::forward<_Tag>(__tag), ::std::forward<_Args>(__args)...);
			}
		};
	} // namespace __tginv_detail

	inline namespace __fn {
		//////
		/// @brief The tag invoke function.
		///
		inline constexpr __tginv_detail::tag_invoke_fn tag_invoke {};
	} // namespace __fn

	namespace __tginv_detail {
		template <bool, typename _Tag, typename... _Args>
		class __is_nothrow_tag_invocable_i : public ::std::false_type { };

		template <typename _Tag, typename... _Args>
		class __is_nothrow_tag_invocable_i<true, _Tag, _Args...>
		: public ::std::integral_constant<bool, ::std::is_nothrow_invocable_v<decltype(tag_invoke), _Tag, _Args...>> {
		};
	} // namespace __tginv_detail

	//////
	/// @brief Whether or not a given tag type and its arguments are tag invocable.
	///
	template <typename _Tag, typename... _Args>
	class is_tag_invocable : public ::std::is_invocable<decltype(tag_invoke), _Tag, _Args...> { };

	//////
	/// @brief A @c _v alias for ztd::is_tag_invocable.
	///
	template <typename _Tag, typename... _Args>
	inline constexpr bool is_tag_invocable_v = is_tag_invocable<_Tag, _Args...>::value;

	//////
	/// @brief Whether or not a given tag type and its arguments are both invocable and marked as a @c noexcept
	/// invocation.
	///
	template <typename _Tag, typename... _Args>
	class is_nothrow_tag_invocable
	: public __tginv_detail::__is_nothrow_tag_invocable_i<is_tag_invocable_v<_Tag, _Args...>, _Tag, _Args...> { };

	//////
	/// @brief A @c _v alias for ztd::is_nothrow_tag_invocable.
	///
	template <typename _Tag, typename... _Args>
	inline constexpr bool is_nothrow_tag_invocable_v = is_nothrow_tag_invocable<_Tag, _Args...>::value;

	//////
	/// @brief A class representing the type that results from a tag invocation.
	///
	template <typename _Tag, typename... _Args>
	using tag_invoke_result = ::std::invoke_result<decltype(tag_invoke), _Tag, _Args...>;

	//////
	/// @brief A `_t` alias that gives the actual type that results from a tag invocation.
	///
	template <typename _Tag, typename... _Args>
	using tag_invoke_result_t = typename tag_invoke_result<_Tag, _Args...>::type;

	ZTD_TAG_INVOKE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
