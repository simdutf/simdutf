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

#ifndef ZTD_CUNEICODE_MCCHAR_GENERIC_H
#define ZTD_CUNEICODE_MCCHAR_GENERIC_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/cuneicode/detail/mccharn.h>
#include <ztd/cuneicode/detail/mccharsn.h>

#if ZTD_IS_ON(ZTD_C)

// clang-format off
#define CNC_CXNTOC32N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                              \
		ztd_char32_t: cnc_c32ntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char32_t**)(__p_src)), \
		ztd_char16_t: cnc_c16ntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char16_t**)(__p_src)), \
		ztd_char8_t: cnc_c8ntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src)),   \
		wchar_t: cnc_mwcntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const wchar_t**)(__p_src)),      \
		char: cnc_mcntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const char**)(__p_src)),          \
		default: cnc_c8ntoc32n((__p_maybe_dst_len), (ztd_char32_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src))        \
	)

#define CNC_CXNTOC16N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                              \
		ztd_char32_t: cnc_c32ntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char32_t**)(__p_src)), \
		ztd_char16_t: cnc_c16ntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char16_t**)(__p_src)), \
		ztd_char8_t: cnc_c8ntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src)),   \
		wchar_t: cnc_mwcntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const wchar_t**)(__p_src)),      \
		char: cnc_mcntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const char**)(__p_src)),          \
		default: cnc_c8ntoc16n((__p_maybe_dst_len), (ztd_char16_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src))        \
	)

#define CNC_CXNTOC8N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                                    \
		ztd_char32_t: cnc_c32ntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char32_t**)(__p_src)), \
		ztd_char16_t: cnc_c16ntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char16_t**)(__p_src)), \
		ztd_char8_t: cnc_c8ntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src)),   \
		wchar_t: cnc_mwcntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const wchar_t**)(__p_src)),      \
		char: cnc_mcntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const char**)(__p_src)),          \
		default: cnc_c8ntoc8n((__p_maybe_dst_len), (ztd_char8_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src))        \
	)

#define CNC_CXNTOMWCN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                              \
		ztd_char32_t: cnc_c32ntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char32_t**)(__p_src)), \
		ztd_char16_t: cnc_c16ntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char16_t**)(__p_src)), \
		ztd_char8_t: cnc_c8ntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src)),   \
		wchar_t: cnc_mwcntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const wchar_t**)(__p_src)),      \
		char: cnc_mcntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const char**)(__p_src)),          \
		default: cnc_c8ntomwcn((__p_maybe_dst_len), (wchar_t**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src))        \
	)

#define CNC_CXNTOMCN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                                    \
		ztd_char32_t: cnc_c32ntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const ztd_char32_t**)(__p_src)), \
		ztd_char16_t: cnc_c16ntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const ztd_char16_t**)(__p_src)), \
		ztd_char8_t: cnc_c8ntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src)),   \
		wchar_t: cnc_mwcntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const wchar_t**)(__p_src)),      \
		char: cnc_mcntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const char**)(__p_src)),          \
		default: cnc_c8ntomcn((__p_maybe_dst_len), (char**)(__p_maybe_dst), (__p_src_len), (const ztd_char8_t**)(__p_src))        \
	)

