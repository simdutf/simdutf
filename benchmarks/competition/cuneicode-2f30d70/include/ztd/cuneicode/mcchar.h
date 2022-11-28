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

#ifndef ZTD_CUNEICODE_MCCHAR_H
#define ZTD_CUNEICODE_MCCHAR_H

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/cuneicode/detail/mccharn.h>
#include <ztd/cuneicode/detail/mccharsn.h>
#include <ztd/cuneicode/detail/mcchar_generic.hpp>
#include <ztd/cuneicode/detail/mcchar_generic.h>

//////
/// @addtogroup ztd_cuneicode_generic_typed_conversions Generic Typed Conversion Functions
///
/// @{

//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type, one at a time. Identical in behavior to @ref cnc_cxnrtocyn, with the @p
/// __p_state argument passed in from a properly initialized cnc_mcstate_t object of automatic
/// storage duration (it is a "stack"-based variable that does not live beyond the call of this
/// function).
#define cnc_cxntocyn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src) \
	__cnc_cxntocxn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)

//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type.
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
/// @remarks Note that it is impossible for this type to handle `nullptr` properly for its
/// destination and source arguments because of its templated nature, since that is how it derives
/// the type. Therefore, the `nullptr` passed in must first be coerced to a type with a cast, for
/// example:
/// ```cpp
/// cnc_mcerror err = cnc_cxsntocysn(&required_len, (char**)nullptr, &src_len, &src_ptr, &state);
/// ```
#define cnc_cxnrtocyn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state) \
	__cnc_cxnrtocxn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)

//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type in **bulk**. Identical in behavior to @ref cnc_cxsnrtocysn, with the @p
/// __p_state argument passed in from a properly initialized cnc_mcstate_t object of automatic
/// storage duration (it is a "stack"-based variable that does not live beyond the call of this
/// function).
#define cnc_cxsntocysn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src) \
	__cnc_cxsntocxsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src)

//////
/// @brief Converts from the encoding given by `__p_src`'s character type to `__p_maybe_dst`'s
/// character type **one at a time**.
///
/// @param[in, out] __p_maybe_dst_len A poinvter to the size of the output buffer. If this is
/// `nullptr`, then it will not update the count (and the output stream will automatically be
/// considered large enough to handle all data, if
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
/// @remarks Note that it is impossible for this type to handle `nullptr` properly for its
/// destination and source arguments because of its templated nature, since that is how it derives
/// the type. Therefore, the `nullptr` passed in must first be coerced to a type with a cast, for
/// example:
/// ```cpp
/// cnc_mcerror err = cnc_cxsntocysn(&required_len, (char**)nullptr, &src_len, &src_ptr, &state);
/// ```
#define cnc_cxsnrtocysn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state) \
	__cnc_cxsnrtocxsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state)

//////
/// @}

#endif // ZTD_CUNEICODE_MCCHAR_H
