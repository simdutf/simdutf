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

#ifndef ZTD_CUNEICODE_DETAIL_MCCHAR_INTERNAL_HPP
#define ZTD_CUNEICODE_DETAIL_MCCHAR_INTERNAL_HPP

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcchar.h>
#include <ztd/cuneicode/detail/transcode.hpp>
#include <ztd/cuneicode/detail/err.hpp>

#include <ztd/idk/encoding_detection.h>
#include <ztd/idk/detail/unicode.hpp>

#if ZTD_IS_ON(ZTD_CUCHAR)
#include <cuchar>
#elif ZTD_IS_ON(ZTD_UCHAR)
ZTD_EXTERN_C_OPEN_I_
#include <uchar.h>
ZTD_EXTERN_C_CLOSE_I_
#endif
#if ZTD_IS_ON(ZTD_CWCHAR)
#include <cwchar>
#elif ZTD_IS_ON(ZTD_WCHAR)
ZTD_EXTERN_C_OPEN_I_
#include <wchar.h>
ZTD_EXTERN_C_CLOSE_I_
#endif
#include <cstring>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_
	namespace __cnc_detail {

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c32nrtoc8n(size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char32_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len          = *__p_src_len;
			const ztd_char32_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			ztd_char32_t __c0 = *__src;
			if (__c0 > __ztd_idk_detail_last_unicode_code_point) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			size_t __seq_len = static_cast<size_t>(__ztd_idk_detail_utf8_decode_length(__c0));
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < __seq_len) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
				*__p_maybe_dst_len -= __seq_len;
			}
			if constexpr (!_IsCounting) {
				if (__seq_len == 1) {
					// 1 byte
					**__p_maybe_dst = (ztd_char8_t)__c0;
					++*__p_maybe_dst;
				}
				else if (__seq_len == 2) {
					// 2 bytes
					**__p_maybe_dst = (ztd_char8_t)(((__c0 >> 6) & 0b00011111) | (0b11000000));
					++*__p_maybe_dst;
					**__p_maybe_dst = (ztd_char8_t)(((__c0 >> 0) & 0b00111111) | (0b10000000));
					++*__p_maybe_dst;
				}
				else if (__seq_len == 3) {
					// 3 bytes
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0xC) & 0b00001111u) | 0b11100000u);
					++*__p_maybe_dst;
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x6) & 0b00111111u) | 0b10000000u);
					++*__p_maybe_dst;
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x0) & 0b00111111u) | 0b10000000u);
					++*__p_maybe_dst;
				}
				else {
					// 4 bytes
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x12) & 0b00000111u) | 0b11110000u);
					++*__p_maybe_dst;
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x0C) & 0b00111111u) | 0b10000000u);
					++*__p_maybe_dst;
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x06) & 0b00111111u) | 0b10000000u);
					++*__p_maybe_dst;
					**__p_maybe_dst
					     = (ztd_char8_t)(((__c0 >> 0x00) & 0b00111111u) | 0b10000000u);
					++*__p_maybe_dst;
				}
			}
			__src += 1;
			__src_len -= 1;
			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c32nrtoc16n(size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char32_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len          = *__p_src_len;
			const ztd_char32_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			ztd_char32_t __c0 = *__src;
			if (__c0 > __ztd_idk_detail_last_unicode_code_point
			     || __ztd_idk_detail_is_surrogate(__c0)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}

			__src += 1;
			__src_len -= 1;
			if (__c0 <= __ztd_idk_detail_last_bmp_value) {
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = static_cast<ztd_char16_t>(__c0);
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
			}
			else {
				if constexpr (!_IsCounting) {
					ztd_char32_t __normal = __c0 - __ztd_idk_detail_normalizing_value;
					ztd_char32_t __lead   = __ztd_idk_detail_first_lead_surrogate
					     + ((__normal & __ztd_idk_detail_lead_surrogate_bitmask)
					          >> __ztd_idk_detail_lead_shifted_bits);
					ztd_char32_t __trail = __ztd_idk_detail_first_trail_surrogate
					     + (__normal & __ztd_idk_detail_trail_surrogate_bitmask);
					(*__p_maybe_dst)[0] = static_cast<ztd_char16_t>(__lead);
					(*__p_maybe_dst)[1] = static_cast<ztd_char16_t>(__trail);
					*__p_maybe_dst += 2;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 2;
				}
			}

			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c32nrtoc32n(size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char32_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len          = *__p_src_len;
			const ztd_char32_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < 1) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
			}
			ztd_char32_t __c0 = *__src;
			if (__ztd_idk_detail_is_surrogate(__c0)
			     || __c0 > __ztd_idk_detail_last_unicode_code_point) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			__src += 1;
			__src_len -= 1;
			if constexpr (!_IsCounting) {
				**__p_maybe_dst = __c0;
				*__p_maybe_dst += 1;
			}
			if constexpr (!_IsUnbounded) {
				*__p_maybe_dst_len -= 1;
			}
			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c32nrtomcn(size_t* __p_maybe_dst_len, char** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char32_t** __p_src,
		     cnc_mcstate_t* __p_state) noexcept {
			if constexpr ((sizeof(ztd_char_t) == sizeof(ztd_char8_t))
			     && (alignof(ztd_char_t) == alignof(ztd_char8_t))) {
				if (ztdc_is_execution_encoding_utf8()) {
					return ::cnc::__cnc_detail::__c32nrtoc8n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len,
					     reinterpret_cast<ztd_char8_t**>(__p_maybe_dst), __p_src_len, __p_src,
					     __p_state);
				}
			}
