# =============================================================================
#
# ztd.cuneicode
# Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.cuneicode licenses may use this file in
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

# Dependencies
find_package(Doxygen REQUIRED)
find_package(Python3 REQUIRED)
find_package(Sphinx QUIET)

# For the install target and related directories
include(GNUInstallDirs)

# Find all the public headers
file(GLOB_RECURSE ztd_cuneicode_public_headers
	LIST_DIRECTORIES NO
	CONFIGURE_DEPENDS
	${PROJECT_SOURCE_DIR}/include/**.*)
# make a list copy for use with dependency tracking later down below
set(ztd_cuneicode_doxygen_public_headers ${ztd_cuneicode_public_headers})
# separate text with spaces and surround them with quotes for Doxygen to understand them all
list(TRANSFORM ztd_cuneicode_doxygen_public_headers REPLACE "(.+)" [["\1"]])
list(JOIN ztd_cuneicode_doxygen_public_headers " " ztd_cuneicode_doxygen_public_headers)
# do the same for the include paths
get_target_property(ztd_cuneicode_doxygen_include_paths ztd::cuneicode INCLUDE_DIRECTORIES)
get_target_property(ztd_idk_doxygen_include_paths ztd::idk INTERFACE_INCLUDE_DIRECTORIES)
list(TRANSFORM ztd_cuneicode_doxygen_include_paths REPLACE "(.+)" [["\1"]])
list(TRANSFORM ztd_idk_doxygen_include_paths REPLACE "(.+)" [["\1"]])
list(JOIN ztd_cuneicode_doxygen_include_paths " " ztd_cuneicode_doxygen_include_paths)
list(JOIN ztd_idk_doxygen_include_paths " " ztd_cuneicode_doxygen_include_paths)

set(ZTD_CUNEICODE_DOXYGEN_PROJECT_DESCRIPTION ${PROJECT_DESCRIPTION})
set(ZTD_CUNEICODE_DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
set(ZTD_CUNEICODE_DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
set(ZTD_CUNEICODE_DOXYGEN_INPUT ${ztd_cuneicode_doxygen_public_headers})
set(ZTD_CUNEICODE_DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(ZTD_CUNEICODE_DOXYGEN_XML_OUTPUT_DIR ${ZTD_CUNEICODE_DOXYGEN_OUTPUT_DIR}/xml)
set(ZTD_CUNEICODE_DOXYGEN_HTML_OUTPUT_DIR ${ZTD_CUNEICODE_DOXYGEN_OUTPUT_DIR}/html)
set(ZTD_CUNEICODE_DOXYGEN_INCLUDE_PATH ${ztd_cuneicode_doxygen_include_paths})
set(ZTD_CUNEICODE_DOXYGEN_INDEX_FILE ${ZTD_CUNEICODE_DOXYGEN_XML_OUTPUT_DIR}/index.xml)
set(ZTD_CUNEICODE_DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(ZTD_CUNEICODE_DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
set(ZTD_CUNEICODE_SPHINX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(ZTD_CUNEICODE_SPHINX_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(ZTD_CUNEICODE_SPHINX_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/sphinx/index.html)

# Get all Sphinx source files
file(GLOB_RECURSE ztd_cuneicode_sphinx_sources
	LIST_DIRECTORIES NO
	CONFIGURE_DEPENDS ${ZTD_CUNEICODE_SPHINX_SOURCE_DIR}/**.*)

# Ensure Doxygen/Sphinx won't crash because it doesn't create directories for us
file(MAKE_DIRECTORY ${ZTD_CUNEICODE_DOXYGEN_OUTPUT_DIR})
file(MAKE_DIRECTORY ${ZTD_CUNEICODE_DOXYGEN_XML_OUTPUT_DIR})
file(MAKE_DIRECTORY ${ZTD_CUNEICODE_DOXYGEN_HTML_OUTPUT_DIR})
file(MAKE_DIRECTORY ${ZTD_CUNEICODE_SPHINX_BUILD_DIR})

# Do generator expressions and
# Replace variables inside @...@ with the current values
set(ZTD_CUNEICODE_DOXYFILE_IN_CONFIGURED ${ZTD_CUNEICODE_DOXYFILE_OUT}.conf)
configure_file(${ZTD_CUNEICODE_DOXYFILE_IN} ${ZTD_CUNEICODE_DOXYFILE_IN_CONFIGURED} @ONLY)
file(GENERATE OUTPUT ${ZTD_CUNEICODE_DOXYFILE_OUT}
	INPUT ${ZTD_CUNEICODE_DOXYFILE_IN_CONFIGURED})

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
	${ZTD_CUNEICODE_DOXYGEN_XML_OUTPUT_DIR}
	${ZTD_CUNEICODE_DOXYGEN_HTML_OUTPUT_DIR}
	${ZTD_CUNEICODE_DOXYGEN_OUTPUT_DIR}
	${ZTD_CUNEICODE_SPHINX_BUILD_DIR})

add_custom_command(OUTPUT ${ZTD_CUNEICODE_DOXYGEN_INDEX_FILE}
	COMMAND ${DOXYGEN_EXECUTABLE} ${ZTD_CUNEICODE_DOXYFILE_OUT}
	MAIN_DEPENDENCY ${ZTD_CUNEICODE_DOXYFILE_IN} ${ZTD_CUNEICODE_DOXYFILE_OUT}
	DEPENDS ${ztd_cuneicode_public_headers}
	COMMENT "[ztd.cuneicode/documentation] Generating Doxygen XML..."
)
add_custom_target(ztd.cuneicode.documentation.doxygen ALL
	DEPENDS ${ZTD_CUNEICODE_DOXYGEN_INDEX_FILE}
)

if (NOT ZTD_CUNEICODE_DOCUMENTATION_NO_SPHINX)
	add_custom_command(OUTPUT ${ZTD_CUNEICODE_SPHINX_INDEX_FILE}
		COMMAND Sphinx::Build
			-b html
			-Dbreathe_projects.ztd.cuneicode=${ZTD_CUNEICODE_DOXYGEN_XML_OUTPUT_DIR}
			${ZTD_CUNEICODE_SPHINX_SOURCE_DIR}
			${ZTD_CUNEICODE_SPHINX_BUILD_DIR}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${ZTD_CUNEICODE_DOXYGEN_INDEX_FILE} ${ztd_cuneicode_sphinx_sources}
		COMMENT "[ztd.cuneicode/documentation] Generating Sphinx documentation..."
		COMMAND_EXPAND_LISTS
		VERBATIM)
	
	add_custom_target(ztd.cuneicode.documentation.sphinx ALL
		DEPENDS ${ZTD_CUNEICODE_SPHINX_INDEX_FILE}
	)
	add_dependencies(ztd.cuneicode.documentation.sphinx ztd.cuneicode.documentation.doxygen)
	install(DIRECTORY ${ZTD_CUNEICODE_SPHINX_BUILD_DIR}
		DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
