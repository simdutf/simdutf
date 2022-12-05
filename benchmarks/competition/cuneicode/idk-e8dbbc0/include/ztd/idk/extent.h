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

#ifndef ZTD_IDK_EXTENT_H
#define ZTD_IDK_EXTENT_H

#include <ztd/idk/version.h>

//////
///	@addtogroup ztd_idk_extent Extent Utilities
///
/// @{
//////

//////
/// @brief Provides the `T arg[static N]` functionality ("sized at least `N` large" hint).
///
/// @param[in] ... An expression which computes the intended size of the pointer argument.
///
/// @remarks Expands to the proper notation for C compilers, and expands to nothing for C++ compilers. It is meant to be
/// used as in the declaration: `void f(T arg[ZTD_PTR_EXTENT(N)]);`.
//////
#define ZTD_PTR_EXTENT(...) ZTD_STATIC_PTR_EXTENT_I_(__VA_ARGS__)

//////
/// @}
//////

#endif