#define CNC_CXNRTOC32N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                            \
		ztd_char32_t: cnc_c32nrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16nrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8nrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcnrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcnrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8nrtoc32n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define CNC_CXNRTOC16N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                            \
		ztd_char32_t: cnc_c32nrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16nrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8nrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcnrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcnrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8nrtoc16n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define CNC_CXNRTOC8N_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                           \
		ztd_char32_t: cnc_c32nrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16nrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8nrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcnrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcnrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8nrtoc8n((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define CNC_CXNRTOMWCN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                            \
		ztd_char32_t: cnc_c32nrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16nrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8nrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcnrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcnrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8nrtomwcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define CNC_CXNRTOMCN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                           \
		ztd_char32_t: cnc_c32nrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16nrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8nrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcnrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcnrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8nrtomcn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define CNC_CXSNTOC32SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                                \
		ztd_char32_t: cnc_c32sntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: cnc_c16sntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: cnc_c8sntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: cnc_mwcsntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: cnc_mcsntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: cnc_c8sntoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define CNC_CXSNTOC16SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                                \
		ztd_char32_t: cnc_c32sntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: cnc_c16sntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: cnc_c8sntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: cnc_mwcsntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: cnc_mcsntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: cnc_c8sntoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define CNC_CXSNTOC8SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                               \
		ztd_char32_t: cnc_c32sntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: cnc_c16sntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: cnc_c8sntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: cnc_mwcsntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: cnc_mcsntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: cnc_c8sntoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define CNC_CXSNTOMWCSN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                                \
		ztd_char32_t: cnc_c32sntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: cnc_c16sntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: cnc_c8sntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: cnc_mwcsntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: cnc_mcsntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: cnc_c8sntomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define CNC_CXSNTOMCSN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                        \
	_Generic(**(__p_src),                                                                               \
		ztd_char32_t: cnc_c32sntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: cnc_c16sntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: cnc_c8sntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: cnc_mwcsntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: cnc_mcsntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: cnc_c8sntomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define CNC_CXSNRTOC32SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                              \
		ztd_char32_t: cnc_c32snrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16snrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8snrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcsnrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcsnrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8snrtoc32sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)

#define CNC_CXSNRTOC16SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                              \
		ztd_char32_t: cnc_c32snrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16snrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8snrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcsnrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcsnrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8snrtoc16sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)

#define CNC_CXSNRTOC8SN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                             \
		ztd_char32_t: cnc_c32snrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16snrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8snrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcsnrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcsnrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8snrtoc8sn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)

#define CNC_CXSNRTOMWCSN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                              \
		ztd_char32_t: cnc_c32snrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16snrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8snrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcsnrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcsnrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8snrtomwcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)

#define CNC_CXSNRTOMCSN_I_(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                          \
	_Generic(**(__p_src),                                                                                             \
		ztd_char32_t: cnc_c32snrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: cnc_c16snrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: cnc_c8snrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: cnc_mwcsnrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: cnc_mcsnrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: cnc_c8snrtomcsn((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)

#define __cnc_cxntocxn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                              \
	_Generic(**(__p_maybe_dst),                                                                          \
		ztd_char32_t: CNC_CXNTOC32N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: CNC_CXNTOC16N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: CNC_CXNTOC8N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: CNC_CXNTOMWCN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: CNC_CXNTOMCN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: CNC_CXNTOC8N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define __cnc_cxnrtocxn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                                \
	_Generic(**(__p_maybe_dst),                                                                                        \
		ztd_char32_t: CNC_CXNRTOC32N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: CNC_CXNRTOC16N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: CNC_CXNRTOC8N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: CNC_CXNRTOMWCN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: CNC_CXNRTOMCN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: CNC_CXNRTOC8N_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state))        \
	)

#define __cnc_cxsntocxsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)                              \
	_Generic(**(__p_maybe_dst),                                                                            \
		ztd_char32_t: CNC_CXSNTOC32SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char16_t: CNC_CXSNTOC16SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)), \
		ztd_char8_t: CNC_CXSNTOC8SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),   \
		wchar_t: CNC_CXSNTOMWCSN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),      \
		char: CNC_CXSNTOMCSN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src)),          \
		default: CNC_CXSNTOC8SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))        \
	)

#define __cnc_cxsnrtocxsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)                                \
	_Generic(**(__p_maybe_dst),                                                                                          \
		ztd_char32_t: CNC_CXSNRTOC32SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char16_t: CNC_CXSNRTOC16SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)), \
		ztd_char8_t: CNC_CXSNRTOC8SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),   \
		wchar_t: CNC_CXSNRTOMWCSN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),      \
		char: CNC_CXSNRTOMCSN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src), (__p_state)),          \
		default: CNC_CXSNRTOC8SN_I_((__p_maybe_dst_len), (__p_maybe_dst), (__p_src_len), (__p_src))                     \
	)
// clang-format on

#endif

#endif
