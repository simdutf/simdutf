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

#include <ztd/cuneicode/mcchar.h>

#include <ztd/cuneicode/detail/basic_conv.hpp>
#include <ztd/cuneicode/detail/err.hpp>
#include <ztd/cuneicode/detail/core_mcharn.hpp>
#include <ztd/cuneicode/detail/transcode.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <cstdlib>
#include <memory>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (ztdc_is_execution_encoding_utf8()) {
		ztd_char8_t** __p_maybe_c8_dst = reinterpret_cast<ztd_char8_t**>(__p_maybe_dst);
		return ::cnc_c8nrtoc8n(
		     __p_maybe_dst_len, __p_maybe_c8_dst, __p_src_len, __p_src, __p_state);
	}
	cnc_mcstate_t __substitute_state {};
	if (__p_state == nullptr)
		__p_state = &__substitute_state;
	_ZTDC_CUNEICODE_TRANSCODE_ONE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, decltype(&cnc_c8nrtoc32n), &cnc_c8nrtoc32n, decltype(&cnc_c32nrtomcn),
	     &cnc_c32nrtomcn);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntomcn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_c8nrtomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __substitute_state {};
	if (__p_state == nullptr)
		__p_state = &__substitute_state;
	_ZTDC_CUNEICODE_TRANSCODE_ONE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, decltype(&cnc_c8nrtoc32n), &cnc_c8nrtoc32n, decltype(&cnc_c32nrtomwcn),
	     &cnc_c32nrtomwcn);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntomwcn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_c8nrtomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __substitute_state {};
	if (__p_state == nullptr)
		__p_state = &__substitute_state;
	_ZTDC_CUNEICODE_SINGLE_N_DEST_TEMPLATE_BODY(::cnc::__cnc_detail::__c8nrtoc8n,
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc8n(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_c8nrtoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __substitute_state {};
	if (__p_state == nullptr)
		__p_state = &__substitute_state;
	_ZTDC_CUNEICODE_TRANSCODE_ONE_BODY(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src,
	     __p_state, decltype(&cnc_c8nrtoc32n), &cnc_c8nrtoc32n, decltype(&cnc_c32nrtoc16n),
	     &cnc_c32nrtoc16n);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc16n(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_c8nrtoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8nrtoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __substitute_state {};
	if (__p_state == nullptr)
		__p_state = &__substitute_state;
	_ZTDC_CUNEICODE_SINGLE_N_DEST_TEMPLATE_BODY(::cnc::__cnc_detail::__c8nrtoc32n,
	     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_c8ntoc32n(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const ztd_char8_t** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_c8nrtoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}
