# =============================================================================
#
# ztd.idk
# Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
# Contact: opensource@soasis.org
#
# Commercial License Usage
# Licensees holding valid commercial ztd.idk licenses may use this file in
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
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============================================================================>

cmake_minimum_required(VERSION 3.21.0)
cmake_policy(VERSION 3.21)

# # Project kickstart
# Includes a bunch of basic flags and utilities shared across projects
# See more at the github repository link below
include(FetchContent)
FetchContent_Declare(ztd.cmake
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cmake-d7888ab
)

FetchContent_MakeAvailable(ztd.cmake)
set(CMAKE_PROJECT_INCLUDE ${ZTD_CMAKE_PROJECT_PRELUDE})

# # Project declaration
# informs about the project, gives a description, version and MOST IMPORTANTLY
# the languages the project is going to use. Required.
project(ztd.idk
	VERSION 0.0.0
	DESCRIPTION "The IDK (Industrial Development Kit) Library, for useful tools that get the job done!"
	HOMEPAGE_URL "https://ztdidk.rtfd.io/"
	LANGUAGES CXX C)

if(ZTD_IDK_READTHEDOCS)
	# ReadTheDocs seems unable to handle the include at the project level: something must be going wrong?
	include(CheckCXXCompilerFlag)
	include(CheckCCompilerFlag)
	include(CheckIPOSupported)
	include(CMakePackageConfigHelpers)
	include(CMakeDependentOption)
	include(CMakePrintHelpers)
	include(GNUInstallDirs)
	include(FeatureSummary)
	include(FetchContent)
	include(CTest)
endif()

# # # Top-Level Directories
# Check if this is the top-level project or not
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(ZTD_IDK_IS_TOP_LEVEL_PROJECT YES)
else()
	set(ZTD_IDK_IS_TOP_LEVEL_PROJECT NO)
endif()

# # Options
option(ZTD_IDK_CI "Whether or not we are in continuous integration mode" OFF)
option(ZTD_IDK_READTHEDOCS "Whether or not we are building inside of ReadTheDocs" OFF)
option(ZTD_IDK_TESTS "Enable build of tests" OFF)
option(ZTD_IDK_DOCUMENTATION "Enable build of documentation" OFF)
option(ZTD_IDK_DOCUMENTATION_NO_SPHINX "Turn off Sphinx usage (useful for ReadTheDocs builds)" OFF)
option(ZTD_IDK_EXAMPLES "Enable build of examples" OFF)
option(ZTD_IDK_BENCHMARKS "Enable build of benchmarks" OFF)
option(ZTD_IDK_GENERATE_SINGLE "Enable generation of a single header and its target" OFF)
set(ZTD_IDK_BENCHMARKS_REPETITIONS 50 CACHE STRING "Number of repetitions to perform. Each repetition has a number of iterations: higher means a lot more time is spent benchmarking.")

# Modify bad flags / change defaults if we are the top level
if(ZTD_IDK_IS_TOP_LEVEL_PROJECT)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

	if(NOT DEFINED CMAKE_CXX_STANDARD)
		set(CMAKE_CXX_STANDARD 17)
	endif()

	if(NOT DEFINED CMAKE_C_STANDARD)
		set(CMAKE_C_STANDARD 11)
	endif()

	if(ZTD_IDK_BENCHMARKS OR ZTD_IDK_EXAMPLES OR ZTD_IDK_TESTS OR ZTD_IDK_SCRATCH)
		# normal flags
		check_compiler_flag(disable-permissive MSVC /permissive- GCC -pedantic)
		check_compiler_flag(disable-aliasing GCC -fno-strict-aliasing)

		# Warning flags
		check_compiler_flag(warn-pedantic MSVC /permissive- GCC -pedantic)
		check_compiler_flag(warn-all MSVC /W4 GCC -Wall)
		check_compiler_flag(warn-errors MSVC /WX GCC -Werror)
		check_compiler_flag(warn-extra GCC -Wextra Clang -Wextra)
		check_compiler_flag(utf8-literal-encoding MSVC /execution-charset:utf-8 GCC -fexec-charset=utf-8)
		check_compiler_flag(utf8-source-encoding MSVC /source-charset:utf-8 GCC -finput-charset=utf-8)
		check_compiler_flag(extra-constexpr-depth MSVC /constexpr:depth2147483647 GCC -fconstexpr-depth=2147483647 Clang -fconstexpr-depth=2147483647)
		check_compiler_flag(extra-constexpr-steps MSVC /constexpr:steps2147483647 GCC -fconstexpr-ops-limit=2147483647 Clang -fconstexpr-steps=2147483647)
		check_compiler_flag(template-debugging-mode GCC -ftemplate-backtrace-limit=0)
		check_compiler_diagnostic(microsoft-enum-value)
		check_compiler_diagnostic(gnu-zero-variadic-macro-arguments)
		check_compiler_diagnostic(type-limits)
	endif()
