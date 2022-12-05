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

#ifndef ZTD_IDK_DETAIL_BIT_LOAD_STORE_IMPL_H
#define ZTD_IDK_DETAIL_BIT_LOAD_STORE_IMPL_H

#include <ztd/idk/version.h>

#include <ztd/idk/bit.h>

#include <ztd/idk/endian.h>
#include <ztd/idk/extent.h>
#include <ztd/idk/static_assert.h>
#include <ztd/idk/assume_aligned.h>
#include <ztd/idk/bit_width_to_max_value.h>

#if ZTD_IS_ON(ZTD_C)
#include <string.h>
#include <stdint.h>
#else
#include <cstring>
#include <cstdint>
#endif

#define ZTDC_GENERATE_LOAD8_BODY_LE_UNSIGNED_TYPE(_TYPE, _N, _PTR, _VALUE)                                         \
	typedef uint_least##_N##_t __unsigned_type;                                                                   \
	const size_t __width     = sizeof(_TYPE) * CHAR_BIT;                                                          \
	const size_t __idx_limit = (_N);                                                                              \
	const size_t __sign_idx  = (_N)-8;                                                                            \
	__unsigned_type _VALUE   = { 0 };                                                                             \
	for (size_t __idx = 0, __ptr_idx = 0; __idx < __idx_limit; __idx += 8, __ptr_idx += 8) {                      \
		const size_t __value_idx = __idx % __width;                                                              \
		const _TYPE __byte8_mask = ((_TYPE)0xFF) << __value_idx;                                                 \
		_VALUE |= (__unsigned_type)(((__unsigned_type)(_PTR[__ptr_idx / __width] & __byte8_mask) >> __value_idx) \
		     << __idx);                                                                                          \
	}                                                                                                             \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_STORE8_BODY_LE_UNSIGNED_TYPE(_TYPE, _N, _PTR, _VALUE)                                 \
	const size_t __width     = sizeof(_TYPE) * CHAR_BIT;                                                   \
	const size_t __idx_limit = (_N);                                                                       \
	const size_t __sign_idx  = (_N)-8;                                                                     \
	memset(_PTR, 0, ((_N) / __width));                                                                     \
	for (size_t __idx = 0, __ptr_idx = 0; __idx < __idx_limit; __idx += 8, __ptr_idx += 8) {               \
		const size_t __value_idx = __idx % __width;                                                       \
		const _TYPE __byte8_mask = 0xFF;                                                                  \
		_PTR[__ptr_idx / __width] |= (_TYPE)((((_TYPE)(_VALUE >> __idx)) & __byte8_mask) << __value_idx); \
	}                                                                                                      \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_LOAD8_BODY_BE_UNSIGNED_TYPE(_TYPE, _N, _PTR, _VALUE)                                           \
	typedef uint_least##_N##_t __unsigned_type;                                                                     \
	const size_t __width     = sizeof(_TYPE) * CHAR_BIT;                                                            \
	const size_t __idx_limit = 0;                                                                                   \
	const size_t __sign_idx  = 0;                                                                                   \
	__unsigned_type _VALUE   = { 0 };                                                                               \
	for (size_t __idx = (_N), __ptr_idx = 0; __idx > __idx_limit; __ptr_idx += 8) {                                 \
		__idx -= 8;                                                                                                \
		const size_t __value_idx = __idx % __width;                                                                \
		const _TYPE __byte8_mask = (_TYPE)((0xFF) << __value_idx);                                                 \
		_VALUE |= (__unsigned_type)((((__unsigned_type)(_PTR[__ptr_idx / __width] & __byte8_mask)) >> __value_idx) \
		     << __idx);                                                                                            \
	}                                                                                                               \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_STORE8_BODY_BE_UNSIGNED_TYPE(_TYPE, _N, _PTR, _VALUE)                                 \
	const size_t __width     = sizeof(_TYPE) * CHAR_BIT;                                                   \
	const size_t __idx_limit = 0;                                                                          \
	const size_t __sign_idx  = 0;                                                                          \
	memset(_PTR, 0, ((_N) / __width));                                                                     \
	for (size_t __idx = (_N), __ptr_idx = 0; __idx > __idx_limit; __ptr_idx += 8) {                        \
		__idx -= 8;                                                                                       \
		const size_t __value_idx = __idx % __width;                                                       \
		const _TYPE __byte8_mask = ((_TYPE)0xFF);                                                         \
		_PTR[__ptr_idx / __width] |= (_TYPE)((((_TYPE)(_VALUE >> __idx)) & __byte8_mask) << __value_idx); \
	}                                                                                                      \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_LOAD8_UNSIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR) \
	_UNSIGNED_BODY(_TYPE, _N, _PTR, __value);                             \
	return __value

#define ZTDC_GENERATE_LOAD8_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR)                                             \
	typedef int_least##_N##_t __type;                                                                               \
	_UNSIGNED_BODY(_TYPE, _N, _PTR, __unsigned_value);                                                              \
	if (__unsigned_value < ((__unsigned_type)ZTD_WIDTH_TO_MAX_VALUE_SIGNED(_N))) {                                  \
		return (__type)__unsigned_value;                                                                           \
	}                                                                                                               \
	else {                                                                                                          \
		__type __value = (__type)(__unsigned_value - ((__unsigned_type)ZTD_WIDTH_TO_MAX_VALUE(_N))) - (__type)(1); \
		return __value;                                                                                            \
	}                                                                                                               \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_STORE8_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR, _VALUE, _SIGN_IDX) \
	typedef int_least##_N##_t __type;                                                       \
	typedef uint_least##_N##_t __unsigned_type;                                             \
	const __unsigned_type __unsigned_value = (__unsigned_type)(_VALUE);                     \
	_UNSIGNED_BODY(_TYPE, _N, _PTR, __unsigned_value);                                      \
	ztd_static_assert(1, "")

