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

include(FindPackageHandleStandardArgs)
include(FeatureSummary)

find_program(Sphinx_Build_EXECUTABLE NAMES sphinx-build DOC "Path to sphinx-build executable")

if (Sphinx_Build_EXECUTABLE)
	set(Sphinx_Build_FOUND YES)
endif()

if (Sphinx_Build_FOUND)
	execute_process(
		COMMAND ${Sphinx_Build_EXECUTABLE} --version
		OUTPUT_VARIABLE Sphinx_Build_VERSION_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ENCODING UTF-8)
	if (Sphinx_Build_VERSION_OUTPUT)
		string(REGEX
			MATCH "[^0-9]*([0-9]+)[.]([0-9]+)?[.]?([0-9]+)?[.]?([0-9]+)?.*"
			Sphinx_Build_VERSION_CHECK ${Sphinx_Build_VERSION_OUTPUT})
	endif()
	if (Sphinx_Build_VERSION_CHECK)
		string(JOIN "." Sphinx_Build_VERSION
			${CMAKE_MATCH_1}
			${CMAKE_MATCH_2}
			${CMAKE_MATCH_3}
			${CMAKE_MATCH_4})
		set(Sphinx_Build_VERSION "${Sphinx_Build_VERSION}" CACHE STRING "sphinx-build version" FORCE)
	endif()
endif()

find_package_handle_standard_args(Sphinx
	REQUIRED_VARS Sphinx_Build_EXECUTABLE
	VERSION_VAR Sphinx_Build_VERSION
	HANDLE_COMPONENTS)

set_package_properties(Sphinx
	PROPERTIES
		DESCRIPTION "Sphinx Documentation Generator"
		URL "https://sphinx-doc.org")

if (Sphinx_Build_FOUND AND NOT TARGET Sphinx::Build)
	add_executable(Sphinx::Build IMPORTED)
	set_property(TARGET Sphinx::Build PROPERTY IMPORTED_LOCATION ${Sphinx_Build_EXECUTABLE})
	set_property(TARGET Sphinx::Build PROPERTY VERSION ${Sphinx_Build_VERSION})
	mark_as_advanced(Sphinx_Build_EXECUTABLE Sphinx_Build_VERSION)
endif()
