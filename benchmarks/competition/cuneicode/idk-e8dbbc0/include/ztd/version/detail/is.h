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

#ifndef ZTD_VERSION_DETAIL_IS_H
#define ZTD_VERSION_DETAIL_IS_H

#define ZTD_RAW_IS_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) != 0)
#define ZTD_RAW_IS_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3) == 0)
#define ZTD_RAW_IS_DEFAULT_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) > 3)
#define ZTD_RAW_IS_DEFAULT_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3 OP_SYMBOL 3) < 0)

#define ZTD_IS_ON(OP_SYMBOL) ZTD_RAW_IS_ON(OP_SYMBOL##_I_)
#define ZTD_IS_OFF(OP_SYMBOL) ZTD_RAW_IS_OFF(OP_SYMBOL##_I_)
#define ZTD_IS_DEFAULT_ON(OP_SYMBOL) ZTD_RAW_IS_DEFAULT_ON(OP_SYMBOL##_I_)
#define ZTD_IS_DEFAULT_OFF(OP_SYMBOL) ZTD_RAW_IS_DEFAULT_OFF(OP_SYMBOL##_I_)

#define ZTD_ON |
#define ZTD_OFF ^
#define ZTD_DEFAULT_ON +
#define ZTD_DEFAULT_OFF -

#define ZTD_TOKEN_TO_STRING_POST_EXPANSION_I_(_TOKEN) #_TOKEN
#define ZTD_TOKEN_TO_STRING_I_(_TOKEN) ZTD_TOKEN_TO_STRING_POST_EXPANSION_I_(_TOKEN)

#define ZTD_CONCAT_TOKENS_POST_EXPANSION_I_(_LEFT, _RIGHT) _LEFT##_RIGHT
#define ZTD_CONCAT_TOKENS_I_(_LEFT, _RIGHT) ZTD_CONCAT_TOKENS_POST_EXPANSION_I_(_LEFT, _RIGHT)

#define ZTD_TOKEN_EXPAND_I_(...) __VA_ARGS__

// clang-format off
#if defined(ZTD_CXX)
	#if (ZTD_CXX != 0)
		#define ZTD_CXX_I_ ZTD_ON
	#else
		#define ZTD_CXX_I_ ZTD_OFF
	#endif
#elif defined(__cplusplus)
	#define ZTD_CXX_I_ ZTD_DEFAULT_ON
#else
	#define ZTD_CXX_I_ ZTD_DEFAULT_OFF
#endif

#if defined(ZTD_C)
	#if (ZTD_C != 0)
		#define ZTD_C_I_ ZTD_ON
	#else
		#define ZTD_C_I_ ZTD_OFF
	#endif
#elif ZTD_IS_ON(ZTD_CXX)
	#define ZTD_C_I_ ZTD_DEFAULT_OFF
#else
	#define ZTD_C_I_ ZTD_DEFAULT_ON
#endif

#if defined(ZTD_HAS_INCLUDE)
	#define ZTD_HAS_INCLUDE_I_(...) ZTD_HAS_INCLUDE(__VA_ARGS__)
#elif defined(__has_include)
	#define ZTD_HAS_INCLUDE_I_(...) __has_include(__VA_ARGS__)
#else
	#define ZTD_HAS_INCLUDE_I_(...) 0L
#endif

#if defined(ZTD_HAS_ATTRIBUTE)
	#define ZTD_HAS_ATTRIBUTE_I_(...) ZTD_HAS_ATTRIBUTE(__VA_ARGS__)
#elif ZTD_IS_ON(ZTD_CXX) && defined(__has_cpp_attribute)
	#define ZTD_HAS_ATTRIBUTE_I_(...) __has_cpp_attribute(__VA_ARGS__)
#elif ZTD_IS_ON(ZTD_C) && defined(__has_c_attribute)
	#define ZTD_HAS_ATTRIBUTE_I_(...) __has_c_attribute(__VA_ARGS__)
#elif defined(__has_attribute)
	#define ZTD_HAS_ATTRIBUTE_I_(...) __has_attribute(__VA_ARGS__)
#else
	#define ZTD_HAS_ATTRIBUTE_I_(...) 0L
#endif

#if defined(ZTD_HAS_BUILTIN)
	#define ZTD_HAS_BUILTIN_I_(...) ZTD_HAS_BUILTIN(__VA_ARGS__)
#elif defined(__has_builtin)
	#define ZTD_HAS_BUILTIN_I_(...) __has_builtin(__VA_ARGS__)
#else
	#define ZTD_HAS_BUILTIN_I_(...) 0
#endif
// clang-format on

#endif
