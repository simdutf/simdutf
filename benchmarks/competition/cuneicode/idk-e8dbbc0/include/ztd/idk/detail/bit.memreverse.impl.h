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

#ifndef ZTD_IDK_DETAIL_BIT_MEMREVERSE_IMPL_H
#define ZTD_IDK_DETAIL_BIT_MEMREVERSE_IMPL_H

#include <ztd/idk/version.h>

#include <ztd/idk/bit.h>

#include <ztd/idk/extent.h>
#include <ztd/idk/static_assert.h>

#if ZTD_IS_ON(ZTD_C)
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#else
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <climits>
#endif

#define __ZTDC_MEMREVERSE8_IMPL(_TYPE, _PTR_COUNT, _PTR)                                                       \
	for (size_t __index = 0, __limit = ((_PTR_COUNT * (sizeof(_TYPE) * CHAR_BIT)) / 2); __index < __limit;) { \
		const size_t __ptr_index         = __index / (sizeof(_TYPE) * CHAR_BIT);                             \
		const size_t __reverse_ptr_index = _PTR_COUNT - 1 - __ptr_index;                                     \
		_TYPE* __p                       = _PTR + __ptr_index;                                               \
		_TYPE* __reverse_p               = _PTR + __reverse_ptr_index;                                       \
		const _TYPE __b_temp             = *__p;                                                             \
		const _TYPE __reverse_b_temp     = *__reverse_p;                                                     \
		*__p                             = 0;                                                                \
		*__reverse_p                     = 0;                                                                \
		for (size_t __bit_index = 0, __bit_limit = ((sizeof(_TYPE) * CHAR_BIT)); __bit_index < __bit_limit;  \
		     __bit_index += 8) {                                                                             \
			const size_t __reverse_bit_index = (sizeof(_TYPE) * CHAR_BIT) - 8 - __bit_index;                \
			const _TYPE __bit_mask           = ((_TYPE)0xFF) << __bit_index;                                \
			const _TYPE __reverse_bit_mask   = ((_TYPE)0xFF) << __reverse_bit_index;                        \
			*__p |= (((__reverse_b_temp & __reverse_bit_mask) >> __reverse_bit_index) << __bit_index);      \
			*__reverse_p |= (((__b_temp & __bit_mask) >> __bit_index) << __reverse_bit_index);              \
			__index += 8;                                                                                   \
		}                                                                                                    \
	}                                                                                                         \
	ztd_static_assert(true, "end-of-macro")

#endif
