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

#ifndef ZTD_VERSION_DETAIL_API_H
#define ZTD_VERSION_DETAIL_API_H

#include <ztd/version/detail/is.h>
#include <ztd/version/detail/version.c.h>

// clang-format off

#if defined(ZTD_IDK_BUILD)
	#if (ZTD_IDK_BUILD != 0)
		#define ZTD_IDK_BUILD_I_ ZTD_ON
	#else
		#define ZTD_IDK_BUILD_I_ ZTD_OFF
	#endif
#else
	#define ZTD_IDK_BUILD_I_ ZTD_DEFAULT_OFF
#endif // Building or not

#if defined(ZTD_IDK_DLL)
	#if (ZTD_IDK_DLL != 0)
		#define ZTD_IDK_DLL_I_ ZTD_ON
	#else
		#define ZTD_IDK_DLL_I_ ZTD_ON
	#endif
#else
	#define ZTD_IDK_DLL_I_ ZTD_OFF
#endif // Building or not

#if defined(ZTD_HEADER_ONLY)
	#if (ZTD_HEADER_ONLY != 0)
		#define ZTD_HEADER_ONLY_I_ ZTD_ON
	#else
		#define ZTD_HEADER_ONLY_I_ ZTD_OFF
	#endif
#else
	#define ZTD_HEADER_ONLY_I_ ZTD_DEFAULT_OFF
#endif // Header only library

#if defined(ZTD_INLINE_BUILD)
	#if (ZTD_INLINE_BUILD != 0)
		#define ZTD_INLINE_BUILD_I_ ZTD_ON
	#else
		#define ZTD_INLINE_BUILD_I_ ZTD_OFF
	#endif
#else
	#define ZTD_INLINE_BUILD_I_ ZTD_DEFAULT_OFF
#endif // Header only library

#if defined(ZTD_EXTERN_C)
	#define ZTD_EXTERN_C_I_ ZTD_EXTERN_C
#else
	#if ZTD_IS_ON(ZTD_CXX)
		// C++
		#define ZTD_EXTERN_C_I_ extern "C"
	#else
		// normal
		#define ZTD_EXTERN_C_I_
	#endif // C++ or not
#endif // Linkage specification

#if defined(ZTD_EXTERN_IF_CXX)
	#define ZTD_EXTERN_IF_CXX_I_ ZTD_EXTERN_IF_CXX
#else
	#if ZTD_IS_ON(ZTD_CXX)
		#define ZTD_EXTERN_IF_CXX_I_ extern
	#else
		#define ZTD_EXTERN_IF_CXX_I_
	#endif
#endif // Linkage specification

#if defined(ZTD_EXTERN_IF_C)
	#define ZTD_EXTERN_IF_C_I_ ZTD_EXTERN_IF_C
#else
	#if ZTD_IS_ON(ZTD_CXX)
		#define ZTD_EXTERN_IF_C_I_
	#else
		#define ZTD_EXTERN_IF_C_I_ extern
	#endif
#endif // Linkage specification

#if defined(ZTD_C_FUNCTION_LINKAGE)
	#define ZTD_C_FUNCTION_LINKAGE_I_ ZTD_C_FUNCTION_LINKAGE
#else
	#define ZTD_C_FUNCTION_LINKAGE_I_ ZTD_EXTERN_C_I_
#endif // Linkage specification

#if defined(ZTD_C_FUNCTION_INLINE)
	#define ZTD_C_FUNCTION_INLINE_I_ ZTD_C_FUNCTION_INLINE
#else
	#if ZTD_IS_ON(ZTD_CXX)
		// C++
		#define ZTD_C_FUNCTION_INLINE_I_ inline
	#else
		// normal
		#define ZTD_C_FUNCTION_INLINE_I_
	#endif // C++ or not
#endif // Linkage specification

#if defined(ZTD_C_STRUCT_LINKAGE)
	#define ZTD_C_STRUCT_LINKAGE_I_ ZTD_C_STRUCT_LINKAGE
#else
	#if ZTD_IS_ON(ZTD_CXX)
		// C++
		#define ZTD_C_STRUCT_LINKAGE_I_ extern "C"
	#else
		// normal
		#define ZTD_C_STRUCT_LINKAGE_I_
	#endif // C++ or not
#endif // Linkage specification

#if defined(ZTD_API_LINKAGE)
	#define ZTD_API_LINKAGE_I_ ZTD_API_LINKAGE
#else
	#if ZTD_IS_ON(ZTD_IDK_DLL)
		#if ZTD_IS_ON(ZTD_COMPILER_VCXX) || ZTD_IS_ON(ZTD_PLATFORM_WINDOWS) || ZTD_IS_ON(ZTD_PLATFORM_CYGWIN)
			// MSVC Compiler; or, Windows, or Cygwin platforms
			#if ZTD_IS_ON(ZTD_IDK_BUILD)
				// Building the library
				#if ZTD_IS_ON(ZTD_COMPILER_GCC)
					// Using GCC
					#define ZTD_API_LINKAGE_I_ __attribute__((dllexport))
				#else
					// Using Clang, MSVC, etc...
					#define ZTD_API_LINKAGE_I_ __declspec(dllexport)
				#endif
			#else
				#if ZTD_IS_ON(ZTD_COMPILER_GCC)
					#define ZTD_API_LINKAGE_I_ __attribute__((dllimport))
				#else
					#define ZTD_API_LINKAGE_I_ __declspec(dllimport)
				#endif
			#endif
		#else
			#define ZTD_API_LINKAGE_I_
		#endif
	#elif ZTD_IS_ON(ZTD_INLINE_BUILD)
		// Built-in library, like how stb typical works
		#if ZTD_IS_ON(ZTD_HEADER_ONLY)
			// Header only, so functions are defined "inline"
			#define ZTD_API_LINKAGE_I_ inline
		#else
			// Not header only, so seperately compiled files
			#define ZTD_API_LINKAGE_I_
		#endif
	#else
		#define ZTD_API_LINKAGE_I_
	#endif // DLL or not
#endif // Build definitions


// clang-format on

#endif
