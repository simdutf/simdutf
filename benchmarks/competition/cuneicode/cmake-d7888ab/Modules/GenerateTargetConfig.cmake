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
Generates a manifest and a config file (windows) for a specific target. This technique just uses up to nine (9) directories and links them in as the target privateDirs for the application.
]]
function (generate_target_config target)
	cmake_parse_arguments(PARSE_ARGV 1 flag "" "" "SYMLINK_SYMLINK_DEPENDENCY_DIR")
	if (NOT flag_SYMLINK_SYMLINK_DEPENDENCY_DIR)
		set(flag_SYMLINK_SYMLINK_DEPENDENCY_DIR "trash")
	endif()

	if (NOT WIN32)
		return()
	endif()

	# The basic configuration file. This has to be placed next to the application/dll and must have
	# the same name as it, plus the suffix ".manifest".
	set(raw_basic_config [=[<?xml version="1.0"?>
<configuration>
	<runtime>
		<assemblyBinding xmlns="urn:schemas-microsoft-com.asm.v1">
			<probing privatePath="${ZTD_GCAMF_DEPENDENCY_DIR}" />
		</assemblyBinding>
	</runtime>
</configuration>]=])
endfunction()
