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

#include <ztd/idk/bit.h>

#include <ztd/tests/bit_constant.h>

#include <stdlib.h>

#if defined(UINT8_MAX)
#define MEMREV8(TYPE)                                      \
	if ((sizeof(TYPE) * CHAR_BIT) == 8) {                 \
		return (TYPE)ztdc_memreverse8u8((uint8_t)value); \
	}                                                     \
	ztd_static_assert(1, "")
#else
#define MEMREV8(TYPE)
#endif

#if defined(UINT16_MAX)
#define MEMREV16(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 16) {                  \
		return (TYPE)ztdc_memreverse8u16((uint16_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV16(TYPE)
#endif

#if defined(UINT24_MAX)
#define MEMREV24(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 24) {                  \
		return (TYPE)ztdc_memreverse8u24((uint24_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV24(TYPE)
#endif

#if defined(UINT32_MAX)
#define MEMREV32(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 32) {                  \
		return (TYPE)ztdc_memreverse8u32((uint32_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV32(TYPE)
#endif

#if defined(UINT40_MAX)
#define MEMREV40(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 40) {                  \
		return (TYPE)ztdc_memreverse8u40((uint40_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV40(TYPE)
#endif

#if defined(UINT48_MAX)
#define MEMREV48(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 48) {                  \
		return (TYPE)ztdc_memreverse8u48((uint48_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV48(TYPE)
#endif

#if defined(UINT8_MAX)
#define MEMREV8(TYPE)                                      \
	if ((sizeof(TYPE) * CHAR_BIT) == 8) {                 \
		return (TYPE)ztdc_memreverse8u8((uint8_t)value); \
	}                                                     \
	ztd_static_assert(1, "")
#else
#define MEMREV8(TYPE)
#endif

#if defined(UINT56_MAX)
#define MEMREV56(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 56) {                  \
		return (TYPE)ztdc_memreverse8u56((uint56_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV56(TYPE)
#endif

#if defined(UINT64_MAX)
#define MEMREV64(TYPE)                                       \
	if ((sizeof(TYPE) * CHAR_BIT) == 64) {                  \
		return (TYPE)ztdc_memreverse8u64((uint64_t)value); \
	}                                                       \
	ztd_static_assert(1, "")
#else
#define MEMREV64(TYPE)
#endif


#define DEFINE_MEMREV_FUNCTION(TYPE, SUFFIX)          \
	TYPE ztdc_test_memreverse8##SUFFIX(TYPE value) { \
		MEMREV8(TYPE);                              \
		MEMREV16(TYPE);                             \
		MEMREV24(TYPE);                             \
		MEMREV32(TYPE);                             \
		MEMREV40(TYPE);                             \
		MEMREV48(TYPE);                             \
		MEMREV56(TYPE);                             \
		MEMREV64(TYPE);                             \
		abort();                                    \
		return (TYPE)0;                             \
	}                                                \
	ztd_static_assert(1, "")

DEFINE_MEMREV_FUNCTION(unsigned char, uc);
DEFINE_MEMREV_FUNCTION(unsigned short, us);
DEFINE_MEMREV_FUNCTION(unsigned int, ui);
DEFINE_MEMREV_FUNCTION(unsigned long, ul);
DEFINE_MEMREV_FUNCTION(unsigned long long, ull);

#define SELECT_BIT_CONSTANT_(TEST_TYPE, TEST_TYPE_SIZE) ZTD_TESTS_DISTINCT_BIT_CONSTANT(TEST_TYPE, TEST_TYPE_SIZE)
#define SELECT_BIT_CONSTANT(TEST_TYPE)                                                                           \
	((sizeof(TEST_TYPE) * CHAR_BIT == 8)                                                                        \
	          ? SELECT_BIT_CONSTANT_(TEST_TYPE, 8)                                                              \
	          : ((sizeof(TEST_TYPE) * CHAR_BIT == 16)                                                           \
	                    ? SELECT_BIT_CONSTANT_(TEST_TYPE, 16)                                                   \
	                    : ((sizeof(TEST_TYPE) * CHAR_BIT == 32)                                                 \
	                              ? SELECT_BIT_CONSTANT_(TEST_TYPE, 32)                                         \
	                              : ((sizeof(TEST_TYPE) * CHAR_BIT == 64) ? SELECT_BIT_CONSTANT_(TEST_TYPE, 64) \
	                                                                      : SELECT_BIT_CONSTANT_(TEST_TYPE, 64)))))

#define SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, TEST_TYPE_SIZE) \
	ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE(TEST_TYPE, TEST_TYPE_SIZE)
#define SELECT_REVERSE_BIT_CONSTANT(TEST_TYPE)                                             \
	((sizeof(TEST_TYPE) * CHAR_BIT == 8)                                                  \
	          ? SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, 8)                                \
	          : ((sizeof(TEST_TYPE) * CHAR_BIT == 16)                                     \
	                    ? SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, 16)                     \
	                    : ((sizeof(TEST_TYPE) * CHAR_BIT == 32)                           \
	                              ? SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, 32)           \
	                              : ((sizeof(TEST_TYPE) * CHAR_BIT == 64)                 \
	                                        ? SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, 64) \
	                                        : SELECT_REVERSE_BIT_CONSTANT_(TEST_TYPE, 64)))))

#define GENERATE_TEST_CASE(TEST_TYPE, SUFFIX)                                                                       \
	TEST_CASE(                                                                                                     \
	     "Ensure that the 8-bit memory reverse algorithm works on appropriately-sized variables for \"" #TEST_TYPE \
	     "\".",                                                                                                    \
	     "[bit][memreverse][8-bit][small][" #TEST_TYPE "]") {                                                      \
		const TEST_TYPE expected_value         = SELECT_BIT_CONSTANT(TEST_TYPE);                                  \
		const TEST_TYPE expected_reverse_value = SELECT_REVERSE_BIT_CONSTANT(TEST_TYPE);                          \
                                                                                                                    \
		SECTION("raw memory reverse") {                                                                           \
			TEST_TYPE value = expected_value;                                                                    \
			REQUIRE(value == expected_value);                                                                    \
			ztdc_memreverse8(sizeof(value), (unsigned char*)(&value));                                           \
			REQUIRE(value == expected_reverse_value);                                                            \
		}                                                                                                         \
		SECTION("value-based memory reverse") {                                                                   \
			TEST_TYPE value = expected_value;                                                                    \
			REQUIRE(value == expected_value);                                                                    \
			TEST_TYPE reverse_value = ztdc_test_memreverse8##SUFFIX(value);                                      \
			REQUIRE(value == expected_value);                                                                    \
			REQUIRE(reverse_value == expected_reverse_value);                                                    \
		}                                                                                                         \
	}                                                                                                              \
	ztd_static_assert(1, "")

extern int bit_memreverse_tests(void) {
	BEGIN_TEST("bit.memreverse");
#if ((CHAR_BIT % 8) == 0)
	GENERATE_TEST_CASE(unsigned char, uc);
	GENERATE_TEST_CASE(unsigned short, us);
	GENERATE_TEST_CASE(unsigned int, ui);
	GENERATE_TEST_CASE(unsigned long, ul);
	GENERATE_TEST_CASE(unsigned long long, ull);

	TEST_CASE("Ensure that hte 8-bit memory reverse algorithm works on large memory region.",
	     "[bit][memreverse][8-bit][large]") {
		SECTION("with canonical implementation") {
		}
	}
#endif
	END_TEST();
}


#undef GENERATE_TEST_CASE
#undef SELECT_REVERSE_BIT_CONSTANT_
#undef SELECT_REVERSE_BIT_CONSTANT
#undef SELECT_BIT_CONSTANT_
#undef SELECT_BIT_CONSTANT