#if (ZTD_IS_ON(ZTD_CUCHAR) || ZTD_IS_ON(ZTD_UCHAR)) && ZTD_IS_OFF(ZTD_PLATFORM_MAC_OS)
			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}

			constexpr const size_t __max_accumulated_writeouts = 4;
			const ztd_char32_t*& __src                         = *__p_src;
			char __multibyte_intermediate[(MB_LEN_MAX * __max_accumulated_writeouts)] {};
			for (size_t __src_idx = 0; __src_idx < __max_accumulated_writeouts; ++__src_idx) {
				const ztd_char32_t& __c32 = __src[__src_idx];
				size_t __res              = ZTD_UCHAR_ACCESSOR_I_ c32rtomb(
				                  __multibyte_intermediate, __c32, &__p_state->__locale.__state1);
				if (__res == ::cnc::__cnc_detail::__stdc_ret_err_invalid) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				else if (__res == ::cnc::__cnc_detail::__stdc_ret_no_write_out_yet) {
					continue;
				}
				if constexpr (!_IsUnbounded) {
					if (*__p_maybe_dst_len < __res) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					*__p_maybe_dst_len -= __res;
				}
				if constexpr (!_IsCounting) {
					::std::memcpy(*__p_maybe_dst, (__multibyte_intermediate + 0),
					     __res * sizeof(*__multibyte_intermediate));
					*__p_maybe_dst += __res;
				}
				size_t __source_read = (__src_idx + 1);
				__src += __source_read;
				__src_len -= __source_read;
				break;
			}
			return CNC_MCERROR_OK;
#else
			return ::cnc::__cnc_detail::__c32nrtoc8n<_IsCounting, _IsUnbounded, _SourceChecks>(
			     __p_maybe_dst_len, reinterpret_cast<ztd_char8_t**>(__p_maybe_dst), __p_src_len,
			     __p_src, __p_state);
