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

#ifndef ZTD_IDK_EMPTY_STRING_HPP
#define ZTD_IDK_EMPTY_STRING_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/charN_t.hpp>
#include <ztd/idk/type_traits.hpp>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		using __uchar_one_t                           = unsigned char[1];
		inline constexpr const __uchar_one_t __u_shim = {};

		using __schar_one_t                           = signed char[1];
		inline constexpr const __schar_one_t __s_shim = {};
	} // namespace __idk_detail

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @tparam _CharTy The character type.
	///
	/// @return An empty c-string.
	template <typename _CharTy>
	inline constexpr decltype(auto) empty_string() noexcept {
		static_assert(always_false_v<_CharTy>, "unrecognized character type");
		return "";
	}

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<char>() noexcept {
		return "";
	}

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<unsigned char>() noexcept {
		return (__idk_detail::__u_shim);
	}

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<signed char>() noexcept {
		return (__idk_detail::__s_shim);
	}

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<wchar_t>() noexcept {
		return L"";
	}

#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<char8_t>() noexcept {
		return u8"";
	}
#endif

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<char16_t>() noexcept {
		return u"";
	}

	//////
	/// @brief Returns an array representing an empty c-style string
	///
	/// @return An empty c-string.
	template <>
	inline constexpr decltype(auto) empty_string<char32_t>() noexcept {
		return U"";
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
