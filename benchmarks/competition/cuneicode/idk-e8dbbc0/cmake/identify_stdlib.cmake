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
Gets the name of the current C++ standard library, or "unknown" if it cannot detect such a standard library.
]]
function (identify_stdlib STDLIBNAME)
	set(idstl_test_dir "${CMAKE_BINARY_DIR}/.identify_stdlib")
	set(idstl_libc++_file "${idstl_test_dir}/test_libc++_file.cpp")
	set(idstl_libstdc++_file "${idstl_test_dir}/test_libstdc++_file.cpp")
	set(idstl_vc++_file "${idstl_test_dir}/test_vc++_file.cpp")

	file(WRITE "${idstl_libc++_file}"
	[==[
	#include <cstddef>
	#include <ciso646>
	int main (int, char*[]) {
	#if defined(_LIBCPP_VERSION)
		return 0;
	#else
		return 1;
	#endif
	}
	]==])
	file(WRITE "${idstl_libstdc++_file}"
	[==[
	#include <cstddef>
	#include <ciso646>
	int main (int, char*[]) {
	#if defined(__GLIBCXX__)
		return 0;
	#else
		return 1;
	#endif
	}
	]==])
	file(WRITE "${idstl_vc++_file}"
	[==[
	#include <cstddef>
	#include <ciso646>
	int main (int, char*[]) {
	#if defined(_STL_LANG) || defined(_YVALS_CORE_H) || defined(_STDEXT)
		return 0;
	#else
		return 1;
	#endif
	}
	]==])

	try_run(is_libc++_run is_libc++_compile "${idstl_test_dir}" "${idstl_libc++_file}"
		CMAKE_FLAGS "CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}" "CMAKE_C_FLAGS:${CMAKE_C_FLAGS}"
		"CMAKE_C_COMPILER:${CMAKE_C_COMPILER}" "CMAKE_CXX_COMPILER:${CMAKE_CXX_COMPILER}")
	try_run(is_libstdc++_run is_libstdc++_compile "${idstl_test_dir}" "${idstl_libstdc++_file}"
		CMAKE_FLAGS "CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}" "CMAKE_C_FLAGS:${CMAKE_C_FLAGS}"
		"CMAKE_C_COMPILER:${CMAKE_C_COMPILER}" "CMAKE_CXX_COMPILER:${CMAKE_CXX_COMPILER}")
	try_run(is_vc++_run is_vc++_compile "${idstl_test_dir}" "${idstl_vc++_file}"
		CMAKE_FLAGS "CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}" "CMAKE_C_FLAGS:${CMAKE_C_FLAGS}"
		"CMAKE_C_COMPILER:${CMAKE_C_COMPILER}" "CMAKE_CXX_COMPILER:${CMAKE_CXX_COMPILER}")

	if (is_libc++_run EQUAL 0)
		set(${STDLIBNAME} libc++ PARENT_SCOPE)
	elseif (is_libstdc++_run EQUAL 0)
		set(${STDLIBNAME} libstdc++ PARENT_SCOPE)
	elseif (is_vc++_run EQUAL 0)
		set(${STDLIBNAME} vc++ PARENT_SCOPE)
	else ()
		set(${STDLIBNAME} unknown PARENT_SCOPE)
	endif()
endfunction()
