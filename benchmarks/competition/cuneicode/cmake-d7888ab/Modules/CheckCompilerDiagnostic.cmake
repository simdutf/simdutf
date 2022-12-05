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
Given a diagnostic name and flag, like
check_cxx_compiler_diagnostic(pig MSVC 1312)
or
check_cxx_compiler_diagnostic(pig GCC acab)
we check if the given flag works C++ compiler. If it does, we then generate
a --warn, --allow, --deny, and --forbid prefixed set of variables. Users are
then free to simply apply them to targets at will.
]]
function (check_compiler_diagnostic diagnostic)
	cmake_parse_arguments(PARSE_ARGV 1 diagnostic_flag "" "" "GCC;MSVC;Clang;LANGUAGES")
	if (NOT diagnostic_flag_MSVC)
		set(diagnostic_flag_MSVC ${diagnostic})
	endif()
	if (NOT diagnostic_flag_GCC)
		set(diagnostic_flag_GCC ${diagnostic})
	endif()
	if (NOT diagnostic_flag_Clang)
		set(diagnostic_flag_Clang ${diagnostic_flag_GCC})
	endif()
	string(MAKE_C_IDENTIFIER "${diagnostic}" suffix)
	string(TOUPPER "${suffix}" suffix)
	if (NOT diagnostic_flag_LANGUAGES)
		get_property(diagnostic_flag_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
	endif()
	if (CXX IN_LIST diagnostic_flag_LANGUAGES)
		if (MSVC)
			check_cxx_compiler_flag(-w1${diagnostic_flag_MSVC} CXX_DIAGNOSTIC_${suffix})
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
			check_cxx_compiler_flag(-W${diagnostic_flag_Clang} CXX_DIAGNOSTIC_${suffix})
		else()
			check_cxx_compiler_flag(-W${diagnostic_flag_GCC} CXX_DIAGNOSTIC_${suffix})
		endif()
	endif()
	if (C IN_LIST diagnostic_flag_LANGUAGES)
		if (MSVC)
			check_c_compiler_flag(-w1${diagnostic_flag_MSVC} C_DIAGNOSTIC_${suffix})
		elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
			check_c_compiler_flag(-W${diagnostic_flag_Clang} C_DIAGNOSTIC_${suffix})
		else()
			check_c_compiler_flag(-W${diagnostic_flag_GCC} C_DIAGNOSTIC_${suffix})
		endif()
	endif()
	string(CONCAT when $<OR:
		$<AND:$<BOOL:${CXX_DIAGNOSTIC_${suffix}}>,$<COMPILE_LANGUAGE:CXX>>,
		$<AND:$<BOOL:${C_DIAGNOSTIC_${suffix}}>,$<COMPILE_LANGUAGE:C>>
	>)
	string(CONCAT diagnostic_flag
		$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${diagnostic_flag_MSVC}>
		$<$<COMPILE_LANG_AND_ID:C,MSVC>:${diagnostic_flag_MSVC}>
		$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${diagnostic_flag_GCC}>
		$<$<COMPILE_LANG_AND_ID:C,GNU>:${diagnostic_flag_GCC}>
		$<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:${diagnostic_flag_Clang}>
		$<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:${diagnostic_flag_Clang}>
	)
	set(forbid_prefix $<IF:$<BOOL:${MSVC}>,-we,-Werror=>)
	set(allow_prefix $<IF:$<BOOL:${MSVC}>,-wd,-Wno->)
	set(warn_prefix $<IF:$<BOOL:${MSVC}>,-w1,-W>)

	set(--forbid-${diagnostic} $<${when}:${forbid_prefix}${diagnostic_flag}> PARENT_SCOPE)
	set(--deny-${diagnostic} ${--forbid-${diagnostic}} PARENT_SCOPE)
	set(--allow-${diagnostic} $<${when}:${allow_prefix}${diagnostic_flag}> PARENT_SCOPE)
	set(--warn-${diagnostic} $<${when}:${warn_prefix}${diagnostic_flag}> PARENT_SCOPE)

endfunction()
