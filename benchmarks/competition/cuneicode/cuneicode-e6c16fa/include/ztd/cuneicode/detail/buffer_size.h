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

#ifndef ZTD_CUNEICODE_DETAIL_BUFFER_SIZE_H
#define ZTD_CUNEICODE_DETAIL_BUFFER_SIZE_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/max_output.h>

#define CNC_DEFAULT_CONVERSION_INTERMEDIATE_BUFFER_SIZE                         \
	(((ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_) < CNC_MC_MAX) \
	          ? 512                                                            \
	          : (ZTD_CUNEICODE_INTERMEDIATE_BUFFER_SUGGESTED_BYTE_SIZE_I_))

#define CNC_DEFAULT_NAME_MAX_SIZE 127

#endif
