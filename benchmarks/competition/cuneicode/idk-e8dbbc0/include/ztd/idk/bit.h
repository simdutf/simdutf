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

#ifndef ZTD_IDK_BIT_H
#define ZTD_IDK_BIT_H

#include <ztd/idk/version.h>

#include <ztd/idk/extent.h>
#include <ztd/idk/detail/bit.load_store.h>
#include <ztd/idk/detail/bit.memreverse.h>
#include <ztd/idk/detail/bit.intrinsic.h>
#include <ztd/idk/detail/bit.intrinsic.generic.h>

//////
/// @addtogroup ztd_idk_bit_endian Endian Functions
///
/// @{
//////

//////
/// @brief Counts the number of ones in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_ones(...) _ZTDC_COUNT_ONES_I_(__VA_ARGS__)
//////
/// @brief Counts the number of zeros in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_zeros(...) _ZTDC_COUNT_ZEROS_I_(__VA_ARGS__)
//////
/// @brief Counts the number of leading zeros in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_leading_zeros(...) _ZTDC_COUNT_LEADING_ZEROS_I_(__VA_ARGS__)
//////
/// @brief Counts the number of trailing zeros in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_trailing_zeros(...) _ZTDC_COUNT_TRAILING_ZEROS_I_(__VA_ARGS__)
//////
/// @brief Counts the number of leading ones in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_leading_ones(...) _ZTDC_COUNT_LEADING_ONES_I_(__VA_ARGS__)
//////
/// @brief Counts the number of trailing ones in a given unsigned integer.
///
/// @param[in] ... The input value.
///
/// @returns An `int` (or suitably large signed integer type) with the count.
//////
#define ztdc_count_trailing_ones(...) _ZTDC_COUNT_TRAILING_ONES_I_(__VA_ARGS__)
//////
/// @brief Finds the first trailing zero in a given unsigned integer value.
///
/// @param[in] ... The input value.
///
/// @returns If the bit is not found, returns `0`. Otherwise, returns an `int` (or suitably large enough signed integer)
/// indicating the index of the found bit,
/// **plus one**.
//////
#define ztdc_first_leading_zero(...) _ZTDC_FIRST_LEADING_ZERO_I_(__VA_ARGS__)
//////
/// @brief Finds the first trailing zero in a given unsigned integer value.
///
/// @param[in] ... The input value.
///
/// @returns If the bit is not found, returns `0`. Otherwise, returns an `int` (or suitably large enough signed integer)
/// indicating the index of the found bit,
/// **plus one**.
//////
#define ztdc_first_trailing_zero(...) _ZTDC_FIRST_TRAILING_ZERO_I_(__VA_ARGS__)
//////
/// @brief Finds the first leading one in a given unsigned integer value.
///
/// @param[in] ... The input value.
///
/// @returns If the bit is not found, returns `0`. Otherwise, returns an `int` (or suitably large enough signed integer)
/// indicating the index of the found bit,
/// **plus one**.
//////
#define ztdc_first_leading_one(...) _ZTDC_FIRST_LEADING_ONE_I_(__VA_ARGS__)
//////
/// @brief Finds the first trailing one in a given unsigned integer value.
///
/// @param[in] ... The input value.
///
/// @returns If the bit is not found, returns `0`. Otherwise, returns an `int` (or suitably large enough signed integer)
/// indicating the index of the found bit,
/// **plus one**.
//////
#define ztdc_first_trailing_one(...) _ZTDC_FIRST_TRAILING_ONE_I_(__VA_ARGS__)
//////
/// @brief Performs a cyclical shift left.
///
/// @param[in] _VALUE The value to perform the cyclical shift left.
/// @param[in] ... The rotation value.
///
/// @remarks If the rotation value is negative, calls ztdc_rotate_right with the negated modulus of the rotation.
//////
#define ztdc_rotate_left(_VALUE, ...) _ZTDC_ROTATE_LEFT_I_(_VALUE, __VA_ARGS__)
//////
/// @brief Performs a cyclical shift right.
///
/// @param[in] _VALUE The value to perform the cyclical shift right.
/// @param[in] ... The rotation value.
///
/// @remarks If the rotation value is negative, calls ztdc_rotate_right with the negated modulus of the rotation.
//////
#define ztdc_rotate_right(_VALUE, ...) _ZTDC_ROTATE_RIGHT_I_(_VALUE, __VA_ARGS__)
//////
/// @brief Returns whether or not there is a single bit set in this unsigned integer value (this making it a power of
/// 2).
///
/// @param[in] ... The input value.
//////
#define ztdc_has_single_bit(...) _ZTDC_HAS_SINGLE_BIT_I_(__VA_ARGS__)
//////
/// @brief Returns the number of bits needed to represent the value exactly.
///
/// @param[in] ... The input value.
//////
#define ztdc_bit_width(...) _ZTDC_BIT_WIDTH_I_(__VA_ARGS__)
//////
/// @brief Returns the value that is the greatest power of 2 that is less than the input value.
///
/// @param[in] ... The input value.
///
/// @returns `0` when the input value is `0`. Otherwise, produces the greatest power of 2 that is less than the input
/// value.
//////
#define ztdc_bit_ceil(...) _ZTDC_BIT_CEIL_I_(__VA_ARGS__)
//////
/// @brief Returns the value that is the next power of 2.
///
/// @param[in] ... The input value.
///
/// @returns `1` when the input value is less than or equal to `1`. Otherwise, produces the power of 2 that is higher
/// than the input value.
//////
#define ztdc_bit_floor(...) _ZTDC_BIT_FLOOR_I_(__VA_ARGS__)

