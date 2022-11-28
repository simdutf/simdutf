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

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ const char* cnc_mcerror_to_str(
     cnc_mcerror err) {
	switch (err) {
	case CNC_MCERROR_OK:
		return "CNC_MCERROR_OK";
	case CNC_MCERROR_INCOMPLETE_INPUT:
		return "CNC_MCERROR_INCOMPLETE_INPUT";
	case CNC_MCERROR_INSUFFICIENT_OUTPUT:
		return "CNC_MCERROR_INSUFFICIENT_OUTPUT";
	case CNC_MCERROR_INVALID_SEQUENCE:
		return "CNC_MCERROR_INVALID_SEQUENCE";
	}
	return "CNC_MCERROR_UNKNOWN_ERROR_CODE";
}
