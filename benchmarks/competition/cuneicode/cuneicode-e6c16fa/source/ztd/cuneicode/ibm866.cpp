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

#include <ztd/cuneicode/ibm866.h>
#include <ztd/cuneicode/max_output.h>
#include <ztd/cuneicode/detail/transcode.hpp>

#include <memory>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32ntomcn_ibm866(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_c32nrtomcn_ibm866(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32nrtomcn_ibm866(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__p_maybe_dst_len;
	(void)__p_maybe_dst;
	(void)__p_src_len;
	(void)__p_src;
	(void)__p_state;
	return CNC_MCERROR_INVALID_SEQUENCE;
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcntoc32n_ibm866(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_mcnrtoc32n_ibm866(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcnrtoc32n_ibm866(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	(void)__p_maybe_dst_len;
	(void)__p_maybe_dst;
	(void)__p_src_len;
	(void)__p_src;
	(void)__p_state;
	return CNC_MCERROR_INVALID_SEQUENCE;
}



ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32sntomcsn_ibm866(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_c32snrtomcsn_ibm866(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c32snrtomcsn_ibm866(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char32_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_c32nrtomcn_ibm866), &cnc_c32nrtomcn_ibm866,
	     ztd_char32_t, ztd_char_t);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc32sn_ibm866(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return cnc_mcsnrtoc32sn_ibm866(
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc32sn_ibm866(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	_ZTDC_CUNEICODE_TRANSCODE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, CNC_MC_MAX, decltype(&cnc_mcnrtoc32n_ibm866), &cnc_mcnrtoc32n_ibm866,
	     ztd_char_t, ztd_char32_t);
}
