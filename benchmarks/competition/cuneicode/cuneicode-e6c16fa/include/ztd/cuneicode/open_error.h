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

#ifndef ZTD_CUNEICODE_OPEN_ERROR_H
#define ZTD_CUNEICODE_OPEN_ERROR_H

#pragma once

#include <ztd/cuneicode/version.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#include <stdbool.h>
#endif

//////
/// @addtogroup ztd_cuneicode_registry_error_types Registry Error Types
///
/// @{

//////
/// @brief The error that occurred when trying to open or create a conversion resource.
typedef enum cnc_open_error {
	//////
	/// @brief Returned when everything was okay.
	CNC_OPEN_ERROR_OK = 0,
	//////
	/// @brief Returned when there is no conversion path between the specified from and to
	/// encodings.
	CNC_OPEN_ERROR_NO_CONVERSION_PATH = -1,
	//////
	/// @brief Returned when there iss not enough output space to write into for creating the
	/// resource.
	CNC_OPEN_ERROR_INSUFFICIENT_OUTPUT = -2,
	//////
	/// @brief Returned when there is an invalid parameter passed in for creating the
	/// resource.
	CNC_OPEN_ERROR_INVALID_PARAMETER = -3,
	//////
	/// @brief Returned when a heap-related or allocation-related failure occurred.
	CNC_OPEN_ERROR_ALLOCATION_FAILURE = -4
} cnc_open_error;

//////
/// @}

#endif
