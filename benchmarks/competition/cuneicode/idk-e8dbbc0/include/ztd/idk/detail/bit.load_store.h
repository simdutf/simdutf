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

#ifndef ZTD_IDK_DETAIL_BIT_LOAD_STORE_H
#define ZTD_IDK_DETAIL_BIT_LOAD_STORE_H

#include <ztd/idk/version.h>

#include <ztd/idk/extent.h>
#include <ztd/idk/static_assert.h>

#if ZTD_IS_ON(ZTD_C)
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#else
#include <cstddef>
#include <climits>
#include <cstdint>
#endif

ZTD_EXTERN_C_OPEN_I_

#define ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(_N)                                                                  \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ uint_least##_N##_t ztdc_load8_leu##_N(                         \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ uint_least##_N##_t ztdc_load8_beu##_N(                         \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ uint_least##_N##_t ztdc_load8_aligned_leu##_N(                 \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ uint_least##_N##_t ztdc_load8_aligned_beu##_N(                 \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int_least##_N##_t ztdc_load8_les##_N(                          \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int_least##_N##_t ztdc_load8_bes##_N(                          \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int_least##_N##_t ztdc_load8_aligned_les##_N(                  \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int_least##_N##_t ztdc_load8_aligned_bes##_N(                  \
	     const unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;                             \
                                                                                                                     \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_leu##_N(                                      \
	     const uint_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_; \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_beu##_N(                                      \
	     const uint_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_; \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_aligned_leu##_N(                              \
	     const uint_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_; \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_aligned_beu##_N(                              \
	     const uint_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_; \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_les##_N(                                      \
	     const int_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;  \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_bes##_N(                                      \
	     const int_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;  \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_aligned_les##_N(                              \
	     const int_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;  \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ void ztdc_store8_aligned_bes##_N(                              \
	     const int_least##_N##_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_;  \
	ztd_static_assert(((_N % 8) == 0), "N must be a multiple of 8")

#if ((CHAR_BIT % 8) == 0)
#if defined(UINT_LEAST8_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(8);
#endif // 8 bits
#if defined(UINT_LEAST16_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(16);
#endif // 16 bits
#if defined(UINT_LEAST24_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(24);
#endif // 24 bits
#if defined(UINT_LEAST32_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(32);
#endif // 32 bits
#if defined(UINT_LEAST48_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(48);
#endif // 48 bits
#if defined(UINT_LEAST56_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(56);
#endif // 56 bits
#if defined(UINT_LEAST64_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(64);
#endif // 64 bits
#if defined(UINT_LEAST72_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(72);
#endif // 72 bits
#if defined(UINT_LEAST80_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(80);
#endif // 80 bits
#if defined(UINT_LEAST88_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(88);
#endif // 88 bits
#if defined(UINT_LEAST96_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(96);
#endif // 96 bits
#if defined(UINT_LEAST104_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(104);
#endif // 104 bits
#if defined(UINT_LEAST112_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(112);
#endif // 112 bits
#if defined(UINT_LEAST120_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(120);
#endif // 120 bits
#if defined(UINT_LEAST128_MAX)
ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS(128);
#endif // 128 bits
#endif // Only on 8-modulus bit systems

#undef ZTDC_GENERATE_LOAD8_STORE8_DECLARATIONS


ZTD_EXTERN_C_CLOSE_I_

#endif