#define ZTDC_GENERATE_STORE8_BE_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR, _VALUE) \
	ZTDC_GENERATE_STORE8_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR, _VALUE, (_N - 8))

#define ZTDC_GENERATE_STORE8_LE_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR, _VALUE) \
	ZTDC_GENERATE_STORE8_SIGNED_BODY(_UNSIGNED_BODY, _TYPE, _N, _PTR, _VALUE, 0)

#define ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(_TYPE, _N, _SUFFIX)                                                \
	uint_least##_N##_t ztdc_load8_leu##_N##_SUFFIX(const _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])                  \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_LOAD8_UNSIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr);              \
	}                                                                                                                 \
	uint_least##_N##_t ztdc_load8_beu##_N##_SUFFIX(const _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])                  \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_LOAD8_UNSIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr);              \
	}                                                                                                                 \
	uint_least##_N##_t ztdc_load8_aligned_leu##_N##_SUFFIX(                                                           \
	     const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ {                            \
		const _TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                         \
		ZTDC_GENERATE_LOAD8_UNSIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr);              \
	}                                                                                                                 \
	uint_least##_N##_t ztdc_load8_aligned_beu##_N##_SUFFIX(                                                           \
	     const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ {                            \
		const _TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                         \
		ZTDC_GENERATE_LOAD8_UNSIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr);              \
	}                                                                                                                 \
	int_least##_N##_t ztdc_load8_les##_N##_SUFFIX(const _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])                   \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_LOAD8_SIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr);                \
	}                                                                                                                 \
	int_least##_N##_t ztdc_load8_bes##_N##_SUFFIX(const _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])                   \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_LOAD8_SIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr);                \
	}                                                                                                                 \
	int_least##_N##_t ztdc_load8_aligned_les##_N##_SUFFIX(const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		const _TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                         \
		ZTDC_GENERATE_LOAD8_SIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr);                \
	}                                                                                                                 \
	int_least##_N##_t ztdc_load8_aligned_bes##_N##_SUFFIX(const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		const _TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                         \
		ZTDC_GENERATE_LOAD8_SIGNED_BODY(ZTDC_GENERATE_LOAD8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr);                \
	}                                                                                                                 \
                                                                                                                       \
	void ztdc_store8_leu##_N##_SUFFIX(const uint_least##_N##_t __value, _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])   \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_STORE8_BODY_LE_UNSIGNED_TYPE(_TYPE, _N, __ptr, __value);                                       \
	}                                                                                                                 \
	void ztdc_store8_beu##_N##_SUFFIX(const uint_least##_N##_t __value, _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])   \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_STORE8_BODY_BE_UNSIGNED_TYPE(_TYPE, _N, __ptr, __value);                                       \
	}                                                                                                                 \
	void ztdc_store8_aligned_leu##_N##_SUFFIX(const uint_least##_N##_t __value,                                       \
	     _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ {                                  \
		_TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                               \
		ZTDC_GENERATE_STORE8_BODY_LE_UNSIGNED_TYPE(_TYPE, _N, __ptr, __value);                                       \
	}                                                                                                                 \
	void ztdc_store8_aligned_beu##_N##_SUFFIX(const uint_least##_N##_t __value,                                       \
	     _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ {                                  \
		_TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                               \
		ZTDC_GENERATE_STORE8_BODY_BE_UNSIGNED_TYPE(_TYPE, _N, __ptr, __value);                                       \
	}                                                                                                                 \
	void ztdc_store8_les##_N##_SUFFIX(const int_least##_N##_t __value, _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])    \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_STORE8_LE_SIGNED_BODY(ZTDC_GENERATE_STORE8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr, __value);  \
	}                                                                                                                 \
	void ztdc_store8_bes##_N##_SUFFIX(const int_least##_N##_t __value, _TYPE __ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)])    \
	     ZTD_CXX_NOEXCEPT_I_ {                                                                                        \
		ZTDC_GENERATE_STORE8_LE_SIGNED_BODY(ZTDC_GENERATE_STORE8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr, __value);  \
	}                                                                                                                 \
	void ztdc_store8_aligned_les##_N##_SUFFIX(                                                                        \
	     const int_least##_N##_t __value, _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ { \
		_TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                               \
		ZTDC_GENERATE_STORE8_LE_SIGNED_BODY(ZTDC_GENERATE_STORE8_BODY_LE_UNSIGNED_TYPE, _TYPE, _N, __ptr, __value);  \
	}                                                                                                                 \
	void ztdc_store8_aligned_bes##_N##_SUFFIX(                                                                        \
	     const int_least##_N##_t __value, _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / CHAR_BIT)]) ZTD_CXX_NOEXCEPT_I_ { \
		_TYPE* __ptr = (_TYPE*)ZTD_ASSUME_ALIGNED(((_N) / CHAR_BIT), __unaligned_ptr);                               \
		ZTDC_GENERATE_STORE8_LE_SIGNED_BODY(ZTDC_GENERATE_STORE8_BODY_BE_UNSIGNED_TYPE, _TYPE, _N, __ptr, __value);  \
	}                                                                                                                 \
	ztd_static_assert((((_N) % 8) == 0), "N must be a multiple of 8")

#define ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(_N) ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned char, _N, )


#endif
