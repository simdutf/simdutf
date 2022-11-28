// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#ifndef ZTD_CUNEICODE_VERSION_H
#define ZTD_CUNEICODE_VERSION_H

#pragma once

#include <ztd/version.h>

// clang-format off

#if defined(ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE)
	#define ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_ ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE
#else
	#define ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_ ZTD_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_
#endif // Intermediate buffer sizing

#if defined(ZTD_CUNEICODE_ABI_NAMESPACE)
	#define ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_ inline namespace ZTD_CUNEICODE_ABI_NAMESPACE {
	#define ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_ }
#else
	#define ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_ inline namespace __v0 {
	#define ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_ }
#endif

// clang-format on

#include <ztd/cuneicode/detail/api.h>

#endif // ZTD_CUNEICODE_VERSION_H
