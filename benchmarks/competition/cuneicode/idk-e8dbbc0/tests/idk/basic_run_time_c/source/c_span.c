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

#include "c_test.h"

#include <ztd/idk/c_span.h>

#define C_SPAN_BASIC_TEST(TYPE, TYPE_NAME)                                                                           \
	TEST_CASE("c_span/basic/" #TYPE_NAME, "basic usages of c_span do not explode") {                                \
		typedef TYPE Type;                                                                                         \
		SECTION("designated initializers") {                                                                       \
			size_t expected_size       = 0;                                                                       \
			Type* expected_pointer     = NULL;                                                                    \
			Type* expected_pointer_end = expected_pointer + expected_size;                                        \
			TYPE_NAME value0           = { .data = expected_pointer, .size = expected_size };                     \
			REQUIRE(TYPE_NAME##_data(value0) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value0) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value0) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value0) == expected_pointer_end);                                             \
		}                                                                                                          \
		SECTION("empty") {                                                                                         \
			size_t expected_size       = 0;                                                                       \
			Type* expected_pointer     = NULL;                                                                    \
			Type* expected_pointer_end = expected_pointer + expected_size;                                        \
			TYPE_NAME value0           = { expected_pointer, expected_size };                                     \
			TYPE_NAME value1           = { expected_pointer, (size_t)(expected_pointer_end - expected_pointer) }; \
			TYPE_NAME value2           = make_##TYPE_NAME(expected_pointer, expected_pointer_end);                \
			TYPE_NAME value3           = make_sized_##TYPE_NAME(expected_pointer, expected_size);                 \
			REQUIRE(TYPE_NAME##_data(value0) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value0) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value0) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value0) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_data(value1) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value1) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value1) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value1) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_data(value2) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value2) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value2) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value2) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_data(value3) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value3) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value3) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value3) == expected_pointer_end);                                             \
		}                                                                                                          \
		SECTION("1 element") {                                                                                     \
			size_t expected_size       = 1;                                                                       \
			Type expected_value        = { 0 };                                                                   \
			Type* expected_pointer     = &expected_value;                                                         \
			Type* expected_pointer_end = expected_pointer + expected_size;                                        \
			TYPE_NAME value0           = { expected_pointer, expected_size };                                     \
			TYPE_NAME value1           = { expected_pointer, (size_t)(expected_pointer_end - expected_pointer) }; \
			TYPE_NAME value2           = make_##TYPE_NAME(expected_pointer, expected_pointer_end);                \
			TYPE_NAME value3           = make_sized_##TYPE_NAME(expected_pointer, expected_size);                 \
			REQUIRE(TYPE_NAME##_data(value0) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value0) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value0) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value0) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value0) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value0) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value1) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value1) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value1) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value1) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value1) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value1) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value2) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value2) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value2) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value2) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value2) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value2) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value3) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value3) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value3) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value3) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value3) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value3) == *(expected_pointer_end - 1));                                     \
		}                                                                                                          \
		SECTION("multi element") {                                                                                 \
			size_t expected_size       = 3;                                                                       \
			Type expected_value[3]     = { 0 };                                                                   \
			Type* expected_pointer     = expected_value + 0;                                                      \
			Type* expected_pointer_end = expected_pointer + expected_size;                                        \
			TYPE_NAME value0           = { expected_pointer, expected_size };                                     \
			TYPE_NAME value1           = { expected_pointer, (size_t)(expected_pointer_end - expected_pointer) }; \
			TYPE_NAME value2           = make_##TYPE_NAME(expected_pointer, expected_pointer_end);                \
			TYPE_NAME value3           = make_sized_##TYPE_NAME(expected_pointer, expected_size);                 \
			REQUIRE(TYPE_NAME##_data(value0) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value0) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value0) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value0) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value0) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value0) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value1) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value1) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value1) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value1) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value1) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value1) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value2) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value2) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value2) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value2) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value2) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value2) == *(expected_pointer_end - 1));                                     \
			REQUIRE(TYPE_NAME##_data(value3) == expected_pointer);                                                \
			REQUIRE(TYPE_NAME##_size(value3) == expected_size);                                                   \
			REQUIRE(TYPE_NAME##_begin(value3) == expected_pointer);                                               \
			REQUIRE(TYPE_NAME##_end(value3) == expected_pointer_end);                                             \
			REQUIRE(TYPE_NAME##_front(value3) == *expected_pointer);                                              \
			REQUIRE(TYPE_NAME##_back(value3) == *(expected_pointer_end - 1));                                     \
		}                                                                                                          \
	}                                                                                                               \
	ztd_static_assert(1, "")

extern int c_span_tests(void) {
	BEGIN_TEST("c_span");
	C_SPAN_BASIC_TEST(char, c_span_char);
	C_SPAN_BASIC_TEST(unsigned char, c_span_uchar);
	C_SPAN_BASIC_TEST(wchar_t, c_span_wchar_t);
	C_SPAN_BASIC_TEST(ztd_char16_t, c_span_char16_t);
	C_SPAN_BASIC_TEST(ztd_char32_t, c_span_char32_t);
	END_TEST();
}
