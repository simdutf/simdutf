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

#ifndef ZTD_IDK_DETAIL_UNICODE_H
#define ZTD_IDK_DETAIL_UNICODE_H

#include <ztd/idk/version.h>

#include <ztd/idk/charN_t.h>

ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_replacement       = 0xFFFD;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_ascii_replacement = 0x003F;

ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_unicode_code_point = 0x10FFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_first_lead_surrogate    = 0xD800;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_lead_surrogate     = 0xDBFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_first_trail_surrogate   = 0xDC00;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_trail_surrogate    = 0xDFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_first_surrogate
     = __ztd_idk_detail_first_lead_surrogate;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_surrogate
     = __ztd_idk_detail_last_trail_surrogate;

ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_1byte_value        = 0x7F;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_2byte_value        = 0x7FF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_3byte_value        = 0xFFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_4byte_value        = 0x1FFFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_5byte_value        = 0x3FFFFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_6byte_value        = 0x7FFFFFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_1byte_mask         = 0x80u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_1byte_continuation = 0x00u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_1byte_shift        = 7u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_2byte_mask         = 0xC0u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_2byte_continuation
     = __ztd_idk_detail_start_2byte_mask;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_2byte_shift = 5u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_3byte_mask  = 0xE0u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_3byte_continuation
     = __ztd_idk_detail_start_3byte_mask;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_3byte_shift = 4u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_4byte_mask  = 0xF0u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_4byte_continuation
     = __ztd_idk_detail_start_4byte_mask;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_4byte_shift = 3u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_5byte_mask  = 0xF8u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_5byte_continuation
     = __ztd_idk_detail_start_5byte_mask;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_5byte_shift = 2u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_6byte_mask  = 0xFCu;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_6byte_continuation
     = __ztd_idk_detail_start_6byte_mask;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_start_6byte_shift       = 1u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_continuation_mask       = 0xC0u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_continuation_signature  = 0x80u;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_continuation_mask_value = 0x3Fu;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char8_t __ztd_idk_detail_single_mask_value       = 0x7Fu;

inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_is_lead_surrogate(ztd_char32_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value >= __ztd_idk_detail_first_lead_surrogate && __value <= __ztd_idk_detail_last_lead_surrogate;
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_is_trail_surrogate(ztd_char32_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value >= __ztd_idk_detail_first_trail_surrogate && __value <= __ztd_idk_detail_last_trail_surrogate;
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_is_surrogate(ztd_char32_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value >= __ztd_idk_detail_first_surrogate && __value <= __ztd_idk_detail_last_surrogate;
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_is_lead_utf8(ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return (__value & __ztd_idk_detail_continuation_mask) == __ztd_idk_detail_continuation_signature;
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_is_single_or_lead_utf8(
     ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value <= static_cast<ztd_char8_t>(0x7F) || __ztd_idk_detail_is_lead_utf8(__value);
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_utf8_is_invalid(ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value == 0xC0 || __value == 0xC1 || __value > 0xF4;
}
inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_mutf8_is_invalid(ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value == 0xC1 || __value > 0xF4;
}

inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_utf8_is_overlong(
     ztd_char32_t __value, ::std::size_t __bytes) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value <= __ztd_idk_detail_last_1byte_value
	     || (__value <= __ztd_idk_detail_last_2byte_value && __bytes > 2)
	     || (__value <= __ztd_idk_detail_last_3byte_value && __bytes > 3);
}

inline ZTD_CONSTEXPR_IF_CXX_I_ bool __ztd_idk_detail_utf8_is_overlong_extended(
     ztd_char32_t __value, ::std::size_t __bytes) ZTD_NOEXCEPT_IF_CXX_I_ {
	return __value <= __ztd_idk_detail_last_1byte_value
	     || (__value <= __ztd_idk_detail_last_2byte_value && __bytes > 2)
	     || (__value <= __ztd_idk_detail_last_3byte_value && __bytes > 3)
	     || (__value <= __ztd_idk_detail_last_4byte_value && __bytes > 4)
	     || (__value <= __ztd_idk_detail_last_5byte_value && __bytes > 5);
}

inline ZTD_CONSTEXPR_IF_CXX_I_ int __ztd_idk_detail_utf8_decode_length(ztd_char32_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__value <= __ztd_idk_detail_last_1byte_value) {
		return 1;
	}
	if (__value <= __ztd_idk_detail_last_2byte_value) {
		return 2;
	}
	if (__value <= __ztd_idk_detail_last_3byte_value) {
		return 3;
	}
	if (__value <= __ztd_idk_detail_last_4byte_value) {
		return 4;
	}
	return 0;
}

inline ZTD_CONSTEXPR_IF_CXX_I_ int __ztd_idk_detail_utf8_decode_length_overlong(
     ztd_char32_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__value <= __ztd_idk_detail_last_1byte_value) {
		return 1;
	}
	if (__value <= __ztd_idk_detail_last_2byte_value) {
		return 2;
	}
	if (__value <= __ztd_idk_detail_last_3byte_value) {
		return 3;
	}
	if (__value <= __ztd_idk_detail_last_4byte_value) {
		return 4;
	}
	if (__value <= __ztd_idk_detail_last_5byte_value) {
		return 5;
	}
	if (__value <= __ztd_idk_detail_last_6byte_value) {
		return 6;
	}
	return 0;
}

inline ZTD_CONSTEXPR_IF_CXX_I_ int __ztd_idk_detail_utf8_sequence_length(ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return (__value & __ztd_idk_detail_start_1byte_mask) == __ztd_idk_detail_start_1byte_continuation ? 1
	     : (__value & __ztd_idk_detail_start_3byte_mask) != __ztd_idk_detail_start_3byte_continuation ? 2
	     : (__value & __ztd_idk_detail_start_4byte_mask) != __ztd_idk_detail_start_4byte_continuation ? 3
	                                                                                                  : 4;
}

inline ZTD_CONSTEXPR_IF_CXX_I_ int __ztd_idk_detail_utf8_sequence_length_overlong(
     ztd_char8_t __value) ZTD_NOEXCEPT_IF_CXX_I_ {
	return (__value & __ztd_idk_detail_start_1byte_mask) == __ztd_idk_detail_start_1byte_continuation ? 1
	     : (__value & __ztd_idk_detail_start_3byte_mask) != __ztd_idk_detail_start_3byte_continuation ? 2
	     : (__value & __ztd_idk_detail_start_4byte_mask) != __ztd_idk_detail_start_4byte_continuation ? 3
	     : (__value & __ztd_idk_detail_start_5byte_mask) != __ztd_idk_detail_start_5byte_continuation ? 4
	     : (__value & __ztd_idk_detail_start_6byte_mask) != __ztd_idk_detail_start_6byte_continuation ? 5
	                                                                                                  : 6;
}

inline ZTD_CONSTEXPR_IF_CXX_I_ ztd_char32_t __ztd_idk_detail_utf8_decode(
     ztd_char8_t __value0, ztd_char8_t __value1) ZTD_NOEXCEPT_IF_CXX_I_ {
	return ((__value0 & 0x1F) << 6) | (__value1 & 0x3F);
}

inline ZTD_CONSTEXPR_IF_CXX_I_ ztd_char32_t __ztd_idk_detail_utf8_decode(
     ztd_char8_t __value0, ztd_char8_t __value1, ztd_char8_t __value2) ZTD_NOEXCEPT_IF_CXX_I_ {
	return ((__value0 & 0x0F) << 12) | ((__value1 & 0x3F) << 6) | (__value2 & 0x3F);
}

inline ZTD_CONSTEXPR_IF_CXX_I_ ztd_char32_t __ztd_idk_detail_utf8_decode(
     ztd_char8_t __value0, ztd_char8_t __value1, ztd_char8_t __value2, ztd_char8_t __value3) ZTD_NOEXCEPT_IF_CXX_I_ {
	return ((__value0 & 0x07) << 18) | ((__value1 & 0x3F) << 12) | ((__value2 & 0x3F) << 6) | (__value3 & 0x3F);
}

ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_ascii_value  = 0x7F;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_last_bmp_value    = 0xFFFF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const ztd_char32_t __ztd_idk_detail_normalizing_value = 0x10000;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const int __ztd_idk_detail_lead_surrogate_bitmask     = 0xFFC00;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const int __ztd_idk_detail_trail_surrogate_bitmask    = 0x3FF;
ZTD_INLINE_CONSTEXPR_IF_CXX_I_ const int __ztd_idk_detail_lead_shifted_bits          = 10;

inline ZTD_CONSTEXPR_IF_CXX_I_ ztd_char32_t __ztd_idk_detail_utf16_combine_surrogates(
     ztd_char16_t __lead, ztd_char16_t __trail) ZTD_NOEXCEPT_IF_CXX_I_ {
	auto __hibits = __lead - __ztd_idk_detail_first_lead_surrogate;
	auto __lobits = __trail - __ztd_idk_detail_first_trail_surrogate;
	return __ztd_idk_detail_normalizing_value + ((__hibits << __ztd_idk_detail_lead_shifted_bits) | __lobits);
}

#endif
