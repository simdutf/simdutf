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

#ifndef ZTD_IDK_ENDIAN_H
#define ZTD_IDK_ENDIAN_H

#include <ztd/idk/version.h>

#if ZTD_IS_ON(ZTD_STDBIT_H)
#include <stdbit.h>
#endif

// clang-format off
#if defined(ZTD_CSTD_LIBRARY_ENDIAN)
	#define ZTD_CSTD_LIBRARY_ENDIAN_I_ ZTD_CSTD_LIBRARY_ENDIAN
#elif defined(__STDC_ENDIAN_LITTLE__)
	#define ZTD_CSTD_LIBRARY_ENDIAN_I_ ZTD_DEFAULT_ON
#else
	#define ZTD_CSTD_LIBRARY_ENDIAN_I_ ZTD_DEFAULT_OFF
#endif

#if ZTD_IS_OFF(ZTD_CSTD_LIBRARY_ENDIAN)
	#if ZTD_IS_ON(ZTD_PLATFORM_WINDOWS)
		#define ZTDC_LITTLE_ENDIAN 0
		#define ZTDC_BIG_ENDIAN 1
		#if defined(REG_DWORD)
			#if (REG_DWORD == REG_DWORD_LITTLE_ENDIAN)
				#define ZTDC_NATIVE_ENDIAN ZTDC_LITTLE_ENDIAN
			#else
				#define ZTDC_NATIVE_ENDIAN ZTDC_BIG_ENDIAN
			#endif
		#else
			#define ZTDC_NATIVE_ENDIAN ZTDC_LITTLE_ENDIAN
		#endif
	#else
		#define ZTDC_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
		#define ZTDC_BIG_ENDIAN __ORDER_BIG_ENDIAN__
		#define ZTDC_NATIVE_ENDIAN __BYTE_ORDER__
	#endif
#else
	#define ZTDC_LITTLE_ENDIAN __STDC_ENDIAN_LITTLE__
	#define ZTDC_BIG_ENDIAN __STDC_ENDIAN_BIG__
	#define ZTDC_NATIVE_ENDIAN __STDC_ENDIAN_NATIVE__
#endif
//clang-format on

#if defined(ZTD_DOXYGEN_PREPROCESSING) && (ZTD_DOXYGEN_PREPROCESSING != 0)
//////
/// @addtogroup ztd_idk_c_endian ztd.idk ZTDC_ENDIAN macros
///
/// @{
//////

//////
/// @brief Little endian, in which the least significant byte as the first byte value.
///
//////
#define ZTDC_LITTLE_ENDIAN
//////
/// @brief Big endian, in which the most significant byte as the first byte value.
///
//////
#define ZTDC_BIG_ENDIAN
//////
/// @brief Native endian, which is one of big, little, or some implementation-defined ordering (e.g., middle endian).
/// If it is big or little, then `ZTD_NATIVE_ENDIAN == ZTD_LITTLE_ENDIAN`, or `ZTD_NATIVE_ENDIAN == ZTD_BIG_ENDIAN`.
//////
#define ZTDC_NATIVE_ENDIAN

//////
/// @}
//////
#endif

#endif
