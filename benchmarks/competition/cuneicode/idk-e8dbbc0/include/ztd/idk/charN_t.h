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

#ifndef ZTD_IDK_CHARN_T_H
#define ZTD_IDK_CHARN_T_H

#include <ztd/idk/version.h>

//////
/// @file charN_t.h
///
/// @brief Code unit types.
//////

//////
/// @addtogroup ztd_idk_c_charN_t ztd.idk char8/16/32_t code unit type definitions
///
/// @{
//////

typedef char ztd_char_t;
typedef ZTD_CHAR8_T_I_ ztd_char8_t;

// clang-format off
#if ZTD_IS_ON(ZTD_CXX)
		typedef wchar_t ztd_wchar_t;
		typedef char16_t ztd_char16_t;
		typedef char32_t ztd_char32_t;
#else
	#if ZTD_IS_ON(ZTD_UCHAR)
		#include <uchar.h>

		typedef char16_t ztd_char16_t;
		typedef char32_t ztd_char32_t;
	#else
		#if ZTD_IS_ON(ZTD_CXX)
			#include <cstdint>
		#else
			#include <stdint.h>
		#endif

		typedef uint_least16_t ztd_char16_t;
		typedef uint_least32_t ztd_char32_t;
	#endif

	#if ZTD_IS_ON(ZTD_WCHAR)
		#include <wchar.h>

		typedef wchar_t ztd_wchar_t;
	#else
		#if ZTD_IS_ON(ZTD_CXX)
			#include <cstdint>
		#else
			#include <stdint.h>
		#endif

		#if ZTD_IS_ON(ZTD_TEXT_PLATFORM_WINDOWS)
			typedef uint_least16_t ztd_wchar_t;
		#else
			typedef uint_least32_t ztd_wchar_t;
		#endif
	#endif
#endif
// clang-format on


//////
/// @typedef ztd_char_t
///
/// @brief An alias to `char`.
///
/// @remarks This is simply for consistency with the other character types in ztd. It's not strictly necessary, but it's
/// nice!
//////

//////
/// @typedef ztd_wchar_t
///
/// @brief An alias to a unsigned representation of an implementation-defined code unit type.
///
/// @remarks This will be a type alias for `wchar_t` from `<wchar.h>` if the header exists. Otherwise, it will use one
/// of `uint_least16_t` (for Windows or Windows-alike platforms) or `uint_least32_t` (for all other platforms).
//////

//////
/// @typedef ztd_char8_t
///
/// @brief An alias to a unsigned representation of an 8-bit (or greater) code unit type.
///
/// @remarks This will be a type alias for the type given in `ZTD_CHAR8_T` if it is defined by the user. Otherwise,
/// it will be a type alias for `char8_t` if present. If neither are available, it will alias `unsigned char`
/// for the type.
//////

//////
/// @typedef ztd_char16_t
///
/// @brief An alias to a unsigned representation of an 16-bit (or greater) code unit type.
///
/// @remarks Certain platforms lack the header `uchar.h`, and therefore sometimes this will be aliased to its
/// standard-defined `uint_least16_t` rather than just `char16_t`.
//////

//////
/// @typedef ztd_char32_t
///
/// @brief An alias to a unsigned representation of an 32-bit (or greater) code unit type.
///
/// @remarks Certain platforms lack the header `uchar.h`, and therefore sometimes this will be aliased to its
/// standard-defined `uint_least32_t` rather than just `char32_t`.
//////

//////
/// @}
//////

#include <ztd/epilogue.hpp>

#endif
