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
Generates a manifest and a config file (windows) for a specific target.
]]
function (generate_target_manifest target)
	cmake_parse_arguments(PARSE_ARGV 1 flag "" "" "SYMLINK_DEPENDENCY_DIR")
	if (NOT flag_SYMLINK_DEPENDENCY_DIR)
		set(flag_SYMLINK_DEPENDENCY_DIR "trash")
	endif()

	# This only works for Win32 at the moment
	if (NOT WIN32)
		return()
	endif()

	# The basic application manifest. This can be added to the sources of the target,
	# and CMake should process it as-is.
	set(basic_manifest [=[<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
<asmv3:application>
	<asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2019/WindowsSettings" xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings"> 
		<activeCodePage>UTF-8</activeCodePage>
		<ws2:longPathAware>true</ws2:longPathAware>
	</asmv3:windowsSettings>
</asmv3:application>
</assembly>]=])

	set(manifest_file ${CMAKE_CURRENT_BINARY_DIR}/ztd.tools.${target}.manifest)
	
	target_sources(${target} PRIVATE ${manifest_file})

	file(CONFIGURE
		OUTPUT ${manifest_file}
		CONTENT ${basic_manifest})
endfunction()