#endif
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		static inline cnc_mcerror __c8nrtoc8n(size_t* __p_maybe_dst_len,
		     ztd_char8_t** __p_maybe_dst, size_t* __p_src_len, const ztd_char8_t** __p_src,
		     cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || __p_src_len == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}

			size_t __src_idx          = 0;
			const ztd_char8_t*& __src = *__p_src;
			ztd_char8_t __c8          = __src[__src_idx];
			++__src_idx;
			if (__c8 <= 0x7F) {
				if constexpr (!_IsUnbounded) {
					if (*__p_maybe_dst_len < 1) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					*__p_maybe_dst_len -= 1;
				}
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = __c8;
					*__p_maybe_dst += 1;
				}
				__src += 1;
				__src_len -= 1;
				return CNC_MCERROR_OK;
			}
			if (__ztd_idk_detail_is_lead_utf8(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			if (__ztd_idk_detail_utf8_is_invalid(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			size_t __seq_len
			     = static_cast<size_t>(__ztd_idk_detail_utf8_sequence_length_overlong(__c8));
			if (__seq_len > 4) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}

			if constexpr (!_IsUnbounded) {
				if (__seq_len > *__p_maybe_dst_len) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
				*__p_maybe_dst_len -= __seq_len;
			}
			if constexpr (!_IsCounting) {
				for (; __src_idx < __seq_len; ++__src_idx) {
					if (__src_idx == __src_len) {
						return CNC_MCERROR_INCOMPLETE_INPUT;
					}
					__c8 = __src[__src_idx];
					if (!__ztd_idk_detail_is_lead_utf8(__c8)) {
						return CNC_MCERROR_INVALID_SEQUENCE;
					}
				}
				::std::memcpy(*__p_maybe_dst, __src, __seq_len * sizeof(*__src));
				*__p_maybe_dst += __seq_len;
			}
			__src += __seq_len;
			__src_len -= __seq_len;
			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c8nrtoc16n(size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char8_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len         = *__p_src_len;
			const ztd_char8_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t __src_idx = 0;
			ztd_char8_t __c8 = __src[__src_idx];
			++__src_idx;
			if (__c8 <= 0x7F) {
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = static_cast<ztd_char16_t>(__c8);
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
				__src += 1;
				__src_len -= 1;
				return CNC_MCERROR_OK;
			}
			if (__ztd_idk_detail_is_lead_utf8(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			if (__ztd_idk_detail_utf8_is_invalid(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			size_t __seq_len
			     = static_cast<size_t>(__ztd_idk_detail_utf8_sequence_length_overlong(__c8));
			if (__seq_len > 4) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}

			ztd_char32_t __c32s[1] = { static_cast<ztd_char32_t>(__c8)
				& static_cast<ztd_char32_t>(0x1F >> (__seq_len - 2)) };
			ztd_char32_t& __c32    = __c32s[0];

			for (; __src_idx < __seq_len; ++__src_idx) {
				__c8 = __src[__src_idx];
				if (__src_idx == __src_len) {
					return CNC_MCERROR_INCOMPLETE_INPUT;
				}
				if (!__ztd_idk_detail_is_lead_utf8(__c8)) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				__c32 <<= 6;
				__c32 += __c8 & 0x3F;
			}

			if (__c32 <= __ztd_idk_detail_last_bmp_value) {
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = static_cast<ztd_char16_t>(__c32);
					++*__p_maybe_dst;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
			}
			else {
				if constexpr (!_IsCounting) {
					ztd_char32_t __normal = __c32 - __ztd_idk_detail_normalizing_value;
					ztd_char32_t __lead   = __ztd_idk_detail_first_lead_surrogate
					     + ((__normal & __ztd_idk_detail_lead_surrogate_bitmask)
					          >> __ztd_idk_detail_lead_shifted_bits);
					ztd_char32_t __trail = __ztd_idk_detail_first_trail_surrogate
					     + (__normal & __ztd_idk_detail_trail_surrogate_bitmask);
					(*__p_maybe_dst)[0] = static_cast<ztd_char16_t>(__lead);
					(*__p_maybe_dst)[1] = static_cast<ztd_char16_t>(__trail);
					*__p_maybe_dst += 2;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 2;
				}
			}

			__src += __seq_len;
			__src_len -= __seq_len;

			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c8nrtoc32n(size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char8_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len         = *__p_src_len;
			const ztd_char8_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t __src_idx = 0;
			ztd_char8_t __c8 = __src[__src_idx];
			++__src_idx;
			if (__c8 <= 0x7F) {
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = static_cast<ztd_char32_t>(__c8);
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
				__src += 1;
				__src_len -= 1;
				return CNC_MCERROR_OK;
			}
			if (__ztd_idk_detail_is_lead_utf8(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			if (__ztd_idk_detail_utf8_is_invalid(__c8)) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			size_t __seq_len
			     = static_cast<size_t>(__ztd_idk_detail_utf8_sequence_length_overlong(__c8));
			if (__seq_len > 4) {
				return CNC_MCERROR_INVALID_SEQUENCE;
			}

			if constexpr (!_IsUnbounded) {
				if (1 > *__p_maybe_dst_len) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				*__p_maybe_dst_len -= 1;
			}
			if constexpr (!_IsCounting) {
				ztd_char32_t __c32s[1] = { static_cast<ztd_char32_t>(__c8)
					& static_cast<ztd_char32_t>(0x1F >> (__seq_len - 2)) };
				ztd_char32_t& __c32    = __c32s[0];

				for (; __src_idx < __seq_len; ++__src_idx) {
					__c8 = __src[__src_idx];
					if (__src_idx == __src_len) {
						return CNC_MCERROR_INCOMPLETE_INPUT;
					}
					if (!__ztd_idk_detail_is_lead_utf8(__c8)) {
						return CNC_MCERROR_INVALID_SEQUENCE;
					}
					__c32 <<= 6;
					__c32 += __c8 & 0x3F;
				}

				**__p_maybe_dst = __c32;
				*__p_maybe_dst += 1;
			}
			__src += __seq_len;
			__src_len -= __seq_len;

			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c16nrtoc8n(size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char16_t** __p_src,
		     cnc_mcstate_t* __p_state) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len          = *__p_src_len;
			const ztd_char16_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			const ztd_char16_t& __c0              = *__src;
			const bool __c0_is_leading_surrogate  = __ztd_idk_detail_is_lead_surrogate(__c0);
			const bool __c0_is_trailing_surrogate = __ztd_idk_detail_is_trail_surrogate(__c0);
			ztd_char32_t __point;
			if (__c0_is_leading_surrogate) {
				if (__src_len < 1) {
					return CNC_MCERROR_INCOMPLETE_INPUT;
				}
				const ztd_char16_t& __c1 = *(__src + 1);
				if (!__ztd_idk_detail_is_trail_surrogate(__c1)) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				__point             = __ztd_idk_detail_utf16_combine_surrogates(__c0, __c1);
				size_t __input_size = 1;
				const ztd_char32_t* __p_input = &__point;
				cnc_mcerror __intermediate_err
				     = ::cnc::__cnc_detail::__c32nrtoc8n<_IsCounting, _IsUnbounded, false>(
				          __p_maybe_dst_len, __p_maybe_dst, &__input_size, &__p_input,
				          __p_state);
				if (__intermediate_err == CNC_MCERROR_OK) {
					__src += 2;
					__src_len -= 2;
				}
				return __intermediate_err;
			}
			else {
				if (__c0_is_trailing_surrogate || __c0 > __ztd_idk_detail_last_bmp_value) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				__point                       = static_cast<ztd_char32_t>(__c0);
				size_t __input_size           = 1;
				const ztd_char32_t* __p_input = &__point;
				cnc_mcerror __intermediate_err
				     = ::cnc::__cnc_detail::__c32nrtoc8n<_IsCounting, _IsUnbounded, false>(
				          __p_maybe_dst_len, __p_maybe_dst, &__input_size, &__p_input,
				          __p_state);
				if (__intermediate_err == CNC_MCERROR_OK) {
					__src += 1;
					__src_len -= 1;
				}
				return __intermediate_err;
			}
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c16nrtoc16n(size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char16_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len          = *__p_src_len;
			const ztd_char16_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < 1) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
			}
			const ztd_char16_t& __c0        = *__src;
			bool __c0_is_leading_surrogate  = __ztd_idk_detail_is_lead_surrogate(__c0);
			bool __c0_is_trailing_surrogate = __ztd_idk_detail_is_trail_surrogate(__c0);
			if (__c0_is_leading_surrogate) {
				if (__src_len < 1) {
					return CNC_MCERROR_INCOMPLETE_INPUT;
				}
				const ztd_char16_t& __c1 = *(__src + 1);
				if (!__ztd_idk_detail_is_trail_surrogate(__c1)) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				if constexpr (!_IsCounting) {
					(*__p_maybe_dst)[0] = __c0;
					(*__p_maybe_dst)[1] = __c1;
					(*__p_maybe_dst) += 2;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 2;
				}
				__src += 2;
				__src_len -= 2;
			}
			else {
				if (__c0_is_trailing_surrogate || __c0 > __ztd_idk_detail_last_bmp_value) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				if constexpr (!_IsCounting) {
					**__p_maybe_dst = __src[0];
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
				__src += 1;
				__src_len -= 1;
			}
			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c16nrtoc32n(size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char16_t** __p_src, cnc_mcstate_t*) noexcept {
			if constexpr (_SourceChecks) {
				if (__p_src_len == nullptr || __p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}

			size_t& __src_len          = *__p_src_len;
			const ztd_char16_t*& __src = *__p_src;
			if constexpr (_SourceChecks) {
				if (__src_len == 0 || __src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < 1) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
			}
			const ztd_char16_t& __c0              = *__src;
			const bool __c0_is_leading_surrogate  = __ztd_idk_detail_is_lead_surrogate(__c0);
			const bool __c0_is_trailing_surrogate = __ztd_idk_detail_is_trail_surrogate(__c0);
			[[maybe_unused]] ztd_char32_t __point;
			if (__c0_is_leading_surrogate) {
				if (__src_len < 1) {
					return CNC_MCERROR_INCOMPLETE_INPUT;
				}
				const ztd_char16_t& __c1 = *(__src + 1);
				if (!__ztd_idk_detail_is_trail_surrogate(__c1)) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				__point = __ztd_idk_detail_utf16_combine_surrogates(__c0, __c1);
				__src += 2;
				__src_len -= 2;
			}
			else {
				if (__c0_is_trailing_surrogate || __c0 > __ztd_idk_detail_last_bmp_value) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				__point = static_cast<ztd_char32_t>(__c0);
				__src += 1;
				__src_len -= 1;
			}
			if constexpr (!_IsCounting) {
				**__p_maybe_dst = __point;
				*__p_maybe_dst += 1;
			}
			if constexpr (!_IsUnbounded) {
				*__p_maybe_dst_len -= 1;
			}
			return CNC_MCERROR_OK;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __mcnrtomwcn(size_t* __p_maybe_dst_len, ztd_wchar_t** __p_maybe_dst,
		     size_t* __p_src_len, const char** __p_src, cnc_mcstate_t* __p_state) noexcept {
			if constexpr ((sizeof(ztd_char_t) == sizeof(ztd_char8_t))
			     && (alignof(ztd_char_t) == alignof(ztd_char8_t))) {
				if (ztdc_is_execution_encoding_utf8()) {
					if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char16_t))
					     && (alignof(ztd_wchar_t) == alignof(ztd_char16_t))) {
						if (ztdc_is_wide_execution_encoding_utf16()) {
							return ::cnc::__cnc_detail::__c8nrtoc16n<_IsCounting,
							     _IsUnbounded, _SourceChecks>(__p_maybe_dst_len,
							     reinterpret_cast<ztd_char16_t**>(__p_maybe_dst),
							     __p_src_len, reinterpret_cast<const ztd_char8_t**>(__p_src),
							     __p_state);
						}
					}
					if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char32_t))
					     && (alignof(ztd_wchar_t) == alignof(ztd_char32_t))) {
						if (ztdc_is_wide_execution_encoding_utf32()) {
							return ::cnc::__cnc_detail::__c8nrtoc32n<_IsCounting,
							     _IsUnbounded, _SourceChecks>(__p_maybe_dst_len,
							     reinterpret_cast<ztd_char32_t**>(__p_maybe_dst),
							     __p_src_len, reinterpret_cast<const ztd_char8_t**>(__p_src),
							     __p_state);
						}
					}
				}
			}
#if ZTD_IS_ON(ZTD_WCHAR) || ZTD_IS_ON(ZTD_CWCHAR)
			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < 1) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
			}
			const char*& __src = *__p_src;
			// can only output one, but we leave space anyhow
			ztd_wchar_t __intermediate_destination[4] {};
			ztd_wchar_t* __dst = _IsCounting ? __intermediate_destination : *__p_maybe_dst;
			size_t __c_err     = ZTD_WCHAR_ACCESSOR_I_ mbrtowc(
			         __dst, __src, __src_len, &__p_state->__locale.__state0);
			switch (__c_err) {
			case ::cnc::__cnc_detail::__stdc_ret_err_invalid:
				return CNC_MCERROR_INVALID_SEQUENCE;
			case ::cnc::__cnc_detail::__stdc_ret_err_incomplete_input:
				return CNC_MCERROR_INCOMPLETE_INPUT;
			case ::cnc::__cnc_detail::__stdc_ret_null_value:
				// read the null character, wrote the null character
				if constexpr (!_IsCounting) {
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
				__src += 1;
				__src_len -= 1;
				break;
			default:
				// number of bytes successfully converted
				if constexpr (!_IsCounting) {
					*__p_maybe_dst += 1;
				}
				if constexpr (!_IsUnbounded) {
					*__p_maybe_dst_len -= 1;
				}
				__src += __c_err;
				__src_len -= __c_err;
				break;
			}
			return CNC_MCERROR_OK;
#else
			::std::abort();
			return CNC_MCERROR_INVALID_SEQUENCE;
#endif
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __mwcnrtomcn(size_t* __p_maybe_dst_len, char** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_wchar_t** __p_src,
		     cnc_mcstate_t* __p_state) noexcept {
			if (ztdc_is_execution_encoding_utf8()) {
				if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char16_t))
				     && (alignof(ztd_wchar_t) == alignof(ztd_char16_t))) {
					if (ztdc_is_wide_execution_encoding_utf16()) {
						return ::cnc::__cnc_detail::__c16nrtoc8n<_IsCounting, _IsUnbounded,
						     _SourceChecks>(__p_maybe_dst_len,
						     reinterpret_cast<ztd_char8_t**>(__p_maybe_dst), __p_src_len,
						     reinterpret_cast<const ztd_char16_t**>(__p_src), __p_state);
					}
				}
				if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char32_t))
				     && (alignof(ztd_wchar_t) == alignof(ztd_char32_t))) {
					if (ztdc_is_wide_execution_encoding_utf32()) {
						return ::cnc::__cnc_detail::__c32nrtoc8n<_IsCounting, _IsUnbounded,
						     _SourceChecks>(__p_maybe_dst_len,
						     reinterpret_cast<ztd_char8_t**>(__p_maybe_dst), __p_src_len,
						     reinterpret_cast<const ztd_char32_t**>(__p_src), __p_state);
					}
				}
			}
#if ZTD_IS_ON(ZTD_WCHAR) || ZTD_IS_ON(ZTD_CWCHAR)
			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}
			if constexpr (!_IsUnbounded) {
				if (*__p_maybe_dst_len < 1) {
					return CNC_MCERROR_INSUFFICIENT_OUTPUT;
				}
			}
			const ztd_wchar_t*& __src = *__p_src;

			constexpr const size_t __max_accumulated_writeouts = 4;
			char __multibyte_intermediate[(MB_LEN_MAX * __max_accumulated_writeouts)] {};
			for (size_t __src_idx = 0; __src_idx < __max_accumulated_writeouts; ++__src_idx) {
				const ztd_wchar_t& __wc = __src[__src_idx];
				size_t __res            = ZTD_WCHAR_ACCESSOR_I_ wcrtomb(
				                __multibyte_intermediate, __wc, &__p_state->__locale.__state1);
				if (__res == ::cnc::__cnc_detail::__stdc_ret_err_invalid) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				else if (__res == ::cnc::__cnc_detail::__stdc_ret_no_write_out_yet) {
					continue;
				}
				if constexpr (!_IsUnbounded) {
					if (*__p_maybe_dst_len < __res) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					*__p_maybe_dst_len -= __res;
				}
				if constexpr (!_IsCounting) {
					::std::memcpy(*__p_maybe_dst, __multibyte_intermediate,
					     __res * sizeof(*__multibyte_intermediate));
					*__p_maybe_dst += __res;
				}
				break;
			}
			return CNC_MCERROR_OK;
