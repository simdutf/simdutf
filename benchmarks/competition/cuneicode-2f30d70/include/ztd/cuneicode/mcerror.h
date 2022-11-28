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

#ifndef ZTD_CUNEICODE_MCERROR_H
#define ZTD_CUNEICODE_MCERROR_H

#pragma once

#include <ztd/cuneicode/version.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#endif

//////
/// @addtogroup ztd_cuneicode_conversion_error_types Typed and Untyped Conversion Error Types
///
/// @{

//////
/// @brief The various errors that can be returned from multi or single conversion functions.
typedef enum cnc_mcerror {
	//////
	/// @brief Returned when processing of the input and writing of the output is successful
	/// and
	/// nothing has gone wrong.
	CNC_MCERROR_OK = 0,
	//////
	/// @brief Returned when there is an error processing any input. No output is written.
	///
	/// @remarks This can only be applied when processing the input finds an invalid sequence
	/// or an
	/// improper input value. It shall not be used for exhausted input or empty input.
	/// Critically,
	/// no output from **that specific unit of work** is written (but any previously written
	/// successful output remains with e.g. any bulk conversion function).
	CNC_MCERROR_INVALID_SEQUENCE = 1,
	//////
	/// @brief Returned when there is not yet an error in processing the input, but the input
	/// is
	/// exhausted before a proper single unit of work is complete.
	///
	/// @remarks It is fundamentally important that this is returned from conversion routines
	/// when
	/// the input is fully exhausted AND the input sequence of bytes/code units/code points is
	/// not
	/// erroneous. If the input values are erroneous, @ref CNC_MCERROR_INVALID_SEQUENCE should
	/// be
	/// used instead.
	CNC_MCERROR_INCOMPLETE_INPUT = 2,
	//////
	/// @brief Returned when the size of the output is not sufficiently large to handle the
	/// value
	/// of the error.
	///
	/// @remarks For single functions (functions that complete only a single unit of work),
	/// @ref CNC_MCERROR_INSUFFICIENT_OUTPUT can be entirely avoided by providing a large
	/// enough
	/// statically sized buffer. This is typically done by using the maximum-output macros
	/// such as
	/// @ref CNC_C32_MAX - see the documentation for more details.
	CNC_MCERROR_INSUFFICIENT_OUTPUT = 3
} cnc_mcerror;

//////
/// @brief Returns a string representing the error code's name.
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ const char* cnc_mcerror_to_str(
     cnc_mcerror err);

//////
/// @}

#endif // ZTD_CUNEICODE_MCERROR_H
