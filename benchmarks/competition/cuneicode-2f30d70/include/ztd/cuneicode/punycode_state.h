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

#ifndef ZTD_CUNEICODE_PUNYCODE_STATE_H
#define ZTD_CUNEICODE_PUNYCODE_STATE_H

#pragma once

#include <ztd/cuneicode/version.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstdint>
#include <climits>
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>
#include <limits.h>
#endif

//////
/// @addtogroup ztd_cuneicode_conversion Conversion Functions
///
/// @{

//////
/// @brief A structure containing all of the necessary information for a general-purpose to-punycode
/// transformation from UTF-32.
typedef struct cnc_pny_encode_state_t {
	//////
	/// @brief Whether or not the input is finished.
	///
	/// @remarks This is only a way to stop input streaming "early" for users who are processing
	/// one bit of punycode at-a-time. Marking the data here as complete will tell punycode to stop
	/// outputting data, and that the input stream is finished.
	size_t input_is_complete : 1;
	//////
	/// @brief Whether or not to output uppercase letters for the values.
	///
	/// @remarks This is mostly for compatibility with specific parsers in other places that cannot
	/// recognize lowercase vs. uppercase, or demand that the input be one or the other. (It is not
	/// compliant with RFC3492 to NOT accept both lowercasea nd uppercase, but who are we kidding
	/// by expecting strict conformance out of software developers?).
	size_t uppercase_letters : 1;
	//////
	/// @brief Private. Do not access.
	size_t __is_initialized : 1;
	//////
	/// @brief Private. Do not access.
	size_t __idna : 1;
	//////
	/// @brief Private. Do not access.
	size_t __action_state : 2;
	//////
	/// @brief Private. Do not access.
	size_t __padding : (sizeof(size_t) * CHAR_BIT) - 6;
	//////
	/// @brief Private. Do not access.
	size_t __has_seen_non_basic;
	//////
	/// @brief Private. Do not access.
	alignas(void*) unsigned char __storage[(sizeof(void*) * 3) + (256 * sizeof(uint_least32_t))
	     + (sizeof(size_t) * 4)];
} cnc_pny_encode_state_t;

//////
/// @brief A structure containing all of the necessary information for a general-purpose
/// from-punycode transformation to UTF-32.
typedef struct cnc_pny_decode_state_t {
	//////
	/// @brief Whether or not the input is finished.
	///
	/// @remarks This is only a way to stop input streaming "early" for users who are processing
	/// one bit of punycode at-a-time. Marking the data here as complete will tell punycode to stop
	/// outputting data, and that the input stream is finished.
	size_t input_is_complete : 1;
	//////
	/// @brief Private. Do not access.
	size_t __is_initialized : 1;
	//////
	/// @brief Private. Do not access.
	size_t __idna : 1;
	//////
	/// @brief Private. Do not access.
	size_t __prefixed : 1;
	//////
	/// @brief Private. Do not access.
	size_t __segment_is_digits : 1;
	//////
	/// @brief Private. Do not access.
	size_t __action_state : 2;
	//////
	/// @brief Private. Do not access.
	size_t __padding : (sizeof(size_t) * CHAR_BIT) - 7;
	//////
	/// @brief Private. Do not access.
	alignas(void*) unsigned char __storage[(sizeof(void*) * 3) + (256 * sizeof(char))
	     + (sizeof(size_t) * 4)];
} cnc_pny_decode_state_t;

//////
/// @brief Returns whether or not the given cnc_pny_encode_state_t has no more data that needs to be
/// output.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_pny_encode_state_is_complete(
     const cnc_pny_encode_state_t* __state);

//////
/// @brief Returns whether or not the given cnc_pny_decode_state_t has no more data that needs to be
/// output.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ bool cnc_pny_decode_state_is_complete(
     const cnc_pny_decode_state_t* __state);

//////
/// @}

#endif // ZTD_CUNEICODE_PUNYCODE_STATE_H
