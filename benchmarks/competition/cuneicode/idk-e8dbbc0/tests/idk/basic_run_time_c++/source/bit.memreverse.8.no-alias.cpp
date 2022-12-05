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

#include <ztd/tests/bit_constant.hpp>
#include <ztd/idk/detail/bit.memreverse.impl.h>
#include <ztd/tests/types.hpp>

#include <random>
#include <vector>
#include <limits>

static auto randomness_seed = std::random_device {}();

static void ztd_idk_basic_run_time_cxx_memreverse8_ushort(
     size_t __n, unsigned short __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(unsigned short, __n, __ptr);
}


static void ztd_idk_basic_run_time_cxx_memreverse8_uint(
     size_t __n, unsigned int __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(unsigned int, __n, __ptr);
}


static void ztd_idk_basic_run_time_cxx_memreverse8_ulong(
     size_t __n, unsigned long __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(unsigned long, __n, __ptr);
}


static void ztd_idk_basic_run_time_cxx_memreverse8_ullong(
     size_t __n, unsigned long long __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(unsigned long long, __n, __ptr);
}

#if ZTD_IS_ON(ZTD___UINT128_T)
static void ztd_idk_basic_run_time_cxx_memreverse8_uint128_t(
     size_t __n, __uint128_t __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(__uint128_t, __n, __ptr);
}
#endif

#if ZTD_IS_ON(ZTD___UINT256_T)
static void ztd_idk_basic_run_time_cxx_memreverse8_uint256_t(
     size_t __n, __uint256_t __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(__uint256_t, __n, __ptr);
}
#endif


TEMPLATE_LIST_TEST_CASE(
     "Ensure that the 8-bit memory reverse algorithm works even if they work over a different base unit that is a "
     "multiple of 8.",
     "[bit][memreverse][N-bit]", ztd::tests::unsigned_integer_types_list) {
	const TestType expected_value         = ztd::tests::get_distinct_bit_constant<TestType>();
	const TestType expected_reverse_value = ztd::tests::get_distinct_bit_constant_reverse<TestType>();

	// this one work for all types, since it's "unsigned char"-based.
	SECTION("unsigned char-based") {
		TestType value = expected_value;
		REQUIRE(value == expected_value); // quick silliness check
		ztdc_memreverse8(sizeof(value), reinterpret_cast<unsigned char*>(&value));
		REQUIRE(value == expected_reverse_value);
	}

	if constexpr ((sizeof(TestType) % sizeof(unsigned short)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("unsigned short-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_ushort(
			     sizeof(value) / sizeof(unsigned short), reinterpret_cast<unsigned short*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}

	if constexpr ((sizeof(TestType) % sizeof(unsigned int)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("unsigned int-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_uint(
			     sizeof(value) / sizeof(unsigned int), reinterpret_cast<unsigned int*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}

	if constexpr ((sizeof(TestType) % sizeof(unsigned long)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("unsigned long-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_ulong(
			     sizeof(value) / sizeof(unsigned long), reinterpret_cast<unsigned long*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}

	if constexpr ((sizeof(TestType) % sizeof(unsigned long long)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("unsigned long long-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_ullong(
			     sizeof(value) / sizeof(unsigned long long), reinterpret_cast<unsigned long long*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}

#if ZTD_IS_ON(ZTD___UINT128_T)
	if constexpr ((sizeof(TestType) % sizeof(__uint128_t)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("__uint128_t-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_uint128_t(
			     sizeof(value) / sizeof(__uint128_t), reinterpret_cast<__uint128_t*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}
#endif

#if ZTD_IS_ON(ZTD___UINT256_T)
	if constexpr ((sizeof(TestType) % sizeof(__uint256_t)) == 0) {
		// for all types which are a multiple of the size of unsigned short,
		// test the unsigned short-based type, to verify the implementation is correct
		SECTION("__uint128_t-based") {
			TestType value = expected_value;
			REQUIRE(value == expected_value); // quick silliness check
			ztd_idk_basic_run_time_cxx_memreverse8_uint256_t(
			     sizeof(value) / sizeof(__uint256_t), reinterpret_cast<__uint256_t*>(&value));
			REQUIRE(value == expected_reverse_value);
		}
	}
#endif
}
