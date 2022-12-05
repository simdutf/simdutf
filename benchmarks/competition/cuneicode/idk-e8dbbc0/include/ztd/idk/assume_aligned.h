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

#ifndef ZTD_IDK_ASSUME_ALIGNED_H
#define ZTD_IDK_ASSUME_ALIGNED_H

#include <ztd/idk/version.h>

// clang-format off
#if ZTD_HAS_BUILTIN_I_(__builtin_assume_aligned)
	#define ZTD_ASSUME_ALIGNED_C(_ALIGNMENT, ...) __builtin_assume_aligned((__VA_ARGS__), _ALIGNMENT)
#else
	#define ZTD_ASSUME_ALIGNED_C(_ALIGNMENT, ...) __VA_ARGS__
#endif

#if ZTD_IS_ON(ZTD_C)
	#define ZTD_ASSUME_ALIGNED(_ALIGNMENT, ...) ZTD_ASSUME_ALIGNED_C(_ALIGNMENT, __VA_ARGS__)
	#define ZTD_ASSUME_ALIGNED_CXX(_ALIGNMENT, ...) ZTD_ASSUME_ALIGNED(_ALIGNMENT, __VA_ARGS__)
#else
	#include <ztd/idk/version.hpp>

	#if ZTD_IS_ON(ZTD_STD_LIBRARY_ASSUME_ALIGNED)
		#include <memory>

		#define ZTD_ASSUME_ALIGNED_CXX(_ALIGNMENT, ...) ::std::assume_aligned<_ALIGNMENT>(__VA_ARGS__);
	#else
		#define ZTD_ASSUME_ALIGNED_CXX(_ALIGNMENT, ...) ZTD_ASSUME_ALIGNED_C(_ALIGNMENT, __VA_ARGS__)
	#endif

	#define ZTD_ASSUME_ALIGNED(_ALIGNMENT, ...) ZTD_ASSUME_ALIGNED_CXX(_ALIGNMENT, __VA_ARGS__)
#endif
// clang-format on

//////
/// @addtogroup ztd_idk_assume_aligned Assume Aligned
///
/// @{
//////

//////
/// @def ZTD_ASSUME_ALIGNED(_ALIGNMENT, ...)
///
/// @brief Returns a pointer suitable-aligned for `_ALIGNMENT`.
///
/// @param[in] _ALIGNMENT An integer constant expression indicating the alignment of the pointer value.
/// @param[in] ... The pointer to assume alignment of.
///
/// @returns A pointer (assumed to be) suitably-aligned to `_ALIGNMENT`.
///
/// @remarks This function does NOT align the pointer, just marks it as such. This uses builtins or other tricks
/// depending on the compiler. It can trigger Undefined Behavior if it is not properly checked and protected against, so
/// make sure the pointer is properly aligned.
//////

//////
/// @}
//////


#endif
