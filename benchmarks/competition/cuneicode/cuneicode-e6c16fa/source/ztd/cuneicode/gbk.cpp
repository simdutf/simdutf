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

#include <ztd/cuneicode/gbk.h>
#include <ztd/cuneicode/max_output.h>
#include <ztd/cuneicode/detail/transcode.hpp>
#include <ztd/cuneicode/detail/index.hpp>
#include <ztd/cuneicode/detail/gb18030_index.hpp>

#include <memory>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomcn_gbk(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_c32nrtomcn_gbk(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomcn_gbk(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t*) ZTD_NOEXCEPT_IF_CXX_I_ {
	using code_point = ztd_char32_t;
	using code_unit  = ztd_char_t;

	_ZTDC_CUNEICODE_BOILERPLATE_NULLPTR_AND_EMPTY_CHECKS(ztd_char32_t);

	const code_point __code = __src[0];
	if (__code <= 0x7F) {
		if (!_IsUnbounded) {
			if (__p_maybe_dst_len[0] == 0) {
				// output is empty :(
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
			__p_maybe_dst_len[0] -= 1;
		}
		if (!_IsCounting) {
			__p_maybe_dst[0][0] = static_cast<code_unit>(__code);
			__p_maybe_dst[0] += 1;
		}
		__src += 1;
		__src_len -= 1;
		return CNC_MCERROR_OK;
	}
	else if (__code == U'\uE5E5') {
		return CNC_MCERROR_INVALID_SEQUENCE;
	}
	else if (__code == U'\u20AC') {
		if (!_IsUnbounded) {
			if (__p_maybe_dst_len[0] == 0) {
				// output is empty :(
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
			__p_maybe_dst_len[0] -= 1;
		}
		if (!_IsCounting) {
			__p_maybe_dst[0][0] = static_cast<code_unit>(0x80);
			__p_maybe_dst[0] += 1;
		}
		__src += 1;
		__src_len -= 1;
		return CNC_MCERROR_OK;
	}

	if (!_IsUnbounded) {
		if (__p_maybe_dst_len[0] < 2) {
			// output is empty :(
			return CNC_MCERROR_INSUFFICIENT_OUTPUT;
		}
	}

	::std::optional<::std::size_t> __maybe_index
	     = ::cnc::__cnc_detail::__general_code_point_to_index(
	          ::cnc::__cnc_detail::__gb18030_index_code_point_map, __code);
	if (__maybe_index) {
		const ::std::size_t __index  = *__maybe_index;
		const ::std::size_t __lead   = (__index / 190) + 0x81;
		const ::std::size_t __trail  = __index % 190;
		const ::std::size_t __offset = __trail < 0x3F ? 0x40 : 0x41;

		if (!_IsUnbounded) {
			__p_maybe_dst_len[0] -= 2;
		}
		if (!_IsCounting) {
			__p_maybe_dst[0][0] = static_cast<code_unit>(__lead);
			__p_maybe_dst[0][1] = static_cast<code_unit>(__trail + __offset);
			__p_maybe_dst[0] += 2;
		}
		__src += 1;
		__src_len -= 1;
		return CNC_MCERROR_OK;
	}

	return CNC_MCERROR_INVALID_SEQUENCE;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc32n_gbk(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_mcnrtoc32n_gbk(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc32n_gbk(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char_t** __p_src, cnc_mcstate_t*) ZTD_NOEXCEPT_IF_CXX_I_ {

	using code_point = ztd_char32_t;
	using code_unit  = ztd_char_t;

	_ZTDC_CUNEICODE_BOILERPLATE_NULLPTR_AND_EMPTY_CHECKS(ztd_char_t);

	const unsigned char __first_byte = static_cast<unsigned char>(__src[0]);
	if (__first_byte <= 0x7F) {
		// Top-Level case 0: it's an ASCII byte
		code_point __code = static_cast<code_point>(__first_byte);
		if (!_IsUnbounded) {
			if (__p_maybe_dst_len[0] == 0) {
				// output is empty :(
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
			__p_maybe_dst_len[0] -= 1;
		}
		if (!_IsCounting) {
			__p_maybe_dst[0][0] = static_cast<code_unit>(__code);
			__p_maybe_dst[0] += 1;
		}

		__src += 1;
		__src_len -= 1;
		return CNC_MCERROR_OK;
	}
	else if (__first_byte == 0x80) {
		// specific output for 0x80
		code_point __code = static_cast<code_point>(U'\u20AC');
		if (!_IsUnbounded) {
			if (__p_maybe_dst_len[0] == 0) {
				// output is empty :(
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
			__p_maybe_dst_len[0] -= 1;
		}
		if (!_IsCounting) {
			__p_maybe_dst[0][0] = static_cast<code_unit>(__code);
			__p_maybe_dst[0] += 1;
		}

		__src += 1;
		__src_len -= 1;
		return CNC_MCERROR_OK;
	}
	else if (__first_byte >= 0xFF) {
		return CNC_MCERROR_INVALID_SEQUENCE;
	}

	// Case: must have 2 bytes
	if (__src_len < 2) {
		return CNC_MCERROR_INCOMPLETE_INPUT;
	}
	unsigned char __second_byte = static_cast<unsigned char>(__src[1]);
	if (__second_byte >= 0x30 && __second_byte <= 0x39) {
		return CNC_MCERROR_INVALID_SEQUENCE;
	}
	const ::std::size_t __lead   = __first_byte;
	const ::std::size_t __offset = __second_byte < 0x7F ? 0x40 : 0x41;
	if ((__second_byte >= 0x40 && __second_byte <= 0x7E)
	     || (__second_byte >= 0x80 && __second_byte <= 0xFE)) {
		const ::std::size_t __index = ((__lead - 0x81) * 190) + (__second_byte - __offset);
		if (!_IsUnbounded) {
			if (__p_maybe_dst_len[0] < 1) {
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
		}
		::std::optional<char32_t> __maybe_code
		     = ::cnc::__cnc_detail::__general_index_to_code_point(
		          ::cnc::__cnc_detail::__gb18030_index_code_point_map, __index);
		if (__maybe_code) {
			const char32_t __code = *__maybe_code;
			if (!_IsUnbounded) {
				__p_maybe_dst_len[0] -= 1;
			}
			if (!_IsCounting) {
				__p_maybe_dst[0][0] = static_cast<code_point>(__code);
				__p_maybe_dst[0] += 1;
			}
			__src += 2;
			__src_len -= 2;
			return CNC_MCERROR_OK;
		}
	}
	return CNC_MCERROR_INVALID_SEQUENCE;
}



ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32sntomcsn_gbk(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_c32snrtomcsn_gbk(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32snrtomcsn_gbk(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_c32nrtomcn_gbk), &cnc_c32nrtomcn_gbk, ztd_char32_t,
	     ztd_char_t);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc32sn_gbk(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_mcsnrtoc32sn_gbk(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc32sn_gbk(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_mcnrtoc32n_gbk), &cnc_mcnrtoc32n_gbk, ztd_char_t,
	     ztd_char32_t);
}
