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

#include <catch2/catch_all.hpp>

#include <ztd/idk/bit.h>

#include <ztd/idk/endian.h>
#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/assume_aligned.hpp>
#include <ztd/tests/bit_constant.hpp>
#include <ztd/idk/assert.hpp>

#include <ztd/idk/detail/bit.load_store.impl.h>

#include <cstring>
#include <climits>

static void explode_and_catch() {
	REQUIRE(false);
}

#define ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(_TYPE, _N, _SUFFIX)                              \
	static uint_least##_N##_t ztdc_load8_leu##_N##_SUFFIX(                                                \
	     [[maybe_unused]] const _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {           \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static uint_least##_N##_t ztdc_load8_beu##_N##_SUFFIX(                                                \
	     [[maybe_unused]] const _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {           \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static uint_least##_N##_t ztdc_load8_aligned_leu##_N##_SUFFIX(                                        \
	     [[maybe_unused]] const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) { \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static uint_least##_N##_t ztdc_load8_aligned_beu##_N##_SUFFIX(                                        \
	     [[maybe_unused]] const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) { \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static int_least##_N##_t ztdc_load8_les##_N##_SUFFIX(                                                 \
	     [[maybe_unused]] const _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {           \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static int_least##_N##_t ztdc_load8_bes##_N##_SUFFIX(                                                 \
	     [[maybe_unused]] const _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {           \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static int_least##_N##_t ztdc_load8_aligned_les##_N##_SUFFIX(                                         \
	     [[maybe_unused]] const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) { \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
	static int_least##_N##_t ztdc_load8_aligned_bes##_N##_SUFFIX(                                         \
	     [[maybe_unused]] const _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) { \
		explode_and_catch();                                                                             \
		return 0;                                                                                        \
	}                                                                                                     \
                                                                                                           \
	static void ztdc_store8_leu##_N##_SUFFIX([[maybe_unused]] const uint_least##_N##_t __value,           \
	     [[maybe_unused]] _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {                 \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_beu##_N##_SUFFIX([[maybe_unused]] const uint_least##_N##_t __value,           \
	     [[maybe_unused]] _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {                 \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_aligned_leu##_N##_SUFFIX([[maybe_unused]] const uint_least##_N##_t __value,   \
	     [[maybe_unused]] _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {       \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_aligned_beu##_N##_SUFFIX([[maybe_unused]] const uint_least##_N##_t __value,   \
	     [[maybe_unused]] _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {       \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_les##_N##_SUFFIX([[maybe_unused]] const int_least##_N##_t __value,            \
	     [[maybe_unused]] _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {                 \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_bes##_N##_SUFFIX([[maybe_unused]] const int_least##_N##_t __value,            \
	     [[maybe_unused]] _TYPE __ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {                 \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_aligned_les##_N##_SUFFIX([[maybe_unused]] const int_least##_N##_t __value,    \
	     [[maybe_unused]] _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {       \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	static void ztdc_store8_aligned_bes##_N##_SUFFIX([[maybe_unused]] const int_least##_N##_t __value,    \
	     [[maybe_unused]] _TYPE __unaligned_ptr[ZTD_PTR_EXTENT(_N / (sizeof(_TYPE) * CHAR_BIT))]) {       \
		explode_and_catch();                                                                             \
	}                                                                                                     \
	ztd_static_assert((((_N) % 8) == 0), "N must be a multiple of 8")

#if CHAR_BIT <= 8
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned char, 8, uc);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned char, 8, uc);
#endif
#if CHAR_BIT <= 16
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned char, 16, uc);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned char, 16, uc);
#endif
#if CHAR_BIT <= 32
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned char, 32, uc);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned char, 32, uc);
#endif
#if CHAR_BIT <= 64
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned char, 64, uc);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned char, 64, uc);
#endif

#if USHRT_MAX <= 0xFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned short, 16, us);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned short, 16, us);
#endif
#if USHRT_MAX <= 0xFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned short, 32, us);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned short, 32, us);
#endif
#if USHRT_MAX <= 0xFFFFFFFFFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned short, 64, us);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned short, 64, us);
#endif

#if UINT_MAX <= 0xFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned int, 16, ui);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned int, 16, ui);
#endif
#if UINT_MAX <= 0xFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned int, 32, ui);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned int, 32, ui);
#endif
#if UINT_MAX <= 0xFFFFFFFFFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned int, 64, ui);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned int, 64, ui);
#endif

#if ULONG_MAX <= 0xFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned long, 32, ul);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned long, 32, ul);
#endif
#if ULONG_MAX <= 0xFFFFFFFFFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned long, 64, ul);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned long, 64, ul);
#endif

#if ULLONG_MAX <= 0xFFFFFFFFFFFFFFFFu
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE(unsigned long long, 64, ull);
#else
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS_TYPE_EMPTY(unsigned long long, 64, ull);
#endif

TEMPLATE_TEST_CASE(
     "Ensure that the 8-bit load and store work properly for all array sizes and when the underlying type used to do "
     "the computation is larger than 8 bits.",
     "[bit][load.store][N-bit]", unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long) {

#define ARR_HELPER(_N, _M) ((_N) < (_M) ? 1 : (_N) / (_M))
#define ALIGNAS_HELPER(_N, _M) ((_N) < (_M) ? (_M) : (_N))

#define SECTION_CASE(_TYPE, _N, _SUFFIX)                                                                            \
	const size_t width_##_SUFFIX    = (sizeof(_TYPE) * CHAR_BIT);                                                  \
	const size_t cmp_size_##_SUFFIX = sizeof(uint_least##_N##_t);                                                  \
                                                                                                                    \
	SECTION("uint_least" #_N "_t, little (" #_SUFFIX ")") {                                                        \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const uint_least##_N##_t data                                                                        \
			     = static_cast<uint_least##_N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##_N##_t>()); \
			ztdc_store8_leu##_N##_SUFFIX(data, arr);                                                             \
			uint_least##_N##_t result = ztdc_load8_leu##_N##_SUFFIX(arr);                                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) == 0);                                 \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) != 0);                                 \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) uint_least##_N##_t arr {};      \
			const uint_least##_N##_t data                                                                        \
			     = static_cast<uint_least##_N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##_N##_t>()); \
			ztdc_store8_aligned_leu##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			uint_least##_N##_t result = ztdc_load8_aligned_leu##_N##_SUFFIX((_TYPE*)&arr);                       \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	SECTION("int_least" #_N "_t, little, negative (" #_SUFFIX ")") {                                               \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##_N##_t>());                           \
			ztdc_store8_les##_N##_SUFFIX(data, (_TYPE*)&arr);                                                    \
			int_least##_N##_t result = ztdc_load8_les##_N##_SUFFIX((_TYPE*)&arr);                                \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) == 0);                                 \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) != 0);                                 \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) int_least##_N##_t arr {};       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##_N##_t>());                           \
			ztdc_store8_aligned_les##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			int_least##_N##_t result = ztdc_load8_aligned_les##_N##_SUFFIX((_TYPE*)&arr);                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	SECTION("int_least" #_N "_t, little, positive (" #_SUFFIX ")") {                                               \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<uint_least##_N##_t>());                          \
			ztdc_store8_les##_N##_SUFFIX(data, arr);                                                             \
			int_least##_N##_t result = ztdc_load8_les##_N##_SUFFIX(arr);                                         \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) == 0);                                 \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) != 0);                                 \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) int_least##_N##_t arr {};       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<uint_least##_N##_t>());                          \
			ztdc_store8_aligned_les##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			int_least##_N##_t result = ztdc_load8_aligned_les##_N##_SUFFIX((_TYPE*)&arr);                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_LITTLE_ENDIAN) {                                                 \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	SECTION("uint_least" #_N "_t, big (" #_SUFFIX ")") {                                                           \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const uint_least##_N##_t data                                                                        \
			     = static_cast<uint_least##_N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##_N##_t>()); \
			ztdc_store8_beu##_N##_SUFFIX(data, arr);                                                             \
			uint_least##_N##_t result = ztdc_load8_beu##_N##_SUFFIX(arr);                                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) uint_least##_N##_t arr {};      \
			const uint_least##_N##_t data                                                                        \
			     = static_cast<uint_least##_N##_t>(ztd::tests::get_distinct_bit_constant<uint_least##_N##_t>()); \
			ztdc_store8_aligned_beu##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			uint_least##_N##_t result = ztdc_load8_aligned_beu##_N##_SUFFIX((_TYPE*)&arr);                       \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	SECTION("int_least" #_N "_t, big, negative (" #_SUFFIX ")") {                                                  \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##_N##_t>());                           \
			ztdc_store8_bes##_N##_SUFFIX(data, arr);                                                             \
			int_least##_N##_t result = ztdc_load8_bes##_N##_SUFFIX(arr);                                         \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) == 0);                                 \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) != 0);                                 \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) int_least##_N##_t arr {};       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_negative<int_least##_N##_t>());                           \
			ztdc_store8_aligned_bes##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			int_least##_N##_t result = ztdc_load8_aligned_bes##_N##_SUFFIX((_TYPE*)&arr);                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	SECTION("int_least" #_N "_t, big, positive (" #_SUFFIX ")") {                                                  \
		SECTION("unaligned") {                                                                                    \
			_TYPE arr[ARR_HELPER(_N, width_##_SUFFIX)] {};                                                       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<uint_least##_N##_t>());                          \
			ztdc_store8_bes##_N##_SUFFIX(data, arr);                                                             \
			int_least##_N##_t result = ztdc_load8_bes##_N##_SUFFIX(arr);                                         \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) == 0);                                 \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(arr, &data, cmp_size_##_SUFFIX) != 0);                                 \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
		SECTION("aligned") {                                                                                      \
			alignas(ALIGNAS_HELPER(alignof(_TYPE), alignof(uint_least##_N##_t))) int_least##_N##_t arr {};       \
			const int_least##_N##_t data = static_cast<int_least##_N##_t>(                                       \
			     ztd::tests::get_distinct_bit_constant_positive<uint_least##_N##_t>());                          \
			ztdc_store8_aligned_bes##_N##_SUFFIX(data, (_TYPE*)&arr);                                            \
			int_least##_N##_t result = ztdc_load8_aligned_bes##_N##_SUFFIX((_TYPE*)&arr);                        \
			if (_N > (width_##_SUFFIX)) {                                                                        \
				if (ZTDC_NATIVE_ENDIAN == ZTDC_BIG_ENDIAN) {                                                    \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) == 0);                                \
				}                                                                                               \
				else {                                                                                          \
					REQUIRE(std::memcmp(&arr, &data, cmp_size_##_SUFFIX) != 0);                                \
				}                                                                                               \
			}                                                                                                    \
			REQUIRE(result == data);                                                                             \
		}                                                                                                         \
	}                                                                                                              \
	static_assert(true, "")

#define SECTION_CASE_OF(_N)                                             \
	if constexpr (std::is_same_v<TestType, unsigned char>) {           \
		SECTION_CASE(TestType, _N, uc);                               \
	}                                                                  \
	else if constexpr (std::is_same_v<TestType, unsigned short>) {     \
		SECTION_CASE(TestType, _N, us);                               \
	}                                                                  \
	else if constexpr (std::is_same_v<TestType, unsigned int>) {       \
		SECTION_CASE(TestType, _N, ui);                               \
	}                                                                  \
	else if constexpr (std::is_same_v<TestType, unsigned long>) {      \
		SECTION_CASE(TestType, _N, ul);                               \
	}                                                                  \
	else if constexpr (std::is_same_v<TestType, unsigned long long>) { \
		SECTION_CASE(TestType, _N, ull);                              \
	}                                                                  \
	static_assert("true", "")


	if constexpr ((sizeof(TestType) * CHAR_BIT) <= 8) {
		SECTION_CASE_OF(8);
	}
	if constexpr ((sizeof(TestType) * CHAR_BIT) <= 16) {
		SECTION_CASE_OF(16);
	}
	if constexpr ((sizeof(TestType) * CHAR_BIT) <= 32) {
		SECTION_CASE_OF(32);
	}
	if constexpr ((sizeof(TestType) * CHAR_BIT) <= 64) {
		SECTION_CASE_OF(64);
	}
}
