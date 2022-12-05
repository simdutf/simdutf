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

#include <catch2/catch_all.hpp>

#include <limits>
#include <cstring>

TEMPLATE_TEST_CASE("bit intrinsics with count_ones", "[bit intrinsics][stdc_count_ones]", unsigned long long,
     unsigned long, unsigned int, unsigned short, unsigned char) {
	static constexpr TestType zeroes = static_cast<TestType>(0);
	static constexpr TestType ones   = static_cast<TestType>(std::numeric_limits<TestType>::max());
	const int zeroes_val             = ztdc_count_ones(zeroes);
	const int expected_zeroes_val    = 0;
	const int ones_val               = ztdc_count_ones(ones);
	const int expected_ones_val      = std::numeric_limits<TestType>::digits;
	REQUIRE(zeroes_val == expected_zeroes_val);
	REQUIRE(ones_val == expected_ones_val);

	SECTION("value lsb -> msb") {
		TestType val {};
		for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
			val |= static_cast<TestType>(static_cast<TestType>(1) << i);
			const int value          = ztdc_count_ones(val);
			const int expected_value = static_cast<int>(i + 1);
			REQUIRE(value == expected_value);
		}
	}
	SECTION("value msb -> lsb") {
		TestType val {};
		for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
			val |= TestType(static_cast<TestType>(1) << i);
			const int value          = ztdc_count_ones(val);
			const int expected_value = static_cast<int>(std::numeric_limits<TestType>::digits - i);
			REQUIRE(value == expected_value);
		}
	}
}

TEMPLATE_TEST_CASE("bit intrinsics with count_zeros", "[bit intrinsics][stdc_count_zeros]", unsigned long long,
     unsigned long, unsigned int, unsigned short, unsigned char) {
	static constexpr TestType zeroes = static_cast<TestType>(0);
	static constexpr TestType ones   = static_cast<TestType>(std::numeric_limits<TestType>::max());
	const int zeroes_val             = ztdc_count_zeros(zeroes);
	const int expected_zeroes_val    = std::numeric_limits<TestType>::digits;
	const int ones_val               = ztdc_count_zeros(ones);
	const int expected_ones_val      = 0;
	REQUIRE(zeroes_val == expected_zeroes_val);
	REQUIRE(ones_val == expected_ones_val);

	SECTION("value lsb -> msb") {
		TestType val {};
		for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
			val |= static_cast<TestType>(static_cast<TestType>(1) << i);
			const int value          = ztdc_count_zeros(val);
			const int expected_value = static_cast<int>(std::numeric_limits<TestType>::digits - i - 1);
			REQUIRE(value == expected_value);
		}
	}
	SECTION("value msb -> lsb") {
		TestType val {};
		for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
			val |= TestType(static_cast<TestType>(1) << i);
			const int value          = ztdc_count_ones(val);
			const int expected_value = static_cast<int>(std::numeric_limits<TestType>::digits - i);
			REQUIRE(value == expected_value);
		}
	}
}

