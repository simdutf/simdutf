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

#[[
This is used to create a version variable. How these are used or set is up
to the user.
TODO: Support find_version(<VAR> SOURCE <FILE>) for parsing a version name from a header file
TODO: Support find_version(<VAR> LIBRARY <FILE>) for .dll/.lib/.so/.dylib parsing
]]
function (find_version output)
	cmake_parse_arguments(ARG "" "OPTION;REGEX;COMMAND;DOC" "" ${ARGN})
	unset(version-output)
	unset(version-check)
	if (NOT ARG_OPTION)
		set(ARG_OPTION "--version")
	endif()
	if (NOT ARG_REGEX)
		set(ARG_REGEX "[^0-9]*([0-9]+)[.]([0-9]+)?[.]?([0-9]+)?[.]?([0-9]+)?.*")
	endif()
	if (ARG_COMMAND AND NOT DEFINED ${output})
		execute_process(
			COMMAND ${ARG_COMMAND} ${ARG_OPTION}
			OUTPUT_VARIABLE version-output
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ENCODING UTF-8)
		if (version-output)
			string(REGEX MATCH "${ARG_REGEX}" version-check ${version-output})
		endif()
		if (version-check)
			string(JOIN "." ${output}
				${CMAKE_MATCH_1}
				${CMAKE_MATCH_2}
				${CMAKE_MATCH_3}
				${CMAKE_MATCH_4})
			set(${output} "${${output}}" CACHE STRING "${ARG_DOC}" FORCE)
		endif()
	endif()
endfunction()
