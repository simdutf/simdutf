// =============================================================================
//
// ztd.idk
// Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.idk licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================ //

#include <ztd/idk/static_assert.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef ZTD_IDK_C_TEST_H
#define ZTD_IDK_C_TEST_H

#define REQUIRE(...)                                                                                              \
	if (!(__VA_ARGS__)) {                                                                                        \
		if (__c_test_section_name != NULL) {                                                                    \
			fprintf(stderr, "Test: \"%s\"\nCase: \"%s\"\nSection:\"%s\"\n", __c_test_name, __c_test_case_name, \
			     __c_test_section_name);                                                                       \
		}                                                                                                       \
		else {                                                                                                  \
			fprintf(stderr, "Test: \"%s\"\nCase: \"%s\"\n", __c_test_name, __c_test_case_name);                \
		}                                                                                                       \
		fprintf(stderr, "\tCondition failed: %s\n", #__VA_ARGS__);                                              \
		__c_test_result_value += 1;                                                                             \
	}                                                                                                            \
	ztd_static_assert(1, "")

#define BEGIN_TEST(NAME)                            \
	{                                              \
		int __c_test_result_value         = 0;    \
		const char* __c_test_name         = NAME; \
		const char* __c_test_case_name    = NAME; \
		const char* __c_test_section_name = NULL; \
		ztd_static_assert(1, "")
#define END_TEST()                 \
	__c_test_name         = NULL; \
	__c_test_case_name    = NULL; \
	__c_test_section_name = NULL; \
	return __c_test_result_value; \
	}                             \
	ztd_static_assert(1, "")
#define TEST_CASE(NAME, ...)       \
	__c_test_name         = NAME; \
	__c_test_section_name = NULL;
#define SECTION(...) __c_test_section_name = __VA_ARGS__;

#endif