TEMPLATE_TEST_CASE("bit intrinsics with count_(leading/trailing)_ones",
     "[bit intrinsics][stdc_count_(leading/trailing)_ones]", unsigned long long, unsigned long, unsigned int,
     unsigned short, unsigned char) {
	static constexpr TestType zeroes = static_cast<TestType>(0);
	static constexpr TestType ones   = static_cast<TestType>(std::numeric_limits<TestType>::max());

	const int trailing_ones_zeroes_val          = ztdc_count_trailing_ones(zeroes);
	const int leading_ones_zeroes_val           = ztdc_count_leading_ones(zeroes);
	const int expected_trailing_ones_zeroes_val = 0;
	const int expected_leading_ones_zeroes_val  = 0;
	const int trailing_ones_ones_val            = ztdc_count_trailing_ones(ones);
	const int leading_ones_ones_val             = ztdc_count_leading_ones(ones);
	const int expected_trailing_ones_ones_val   = std::numeric_limits<TestType>::digits;
	const int expected_leading_ones_ones_val    = std::numeric_limits<TestType>::digits;
	REQUIRE(trailing_ones_ones_val == expected_trailing_ones_ones_val);
	REQUIRE(leading_ones_ones_val == expected_leading_ones_ones_val);
	REQUIRE(trailing_ones_zeroes_val == expected_trailing_ones_zeroes_val);
	REQUIRE(leading_ones_zeroes_val == expected_leading_ones_zeroes_val);

	SECTION("val lsb -> msb") {
		SECTION("trailing_ones") {
			for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
				auto underlying_val        = static_cast<TestType>(1) << i;
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_trailing_ones(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("leading_ones") {
			for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_leading_ones(val);
				const int expected_value = (i == (std::numeric_limits<TestType>::digits - 1))
				     ? std::numeric_limits<TestType>::digits
				     : 0;
				REQUIRE(value == expected_value);
			}
		}
	}
	SECTION("val msb -> lsb") {
		SECTION("trailing_ones") {
			for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_trailing_ones(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("leading_ones") {
			for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_leading_ones(val);
				const int expected_value = (i == (std::numeric_limits<TestType>::digits - 1))
				     ? std::numeric_limits<TestType>::digits
				     : 0;
				REQUIRE(value == expected_value);
			}
		}
	}
}

TEMPLATE_TEST_CASE("bit intrinsics with count_(leading/trailing)_zeros",
     "[bit intrinsics][stdc_count_(leading/trailing)_zeros]", unsigned long long, unsigned long, unsigned int,
     unsigned short, unsigned char) {
	static constexpr TestType zeroes             = static_cast<TestType>(0);
	static constexpr TestType ones               = static_cast<TestType>(std::numeric_limits<TestType>::max());
	const int trailing_zeros_zeroes_val          = ztdc_count_trailing_zeros(zeroes);
	const int leading_zeros_zeroes_val           = ztdc_count_leading_zeros(zeroes);
	const int expected_trailing_zeros_zeroes_val = std::numeric_limits<TestType>::digits;
	const int expected_leading_zeros_zeroes_val  = std::numeric_limits<TestType>::digits;
	const int trailing_zeros_ones_val            = ztdc_count_trailing_zeros(ones);
	const int leading_zeros_ones_val             = ztdc_count_leading_zeros(ones);
	const int expected_trailing_zeros_ones_val   = 0;
	const int expected_leading_zeros_ones_val    = 0;
	REQUIRE(trailing_zeros_zeroes_val == expected_trailing_zeros_zeroes_val);
	REQUIRE(leading_zeros_zeroes_val == expected_leading_zeros_zeroes_val);
	REQUIRE(trailing_zeros_ones_val == expected_trailing_zeros_ones_val);
	REQUIRE(leading_zeros_ones_val == expected_leading_zeros_ones_val);

	SECTION("val lsb -> msb") {
		SECTION("trailing_zeros") {
			for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_trailing_zeros(val);
				const int expected_value = 0;
				REQUIRE(value == expected_value);
			}
		}
		SECTION("leading_zeros") {
			for (std::size_t i = 0; i < std::numeric_limits<TestType>::digits; ++i) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_leading_zeros(val);
				const int expected_value = static_cast<int>(std::numeric_limits<TestType>::digits - (i + 1));
				REQUIRE(value == expected_value);
			}
		}
	}
	SECTION("val msb -> lsb") {
		SECTION("trailing_zeros") {
			for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_trailing_zeros(val);
				const int expected_value = 0;
				REQUIRE(value == expected_value);
			}
		}
		SECTION("leading_zeros") {
			for (std::size_t i = std::numeric_limits<TestType>::digits; i-- > 0;) {
				auto underlying_val        = (static_cast<TestType>(1) << i);
				TestType val_previous_mask = static_cast<TestType>(underlying_val - 1);
				TestType val_mask        = static_cast<TestType>(static_cast<TestType>(1) << i) | val_previous_mask;
				TestType val             = ones & val_mask;
				const int value          = ztdc_count_leading_zeros(val);
				const int expected_value = static_cast<int>(std::numeric_limits<TestType>::digits - (i + 1));
				REQUIRE(value == expected_value);
			}
		}
	}
}

