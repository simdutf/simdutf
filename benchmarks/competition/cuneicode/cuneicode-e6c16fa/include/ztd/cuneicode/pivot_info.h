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

#ifndef ZTD_CUNEICODE_PIVOT_INFO_H
#define ZTD_CUNEICODE_PIVOT_INFO_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#endif

//////
/// @addtogroup ztd_cuneicode_conversion Conversion Functions
///
/// @{

//////
/// @brief A structure containing information for a "pivot buffer".
///
/// @remarks When a failure happens due to an intermediate conversion failing, the `result` member
/// of the cnc_pivot_info will be set to a non-CNC_MCERROR_OK value reflecting the type of
/// failure that happened within the intermediate conversion.
typedef struct __cnc_pivot_info {
	//////
	/// @brief The number of bytes pointed to by bytes.
	size_t bytes_size;
	//////
	/// @brief A pointer to a byte buffer to use for intermediate conversions.
	///
	/// @remarks If this is a null pointer, it signifies that an internal buffer created in some
	/// fashion by the implementation should be used instead to perform the conversion. Otherwise,
	/// even if the buffer is insufficiently small, it will use this buffer.
	unsigned char* bytes;
	//////
	/// @brief The error code representing any failed conversion specific to the intermediate/pivot
	/// step.
	///
	/// @remarks If a conversion involving the intermediate buffer / pivot buffer - even the
	/// implementation-defined one - fails for any reason, it will be reported in this variable.
	cnc_mcerror error;
} cnc_pivot_info;

#endif