#else
			::std::abort();
			return CNC_MCERROR_INVALID_SEQUENCE;
#endif
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __c32nrtomwcn(size_t* __p_maybe_dst_len, ztd_wchar_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_char32_t** __p_src,
		     cnc_mcstate_t* __p_state) noexcept {
			if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char32_t))
			     && (alignof(ztd_wchar_t) == alignof(ztd_char32_t))) {
				if (ztdc_is_wide_execution_encoding_utf16()) {
					return ::cnc::__cnc_detail::__c32nrtoc32n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len,
					     reinterpret_cast<ztd_char32_t**>(__p_maybe_dst), __p_src_len, __p_src,
					     __p_state);
				}
			}
			if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char16_t))
			     && (alignof(ztd_wchar_t) == alignof(ztd_char16_t))) {
				if (ztdc_is_wide_execution_encoding_utf16()) {
					return ::cnc::__cnc_detail::__c32nrtoc16n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len,
					     reinterpret_cast<ztd_char16_t**>(__p_maybe_dst), __p_src_len, __p_src,
					     __p_state);
				}
			}
			constexpr const size_t __max_accumulated_writeouts = 4;
			constexpr const size_t __initial_multibyte_intermediate_size
			     = (MB_LEN_MAX * __max_accumulated_writeouts);

			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}
			const ztd_char32_t*& __src = *__p_src;

			const ztd_char32_t* __initial_src           = __src;
			const size_t __initial_src_len              = __src_len;
			size_t __multibyte_intermediate_output_size = __initial_multibyte_intermediate_size;
			char __multibyte_intermediate[__initial_multibyte_intermediate_size] {};
			char* __p_multibyte_intermediate_output = __multibyte_intermediate;
			cnc_mcerror __intermediate_err
			     = ::cnc::__cnc_detail::__c32nrtomcn<_IsCounting, _IsUnbounded, false>(
			          &__multibyte_intermediate_output_size, &__p_multibyte_intermediate_output,
			          __p_src_len, __p_src, __p_state);
			if (__intermediate_err != CNC_MCERROR_OK) {
				return __intermediate_err;
			}
			size_t __multibyte_intermediate_input_size
			     = __initial_multibyte_intermediate_size - __multibyte_intermediate_output_size;
			const char* __p_multibyte_intermediate_input = __multibyte_intermediate;
			cnc_mcerror __err
			     = ::cnc::__cnc_detail::__mcnrtomwcn<_IsCounting, _IsUnbounded, false>(
			          __p_maybe_dst_len, __p_maybe_dst, &__multibyte_intermediate_input_size,
			          &__p_multibyte_intermediate_input, __p_state);
			if (__err != CNC_MCERROR_OK) {
				__src     = __initial_src;
				__src_len = __initial_src_len;
			}
			return __err;
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __mcnrtoc32n(size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst,
		     size_t* __p_src_len, const char** __p_src, cnc_mcstate_t* __p_state) noexcept {
			if constexpr ((sizeof(ztd_char_t) == sizeof(ztd_char8_t))
			     && (alignof(ztd_char_t) == alignof(ztd_char8_t))) {
				if (ztdc_is_execution_encoding_utf8()) {
					return ::cnc::__cnc_detail::__c8nrtoc32n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len, __p_maybe_dst, __p_src_len,
					     reinterpret_cast<const ztd_char8_t**>(__p_src), __p_state);
				}
			}
