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

cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21)

message(DEBUG "ztd.cmake Library has been included")

set(CMAKE_MESSAGE_CONTEXT_SHOW YES)
set(CMAKE_MESSAGE_CONTEXT ztd.cmake)

message(DEBUG "Disabling in-source builds and source directory changes")
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON CACHE BOOL "Prevents cmake -B. -S.")
set(CMAKE_DISABLE_SOURCE_CHANGES ON CACHE BOOL "Prevents writing files to CMAKE_CURRENT_SOURCE_DIR")

set(ZTD_CMAKE_PACKAGES "${CMAKE_CURRENT_SOURCE_DIR}/Packages"
	CACHE PATH "ztd.cmake Packages" FORCE)
set(ZTD_CMAKE_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/Modules"
	CACHE PATH "ztd.cmake Modules" FORCE)
set(ZTD_CMAKE_PROJECT_PRELUDE "${CMAKE_CURRENT_SOURCE_DIR}/Includes/Project.cmake"
	CACHE FILEPATH "ztd.cmake Project Prelude" FORCE)

string(CONCAT ZTD_CMAKE_IS_TOP_LEVEL $<STREQUAL:${PROJECT_NAME},${CMAKE_PROJECT_NAME}>)
