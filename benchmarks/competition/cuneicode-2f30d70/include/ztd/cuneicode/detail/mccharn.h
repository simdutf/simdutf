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

#ifndef ZTD_CUNEICODE_DETAIL_MCCHARN_H
#define ZTD_CUNEICODE_DETAIL_MCCHARN_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcstate.h>
#include <ztd/cuneicode/mcerror.h>
#include <ztd/idk/charN_t.h>

#if ZTD_IS_ON(ZTD_CXX)
#include <cstddef>
#else
#include <stddef.h>
#endif

//////
/// @addtogroup ztd_cuneicode_single_conversion_functions Single Conversion Functions
/// @{

//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type. Only performs one unit of indivisible work, or returns an error.
///
/// @param[in, out] __p_maybe_dst_len A pointer to the size of the output buffer (in number of
/// **elements**). If this is `nullptr`, then it will not update the count (and the output
/// stream will automatically be considered large enough to handle all data, if
/// `__p_maybe_dst` is not `nullptr`).
/// @param[in, out] __p_maybe_dst A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__p_maybe_dst_len`).
/// @param[in, out] __p_src_len A pointer to the size of the input buffer (in number of
/// **elements**). If this is `nullptr` or points to a value equivalent to `0`, then the input
/// is considered empty and CNC_MCERROR_OK is returned.
/// @param[in, out] __p_src A pointer to the pointer of the input buffer. If this or the
/// pointer within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is
/// returned.
///
/// @remarks The documentation for the `type` to encoding mapping can be found in the
/// @verbatim embed:rst:inline :ref:`design-naming-encoding.table` @endverbatim .
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type. Only performs one unit of indivisible work, or returns an error.
///
/// @param[in, out] __p_maybe_dst_len A pointer to the size of the output buffer (in number of
/// **elements**). If this is `nullptr`, then it will not update the count (and the output stream
/// will automatically be considered large enough to handle all data, if
/// `__p_maybe_dst` is not `nullptr`).
/// @param[in, out] __p_maybe_dst A pointer to the pointer of the output buffer. If this or the
/// pointer within are `nullptr`, than this function will not write output data (it may still
/// decrement the value pointed to by
/// `__p_maybe_dst_len`).
/// @param[in, out] __p_src_len A pointer to the size of the input buffer (in number of
/// **elements**). If this is `nullptr` or points to a value equivalent to `0`, then the input is
/// considered empty and CNC_MCERROR_OK is returned.
/// @param[in, out] __p_src A pointer to the pointer of the input buffer. If this or the pointer
/// within are `nullptr`, than the input is considered empty and CNC_MCERROR_OK is returned.
/// @param[in, out] __p_state A pointer to the conversion state. If this is `nullptr`, a
/// value-initialized (`= {0}` or similar) cnc_mcstate_t is used.
///
/// @remarks This function will create an automatic storage duration `cnc_mcstate_t` object,
/// initialize it to the initial shift sequence, and then use it with this function if @p __p_state
/// is `nullptr`. This object is not recoverable in any way and is not shared, and therefore should
/// only be used if the end-user is sure there is no state to the encoding they are working with
/// (e.g., conversions between Unicode Transformation Formats (UTFs). It is **NOT** recommended to
/// use this with the execution encoding and wide execution encodings, which may have shift state
/// and could lead to invalid reads of later data without that shift state information from the
/// `cnc_mcstate_t` object.)
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const wchar_t** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomcn_utf8_exec(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomcn_utf8_exec(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomwcn_utf8_wide_exec(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomwcn_utf8_wide_exec(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc8n_utf8(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc8n_utf8(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc16n_utf8(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc16n_utf8(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc32n_utf8(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc32n_utf8(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomcn_exec_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomcn_exec_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcntomcn_wide_exec_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const wchar_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mwcnrtomcn_wide_exec_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const wchar_t** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16ntomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c16nrtomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char16_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomcn_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;


//////
/// @copydoc cnc_mcntomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntomcn_utf8_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_;
//////
/// @copydoc cnc_mcnrtomcn
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtomcn_utf8_utf8(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_;

//////
/// @}

#endif // ZTD_CUNEICODE_DETAIL_MCCHARN_H
