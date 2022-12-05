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

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_STATE_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_STATE_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcstate.h>
#include <ztd/idk/static_assert.hpp>

inline constexpr cnc_mcstate_indicator __mc_s_i_locale   = (cnc_mcstate_indicator)0;
inline constexpr cnc_mcstate_indicator __mc_s_i_raw      = (cnc_mcstate_indicator)1;
inline constexpr cnc_mcstate_indicator __mc_s_i_punycode = (cnc_mcstate_indicator)2;

static_assert(__mc_s_i_locale == CNC_MCSTATE_INDICATOR___RESERVED0,
     "mismatch between reserved and internal indicator values (locale)");
static_assert(__mc_s_i_raw == CNC_MCSTATE_INDICATOR_RAW,
     "mismatch between reserved and internal indicator values (raw)");
static_assert(__mc_s_i_punycode == CNC_MCSTATE_INDICATOR___RESERVED1,
     "mismatch between reserved and internal indicator values (punycode)");

#endif
