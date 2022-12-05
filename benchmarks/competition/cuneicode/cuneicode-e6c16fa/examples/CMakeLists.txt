# =============================================================================
#
# ztd.cuneicode
# Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.cuneicode licenses may use this file
# in accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Shepherd's Oasis, LLC.
# For licensing terms and conditions see your agreement. For
# further information contact opensource@soasis.org.
#
# Apache License Version 2 Usage
# Alternatively, this file may be used under the terms of Apache License
# Version 2.0 (the "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# =========================================================================== #

function(ztd_cuneicode_example_options example_target)
	cmake_parse_arguments(PARSE_ARGV 1 CUNEICODE_EXAMPLE_OPT "" "TEST" "")
	target_compile_options(${example_target}
		PRIVATE
		${--utf8-literal-encoding}
		${--utf8-source-encoding}
		${--disable-permissive}
		${--updated-cpp-version-flag}
		${--warn-pedantic}
		${--warn-all}
		${--warn-extra}
		${--warn-errors})
	target_link_libraries(${example_target}
		PRIVATE
		ztd::cuneicode)
	if (NOT DEFINED CUNEICODE_EXAMPLE_OPT_TEST)
		add_test(NAME ${example_target} COMMAND ${example_target})
	elseif (${CUNEICODE_EXAMPLE_OPT_TEST})
		add_test(NAME ${example_target} COMMAND ${example_target})
	endif()
	unset(CUNEICODE_EXAMPLE_OPT_TEST)
endfunction(ztd_cuneicode_example_options)

function(ztd_cuneicode_example_simple example_source_file example_prefix)
	get_filename_component(example_name ${example_source_file} NAME_WLE)
	get_filename_component(example_output_relative_dir ${example_source_file} DIRECTORY)
	file(TO_CMAKE_PATH "${example_output_relative_dir}" example_output_relative_dir_name)
	STRING(REGEX REPLACE "/" "." example_output_relative_dir_name "${example_output_relative_dir}")
	set(example_output_name ${example_name})
	set(example_name "${example_prefix}.${example_name}")

	add_executable(${example_name} ${example_source_file})

	ztd_cuneicode_example_options(${example_name})
endfunction(ztd_cuneicode_example_simple)

add_subdirectory(documentation)
add_subdirectory(basic)
add_subdirectory(simdutf)
add_subdirectory(cconv)
