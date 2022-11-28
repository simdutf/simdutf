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

#ifndef ZTD_CUNEICODE_MAX_OUTPUT_H
#define ZTD_CUNEICODE_MAX_OUTPUT_H

#pragma once

#include <ztd/cuneicode/version.h>

//////
/// @addtogroup ztd_cuneicode_max_macros Maximum-Size Macros
/// @{

//////
/// @brief The maximum size that can be output by a single `cnc_cxnrtomcn` function call.
#define CNC_MC_MAX 64
//////
/// @brief The maximum size that can be output by a single `cnc_cxnrtomwcn` function call.
#define CNC_MWC_MAX 32
//////
/// @brief The maximum size that can be output by a single `cnc_cxnrtoc32n` function call.
#define CNC_C32_MAX 16
//////
/// @brief The maximum size that can be output by a single `cnc_cxnrtoc16n` function call.
#define CNC_C16_MAX (CNC_C32_MAX * 2)
//////
/// @brief The maximum size that can be output by a single `cnc_cxnrtoc8n` function call.
#define CNC_C8_MAX (CNC_C32_MAX * 4)

//////
/// @}

#endif // ZTD_CUNEICODE_MAX_OUTPUT_H