endif()

# # Main library declarations
# ztd.version
add_library(ztd.version INTERFACE)
add_library(ztd::version ALIAS ztd.version)
target_include_directories(ztd.version
	INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
install(DIRECTORY include/
	DESTINATION
	${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
	cmake/ztd.version-config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.version/ztd.version-config.cmake"
	INSTALL_DESTINATION lib/cmake/ztd.version
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.version/ztd.version-config-version.cmake"
	COMPATIBILITY SameMajorVersion
)
export(TARGETS ztd.version
	FILE
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.version/ztd.version-targets.cmake"
)

# ztd.tag_invoke
file(GLOB_RECURSE ztd.tag_invoke.includes
	CONFIGURE_DEPENDS
	include/tag_invoke**.hpp
	include/tag_invoke**.h
)

add_library(ztd.tag_invoke INTERFACE)
add_library(ztd::tag_invoke ALIAS ztd.tag_invoke)
target_include_directories(ztd.tag_invoke
	INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ztd.tag_invoke
	INTERFACE
	ztd::version
)
install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
	cmake/ztd.tag_invoke-config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.tag_invoke/ztd.tag_invoke-config.cmake"
	INSTALL_DESTINATION lib/cmake/ztd.tag_invoke
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.tag_invoke/ztd.tag_invoke-config-version.cmake"
	COMPATIBILITY SameMajorVersion
)
export(TARGETS ztd.tag_invoke
	FILE
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.tag_invoke/ztd.tag_invoke-targets.cmake"
)

# ztd.idk
file(GLOB_RECURSE ztd.idk.includes
	CONFIGURE_DEPENDS
	include/ztd/idk**.hpp
	include/ztd/idk**.h
)
file(GLOB_RECURSE ztd.idk.sources
	CONFIGURE_DEPENDS
	source/ztd/idk/**.cpp
	source/ztd/idk/**.c
)

add_library(ztd.idk ${ztd.idk.sources})
add_library(ztd::idk ALIAS ztd.idk)
target_include_directories(ztd.idk
	PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
	$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/source/include>
)
target_link_libraries(ztd.idk
	PUBLIC
	ztd::version
	ztd::tag_invoke
)
target_compile_definitions(ztd.idk
	PRIVATE
		ZTD_IDK_BUILD=1
	PUBLIC
		$<$<STREQUAL:$<TARGET_PROPERTY:ztd.idk,TYPE>,SHARED_LIBRARY>:ZTD_IDK_DLL=1>
		${CMAKE_DL_LIBS}
)

if(ZTD_IDK_IS_TOP_LEVEL_PROJECT)
	target_compile_options(ztd.idk
		PRIVATE
		${--utf8-literal-encoding}
		${--utf8-source-encoding}
		${--disable-permissive}
		${--warn-pedantic}
		${--warn-default}
		${--warn-extra}
		${--warn-errors}
	)
endif()

install(TARGETS ztd.idk)
install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
	cmake/ztd.idk-config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.idk/ztd.idk-config.cmake"
	INSTALL_DESTINATION lib/cmake/ztd.idk
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.idk/ztd.idk-config-version.cmake"
	COMPATIBILITY SameMajorVersion
)
export(TARGETS ztd.idk
	FILE
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/ztd.idk/ztd.idk-targets.cmake"
)

install(
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cmake"
	TYPE DATA
)

if(ZTD_IDK_GENERATE_SINGLE)
	add_subdirectory(single)
endif()

# # Benchmarks, Tests, Examples and Documentation
if(ZTD_IDK_TESTS)
	include(CTest)
	add_subdirectory(tests)
endif()

if(ZTD_IDK_DOCUMENTATION)
	add_subdirectory(documentation)
endif()

if(ZTD_IDK_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()

if(ZTD_IDK_EXAMPLES)
	add_subdirectory(examples)
endif()

# For quick debugging and development only: don't peek! 🙈
mark_as_advanced(ZTD_IDK_SCRATCH)

if(ZTD_IDK_SCRATCH)
	add_executable(scratch main.cpp)
	target_link_libraries(scratch PRIVATE ztd::idk)
	target_include_directories(scratch PRIVATE tests/shared/include)
	target_compile_options(scratch
		PRIVATE
		${--template-debugging-mode}
		${--disable-permissive}
		${--warn-pedantic}
		${--warn-all}
		${--warn-extra}
		${--warn-errors})
endif()
