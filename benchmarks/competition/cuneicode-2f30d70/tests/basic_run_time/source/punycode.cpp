// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#include <catch2/catch_all.hpp>

#include <ztd/cuneicode.h>

#include <ztd/idk/size.hpp>

#include <iterator>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <tuple>

namespace {

	template <typename Source, typename Expected, typename ExpectedIdna>
	void compare_bulk_unicode_to_punycode_roundtrip(
	     const Source& source, const Expected& expected, const ExpectedIdna& expected_idna) {
		{
			char destination[1024]             = {};
			const std::size_t source_size      = source.size();
			const std::size_t destination_size = ztd_c_array_size(destination);
			const char32_t* input              = source.data();
			std::size_t input_size             = source_size;
			char* output                       = destination;
			std::size_t output_size            = destination_size;
			cnc_pny_encode_state_t state       = {};
			REQUIRE(state.__is_initialized == 0);
			cnc_mcerror err
			     = cnc_c32snrtomcsn_punycode(&output_size, &output, &input_size, &input, &state);
			REQUIRE(err == CNC_MCERROR_OK);
			std::size_t used_size = destination_size - output_size;
			const bool is_equal   = std::equal(
                    expected.cbegin(), expected.cend(), destination + 0, destination + used_size);
			REQUIRE(is_equal);
			REQUIRE(state.__is_initialized == 0);
		}
		{
			char destination[1024]             = {};
			const std::size_t source_size      = source.size();
			const std::size_t destination_size = ztd_c_array_size(destination);
			const char32_t* input              = source.data();
			std::size_t input_size             = source_size;
			char* output                       = destination;
			std::size_t output_size            = destination_size;
			cnc_pny_encode_state_t state       = {};
			REQUIRE(state.__is_initialized == 0);
			cnc_mcerror err = cnc_c32snrtomcsn_punycode_idna(
			     &output_size, &output, &input_size, &input, &state);
			REQUIRE(err == CNC_MCERROR_OK);
			std::size_t used_size = destination_size - output_size;
			const bool is_equal   = std::equal(expected_idna.cbegin(), expected_idna.cend(),
			       destination + 0, destination + used_size);
			REQUIRE(is_equal);
			REQUIRE(state.__is_initialized == 0);
		}
		{
			ztd_char32_t destination[1024]     = {};
			const std::size_t source_size      = expected_idna.size();
			const std::size_t destination_size = ztd_c_array_size(destination);
			const char* input                  = expected_idna.data();
			std::size_t input_size             = source_size;
			ztd_char32_t* output               = destination;
			std::size_t output_size            = destination_size;
			cnc_pny_decode_state_t state       = {};
			REQUIRE(state.__is_initialized == 0);
			cnc_mcerror err = cnc_mcsnrtoc32sn_punycode_idna(
			     &output_size, &output, &input_size, &input, &state);
			REQUIRE(err == CNC_MCERROR_OK);
			std::size_t used_size = destination_size - output_size;
			const bool is_equal   = std::equal(
                    source.cbegin(), source.cend(), destination + 0, destination + used_size);
			REQUIRE(is_equal);
			REQUIRE(state.__is_initialized == 0);
		}
		{
			ztd_char32_t destination[1024]     = {};
			const std::size_t source_size      = expected.size();
			const std::size_t destination_size = ztd_c_array_size(destination);
			const char* input                  = expected.data();
			std::size_t input_size             = source_size;
			ztd_char32_t* output               = destination;
			std::size_t output_size            = destination_size;
			cnc_pny_decode_state_t state       = {};
			REQUIRE(state.__is_initialized == 0);
			cnc_mcerror err
			     = cnc_mcsnrtoc32sn_punycode(&output_size, &output, &input_size, &input, &state);
			REQUIRE(err == CNC_MCERROR_OK);
			std::size_t used_size = destination_size - output_size;
			const bool is_equal   = std::equal(
                    source.cbegin(), source.cend(), destination + 0, destination + used_size);
			REQUIRE(is_equal);
			REQUIRE(state.__is_initialized == 0);
		}
	}

} // namespace

TEST_CASE(
     "check bulk conversion from punycode/punycode_idna to UTF-32 and back to "
     "punycode/punycode_idna",
     "[cuneicode][punycode][roundtrip-c32][bulk]") {
	const std::tuple<std::u32string_view, std::string_view, std::string_view>
	     source_and_expected_bundles[] = {
		     { U"", "", "" },
		     { U"meow", "meow-", "meow" },
		     { U"bücher", "bcher-kva", "xn--bcher-kva" },
		     { U"meow-kv?", "meow-kv?-", "meow-kv?" },
		     { U"bcher-kba", "bcher-kba-", "bcher-kba" },
		     { U"αβγ", "-mxacd", "xn---mxacd" },
		     { U"München", "Mnchen-3ya", "xn--Mnchen-3ya" },
		     { U"例", "-fsq", "xn---fsq" },
		     { U"α", "-mxa", "xn---mxa" },
		     { U"а", "-80a", "xn---80a" },
		     { U"Lloyd-Atkinson", "Lloyd-Atkinson-", "Lloyd-Atkinson" },
		     { U"-> $1.00 <-", "-> $1.00 <--", "-> $1.00 <-" },
		     { U"-", "--", "-" },
		     { U"--", "---", "--" },
		     { U"ü", "-tda", "xn---tda" },
		     { U"München-Ost", "Mnchen-Ost-9db", "xn--Mnchen-Ost-9db" },
		     { U"Bahnhof München-Ost", "Bahnhof Mnchen-Ost-u6b", "xn--Bahnhof Mnchen-Ost-u6b" },
		     { U"ยจฆฟคฏข", "-22cdfh1b8fsa", "xn---22cdfh1b8fsa" },
		     { U"도메인", "-hq1bm8jm9l", "xn---hq1bm8jm9l" },
		     { U"ドメイン名例", "-eckwd4c7cu47r2wf", "xn---eckwd4c7cu47r2wf" },
		     { U"MajiでKoiする5秒前", "MajiKoi5-783gue6qz075azm5e",
		          "xn--MajiKoi5-783gue6qz075azm5e" },
		     { U"「bücher」", "bcher-kva8445foa", "xn--bcher-kva8445foa" },
		     { U"правда", "-80aafi6cg", "xn---80aafi6cg" },
		     { U"abæcdöef", "abcdef-qua4k", "xn--abcdef-qua4k" },
	     };

	for (const auto& source_and_expected : source_and_expected_bundles) {
		const std::u32string_view& source     = std::get<0>(source_and_expected);
		const std::string_view& expected      = std::get<1>(source_and_expected);
		const std::string_view& expected_idna = std::get<2>(source_and_expected);
		compare_bulk_unicode_to_punycode_roundtrip(source, expected, expected_idna);
	}
}
