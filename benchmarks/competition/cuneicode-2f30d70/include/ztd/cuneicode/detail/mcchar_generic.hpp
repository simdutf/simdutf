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

#ifndef ZTD_CUNEICODE_MCCHAR_GENERIC_HPP
#define ZTD_CUNEICODE_MCCHAR_GENERIC_HPP

#pragma once

#include <ztd/cuneicode/version.h>

#include <ztd/cuneicode/mcerror.h>
#include <ztd/cuneicode/detail/mccharn.h>
#include <ztd/cuneicode/detail/mccharsn.h>

#if ZTD_IS_ON(ZTD_CXX)

#include <ztd/idk/type_traits.hpp>

template <typename _Destination, typename _Source>
cnc_mcerror __cnc_cxntocxn(size_t* __p_maybe_dst_len, _Destination** __p_maybe_dst,
     size_t* __p_src_len, const _Source** __p_src) {
	using _UDestination = ::ztd::remove_cvref_t<_Destination>;
	using _USource      = ::ztd::remove_cvref_t<_Source>;
	if constexpr (::std::is_same_v<_USource, char>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mcntomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mcntomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mcntoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mcntoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mcntoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, wchar_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mwcntomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mwcntomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mwcntoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mwcntoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mwcntoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char8_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c8ntomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c8ntomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c8ntoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c8ntoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c8ntoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char16_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c16ntomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c16ntomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c16ntoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c16ntoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c16ntoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char32_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c32ntomcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c32ntomwcn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c32ntoc8n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c32ntoc16n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c32ntoc32n(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else {
		static_assert(::ztd::always_false_v<_USource>, "[ztd.cuneicode] incorrect type");
	}
}

template <typename _Destination, typename _Source>
cnc_mcerror __cnc_cxnrtocxn(size_t* __p_maybe_dst_len, _Destination** __p_maybe_dst,
     size_t* __p_src_len, const _Source** __p_src, cnc_mcstate_t* __p_state) {
	using _UDestination = ::ztd::remove_cvref_t<_Destination>;
	using _USource      = ::ztd::remove_cvref_t<_Source>;
	if constexpr (::std::is_same_v<_USource, char>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mcnrtomcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mcnrtomwcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mcnrtoc8n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mcnrtoc16n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mcnrtoc32n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, wchar_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mwcnrtomcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mwcnrtomwcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mwcnrtoc8n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mwcnrtoc16n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mwcnrtoc32n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char8_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c8nrtomcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c8nrtomwcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c8nrtoc8n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c8nrtoc16n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c8nrtoc32n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char16_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c16nrtomcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c16nrtomwcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c16nrtoc8n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c16nrtoc16n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c16nrtoc32n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char32_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c32nrtomcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c32nrtomwcn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c32nrtoc8n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c32nrtoc16n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c32nrtoc32n(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else {
		static_assert(::ztd::always_false_v<_USource>, "[ztd.cuneicode] incorrect type");
	}
}

template <typename _Destination, typename _Source>
cnc_mcerror __cnc_cxsntocxsn(size_t* __p_maybe_dst_len, _Destination** __p_maybe_dst,
     size_t* __p_src_len, const _Source** __p_src) {
	using _UDestination = ::ztd::remove_cvref_t<_Destination>;
	using _USource      = ::ztd::remove_cvref_t<_Source>;
	if constexpr (::std::is_same_v<_USource, char>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mcsntomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mcsntomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mcsntoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mcsntoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mcsntoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, wchar_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mwcsntomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mwcsntomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mwcsntoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mwcsntoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mwcsntoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char8_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c8sntomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c8sntomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c8sntoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c8sntoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c8sntoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char16_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c16sntomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c16sntomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c16sntoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c16sntoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c16sntoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char32_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c32sntomcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c32sntomwcsn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c32sntoc8sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c32sntoc16sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c32sntoc32sn(__p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else {
		static_assert(::ztd::always_false_v<_USource>, "[ztd.cuneicode] incorrect type");
	}
}

template <typename _Destination, typename _Source>
cnc_mcerror __cnc_cxsnrtocxsn(size_t* __p_maybe_dst_len, _Destination** __p_maybe_dst,
     size_t* __p_src_len, const _Source** __p_src, cnc_mcstate_t* __p_state) {
	using _UDestination = ::ztd::remove_cvref_t<_Destination>;
	using _USource      = ::ztd::remove_cvref_t<_Source>;
	if constexpr (::std::is_same_v<_USource, char>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mcsnrtomcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mcsnrtomwcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mcsnrtoc8sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mcsnrtoc16sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mcsnrtoc32sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, wchar_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_mwcsnrtomcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_mwcsnrtomwcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_mwcsnrtoc8sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_mwcsnrtoc16sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_mwcsnrtoc32sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char8_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c8snrtomcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c8snrtomwcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c8snrtoc8sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c8snrtoc16sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c8snrtoc32sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char16_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c16snrtomcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c16snrtomwcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c16snrtoc8sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c16snrtoc16sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c16snrtoc32sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else if constexpr (::std::is_same_v<_USource, ztd_char32_t>) {
		if constexpr (::std::is_same_v<_UDestination, char>) {
			return cnc_c32snrtomcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, wchar_t>) {
			return cnc_c32snrtomwcsn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char8_t>) {
			return cnc_c32snrtoc8sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char16_t>) {
			return cnc_c32snrtoc16sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else if constexpr (::std::is_same_v<_UDestination, ztd_char32_t>) {
			return cnc_c32snrtoc32sn(
			     __p_maybe_dst_len, __p_maybe_dst, __p_src_len, __p_src, __p_state);
		}
		else {
			static_assert(
			     ::ztd::always_false_v<_UDestination>, "[ztd.cuneicode] incorrect type");
		}
	}
	else {
		static_assert(::ztd::always_false_v<_USource>, "[ztd.cuneicode] incorrect type");
	}
}

#endif

#endif // ZTD_CUNEICODE_MCCHAR_GENERIC_HPP