#if (ZTD_IS_ON(ZTD_CUCHAR) || ZTD_IS_ON(ZTD_UCHAR)) && ZTD_IS_OFF(ZTD_PLATFORM_MAC_OS)
			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}
			const char*& __src = *__p_src;

			constexpr const size_t __max_writeout_attempts = 4;
			ztd_char32_t __c32_intermediate[__max_writeout_attempts] {};
			ztd_char32_t* __p_c32_intermediate = __c32_intermediate;
			for (size_t __src_idx = 0; __src_idx < __max_writeout_attempts; ++__src_idx) {
				size_t __res = ZTD_UCHAR_ACCESSOR_I_ mbrtoc32(
				     __p_c32_intermediate, __src, __src_len, &__p_state->__locale.__state0);
				if (__res == ::cnc::__cnc_detail::__stdc_ret_err_invalid) {
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
				else if (__res == ::cnc::__cnc_detail::__stdc_ret_err_incomplete_input) {
					return CNC_MCERROR_INCOMPLETE_INPUT;
				}
				else if (ZTD_UCHAR_ACCESSOR_I_ mbsinit(&__p_state->__locale.__state0) == 0) {
					++__p_c32_intermediate;
					continue;
				}
				size_t __c32_written = (__p_c32_intermediate + 1) - __c32_intermediate;
				if constexpr (!_IsUnbounded) {
					if (*__p_maybe_dst_len < __c32_written) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					*__p_maybe_dst_len -= __c32_written;
				}
				if constexpr (!_IsCounting) {
					::std::memcpy(*__p_maybe_dst, (__c32_intermediate + 0),
					     __c32_written * sizeof(*__c32_intermediate));
					*__p_maybe_dst += __c32_written;
				}
				if (__res != ::cnc::__cnc_detail::__stdc_ret_accumulated_write_out) {
					// accumulate write outs and mandated not to read any input
					size_t __source_read
					     = __res == ::cnc::__cnc_detail::__stdc_ret_null_value ? 1 : __res;
					__src += __source_read;
					__src_len -= __source_read;
				}
				else {
					// nothing?
				}
				break;
			}
			return CNC_MCERROR_OK;
#else
			return ::cnc::__cnc_detail::__c8nrtoc32n<_IsCounting, _IsUnbounded, _SourceChecks>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len,
			     reinterpret_cast<const ztd_char8_t**>(__p_src), __p_state);
