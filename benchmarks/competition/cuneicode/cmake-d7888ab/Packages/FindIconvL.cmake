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

# You need to find both:
# iconv.dll AND charset.dll to go with it.
# You cannot have only one and not the other.
# So, do a global search for the correct "pairs" of libraries, starting with DLL-based ones first,
# then spiral out from there.

find_program(
	iconv_dll
	NAMES
		iconv libiconv iconv.so libiconv.so libiconv-2.so iconv-2.so iconv-1.so libiconv-1.so
		iconv.dll libiconv.dll libiconv-2.dll iconv-2.dll iconv-1.dll libiconv-1.dll
)

set(charset_dll_guesses)

if (iconv_dll)
	# first, try to find a relevant import library if we're targeting Windows
	cmake_path(GET iconv_dll PARENT_PATH iconv_dll_dir)
	add_library(iconv IMPORTED SHARED)
	add_library(Iconv::iconv ALIAS iconv)
	set_target_properties(iconv
		PROPERTIES
		IMPORTED_LOCATION ${iconv_dll}
	)
	if (EXISTS ${iconv_dll_dir}/iconv.h)
		target_include_directories(iconv
			INTERFACE
				${iconv_dll_dir}
		)
	else()
		# just assume it's in ../include
		target_include_directories(iconv
			INTERFACE
				${iconv_dll_dir}/../include
		)
	endif()
	if (WIN32)
		# try to make it find a .lib close to the DLL, if at all possible,
		# before doing a general-purpose search
		cmake_path(GET iconv_dll STEM LAST_ONLY iconv_dll_name)
		cmake_path(APPEND iconv_dll_dir "${iconv_dll_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" iconv_import_lib_guess0)
		cmake_path(APPEND iconv_dll_dir "../lib" "${iconv_dll_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" iconv_import_lib_guess1)
		find_program(
			iconv_import_lib
			NAMES
				iconv.lib libiconv.lib libiconv-2.lib iconv-2.lib iconv-1.lib libiconv-1.lib
				iconv.lib libiconv.lib libiconv-2.lib iconv-2.lib iconv-1.lib libiconv-1.lib
			PATHS
				"${iconv_import_lib_guess0}" "${iconv_import_lib_guess1}"
		)
		if (iconv_import_lib)
			set_target_properties(iconv
				PROPERTIES
				IMPORTED_IMPLIB ${iconv_import_lib}
			)
		endif()
	endif()
	# Try to bias CMake to find iconv-related charset library
	cmake_path(GET iconv_dll FILENAME LAST_ONLY iconv_dll_filename)
	string(REPLACE "iconv" "charset" charset_dll_guess_name ${iconv_dll_filename})
	cmake_path(APPEND iconv_dll_dir "${charset_dll_guess_name}" charset_dll_guess0)
	list(PREPEND charset_dll_guesses ${charset_dll_guess0})

	# set found variables
	set(Iconv_FOUND ON)
	set(Iconv_LIBRARIES Iconv::iconv)
endif()

find_program(
	charset_dll
	NAMES
		charset libcharset charset.so libcharset.so libcharset-2.so charset-2.so charset-1.so libcharset-1.so
		charset.dll libcharset.dll libcharset-2.dll charset-2.dll charset-1.dll libcharset-1.dll
	PATHS
		${charset_dll_guesses}
)

if (charset_dll)
	# Okay, we have one. Make a new library, get the import lib, the usual...
	add_library(charset IMPORTED SHARED)
	add_library(Iconv::charset ALIAS charset)
	set_target_properties(charset
		PROPERTIES
		IMPORTED_LOCATION ${charset_dll}
	)
	if (TARGET Iconv::iconv)
		target_link_libraries(iconv
			PUBLIC
				Iconv::charset
		)
		list(APPEND Iconv_LIBRARIES Iconv::charset)
	endif()
	if (WIN32)
		# try to make it find a .lib close to the DLL, if at all possible,
		# before doing a general-purpose search
		cmake_path(GET charset_dll STEM LAST_ONLY charset_dll_name)
		cmake_path(APPEND charset_dll_dir "${charset_dll_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" charset_import_lib_guess0)
		cmake_path(APPEND charset_dll_dir "../lib" "${charset_dll_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" charset_import_lib_guess1)
		find_program(
			charset_import_lib
			NAMES
				charset.lib libcharset.lib libcharset-2.lib charset-2.lib charset-1.lib libcharset-1.lib
				charset.lib libcharset.lib libcharset-2.lib charset-2.lib charset-1.lib libcharset-1.lib
			PATHS
				"${charset_import_lib_guess0}" "${charset_import_lib_guess1}"
		)
		if (charset_import_lib)
			set_target_properties(charset
				PROPERTIES
				IMPORTED_IMPLIB ${charset_import_lib}
			)
		endif()
	endif()
endif()

# TODO: actually get the version out of the headers or something
set(Iconv_VERSION 0.0.0)

find_package_handle_standard_args(Iconv
	REQUIRED_VARS Iconv_LIBRARIES
	VERSION_VAR Iconv_VERSION
	HANDLE_COMPONENTS)

set_package_properties(Iconv
	PROPERTIES
		DESCRIPTION "The iconv character conversion library, as specified by POSIX and implemented by many."
		URL "https://pubs.opengroup.org/onlinepubs/9699919799/functions/iconv.html")
