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

#ifndef ZTD_IDK_TO_UNDERLYING_HPP
#define ZTD_IDK_TO_UNDERLYING_HPP

#include <ztd/idk/version.hpp>

#include <type_traits>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	template <typename _Enum>
	inline constexpr ::std::underlying_type_t<_Enum> to_underlying(_Enum __value) noexcept {
		return static_cast<::std::underlying_type_t<_Enum>>(__value);
	}

	template <typename _MaybeEnum>
	inline constexpr auto any_to_underlying(_MaybeEnum __value) noexcept {
		if constexpr (::std::is_enum_v<_MaybeEnum>) {
			return ::ztd::to_underlying(__value);
		}
		else {
			static_assert(::std::is_integral_v<_MaybeEnum>);
			return __value;
		}
	}

	template <typename _Integralish>
	inline constexpr auto any_enum_or_char_to_underlying(_Integralish __val) noexcept {
		if constexpr (::std::is_same_v<_Integralish, char>) {
			using _UTy = ::std::conditional_t<::std::is_signed_v<char>, ::std::int_least8_t, ::std::uint_least8_t>;
			return static_cast<_UTy>(__val);
		}
		else if constexpr (::std::is_same_v<_Integralish, wchar_t>) {
			if constexpr (sizeof(wchar_t) <= sizeof(::std::uint_least8_t)) {
				using _UTy
				     = ::std::conditional_t<::std::is_signed_v<wchar_t>, ::std::int_least8_t, ::std::uint_least8_t>;
				return static_cast<_UTy>(__val);
			}
			else if constexpr (sizeof(wchar_t) <= sizeof(::std::uint_least16_t)) {
				using _UTy = ::std::conditional_t<::std::is_signed_v<wchar_t>, ::std::int_least16_t,
				     ::std::uint_least16_t>;
				return static_cast<_UTy>(__val);
			}
			else if constexpr (sizeof(wchar_t) <= sizeof(::std::uint_least32_t)) {
				using _UTy = ::std::conditional_t<::std::is_signed_v<wchar_t>, ::std::int_least32_t,
				     ::std::uint_least32_t>;
				return static_cast<_UTy>(__val);
			}
			else {
				static_assert(sizeof(wchar_t) <= sizeof(::std::uint_least64_t),
				     "[ztd.idk] The size of wchar_t exceeds what is anticiapted for "
				     "ztd::any_enum_or_char_to_underlying(...).");
				using _UTy = ::std::conditional_t<::std::is_signed_v<wchar_t>, ::std::int_least64_t,
				     ::std::uint_least64_t>;
				return static_cast<_UTy>(__val);
			}
		}
#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
		else if constexpr (::std::is_same_v<_Integralish, char8_t>) {
			return static_cast<unsigned char>(__val);
		}
#endif // char8_t
		else if constexpr (::std::is_same_v<_Integralish, char16_t>) {
			return static_cast<uint_least16_t>(__val);
		}
		else if constexpr (::std::is_same_v<_Integralish, char32_t>) {
			return static_cast<uint_least32_t>(__val);
		}
		else {
			return any_to_underlying(__val);
		}
	}

	namespace __idk_detail {

		template <typename _Enumish, typename = void>
		struct __any_to_underlying {
			using type = _Enumish;
		};

		template <typename _Enumish>
		struct __any_to_underlying<_Enumish, ::std::enable_if_t<::std::is_enum_v<_Enumish>>> {
			using type = ::std::underlying_type_t<_Enumish>;
		};
	} // namespace __idk_detail

	template <typename _Enumish>
	using any_to_underlying_t = typename ::ztd::__idk_detail::__any_to_underlying<_Enumish>::type;

	template <typename _Enumish>
	using any_enum_or_char_to_underlying_t
	     = decltype(::ztd::any_enum_or_char_to_underlying(::std::declval<_Enumish>()));

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
