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

#include <random>
#include <vector>
#include <limits>

template <typename T>
static auto select_memreverseN_function() noexcept {
	constexpr std::size_t N = sizeof(T) * CHAR_BIT;
#ifdef UINT8_MAX
	if constexpr (N == 8) {
		return &ztdc_memreverse8u8;
	}
	else
#endif
#ifdef UINT16_MAX
	     if constexpr (N == 16) {
		return &ztdc_memreverse8u16;
	}
	else
#endif
#ifdef UINT24_MAX
	     if constexpr (N == 24) {
		return &ztdc_memreverse8u24;
	}
	else
#endif
#ifdef UINT32_MAX
	     if constexpr (N == 32) {
		return &ztdc_memreverse8u32;
	}
	else
#endif
#ifdef UINT40_MAX
	     if constexpr (N == 40) {
		return &ztdc_memreverse8u40;
	}
	else
#endif
#ifdef UINT48_MAX
	     if constexpr (N == 48) {
		return &ztdc_memreverse8u48;
	}
	else
#endif
#ifdef UINT56_MAX
	     if constexpr (N == 56) {
		return &ztdc_memreverse8u56;
	}
	else
#endif
#ifdef UINT64_MAX
	     if constexpr (N == 64) {
		return &ztdc_memreverse8u64;
	}
	else
#endif
	{
		static_assert(ztd::always_false_v<T>, "unusable bit size for the given type");
	}
}

TEMPLATE_TEST_CASE("Ensure that hte 8-bit memory reverse algorithm works on appropriately-sized variables.",
     "[bit][memreverse][8-bit][small]", unsigned char, unsigned short, unsigned int, unsigned long,
     unsigned long long) {
	const TestType expected_value         = ztd::tests::get_distinct_bit_constant<TestType>();
	const TestType expected_reverse_value = ztd::tests::get_distinct_bit_constant_reverse<TestType>();

	SECTION("raw memory reverse") {
		TestType value = expected_value;
		REQUIRE(value == expected_value); // quick silliness check
		ztdc_memreverse8(sizeof(value), reinterpret_cast<unsigned char*>(&value));
		REQUIRE(value == expected_reverse_value);
	}
	SECTION("value-based memory reverse") {
		TestType value = expected_value;
		REQUIRE(value == expected_value);
		auto reversing_function = select_memreverseN_function<TestType>();
		TestType reverse_value  = reversing_function(value);
		REQUIRE(value == expected_value);
		REQUIRE(reverse_value == expected_reverse_value);
	}
}

TEST_CASE("Ensure that hte 8-bit memory reverse algorithm works on large memory region.",
     "[bit][memreverse][8-bit][large]") {

#if CHAR_BIT == 8
	SECTION("with canonical implementation") {
		const auto randomness_seed = std::random_device {}();
		std::mt19937 rng(randomness_seed);
		std::uniform_int_distribution<std::mt19937::result_type> dist(0, std::numeric_limits<unsigned char>::max());
		const std::vector<unsigned char> data = [&rng, &dist]() {
			std::vector<unsigned char> data_init(50000);
			for (unsigned char& val : data_init) {
				val = static_cast<unsigned char>(dist(rng));
			}
			data_init.shrink_to_fit();
			return data_init;
		}();
		const std::vector<unsigned char> reverse_data(data.rbegin(), data.rend());

		// copy to neutral vector that can be manipulated
		std::vector<unsigned char> target = data;

		REQUIRE(target == data);
		ztdc_memreverse8(target.size(), target.data());
		REQUIRE(target == reverse_data);
	}
#else
	// empty test for now
	REQUIRE(true);
#endif
}