TEMPLATE_TEST_CASE("bit_operations with first_(leading/trailing)_one", "[bit_operations][first_(leading/trailing)_one]",
     unsigned long long, unsigned long, unsigned int, unsigned short, unsigned char) {
	static constexpr TestType zeroes = static_cast<TestType>(0);
	static constexpr TestType ones   = static_cast<TestType>(std::numeric_limits<TestType>::max());

	const int first_trailing_one_zeroes_val          = ztdc_first_trailing_one(zeroes);
	const int first_leading_one_zeroes_val           = ztdc_first_leading_one(zeroes);
	const int expected_first_trailing_one_zeroes_val = 0;
	const int expected_first_leading_one_zeroes_val  = 0;
	const int first_trailing_one_ones_val            = ztdc_first_trailing_one(ones);
	const int first_leading_one_ones_val             = ztdc_first_leading_one(ones);
	const int expected_first_trailing_one_ones_val   = 1;
	const int expected_first_leading_one_ones_val    = 1;
	REQUIRE(first_trailing_one_ones_val == expected_first_trailing_one_ones_val);
	REQUIRE(first_leading_one_ones_val == expected_first_leading_one_ones_val);
	REQUIRE(first_trailing_one_zeroes_val == expected_first_trailing_one_zeroes_val);
	REQUIRE(first_leading_one_zeroes_val == expected_first_leading_one_zeroes_val);

	SECTION("val lsb -> msb") {
		SECTION("first_trailing_one") {
			for (std::size_t i = 0; i < (sizeof(TestType) * CHAR_BIT); ++i) {
				TestType val             = static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_trailing_one(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("first_leading_one") {
			for (std::size_t i = 0; i < (sizeof(TestType) * CHAR_BIT); ++i) {
				TestType val             = static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_leading_one(val);
				const int expected_value = static_cast<int>((sizeof(TestType) * CHAR_BIT) - i);
				REQUIRE(value == expected_value);
			}
		}
	}
	SECTION("val msb -> lsb") {
		SECTION("first_trailing_one") {
			for (std::size_t i = (sizeof(TestType) * CHAR_BIT); i-- > 0;) {
				TestType val             = static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_trailing_one(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("first_leading_one") {
			for (std::size_t i = (sizeof(TestType) * CHAR_BIT); i-- > 0;) {
				TestType val             = static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_leading_one(val);
				const int expected_value = static_cast<int>((sizeof(TestType) * CHAR_BIT) - i);
				REQUIRE(value == expected_value);
			}
		}
	}
}

TEMPLATE_TEST_CASE("bit_operations with first_(leading/trailing)_zero",
     "[bit_operations][first_(leading/trailing)_zero]", unsigned long long, unsigned long, unsigned int, unsigned short,
     unsigned char) {
	static constexpr TestType zeroes                  = static_cast<TestType>(0);
	static constexpr TestType ones                    = static_cast<TestType>(std::numeric_limits<TestType>::max());
	const int first_trailing_zero_zeroes_val          = ztdc_first_trailing_zero(zeroes);
	const int first_leading_zero_zeroes_val           = ztdc_first_leading_zero(zeroes);
	const int expected_first_trailing_zero_zeroes_val = 1;
	const int expected_first_leading_zero_zeroes_val  = 1;
	const int first_trailing_zero_ones_val            = ztdc_first_trailing_zero(ones);
	const int first_leading_zero_ones_val             = ztdc_first_leading_zero(ones);
	const int expected_first_trailing_zero_ones_val   = 0;
	const int expected_first_leading_zero_ones_val    = 0;
	REQUIRE(first_trailing_zero_zeroes_val == expected_first_trailing_zero_zeroes_val);
	REQUIRE(first_leading_zero_zeroes_val == expected_first_leading_zero_zeroes_val);
	REQUIRE(first_trailing_zero_ones_val == expected_first_trailing_zero_ones_val);
	REQUIRE(first_leading_zero_ones_val == expected_first_leading_zero_ones_val);

	SECTION("val lsb -> msb") {
		SECTION("first_trailing_zero") {
			for (std::size_t i = 0; i < (sizeof(TestType) * CHAR_BIT); ++i) {
				TestType val             = ones & ~static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_trailing_zero(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("first_leading_zero") {
			for (std::size_t i = 0; i < (sizeof(TestType) * CHAR_BIT); ++i) {
				TestType val             = ones & ~static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_leading_zero(val);
				const int expected_value = static_cast<int>((sizeof(TestType) * CHAR_BIT) - i);
				REQUIRE(value == expected_value);
			}
		}
	}
	SECTION("val msb -> lsb") {
		SECTION("first_trailing_zero") {
			for (std::size_t i = (sizeof(TestType) * CHAR_BIT); i-- > 0;) {
				TestType val             = ones & ~static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_trailing_zero(val);
				const int expected_value = static_cast<int>(i + 1);
				REQUIRE(value == expected_value);
			}
		}
		SECTION("first_leading_zero") {
			for (std::size_t i = (sizeof(TestType) * CHAR_BIT); i-- > 0;) {
				TestType val             = ones & ~static_cast<TestType>(static_cast<TestType>(1) << i);
				const int value          = ztdc_first_leading_zero(val);
				const int expected_value = static_cast<int>((sizeof(TestType) * CHAR_BIT) - i);
				REQUIRE(value == expected_value);
			}
		}
	}
}
