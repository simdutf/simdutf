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

#pragma once

#ifndef ZTD_CUNEICODE_SOURCE_DETAIL_PUNYCODE_HPP
#define ZTD_CUNEICODE_SOURCE_DETAIL_PUNYCODE_HPP

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/cuneicode/punycode_state.h>
#include <ztd/idk/charN_t.hpp>
#include <ztd/idk/to_address.hpp>
#include <ztd/idk/unreachable.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cnc {
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __cnc_detail {

		inline constexpr ztd_char32_t __pny_digit_to_lowercase_codepoint_map[36]
		     = { U'a', U'b', U'c', U'd', U'e', U'f', U'g', U'h', U'i', U'j', U'k', U'l', U'm',
			       U'n', U'o', U'p', U'q', U'r', U's', U't', U'u', U'v', U'w', U'x', U'y', U'z',
			       U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9' };

		inline constexpr ztd_char32_t __pny_digit_to_uppercase_codepoint_map[36]
		     = { U'A', U'B', U'C', U'D', U'E', U'F', U'G', U'H', U'I', U'J', U'K', U'L', U'M',
			       U'N', U'O', U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W', U'X', U'Y', U'Z',
			       U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9' };

		inline constexpr unsigned char __pny_codepoints_to_digit_map[128] = {
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, // [0-15]

			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, // [16-31]

			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, // [32-47]

			26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, // [48-63]

			0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, // [64-79]

			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // [80-95]

			0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, // [96-111]

			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF // [112-127]
		};

		// idna has a maximum limit of 255 bytes per domain name
		// so this should help us get off the floor for now...
		inline constexpr ::std::size_t __idna_max_size = 256;

		struct __pny_encode_state {
			ztd_char32_t __n;
			::std::size_t __h;
			::std::size_t __delta;
			::std::size_t __bias;
			const ztd_char32_t* __input_current;
			// TODO: this type should be a
			// ztd::small_vector<ztd_char32_t, __idna_max_size>
			std::vector<ztd_char32_t> __input;
		};

		struct __pny_decode_state {
			::std::size_t __i;
			const ztd_char_t* __delimeter_at;
			const ztd_char_t* __input_current;
			// TODO: these types should be a
			// ztd::small_vector<T, __idna_max_size>
			std::vector<ztd_char_t> __input;
			std::vector<ztd_char32_t> __output;
		};

		static_assert(sizeof(__pny_encode_state) <= sizeof(cnc_pny_encode_state_t {}.__storage),
		     "the punycode encode state (__pny_encode_state) is too big to fit in the storage "
		     "area.");


		static_assert(sizeof(__pny_decode_state) <= sizeof(cnc_pny_encode_state_t {}.__storage),
		     "the punycode encode state (__pny_encode_state) is too big to fit in the storage "
		     "area.");


		inline constexpr ::std::uintptr_t __pny_encode_state_ace_prefix  = 0u;
		inline constexpr ::std::uintptr_t __pny_encode_state_ascii       = 1u;
		inline constexpr ::std::uintptr_t __pny_encode_state_rle_unicode = 2u;
		inline constexpr ::std::uintptr_t __pny_encode_state_complete    = 3u;

		inline constexpr ::std::uintptr_t __pny_encode_consume_state_has_data      = 0u;
		inline constexpr ::std::uintptr_t __pny_encode_consume_state_no_data       = 1u;
		inline constexpr ::std::uintptr_t __pny_encode_consume_state_no_data_twice = 2u;

		inline constexpr ::std::uintptr_t __pny_decode_state_ascii        = 0u;
		inline constexpr ::std::uintptr_t __pny_decode_state_rle_unicode  = 1u;
		inline constexpr ::std::uintptr_t __pny_decode_state_write_output = 2u;
		inline constexpr ::std::uintptr_t __pny_decode_state_complete     = 3u;

		inline constexpr ::std::uintptr_t __pny_decode_consume_state_has_prefix    = 0u;
		inline constexpr ::std::uintptr_t __pny_decode_consume_state_has_data      = 1u;
		inline constexpr ::std::uintptr_t __pny_decode_consume_state_no_data       = 2u;
		inline constexpr ::std::uintptr_t __pny_decode_consume_state_no_data_twice = 3u;

		inline constexpr ::std::size_t __pny_base         = 36;
		inline constexpr ::std::size_t __pny_tmin         = 1;
		inline constexpr ::std::size_t __pny_tmax         = 26;
		inline constexpr ::std::size_t __pny_skew         = 38;
		inline constexpr ::std::size_t __pny_damp         = 700;
		inline constexpr ::std::size_t __pny_initial_bias = 72;
		inline constexpr ::std::size_t __pny_initial_n    = 0x80;

		inline constexpr bool __pny_will_overflow_add(
		     ::std::size_t __b_value, ::std::size_t __a_value) {
			return (__b_value > (SIZE_MAX - __a_value));
		}

		inline constexpr bool __pny_will_overflow_add32(
		     ::std::uint32_t __a_value, ::std::uint32_t __b_value) {
			return (__b_value > (UINT32_MAX - __a_value));
		}

		inline constexpr bool __pny_will_overflow_add_mul(
		     ::std::size_t __a_value, ::std::size_t __b_value, ::std::size_t __c_value) {
			return (__b_value > ((SIZE_MAX - __a_value) / __c_value));
		}

		inline __pny_encode_state* __get_pny_encode_state(
		     cnc_pny_encode_state_t* __p_state) noexcept {
			return reinterpret_cast<__pny_encode_state*>(
			     static_cast<void*>(&__p_state->__storage[0]));
		}

		inline __pny_decode_state* __get_pny_decode_state(
		     cnc_pny_decode_state_t* __p_state) noexcept {
			return reinterpret_cast<__pny_decode_state*>(
			     static_cast<void*>(&__p_state->__storage[0]));
		}

		inline __pny_encode_state* __init_pny_encode_state(
		     cnc_pny_encode_state_t* __p_state, bool __is_idna) {
			__p_state->input_is_complete    = 0u;
			__p_state->__action_state       = __pny_encode_consume_state_has_data;
			__p_state->__has_seen_non_basic = 0u;
			__p_state->__idna               = __is_idna;
			__pny_encode_state* __actual
			     = new (static_cast<void*>(&__p_state->__storage[0])) __pny_encode_state();
			__p_state->__is_initialized = 1u;
			__actual->__input_current   = nullptr;
			__actual->__input.reserve(__idna_max_size);
			__actual->__h     = 0u;
			__actual->__delta = 0u;
			__actual->__n     = 0u;
			__actual->__bias  = 0u;
			return __actual;
		}

		inline void __start_non_basic_pny_encode_state(
		     __pny_encode_state& __pny, ::std::uintptr_t __has_seen_non_basic) {
			__pny.__h = __pny.__input.size() - static_cast<::std::size_t>(__has_seen_non_basic);
			__pny.__delta = 0u;
			__pny.__n     = __pny_initial_n;
			__pny.__bias  = __pny_initial_bias;
		}

		inline __pny_encode_state* __destroy_pny_encode_state(
		     cnc_pny_encode_state_t* __p_state) noexcept {
			__p_state->input_is_complete    = 1u;
			__p_state->__action_state       = __pny_encode_state_complete;
			__p_state->__has_seen_non_basic = 0u;
			__pny_encode_state* __actual    = __get_pny_encode_state(__p_state);
			using __pseudo                  = __pny_encode_state;
			__actual->~__pseudo();
			__p_state->__is_initialized = 0u;
			return __actual;
		}

		inline __pny_decode_state* __init_pny_decode_state(
		     cnc_pny_decode_state_t* __p_state, bool __is_idna) {
			__p_state->input_is_complete   = 0u;
			__p_state->__action_state      = __is_idna ? __pny_decode_consume_state_has_prefix
			                                           : __pny_decode_consume_state_has_data;
			__p_state->__idna              = __is_idna;
			__p_state->__prefixed          = 0u;
			__p_state->__segment_is_digits = 0u;
			__pny_decode_state* __actual
			     = new (static_cast<void*>(&__p_state->__storage[0])) __pny_decode_state();
			__p_state->__is_initialized = 1u;
			__actual->__input_current   = nullptr;
			__actual->__delimeter_at    = nullptr;
			__actual->__input.reserve(__idna_max_size);
			__actual->__output.reserve(__idna_max_size);
			__actual->__i = 0;
			return __actual;
		}

		inline __pny_decode_state* __destroy_pny_decode_state(
		     cnc_pny_decode_state_t* __p_state) noexcept {
			__p_state->input_is_complete = 1u;
			__p_state->__action_state    = __pny_decode_state_complete;
			__pny_decode_state* __actual = __get_pny_decode_state(__p_state);
			using __pseudo               = __pny_decode_state;
			__actual->~__pseudo();
			__p_state->__is_initialized = 0u;
			return __actual;
		}

		inline ::std::size_t __pny_adapt_bias(
		     ::std::size_t __original_delta, ::std::size_t __num_points, bool __first_time) {
			::std::size_t __delta
			     = __first_time ? (__original_delta / __pny_damp) : (__original_delta / 2);
			__delta           = __delta + (__delta / __num_points);
			::std::size_t __k = 0;
			while (__delta > (((__pny_base - __pny_tmin) * __pny_tmax) / 2u)) {
				__delta = __delta / (__pny_base - __pny_tmin);
				__k     = __k + __pny_base;
			}
			return __k + (((__pny_base - __pny_tmin + 1u) * __delta) / (__delta + __pny_skew));
		}

		template <bool _IsIdna = false>
		cnc_mcerror __cnc_mcnrtoc32n_punycode_maybe_idna(size_t* __p_maybe_dst_len,
		     ztd_char32_t** __p_maybe_dst, size_t* __p_src_len, const ztd_char_t** __p_src,
		     cnc_pny_decode_state_t* __p_state) noexcept {
			if (__p_src == nullptr || *__p_src == nullptr) {
				return CNC_MCERROR_OK;
			}
			size_t& __src_len        = *__p_src_len;
			const ztd_char_t*& __src = *__p_src;
			if (!__p_state->__is_initialized) {
				if (__src_len == 0) {
					// just... don't bother, everything is fine.
					return CNC_MCERROR_OK;
				}
				::cnc::__cnc_detail::__init_pny_decode_state(__p_state, _IsIdna);
			}
			if constexpr (_IsIdna) {
				if (!__p_state->__idna) {
					// If this was not started by an IDNA call, then we just gotta
					// throw it out. There's no way to know if the input is good/bad
					// if we do not IDNA-proof it first.
					::cnc::__cnc_detail::__destroy_pny_decode_state(__p_state);
					return CNC_MCERROR_INVALID_SEQUENCE;
				}
			}
			const bool _IsCounting  = __p_maybe_dst == nullptr || __p_maybe_dst[0] == nullptr;
			const bool _IsUnbounded = __p_maybe_dst_len == nullptr;
			::cnc::__cnc_detail::__pny_decode_state& __pny
			     = *::cnc::__cnc_detail::__get_pny_decode_state(__p_state);
			if (__p_state->input_is_complete) {
				for (;;) {
					switch (__p_state->__action_state) {
					case ::cnc::__cnc_detail::__pny_decode_state_ascii: {
						// copy all ASCII before the last delimeter to the output, one at a
						// time.
						const ztd_char_t*& __input_current = __pny.__input_current;
						const ztd_char_t* __input_last
						     = __pny.__input.data() + __pny.__input.size();
						for (;;) {
							if (__input_last == __input_current) {
								// move on to the write-out action, once we reach this step
								// and there's no delimeter.
								__pny.__i = 0;
								__p_state->__action_state
								     = ::cnc::__cnc_detail::__pny_decode_state_write_output;
								break;
							}
							if (__pny.__delimeter_at == __input_current) {
								// if the last segment contains MORE than just '-',
								// or if we have a proper set of digits,
								if constexpr (!_IsIdna) {
									// if we have a '-' at the end of the input,
									// and this is NOT and IDNA-compliant parsing,
									// then we simply have to assume it was added there,
									// and thusly deserves to be removed.
									if (__input_current == (__input_last - 1)) {
										__input_current += 1;
										continue;
									}
									else if (__p_state->__segment_is_digits) {
										__p_state->__action_state = ::cnc::__cnc_detail::
										     __pny_decode_state_rle_unicode;
										__input_current += 1;
										break;
									}
								}
								else {
									// If we had a prefix, and this is the delimiting '-'.
									// then it's time to transition to unicode decoding
									if (__p_state->__prefixed) {
										// we need to move to the non-ASCII encoding bits.
										__p_state->__action_state = ::cnc::__cnc_detail::
										     __pny_decode_state_rle_unicode;
										__input_current += 1;
										break;
									}
									else {
										// otherwise, there is no prefix, so this is just
										// a normal character.
									}
								}
							}
							const ztd_char_t __final_code_unit = __input_current[0];
							// here, we can do a quick-check to just force the whole thing to
							// be copied like ASCII, wholesale.
							__pny.__output.push_back(
							     static_cast<ztd_char32_t>(__final_code_unit));
							__input_current += 1;
						}
						continue;
					} break;
					case ::cnc::__cnc_detail::__pny_decode_state_rle_unicode: {
						constexpr auto& __tmin             = ::cnc::__cnc_detail::__pny_tmin;
						constexpr auto& __tmax             = ::cnc::__cnc_detail::__pny_tmax;
						constexpr auto& __base             = ::cnc::__cnc_detail::__pny_base;
						const ztd_char_t*& __input_current = __pny.__input_current;
						const ztd_char_t* __input_last
						     = __pny.__input.data() + __pny.__input.size();
						if (__input_last == __input_current) {
							// okay, we've consumed eerything. Start writing output.
							__pny.__i = 0;
							__p_state->__action_state
							     = ::cnc::__cnc_detail::__pny_decode_state_write_output;
							continue;
						}
						ztd_char32_t __n     = ::cnc::__cnc_detail::__pny_initial_n;
						::std::size_t __i    = 0;
						::std::size_t __bias = ::cnc::__cnc_detail::__pny_initial_bias;
						for (; __input_current != __input_last;) {
							const ::std::size_t __old_i = __i;
							::std::size_t __w           = 1;
							for (::std::size_t __k = __base;; __k += __base) {
								if (__input_current == __input_last) {
									// this was never supposed to happen!!
									__p_state->__action_state = ::cnc::__cnc_detail::
									     __pny_decode_state_write_output;
									__pny.__output.assign(
									     __pny.__input.begin(), __pny.__input.end());
									return CNC_MCERROR_OK;
								}
								const ztd_char_t __code_unit = __input_current[0];
								__input_current += 1;
								unsigned char __digit = __pny_codepoints_to_digit_map
								     [static_cast<unsigned char>(__code_unit)];
								if (__digit == 0xFF) {
									// this is a failure and we must bail.
									__p_state->__action_state = ::cnc::__cnc_detail::
									     __pny_decode_state_write_output;
									__pny.__output.assign(
									     __pny.__input.begin(), __pny.__input.end());
									return CNC_MCERROR_OK;
								}
								if (__pny_will_overflow_add_mul(__i, __digit, __w)) {
									// this is a failure and we must bail.
									__p_state->__action_state = ::cnc::__cnc_detail::
									     __pny_decode_state_write_output;
									__pny.__output.assign(
									     __pny.__input.begin(), __pny.__input.end());
									return CNC_MCERROR_OK;
								}
								__i                     = __i + (__digit * __w);
								const ::std::size_t __t = __k <= __bias
								     ? __tmin
								     : ((__k >= (__bias + __tmax)) ? __tmax : __k - __bias);
								if (__digit < __t) {
									break;
								}
								if (__pny_will_overflow_add_mul(0, __w, (__base - __t))) {
									// this is a failure and we must bail.
									__p_state->__action_state = ::cnc::__cnc_detail::
									     __pny_decode_state_write_output;
									__pny.__output.assign(
									     __pny.__input.begin(), __pny.__input.end());
									return CNC_MCERROR_OK;
								}
								__w = __w * (__base - __t);
							}
							const ::std::size_t __output_size       = __pny.__output.size();
							const ::std::size_t __output_size_plus1 = (__output_size + 1);
							__bias                                  = __pny_adapt_bias(
							                                      __i - __old_i, __output_size + 1, __old_i == 0);
							const ::std::size_t __i_div_output_size_plus1
							     = __i / __output_size_plus1;
							if (__pny_will_overflow_add(static_cast<::std::size_t>(__n),
							         __i_div_output_size_plus1)) {
								// this is a failure and we must bail.
								__p_state->__action_state
								     = ::cnc::__cnc_detail::__pny_decode_state_write_output;
								__pny.__output.assign(
								     __pny.__input.begin(), __pny.__input.end());
								return CNC_MCERROR_OK;
							}
							__n = static_cast<ztd_char32_t>(__n + __i_div_output_size_plus1);
							__i = __i % __output_size_plus1;
							if (__n < __ztd_idk_detail_last_ascii_value
							     || __n > __ztd_idk_detail_last_unicode_code_point) {
								// this was never supposed to happen:
								// bail and simply re-do this as an ASCII string instead!
								__p_state->__action_state
								     = ::cnc::__cnc_detail::__pny_decode_state_write_output;
								__pny.__output.assign(
								     __pny.__input.begin(), __pny.__input.end());
								return CNC_MCERROR_OK;
							}
							__pny.__output.insert(__pny.__output.begin() + __i, __n);
							__i += 1;
						}
						__pny.__i = 0;
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_decode_state_write_output;
						continue;
					} break;
					case ::cnc::__cnc_detail::__pny_decode_state_write_output: {
						::std::size_t& __i                = __pny.__i;
						const ::std::size_t __output_size = __pny.__output.size();
						if (__i >= __output_size) {
							// we are DONE!
							::cnc::__cnc_detail::__destroy_pny_decode_state(__p_state);
							__p_state->__action_state
							     = ::cnc::__cnc_detail::__pny_decode_state_complete;
							return CNC_MCERROR_OK;
						}
						const ztd_char32_t __code_point = __pny.__output[__pny.__i];
						if (!_IsUnbounded) {
							if (__p_maybe_dst_len[0] < 1) {
								return CNC_MCERROR_INSUFFICIENT_OUTPUT;
							}
							__p_maybe_dst_len[0] -= 1;
						}
						if (!_IsCounting) {
							__p_maybe_dst[0][0] = __code_point;
							__p_maybe_dst[0] += 1;
						}
						__i += 1;
						return CNC_MCERROR_OK;
					} break;
					default:
						break;
					}
					break;
				}
				::cnc::__cnc_detail::__destroy_pny_decode_state(__p_state);
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			if constexpr (_IsIdna) {
				if (__p_state->__action_state
				     == ::cnc::__cnc_detail::__pny_decode_consume_state_has_prefix) {
					// we are checking if it has the appropriate xn-- prefix.
					// if it does not, we must bail.
					for (; __pny.__i < 4; (void)++__pny.__i, (void)++__src, --__src_len) {
						// if we are out of source, just come back around.
						if (__src_len == 0) {
							return CNC_MCERROR_OK;
						}
						__pny.__input.push_back(__src[0]);
						switch (__pny.__i) {
						case 0:
							// must read "xn--" still
							if (__src[0] == 'x') {
								// we are good, keep going
								continue;
							}
							break;
						case 1:
							// must read "n--" still
							if (__src[0] == 'n') {
								// we are good, keep going
								continue;
							}
							break;
						case 2:
							// must read "--" still
							if (__src[0] == '-') {
								// we are good, keep going
								continue;
							}
							break;
						case 3:
							// must read "-" still
							if (__src[0] == '-') {
								// okay, we have consumed the entire prefix. This means we
								// are dealing with Unicode.
								__p_state->__prefixed     = 1u;
								__p_state->__action_state = ::cnc::__cnc_detail::
								     __pny_decode_consume_state_has_data;
								__pny.__input.clear();
								continue;
							}
							break;
						default:
							ZTD_UNREACHABLE();
							break;
						}
						// we have no prefix, simply assume the entire input is ASCII.
						__src -= __pny.__i;
						__src_len += __pny.__i;
						__p_state->__prefixed = 0u;
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_decode_consume_state_has_data;
						break;
					}
					__pny.__i = 0u;
					return CNC_MCERROR_OK;
				}
			}
			const bool __is_directly_outputting = _IsIdna ? !__p_state->__prefixed : false;
			if (__src_len == 0) {
				// we need to write out data from the current and last now...
				// gather up data until we are complete...
				const bool __in_prefix_state = (__p_state->__action_state
				     == ::cnc::__cnc_detail::__pny_decode_consume_state_has_prefix);
				__p_state->__action_state += 1 + static_cast<uintptr_t>(__in_prefix_state);
				if (__p_state->__action_state
				     == ::cnc::__cnc_detail::__pny_encode_consume_state_no_data_twice) {
					__p_state->input_is_complete = 1u;
					if (__is_directly_outputting) {
						// we are done; bail out
						::cnc::__cnc_detail::__destroy_pny_decode_state(__p_state);
						__p_state->__action_state
						     = ::cnc::__cnc_detail::__pny_decode_state_complete;
						return CNC_MCERROR_OK;
					}
					// we have had 0 input twice in a row; switch the completeness switch on to
					// FORCE additional processing.
					__p_state->__action_state = ::cnc::__cnc_detail::__pny_decode_state_ascii;
					__pny.__input_current     = __pny.__input.data();
				}
				return CNC_MCERROR_OK;
			}
			const ztd_char_t __initial_code_unit = __src[0];
			if (__initial_code_unit
			     > static_cast<ztd_char_t>(__ztd_idk_detail_last_ascii_value)) {
				// We have an invalid input, so this is just not worth having anymore.
				::cnc::__cnc_detail::__destroy_pny_decode_state(__p_state);
				return CNC_MCERROR_INVALID_SEQUENCE;
			}
			if (__is_directly_outputting) {
				// direct-copy
				if (!_IsUnbounded) {
					if (__p_maybe_dst_len[0] < 1) {
						return CNC_MCERROR_INSUFFICIENT_OUTPUT;
					}
					__p_maybe_dst_len[0] -= 1;
				}
				if (!_IsCounting) {
					__p_maybe_dst[0][0] = static_cast<ztd_char32_t>(__initial_code_unit);
					__p_maybe_dst[0] += 1;
				}
			}
			else {
				// delayed copy
				__pny.__input.push_back(__initial_code_unit);
				if (__initial_code_unit == '-') {
					// this must happen after to prevent iterator invalidation from destroying
					// the pointer we have...
					__pny.__delimeter_at = __pny.__input.data() + __pny.__input.size() - 1;
					__p_state->__segment_is_digits = 1u;
				}
				else {
					const bool __is_valid_rle_digit
					     = (__pny_codepoints_to_digit_map[static_cast<unsigned char>(
					             __initial_code_unit)]
					          != (unsigned char)0xFF);
					__p_state->__segment_is_digits &= (__is_valid_rle_digit ? 1u : 0u);
				}
			}
			__src_len -= 1;
			__src += 1;
			return CNC_MCERROR_OK;
		}

	} // namespace __cnc_detail
	ZTD_CUNEICODE_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace cnc

#endif
