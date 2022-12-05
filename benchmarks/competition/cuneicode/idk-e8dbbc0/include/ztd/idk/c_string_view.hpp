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

#ifndef ZTD_IDK_C_STRING_VIEW_HPP
#define ZTD_IDK_C_STRING_VIEW_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/basic_c_string_view.hpp>
#include <ztd/idk/charN_t.hpp>
#include <ztd/idk/type_traits.hpp>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief A basic_c_string_view for a `char` sequence.
	using c_string_view = basic_c_string_view<char>;
	//////
	/// @brief A basic_c_string_view for a `wchar_t` sequence.
	using nc_string_view = basic_c_string_view<char>;
	//////
	/// @brief A basic_c_string_view for a `wchar_t` sequence.
	using wc_string_view = basic_c_string_view<wchar_t>;
	//////
	/// @brief A basic_c_string_view for a `char8_t` sequence.
	using u8c_string_view = basic_c_string_view<uchar8_t>;
	//////
	/// @brief A basic_c_string_view for a `char16_t` sequence.
	using u16c_string_view = basic_c_string_view<char16_t>;
	//////
	/// @brief A basic_c_string_view for a `char32_t` sequence.
	using u32c_string_view = basic_c_string_view<char32_t>;

	inline namespace literals { inline namespace string_view_literals {
		//////
		/// @brief A literal suffix to obtain a ztd::c_string_view from a @c "abc" string literal.
		inline constexpr c_string_view operator"" _csv(const char* __str, size_t __len) noexcept {
			return c_string_view(__str, __len);
		}

		//////
		/// @brief A literal suffix to obtain a ztd::c_string_view from a @c "abc" string literal.
		inline constexpr nc_string_view operator"" _ncsv(const char* __str, size_t __len) noexcept {
			return nc_string_view(__str, __len);
		}

		//////
		/// @brief A literal suffix to obtain a ztd::wc_string_view from a `L`"abc" string literal.
		inline constexpr wc_string_view operator"" _wcsv(const wchar_t* __str, size_t __len) noexcept {
			return wc_string_view(__str, __len);
		}

#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
		//////
		/// @brief A literal suffix to obtain a ztd::u8c_string_view from a `u8`"abc" string literal.
		inline constexpr u8c_string_view operator"" _u8csv(const char8_t* __str, size_t __len) noexcept {
			if constexpr (::std::is_same_v<uchar8_t, char8_t>) {
				return u8c_string_view(__str, __len);
			}
			else {
				return u8c_string_view(reinterpret_cast<const uchar8_t*>(__str), __len);
			}
		}
#else
		//////
		/// @brief A literal suffix to obtain a ztd::u8c_string_view from a `u8`"abc" string literal.
		inline u8c_string_view operator"" _u8csv(const char* __str, size_t __len) noexcept {
			return u8c_string_view(reinterpret_cast<const uchar8_t*>(__str), __len);
		}
#endif // char8_t hacks

		//////
		/// @brief A literal suffix to obtain a ztd::u16c_string_view from a `u`"abc" string literal.
		inline constexpr u16c_string_view operator"" _u16csv(const char16_t* __str, size_t __len) noexcept {
			return u16c_string_view(__str, __len);
		}

		//////
		/// @brief A literal suffix to obtain a ztd::u32c_string_view from a `U`"abc" string literal.
		inline constexpr u32c_string_view operator"" _u32csv(const char32_t* __str, size_t __len) noexcept {
			return u32c_string_view(__str, __len);
		}
	}} // namespace literals::string_view_literals

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
