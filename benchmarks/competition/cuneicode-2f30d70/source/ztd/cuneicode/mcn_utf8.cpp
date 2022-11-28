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

#include <ztd/cuneicode.h>

#define CNC_MAKE_SLIM_SHIM_ROOT_I_(_SHIM_NAME, _SHIM_R_NAME, _INTERNAL_NAME, _INTERNAL_R_NAME,     \
     _SHIM_FROM, _SHIM_TO, _INTERNAL_FROM, _INTERNAL_TO)                                           \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror _SHIM_NAME(                \
	     size_t* __p_maybe_dst_len, _SHIM_TO** __p_maybe_dst, size_t* __p_src_len,                \
	     const _SHIM_FROM** __p_src) ZTD_NOEXCEPT_IF_CXX_I_ {                                     \
		return (_INTERNAL_NAME)(__p_maybe_dst_len, (_INTERNAL_TO**)__p_maybe_dst, __p_src_len,   \
		     (const _INTERNAL_FROM**)__p_src);                                                   \
	}                                                                                             \
	ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_CUNEICODE_API_LINKAGE_I_ cnc_mcerror _SHIM_R_NAME(              \
	     size_t* __p_maybe_dst_len, _SHIM_TO** __p_maybe_dst, size_t* __p_src_len,                \
	     const _SHIM_FROM** __p_src, cnc_mcstate_t* __p_state) ZTD_NOEXCEPT_IF_CXX_I_ {           \
		return (_INTERNAL_R_NAME)(__p_maybe_dst_len, (_INTERNAL_TO**)__p_maybe_dst, __p_src_len, \
		     (const _INTERNAL_FROM**)__p_src, __p_state);                                        \
	}                                                                                             \
	static_assert(true, "")

#define mc_TYPE_I_ char
#define mwc_TYPE_I_ wchar_t
#define c8_TYPE_I_ ztd_char8_t
#define c16_TYPE_I_ ztd_char16_t
#define c32_TYPE_I_ ztd_char32_t

#define TAG_TO_TYPE_EXPANDED_I_(_TAG) _TAG##_TYPE_I_
#define TAG_TO_TYPE_I_(_TAG) TAG_TO_TYPE_EXPANDED_I_(_TAG)

#define CNC_MAKE_SLIM_SHIM_I_(                                                               \
     _SHIM_PREFIX, _SHIM_SUFFIX, _INTERNAL_PREFIX, _INTERNAL_SUFFIX, _TAIL)                  \
	CNC_MAKE_SLIM_SHIM_ROOT_I_(cnc_##_SHIM_PREFIX##nto##_SHIM_SUFFIX##n##_TAIL,             \
	     cnc_##_SHIM_PREFIX##nrto##_SHIM_SUFFIX##n##_TAIL,                                  \
	     cnc_##_INTERNAL_PREFIX##nto##_INTERNAL_SUFFIX##n,                                  \
	     cnc_##_INTERNAL_PREFIX##nrto##_INTERNAL_SUFFIX##n, TAG_TO_TYPE_I_(_SHIM_PREFIX),   \
	     TAG_TO_TYPE_I_(_SHIM_SUFFIX), TAG_TO_TYPE_I_(_INTERNAL_PREFIX),                    \
	     TAG_TO_TYPE_I_(_INTERNAL_SUFFIX));                                                 \
	CNC_MAKE_SLIM_SHIM_ROOT_I_(cnc_##_SHIM_PREFIX##snto##_SHIM_SUFFIX##sn##_TAIL,           \
	     cnc_##_SHIM_PREFIX##snrto##_SHIM_SUFFIX##sn##_TAIL,                                \
	     cnc_##_INTERNAL_PREFIX##snto##_INTERNAL_SUFFIX##sn,                                \
	     cnc_##_INTERNAL_PREFIX##snrto##_INTERNAL_SUFFIX##sn, TAG_TO_TYPE_I_(_SHIM_PREFIX), \
	     TAG_TO_TYPE_I_(_SHIM_SUFFIX), TAG_TO_TYPE_I_(_INTERNAL_PREFIX),                    \
	     TAG_TO_TYPE_I_(_INTERNAL_SUFFIX))

CNC_MAKE_SLIM_SHIM_I_(mc, mc, c8, mc, _utf8_exec);
CNC_MAKE_SLIM_SHIM_I_(mc, c8, mc, c8, _exec_utf8);
CNC_MAKE_SLIM_SHIM_I_(mc, mc, c8, c8, _utf8_utf8);

CNC_MAKE_SLIM_SHIM_I_(mc, c32, c8, c32, _utf8);
CNC_MAKE_SLIM_SHIM_I_(c32, mc, c32, c8, _utf8);

CNC_MAKE_SLIM_SHIM_I_(mc, c16, c8, c16, _utf8);
CNC_MAKE_SLIM_SHIM_I_(c16, mc, c16, c8, _utf8);

CNC_MAKE_SLIM_SHIM_I_(mc, mwc, c8, mwc, _utf8_wide_exec);
CNC_MAKE_SLIM_SHIM_I_(mwc, mc, mwc, c8, _wide_exec_utf8);