#endif
		}

		template <bool _IsCounting, bool _IsUnbounded, bool _SourceChecks = true>
		cnc_mcerror __mwcnrtoc32n(size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst,
		     size_t* __p_src_len, const ztd_wchar_t** __p_src,
		     cnc_mcstate_t* __p_state) noexcept {
			if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char32_t))
			     && (alignof(ztd_wchar_t) == alignof(ztd_char32_t))) {
				if (ztdc_is_wide_execution_encoding_utf32()) {
					return ::cnc::__cnc_detail::__c32nrtoc32n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len, __p_maybe_dst, __p_src_len,
					     reinterpret_cast<const ztd_char32_t**>(__p_src), __p_state);
				}
			}
			if constexpr ((sizeof(ztd_wchar_t) == sizeof(ztd_char16_t))
			     && (alignof(ztd_wchar_t) == alignof(ztd_char16_t))) {
				if (ztdc_is_wide_execution_encoding_utf16()) {
					return ::cnc::__cnc_detail::__c16nrtoc32n<_IsCounting, _IsUnbounded,
					     _SourceChecks>(__p_maybe_dst_len, __p_maybe_dst, __p_src_len,
					     reinterpret_cast<const ztd_char16_t**>(__p_src), __p_state);
				}
			}
			constexpr const size_t __max_accumulated_writeouts = 4;
			constexpr const size_t __initial_multibyte_intermediate_size
			     = (MB_LEN_MAX * __max_accumulated_writeouts);

			if constexpr (_SourceChecks) {
				if (__p_src == nullptr || *__p_src == nullptr) {
					return CNC_MCERROR_OK;
				}
			}
			size_t& __src_len = *__p_src_len;
			if constexpr (_SourceChecks) {
				if (__src_len == 0) {
					return CNC_MCERROR_OK;
				}
			}
			const ztd_wchar_t*& __src = *__p_src;

			const ztd_wchar_t* __initial_src            = __src;
			const size_t __initial_src_len              = __src_len;
			size_t __multibyte_intermediate_output_size = __initial_multibyte_intermediate_size;
			char __multibyte_intermediate[__initial_multibyte_intermediate_size] {};
			char* __p_multibyte_intermediate_output = __multibyte_intermediate;
			cnc_mcerror __intermediate_err
			     = ::cnc::__cnc_detail::__mwcnrtomcn<_IsCounting, _IsUnbounded, false>(
			          &__multibyte_intermediate_output_size, &__p_multibyte_intermediate_output,
			          __p_src_len, __p_src, __p_state);
			if (__intermediate_err != CNC_MCERROR_OK) {
				return __intermediate_err;
			}
			size_t __multibyte_intermediate_input_size
			     = __initial_multibyte_intermediate_size - __multibyte_intermediate_output_size;
			const char* __p_multibyte_intermediate_input = __multibyte_intermediate;
			cnc_mcerror __err
			     = ::cnc::__cnc_detail::__mcnrtoc32n<_IsCounting, _IsUnbounded, false>(
			          __p_maybe_dst_len, __p_maybe_dst, &__multibyte_intermediate_input_size,
			          &__p_multibyte_intermediate_input, __p_state);
			if (__err != CNC_MCERROR_OK) {
				__src     = __initial_src;
				__src_len = __initial_src_len;
			}
			return __err;
		}

#define _ZTDC_CUNEICODE_SINGLE_N_DEST_TEMPLATE_BODY(_FN, _DL, _D, _SL, _S, _ST) \
	if (_D == nullptr || *_D == nullptr) {                                     \
		if (_DL == nullptr) {                                                 \
			return _FN<true, true>(_DL, _D, _SL, _S, _ST);                   \
		}                                                                     \
		else {                                                                \
			return _FN<true, false>(_DL, _D, _SL, _S, _ST);                  \
		}                                                                     \
	}                                                                          \
	else {                                                                     \
		if (_DL == nullptr) {                                                 \
			return _FN<false, true>(_DL, _D, _SL, _S, _ST);                  \
		}                                                                     \
		else {                                                                \
			return _FN<false, false>(_DL, _D, _SL, _S, _ST);                 \
		}                                                                     \
	}

	} // namespace __cnc_detail

	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif // ZTD_CUNEICODE_DETAIL_MCCHAR_INTERNAL_HPP
