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

file(GLOB_RECURSE ztd.cuneicode.tests.registry.sources
	LIST_DIRECTORIES FALSE
	CONFIGURE_DEPENDS
	source/*.cpp)

add_executable(ztd.cuneicode.tests.registry ${ztd.cuneicode.tests.registry.sources})
set_target_properties(ztd.cuneicode.tests.registry PROPERTIES
	VERSION ${PROJECT_VERSION}
)
target_compile_options(ztd.cuneicode.tests.registry
	PRIVATE
	${--utf8-literal-encoding}
	${--utf8-source-encoding}
	${--disable-permissive}
	${--updated-cpp-version-flag}
	${--warn-pedantic}
	${--warn-all}
	${--warn-extra}
	${--warn-errors})
target_include_directories(ztd.cuneicode.tests.registry PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(ztd.cuneicode.tests.registry
	PRIVATE
	Catch2::Catch2
	ztd::cuneicode
	ztd::cuneicode::shared)

add_test(NAME ztd.cuneicode.tests.registry COMMAND ztd.cuneicode.tests.registry)
