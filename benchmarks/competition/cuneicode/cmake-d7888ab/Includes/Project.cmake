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

include_guard(DIRECTORY)

list(PREPEND CMAKE_MODULE_PATH "${ZTD_CMAKE_PACKAGES}")
list(PREPEND CMAKE_MODULE_PATH "${ZTD_CMAKE_MODULES}")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MESSAGE_CONTEXT "${PROJECT_NAME}")

# # CMake and ztd Includes
# CMake
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
# ztd
include(BenchmarkGrapher)
include(CheckCompilerDiagnostic)
include(CheckCompilerFlag)
include(FindVersion)
include(GenerateTargetManifest)
include(GenerateTargetConfig)
