
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

#ifndef ZTD_IDK_DETAIL_BIT_WIDTH_TO_MAX_VALUE_H
#define ZTD_IDK_DETAIL_BIT_WIDTH_TO_MAX_VALUE_H

#include <ztd/idk/version.h>

#if ZTD_IS_ON(ZTD_C)
#include <limits.h>
#else
#include <climits>
#endif

#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_8_I_ 0xFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_16_I_ 0xFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_24_I_ 0xFFFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_32_I_ 0xFFFFFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_40_I_ 0xFFFFFFFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_48_I_ 0xFFFFFFFFFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_56_I_ 0xFFFFFFFFFFFFFFu
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_64_I_ 0xFFFFFFFFFFFFFFFFu

#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_8_I_ 0x7F
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_16_I_ 0x7FFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_24_I_ 0x7FFFFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_32_I_ 0x7FFFFFFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_40_I_ 0x7FFFFFFFFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_48_I_ 0x7FFFFFFFFFFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_56_I_ 0x7FFFFFFFFFFFFF
#define ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_64_I_ 0x7FFFFFFFFFFFFFFF

#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_8_I_ (-0x7F)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_16_I_ (-0x7FFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_24_I_ (-0x7FFFFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_32_I_ (-0x7FFFFFFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_40_I_ (-0x7FFFFFFFFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_48_I_ (-0x7FFFFFFFFFFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_56_I_ (-0x7FFFFFFFFFFFFF)
#define ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_64_I_ (-0x7FFFFFFFFFFFFFFF)

#define ZTD_WIDTH_TO_MAX_VALUE_SIGNED(_N) ZTD_TOKEN_EXPAND_I_(ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_SIGNED_##_N##_I_)
#define ZTD_WIDTH_TO_MIN_VALUE_SIGNED(_N) ZTD_TOKEN_EXPAND_I_(ZTD_IDK_DETAIL_WIDTH_TO_MIN_VALUE_SIGNED_##_N##_I_)
#define ZTD_WIDTH_TO_MAX_VALUE(_N) ZTD_TOKEN_EXPAND_I_(ZTD_IDK_DETAIL_WIDTH_TO_MAX_VALUE_##_N##_I_)

#endif
