# =============================================================================
#
# ztd.cmake
# Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.cmake licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Shepherd's Oasis, LLC.
# For licensing terms and conditions see your agreement. For
# further information contact opensource@soasis.org.
#
# Apache License Version 2 Usage
# Alternatively, this file may be used under the terms of Apache License
# Version 2.0 (the "License") for non-commercial use; you may not use this
# file except in compliance with the License. You may obtain a copy of the
# License at
#
#		http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============================================================================>

include_guard(GLOBAL)

include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

#[[
Given a flag name and the actual flag, like
check_cxx_compiler_flag(strict-conformance MSVC /permissive- GCC -pedantic)
we check if the given flag works C++ compiler. If it is, then
--strict-conformance will be the provided flag. MSVC and GCC are the 2 different
"style" of flags to be tested for.
]]
function (check_compiler_flag flag)
	cmake_parse_arguments(PARSE_ARGV 1 compiler_flag "" "" "GCC;MSVC;Clang;LANGUAGES")
	if (NOT compiler_flag_MSVC)
		set(compiler_flag_MSVC /${flag})
	endif()
	if (NOT compiler_flag_GCC)
		set(compiler_flag_GCC -${flag})
	endif()
	if (NOT compiler_flag_Clang)
		set(compiler_flag_Clang ${compiler_flag_GCC})
	endif()
	string(MAKE_C_IDENTIFIER "${flag}" suffix)
	string(TOUPPER "${suffix}" suffix)
	if (NOT compiler_flag_LANGUAGES)
		get_property(compiler_flag_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
	endif()
	if (CXX IN_LIST compiler_flag_LANGUAGES)
		if (MSVC)
			check_cxx_compiler_flag(${compiler_flag_MSVC} CXX_CHECK_FLAG_${suffix})
		elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)
			check_cxx_compiler_flag(${compiler_flag_Clang} CXX_CHECK_FLAG_${suffix})
		else()
			check_cxx_compiler_flag(${compiler_flag_GCC} CXX_CHECK_FLAG_${suffix})
		endif()
	endif()
	if (C IN_LIST compiler_flag_LANGUAGES)
		if (MSVC)
			check_c_compiler_flag(${compiler_flag_MSVC} C_CHECK_FLAG_${suffix})
		elseif (CMAKE_C_COMPILER_ID MATCHES Clang)
			check_c_compiler_flag(${compiler_flag_Clang} C_CHECK_FLAG_${suffix})
		else()
			check_c_compiler_flag(${compiler_flag_GCC} C_CHECK_FLAG_${suffix})
		endif()
	endif()
	string(CONCAT when $<OR:
		$<AND:$<BOOL:${CXX_CHECK_FLAG_${suffix}}>,$<COMPILE_LANGUAGE:CXX>>,
		$<AND:$<BOOL:${C_CHECK_FLAG_${suffix}}>,$<COMPILE_LANGUAGE:C>>
	>)
	string(CONCAT compiler_flag
		$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${compiler_flag_MSVC}>
		$<$<COMPILE_LANG_AND_ID:C,MSVC>:${compiler_flag_MSVC}>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${compiler_flag_GCC}>
		$<$<COMPILE_LANG_AND_ID:C,GNU>:${compiler_flag_GCC}>
		$<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:${compiler_flag_Clang}>
		$<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:${compiler_flag_Clang}>
	)

	set(--${flag} $<${when}:${compiler_flag}> PARENT_SCOPE)
endfunction()
