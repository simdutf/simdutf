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
#include <ztd/idk/charN_t.h>
#include <ztd/cuneicode/detail/transcode.hpp>

#include <memory>

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtomcsn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_maybe_dst == nullptr || *__p_maybe_dst == nullptr) {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<true, true, CNC_MC_MAX,
			     decltype(&::cnc_mcnrtomcn), &::cnc_mcnrtomcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<true, false, CNC_MC_MAX,
			     decltype(&::cnc_mcnrtomcn), &::cnc_mcnrtomcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
	else {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<false, true, CNC_MC_MAX,
			     decltype(&::cnc_mcnrtomcn), &::cnc_mcnrtomcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<false, false, CNC_MC_MAX,
			     decltype(&::cnc_mcnrtomcn), &::cnc_mcnrtomcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntomcsn(
     size_t* __p_maybe_dst_len, char** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_mcsnrtomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtomwcsn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len, const char** __p_src,
     cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_maybe_dst == nullptr || *__p_maybe_dst == nullptr) {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<true, true, CNC_MWC_MAX,
			     decltype(&::cnc_mcnrtomwcn), &::cnc_mcnrtomwcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<true, false, CNC_MWC_MAX,
			     decltype(&::cnc_mcnrtomwcn), &::cnc_mcnrtomwcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
	else {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<false, true, CNC_MWC_MAX,
			     decltype(&::cnc_mcnrtomwcn), &::cnc_mcnrtomwcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<false, false, CNC_MWC_MAX,
			     decltype(&::cnc_mcnrtomwcn), &::cnc_mcnrtomwcn>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntomwcsn(
     size_t* __p_maybe_dst_len, wchar_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_mcsnrtomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc8sn(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_maybe_dst == nullptr || *__p_maybe_dst == nullptr) {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<true, true, CNC_C8_MAX,
			     decltype(&::cnc_mcnrtoc8n), &::cnc_mcnrtoc8n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<true, false, CNC_C8_MAX,
			     decltype(&::cnc_mcnrtoc8n), &::cnc_mcnrtoc8n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
	else {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<false, true, CNC_C8_MAX,
			     decltype(&::cnc_mcnrtoc8n), &::cnc_mcnrtoc8n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<false, false, CNC_C8_MAX,
			     decltype(&::cnc_mcnrtoc8n), &::cnc_mcnrtoc8n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc8sn(
     size_t* __p_maybe_dst_len, ztd_char8_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_mcsnrtoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc16sn(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_maybe_dst == nullptr || *__p_maybe_dst == nullptr) {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<true, true, CNC_C16_MAX,
			     decltype(&::cnc_mcnrtoc16n), &::cnc_mcnrtoc16n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<true, false, CNC_C16_MAX,
			     decltype(&::cnc_mcnrtoc16n), &::cnc_mcnrtoc16n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
	else {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<false, true, CNC_C16_MAX,
			     decltype(&::cnc_mcnrtoc16n), &::cnc_mcnrtoc16n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<false, false, CNC_C16_MAX,
			     decltype(&::cnc_mcnrtoc16n), &::cnc_mcnrtoc16n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc16sn(
     size_t* __p_maybe_dst_len, ztd_char16_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_mcsnrtoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsnrtoc32sn(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {
	if (__p_maybe_dst == nullptr || *__p_maybe_dst == nullptr) {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<true, true, CNC_C32_MAX,
			     decltype(&::cnc_mcnrtoc32n), &::cnc_mcnrtoc32n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<true, false, CNC_C32_MAX,
			     decltype(&::cnc_mcnrtoc32n), &::cnc_mcnrtoc32n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
	else {
		if (__p_maybe_dst_len == nullptr) {
			return ::cnc::__cnc_detail::__transcode<false, true, CNC_C32_MAX,
			     decltype(&::cnc_mcnrtoc32n), &::cnc_mcnrtoc32n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			return ::cnc::__cnc_detail::__transcode<false, false, CNC_C32_MAX,
			     decltype(&::cnc_mcnrtoc32n), &::cnc_mcnrtoc32n>(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
	}
}

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror cnc_mcsntoc32sn(
     size_t* __p_maybe_dst_len, ztd_char32_t** __p_maybe_dst, size_t* __p_src_len,
     const char** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {
	cnc_mcstate_t __state    = {};
	cnc_mcstate_t* __p_state = ::std::addressof(__state);
	return ::cnc_mcsnrtoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
}
