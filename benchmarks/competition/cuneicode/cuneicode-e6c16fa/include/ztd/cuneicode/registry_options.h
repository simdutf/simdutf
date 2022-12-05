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

#ifndef ZTD_CUNEICODE_REGISTRY_OPTIONS_H
#define ZTD_CUNEICODE_REGISTRY_OPTIONS_H

#pragma once

#include <ztd/cuneicode/version.h>

//////
/// @addtogroup ztd_cuneicode_registry_options Conversion Registry Options
///
/// @{

//////
/// @brief The options which change how a registry is initialized and adjusted upon creation.
typedef enum cnc_registry_options {
	//////
	/// @brief No options.
	CNC_REGISTRY_OPTIONS_NONE = 0,
	//////
	/// @brief Start with an empty registry that contains none of the platorm's default
	/// conversion
	/// entries.
	CNC_REGISTRY_OPTIONS_EMPTY = 1,
	//////
	/// @brief Use the default options recommended when starting a registry.
	CNC_REGISTRY_OPTIONS_DEFAULT = CNC_REGISTRY_OPTIONS_NONE,
} cnc_registry_options;

//////
/// @}

#endif