//////
/// @}
//////

#if defined(ZTD_DOXYGEN_PREPROCESSING) && (ZTD_DOXYGEN_PREPROCESSING != 0)
//////
/// @addtogroup ztd_idk_bit_endian Endian Functions
///
/// @{
//////

//////
/// @brief Reverses each 8-bit byte in a region of memory.
///
/// @param[in] __n The number of bytes to reverse.
/// @param[in] __ptr The pointer whose 8-bit bytes will be reversed.
///
/// @remarks Constraints:
/// - `CHAR_BIT` is a multiple of 8.
///
/// Each 8-bit byte is considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the
/// range [0, `CHAR_BIT`).
//////
void ztdc_memreverse8(size_t __n, unsigned char __ptr[ZTD_PTR_EXTENT(__n)]);
//////
/// @brief Reverses the 8-bits of a given _N_-width integer type.
///
/// @param[in] __value The exact-width integer value to be reversed.
///
/// @remarks Equivalent to: `ztdc_memreverse8(sizeof(__value), (unsigned char*)(&__value)); return __value;`.
//////
uintN_t ztdc_memreverse8uN(uintN_t __value);


//////
/// @brief Stores an 8-bit byte-specific unsigned integer in little endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __value The value to be stored.
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores _N_ bits, as if by performing `__value = __value & (UINTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`).
//////
void ztdc_store8_leuN(uint_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Stores an 8-bit byte-specific unsigned integer in big endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __value The value to be stored.
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores _N_ bits, as if by performing `__value = __value & (UINTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`).
//////
void ztdc_store8_beuN(uint_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Loads an 8-bit byte-specific unsigned integer in little endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only loads _N_ bits and leaves the rest at 0. Each 8-bit byte is considered according to `0xFF <<
/// multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0, `CHAR_BIT`).
//////
uint_leastN_t ztdc_load8_leuN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Loads an 8-bit byte-specific unsigned integer in big endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only loads _N_ bits and leaves the rest at 0. Each 8-bit byte is considered according to `0xFF <<
/// multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0, `CHAR_BIT`).
//////
uint_leastN_t ztdc_load8_beuN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_store8_leuN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
void ztdc_store8_aligned_leuN(uint_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_store8_beuN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
void ztdc_store8_aligned_beuN(uint_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_load8_leuN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
uint_leastN_t ztdc_load8_aligned_leuN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_load8_beuN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
uint_leastN_t ztdc_load8_aligned_beuN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);

//////
/// @brief Stores an 8-bit byte-specific signed integer in little endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __value The value to be stored.
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores (_N_ - 1) bits, as if by performing `__value = __value & (INTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`). The sign bit is serialized into the proper location in the array as the leading (high) bit, and the
/// mask for that is `0x7F << multiple-of-8`.
//////
void ztdc_store8_lesN(int_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Stores an 8-bit byte-specific signed integer in big endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __value The value to be stored.
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores (_N_ - 1) bits, as if by performing `__value = __value & (INTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`). The sign bit is serialized into the proper location in the array as the leading (high) bit, and the
/// mask for that is `0x7F << multiple-of-8`.
//////
void ztdc_store8_besN(int_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Loads an 8-bit byte-specific signed integer in little endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores (_N_ - 1) bits, as if by performing `__value = __value & (INTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`). The sign bit is serialized into the proper location in the array as the leading (high) bit, and the
/// mask for that is `0x7F << multiple-of-8`.
//////
int_leastN_t ztdc_load8_lesN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @brief Loads an 8-bit byte-specific signed integer in big endian format in the array pointed to by `__ptr` by
/// reading from `__value`.
///
/// @param[in] __ptr A non-null pointer to the at least `N / CHAR_BIT` elements.
///
/// @par Constraints
/// - `CHAR_BIT` is a multiple if 8.
/// - _N_ is a multiple of 8.
///
/// @remarks Only stores (_N_ - 1) bits, as if by performing `__value = __value & (INTN_MAX)` first. Each 8-bit byte is
/// considered according to `0xFF << multiple-of-8`, where `multiple-of-8` is a multiple of in the range [0,
/// `CHAR_BIT`). The sign bit is serialized into the proper location in the array as the leading (high) bit, and the
/// mask for that is `0x7F << multiple-of-8`.
//////
int_leastN_t ztdc_load8_besN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_store8_lesN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
void ztdc_store8_aligned_lesN(int_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_store8_besN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
void ztdc_store8_aligned_besN(int_leastN_t __value, unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_load8_lesN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
int_leastN_t ztdc_load8_aligned_lesN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);
//////
/// @copydoc ztdc_load8_besN
///
/// @par Precondition
/// The input pointer `__ptr` has an alignment suitable to be treated as an integral type of width _N_.
//////
int_leastN_t ztdc_load8_aligned_besN(const unsigned char __ptr[ZTD_PTR_EXTENT(N / CHAR_BIT)]);

//////
/// @}
//////

#endif

#endif
