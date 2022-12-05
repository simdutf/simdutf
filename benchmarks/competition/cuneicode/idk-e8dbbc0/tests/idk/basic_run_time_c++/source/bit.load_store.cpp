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

#include <ztd/idk/bit.h>
#include <ztd/idk/endian.h>
#include <ztd/idk/type_traits.hpp>
#include <ztd/tests/bit_constant.hpp>

#include <catch2/catch_all.hpp>

#include <cstring>

TEST_CASE("Ensure that the 8-bit load and store work properly for all array sizes.", "[bit][load.store]") {
#define SECTION_CASE(N)                                                                                           \
	SECTION("uint_least" #N "_t, little") {                                                                      \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const uint_least##N##_t data                                                                       \
			     = static_cast<uint_least##N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##N##_t>()); \
			ztdc_store8_leu##N(data, arr);                                                                     \
			uint_least##N##_t result = ztdc_load8_leu##N(arr);                                                 \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) == 0);                                      \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) != 0);                                      \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) uint_least##N##_t arr {};                                                    \
			const uint_least##N##_t data                                                                       \
			     = static_cast<uint_least##N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##N##_t>()); \
			ztdc_store8_aligned_leu##N(data, (unsigned char*)&arr);                                            \
			uint_least##N##_t result = ztdc_load8_aligned_leu##N((unsigned char*)&arr);                        \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	SECTION("int_least" #N "_t, little, negative") {                                                             \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##N##_t>());                          \
			ztdc_store8_les##N(data, (unsigned char*)&arr);                                                    \
			int_least##N##_t result = ztdc_load8_les##N((unsigned char*)&arr);                                 \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) == 0);                                      \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) != 0);                                      \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) int_least##N##_t arr {};                                                     \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##N##_t>());                          \
			ztdc_store8_aligned_les##N(data, (unsigned char*)&arr);                                            \
			int_least##N##_t result = ztdc_load8_aligned_les##N((unsigned char*)&arr);                         \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	SECTION("int_least" #N "_t, little, positive") {                                                             \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<int_least##N##_t>());                          \
			ztdc_store8_les##N(data, arr);                                                                     \
			int_least##N##_t result = ztdc_load8_les##N(arr);                                                  \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) == 0);                                      \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) != 0);                                      \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) int_least##N##_t arr {};                                                     \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<int_least##N##_t>());                          \
			ztdc_store8_aligned_les##N(data, (unsigned char*)&arr);                                            \
			int_least##N##_t result = ztdc_load8_aligned_les##N((unsigned char*)&arr);                         \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                               \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	SECTION("uint_least" #N "_t, big") {                                                                         \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const uint_least##N##_t data                                                                       \
			     = static_cast<uint_least##N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##N##_t>()); \
			ztdc_store8_beu##N(data, arr);                                                                     \
			uint_least##N##_t result = ztdc_load8_beu##N(arr);                                                 \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) uint_least##N##_t arr {};                                                    \
			const uint_least##N##_t data                                                                       \
			     = static_cast<uint_least##N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##N##_t>()); \
			ztdc_store8_aligned_beu##N(data, (unsigned char*)&arr);                                            \
			uint_least##N##_t result = ztdc_load8_aligned_beu##N((unsigned char*)&arr);                        \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	SECTION("int_least" #N "_t, big, negative") {                                                                \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##N##_t>());                          \
			ztdc_store8_bes##N(data, arr);                                                                     \
			int_least##N##_t result = ztdc_load8_bes##N(arr);                                                  \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) == 0);                                      \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) != 0);                                      \
				}                                                                                             \
				REQUIRE(result == data);                                                                      \
			}                                                                                                  \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) int_least##N##_t arr {};                                                     \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##N##_t>());                          \
			ztdc_store8_aligned_bes##N(data, (unsigned char*)&arr);                                            \
			int_least##N##_t result = ztdc_load8_aligned_bes##N((unsigned char*)&arr);                         \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	SECTION("int_least" #N "_t, big, positive") {                                                                \
		SECTION("unaligned") {                                                                                  \
			unsigned char arr[N / CHAR_BIT] {};                                                                \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<int_least##N##_t>());                          \
			ztdc_store8_bes##N(data, arr);                                                                     \
			int_least##N##_t result = ztdc_load8_bes##N(arr);                                                  \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) == 0);                                      \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(arr, &data, sizeof(arr)) != 0);                                      \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
		SECTION("aligned") {                                                                                    \
			alignas(N / CHAR_BIT) int_least##N##_t arr {};                                                     \
			const int_least##N##_t data = static_cast<int_least##N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<int_least##N##_t>());                          \
			ztdc_store8_aligned_bes##N(data, (unsigned char*)&arr);                                            \
			int_least##N##_t result = ztdc_load8_aligned_bes##N((unsigned char*)&arr);                         \
			if (N > width) {                                                                                   \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                  \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) == 0);                                     \
				}                                                                                             \
				else {                                                                                        \
					REQUIRE(std::memcmp(&arr, &data, sizeof(arr)) != 0);                                     \
				}                                                                                             \
			}                                                                                                  \
			REQUIRE(result == data);                                                                           \
		}                                                                                                       \
	}                                                                                                            \
	static_assert(true, "")

	const size_t width = CHAR_BIT;
	SECTION_CASE(8);
	SECTION_CASE(16);
	SECTION_CASE(32);
	SECTION_CASE(64);
}
