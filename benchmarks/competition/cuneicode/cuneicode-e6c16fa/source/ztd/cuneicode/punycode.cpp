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

#include <ztd/cuneicode/punycode.h>
#include <ztd/cuneicode/max_output.h>
#include <ztd/cuneicode/detail/transcode.hpp>
#include <ztd/cuneicode/detail/punycode.hpp>
#include <ztd/idk/unreachable.hpp>
#include <ztd/idk/size.hpp>
#include <ztd/idk/assert.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <memory>
#include <algorithm>
#include <vector>
#include <optional>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ const ztd_char32_t __u32_xn_prefix[4]
     = { U'x', U'n', U'-', U'-' };

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ const ztd_char_t __xn_prefix[4]
     = { 'x', 'n', '-', '-' };

namespace {
	static bool __is_not_ascii_codepoint(ztd_char32_t __codepoint) noexcept {
		return __codepoint > 0x7Fu;
	}

	static ::std::optional<ztd_char32_t> __get_minimum_codepoint(
	     const ::std::vector<ztd_char32_t>& __input, const ztd_char32_t __current) noexcept {
		::std::optional<ztd_char32_t> __maybe_target_codepoint = __current;
		for (auto __codepoint : __input) {
			const bool __found = __codepoint >= __current
			     && (__maybe_target_codepoint ? __codepoint <= *__maybe_target_codepoint : true);
			if (__found) {
				__maybe_target_codepoint.emplace(__codepoint);
			}
		}
		return __maybe_target_codepoint;
	}
} // namespace

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomcn_punycode(
     size_t* __p_maybe_dst_len, ztd_char_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_pny_encode_state_t __state    = {};
	cnc_pny_encode_state_t* __p_state = ::std::addressof(__state);
	return cnc_c32nrtomcn_punycode(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomcn_punycode(
     size_t* __p_maybe_dst_len, ztd_char_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_pny_encode_state_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_src == nullptr || *__p_src == nullptr) {
		return CNC_MCERROR_OK;
	}
	size_t& __src_len          = *__p_src_len;
	const ztd_char32_t*& __src = *__p_src;
	if (!__p_state->__is_initialized) {
		::cnc::__cnc_detail::__init_pny_encode_state(__p_state, false);
	}
	::cnc::__cnc_detail::__pny_encode_state& __pny
	     = *::cnc::__cnc_detail::__get_pny_encode_state(__p_state);
	if (__p_state->input_is_complete) {
		// now we are reading the input to do work...
		const bool _IsCounting  = __p_maybe_dst == nullptr || __p_maybe_dst[0] == nullptr;
		const bool _IsUnbounded = __p_maybe_dst_len == nullptr;
		const ztd_char32_t*& __modified_src = __pny.__input_current;
		const ztd_char32_t* const __modified_src_last
		     = __pny.__input.data() + __pny.__input.size();
		for (;;) {
			switch (__p_state->__action_state) {
			case ::cnc::__cnc_detail::__pny_encode_state_ace_prefix: {
				if (__modified_src == nullptr) {
					// write out single '-'
					// write out the single prefix bit
					if (!_IsUnbounded) {
						if (__p_maybe_dst_len[0] < 1) {
							return CNC_MCERROR_INSUFFICIENT_OUTPUT;
						}
						__p_maybe_dst_len[0] -= 1;
					}
					if (!_IsCounting) {
						__p_maybe_dst[0][0] = static_cast<ztd_char_t>('-');
						__p_maybe_dst[0] += 1;
					}
					if (__p_state->__has_seen_non_basic != 0) {
						// there are still unicode characters to write:
						// let's do that!
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_encode_state_rle_unicode;
						__modified_src = __pny.__input.data();
						::cnc::__cnc_detail::__start_non_basic_pny_encode_state(
						     __pny, __p_state->__has_seen_non_basic);
					}
					else {
						// there are no more characters.
						::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
					}
					return CNC_MCERROR_OK;
				}
				else if (__modified_src
				     == &__u32_xn_prefix[ztd_c_array_size(__u32_xn_prefix)]) {
					// it's onto the next stage
					__p_state->__action_state = ::cnc::__cnc_detail::__pny_encode_state_ascii;
					__modified_src            = __pny.__input.data();
					continue;
				}
				else {
					// write out the single prefix bit
					if (!_IsUnbounded) {
						if (__p_maybe_dst_len[0] < 1) {
							return CNC_MCERROR_INSUFFICIENT_OUTPUT;
						}
						__p_maybe_dst_len[0] -= 1;
					}
					if (!_IsCounting) {
						__p_maybe_dst[0][0] = static_cast<ztd_char_t>(*__modified_src);
						__p_maybe_dst[0] += 1;
					}
					++__modified_src;
					return CNC_MCERROR_OK;
				}
			}
			case ::cnc::__cnc_detail::__pny_encode_state_ascii: {
				bool __is_not_last_code_point;
				for (;; ++__modified_src) {
					__is_not_last_code_point = (__modified_src != __modified_src_last);
					if (__is_not_last_code_point
					     && __is_not_ascii_codepoint(*__modified_src)) {
						// Skip all non-ascii codepoints
						continue;
					}
					break;
				}
				if (!__is_not_last_code_point) {
					// no more ASCII characters left to write!
					if (!__p_state->__idna) {
						// we have to output a - no matter what, since this is not idna,
						// even if there are no more characters
						__pny.__input_current = nullptr;
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_encode_state_ace_prefix;
						continue;
					}
					else if (__p_state->__has_seen_non_basic == 0) {
						// there are no more characters to output; get out of here.
						::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
						return CNC_MCERROR_OK;
					}
					else {
						// output a - and then go to the unicode section
						if (!_IsUnbounded) {
							if (__p_maybe_dst_len[0] < 1) {
								return CNC_MCERROR_INSUFFICIENT_OUTPUT;
							}
							__p_maybe_dst_len[0] -= 1;
						}
						if (!_IsCounting) {
							__p_maybe_dst[0][0] = static_cast<ztd_char_t>('-');
							__p_maybe_dst[0] += 1;
						}
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_encode_state_rle_unicode;
						__modified_src = __pny.__input.data();
						::cnc::__cnc_detail::__start_non_basic_pny_encode_state(
						     __pny, __p_state->__has_seen_non_basic);
						return CNC_MCERROR_OK;
					}
				}
				// write out the single ASCII character
				if (!_IsUnbounded) {
					if (__p_maybe_dst_len[0] < 1) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					__p_maybe_dst_len[0] -= 1;
				}
				if (!_IsCounting) {
					__p_maybe_dst[0][0] = static_cast<ztd_char_t>(*__modified_src);
					__p_maybe_dst[0] += 1;
				}
				++__modified_src;
				return CNC_MCERROR_OK;
			}
			case ::cnc::__cnc_detail::__pny_encode_state_rle_unicode: {
				// TODO: actually write this monstrosity...
				constexpr auto& __tmin           = ::cnc::__cnc_detail::__pny_tmin;
				constexpr auto& __tmax           = ::cnc::__cnc_detail::__pny_tmax;
				constexpr auto& __base           = ::cnc::__cnc_detail::__pny_base;
				ztd_char32_t& __n                = __pny.__n;
				::std::size_t& __delta           = __pny.__delta;
				::std::size_t& __bias            = __pny.__bias;
				size_t& __h                      = __pny.__h;
				const ::std::size_t __input_size = __pny.__input.size();
				const ::std::size_t __b          = static_cast<::std::size_t>(
                         __input_size - __p_state->__has_seen_non_basic);
				for (;;) {
					if (__h >= __input_size) {
						break;
						// we are fully done!
					}
					const ::std::optional<ztd_char32_t> __maybe_m
					     = __get_minimum_codepoint(__pny.__input, __n);
					if (!__maybe_m) {
						// This should never happen, but if we end up in a state where
						// there's a minimum codepoint and it can't be found, we need to
						// destroy the state and return the invalid code.
						::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
						return CNC_MCERROR_INVALID_SEQUENCE;
					}
					const ztd_char32_t __m         = *__maybe_m;
					const ztd_char32_t __m_minus_n = (__m - __n);
					const ::std::size_t __h_plus_1 = (__h + 1);
					if (::cnc::__cnc_detail::__pny_will_overflow_add_mul(
					         __delta, __m_minus_n, __h_plus_1)) {
						::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
						return CNC_MCERROR_INVALID_SEQUENCE;
					}
					__delta = __delta + (__m_minus_n) * (__h_plus_1);
					__n     = __m;
					for (ztd_char32_t __c32 : __pny.__input) {
						if (__c32 < __n) {
							if (::cnc::__cnc_detail::__pny_will_overflow_add(__delta, 1)) {
								::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
								return CNC_MCERROR_INVALID_SEQUENCE;
							}
							__delta += 1;
						}
						else if (__c32 == __n) {
							::std::size_t __q = __delta;
							for (::std::size_t __k = __base;; __k += __base) {
								::std::size_t __t = __k <= __bias
								     ? __tmin
								     : (__k >= (__bias + __tmax) ? __tmax : (__k - __bias));
								if (__q < __t) {
									break;
								}
								if (!_IsUnbounded) {
									if (__p_maybe_dst_len[0] < 1) {
										return CNC_MCERROR_INSUFFICIENT_OUTPUT;
									}
									__p_maybe_dst_len[0] -= 1;
								}
								if (!_IsCounting) {
									size_t __digit = __t + ((__q - __t) % (__base - __t));
									const ztd_char32_t* __digit_map
									     = __p_state->uppercase_letters
									     ? ::cnc::__cnc_detail::
									          __pny_digit_to_uppercase_codepoint_map
									     : ::cnc::__cnc_detail::
									          __pny_digit_to_lowercase_codepoint_map;

									const ztd_char32_t __out_c32 = __digit_map[__digit];
									__p_maybe_dst[0][0]
									     = static_cast<ztd_char_t>(__out_c32);
									__p_maybe_dst[0] += 1;
								}
								__q = (__q - __t) / (__base - __t);
							}
							if (!_IsUnbounded) {
								if (__p_maybe_dst_len[0] < 1) {
									return CNC_MCERROR_INSUFFICIENT_OUTPUT;
								}
								__p_maybe_dst_len[0] -= 1;
							}
							if (!_IsCounting) {
								// TODO: what the hell are they asking us to output here??
								const ztd_char32_t* __digit_map
								     = __p_state->uppercase_letters
								     ? ::cnc::__cnc_detail::
								          __pny_digit_to_uppercase_codepoint_map
								     : ::cnc::__cnc_detail::
								          __pny_digit_to_lowercase_codepoint_map;
								const ztd_char32_t __out_c32 = __digit_map[__q];
								__p_maybe_dst[0][0] = static_cast<ztd_char_t>(__out_c32);
								__p_maybe_dst[0] += 1;
							}
							__bias = ::cnc::__cnc_detail::__pny_adapt_bias(
							     __delta, __h + 1, __h == __b);
							__delta = 0;
							__h += 1;
						}
					}
					__delta += 1;
					__n += 1;
					if (__h >= __input_size) {
						break;
						// we are fully done!
					}
					return CNC_MCERROR_OK;
				}
				// we are completely done!
				::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
				return CNC_MCERROR_OK;
			} break;
			default: {
				ZTD_UNREACHABLE();
				break;
			}
			}
		}
		return CNC_MCERROR_INVALID_SEQUENCE;
	}

	if (__src_len == 0) {
		// we need to write out data from the current and last now...
		// gather up data until we are complete...
		__p_state->__action_state += 1;
		if (__p_state->__action_state
		     == ::cnc::__cnc_detail::__pny_encode_consume_state_no_data_twice) {
			// we have had 0 input twice in a row; switch the completeness switch on to FORCE
			// additional processing.
			const bool __need_ace_prefix
			     = __p_state->__idna && __p_state->__has_seen_non_basic != 0;
			if (__need_ace_prefix) {
				__p_state->__action_state = ::cnc::__cnc_detail::__pny_encode_state_ace_prefix;
				__pny.__input_current     = &__u32_xn_prefix[0];
			}
			else {
				__p_state->__action_state = ::cnc::__cnc_detail::__pny_encode_state_ascii;
				__pny.__input_current     = __pny.__input.data();
			}
			__p_state->input_is_complete = 1u;
		}
	}
	else {
		const ztd_char32_t __c32 = __src[0];
		if (__c32 > __ztd_idk_detail_last_unicode_code_point) {
			::cnc::__cnc_detail::__destroy_pny_encode_state(__p_state);
			return CNC_MCERROR_INVALID_SEQUENCE;
		}
		__p_state->__has_seen_non_basic
		     += static_cast<uintptr_t>(__is_not_ascii_codepoint(__c32));
		__pny.__input.push_back(__c32);
		__src_len -= 1;
		__src += 1;
	}
	return CNC_MCERROR_OK;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc32n_punycode(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_pny_decode_state_t __state    = {};
	cnc_pny_decode_state_t* __p_state = ::std::addressof(__state);
	return cnc_mcnrtoc32n_punycode(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc32n_punycode(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char_t** __p_src, cnc_pny_decode_state_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	return ::cnc::__cnc_detail::__cnc_mcnrtoc32n_punycode_maybe_idna<false>(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}



ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32sntomcsn_punycode(
     size_t* __p_maybe_dst_len, ztd_char_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_pny_encode_state_t __state    = {};
	cnc_pny_encode_state_t* __p_state = ::std::addressof(__state);
	return cnc_c32snrtomcsn_punycode(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32snrtomcsn_punycode(
     size_t* __p_maybe_dst_len, ztd_char_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_pny_encode_state_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_c32nrtomcn_punycode), &cnc_c32nrtomcn_punycode,
	     ztd_char32_t, ztd_char_t, decltype(&cnc_pny_encode_state_is_complete),
	     &cnc_pny_encode_state_is_complete);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc32sn_punycode(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_pny_decode_state_t __state    = {};
	cnc_pny_decode_state_t* __p_state = ::std::addressof(__state);
	return cnc_mcsnrtoc32sn_punycode(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc32sn_punycode(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char_t** __p_src, cnc_pny_decode_state_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_mcnrtoc32n_punycode), &cnc_mcnrtoc32n_punycode,
	     ztd_char_t, ztd_char32_t, decltype(&cnc_pny_decode_state_is_complete),
	     &cnc_pny_decode_state_is_complete);
}
