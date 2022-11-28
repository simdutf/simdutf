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

#include <ztd/tests/basic_unicode_strings.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <iterator>
#include <algorithm>
#include <cstring>

namespace {
	template <typename Char, typename InputChar>
	void compare_roundtrip_X(const InputChar* initial_input, size_t initial_input_size) {
		using output_char       = Char;
		using input_char        = InputChar;
		const input_char* input = initial_input;
		size_t input_size       = initial_input_size;
		output_char output_buffer[64];
		output_char* initial_output      = output_buffer;
		output_char* output              = initial_output;
		const size_t initial_output_size = 64;
		size_t output_size               = initial_output_size;
		const cnc_mcerror output_err = cnc_cxntocyn(&output_size, &output, &input_size, &input);
		const size_t input_read      = initial_input_size - input_size;
		const size_t output_written  = initial_output_size - output_size;
		REQUIRE(output_err == CNC_MCERROR_OK);
		REQUIRE(input_read == initial_input_size);
		REQUIRE((output - initial_output) >= (static_cast<std::ptrdiff_t>(0)));
		REQUIRE((initial_input + input_read) == input);
		REQUIRE((initial_input_size - input_read) == input_size);
		REQUIRE((initial_output + output_written) == output);
		REQUIRE((initial_output_size - output_written) == output_size);

		{
			// validate input
			const input_char* initial_validate_input = initial_input;
			const input_char* validate_input         = initial_validate_input;
			size_t validate_input_size               = initial_input_size;
			const cnc_mcerror validate_input_err     = cnc_cxntocyn(nullptr,
			         static_cast<output_char**>(nullptr), &validate_input_size, &validate_input);
			REQUIRE(validate_input_err == CNC_MCERROR_OK);
			REQUIRE(
			     (validate_input - initial_input) == (static_cast<std::ptrdiff_t>(input_read)));
		}
		{
			// count input
			const input_char* initial_count_input = initial_input;
			const input_char* count_input         = initial_count_input;
			size_t count_input_size               = initial_input_size;
			size_t count_output_size              = initial_output_size;
			const cnc_mcerror count_input_err     = cnc_cxntocyn(&count_output_size,
			         static_cast<output_char**>(nullptr), &count_input_size, &count_input);
			const size_t count_output_written     = initial_output_size - count_output_size;
			REQUIRE(count_input_err == CNC_MCERROR_OK);
			REQUIRE((count_input - initial_count_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
			REQUIRE(count_output_written == output_written);
		}
		{
			// unbounded output
			const input_char* initial_unbounded_input = initial_input;
			const input_char* unbounded_input         = initial_unbounded_input;
			size_t unbounded_input_size               = initial_input_size;
			output_char unbounded_output_buffer[64];
			output_char* initial_unbounded_output = unbounded_output_buffer;
			output_char* unbounded_output         = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxntocyn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(output_written)));
			REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
			REQUIRE(std::equal(initial_unbounded_output + 0,
			     initial_unbounded_output + output_written, initial_output + 0,
			     initial_output + output_written));
		}

		const output_char* initial_intermediate = output_buffer;
		const output_char* intermediate         = initial_intermediate;
		const size_t initial_intermediate_size  = output_written;
		size_t intermediate_size                = initial_intermediate_size;
		input_char input_output_buffer[64];
		input_char* initial_input_output       = input_output_buffer;
		input_char* input_output               = initial_input_output;
		const size_t initial_input_output_size = 64;
		size_t input_output_size               = initial_input_output_size;
		const cnc_mcerror input_output_err
		     = cnc_cxntocyn(&input_output_size, &input_output, &intermediate_size, &intermediate);
		const size_t intermediate_read    = initial_intermediate_size - intermediate_size;
		const size_t input_output_written = initial_input_output_size - input_output_size;
		if (input_output_err != CNC_MCERROR_OK)
			REQUIRE(input_output_err == CNC_MCERROR_OK);
		REQUIRE(intermediate_size == 0);
		REQUIRE((intermediate - initial_intermediate) >= (static_cast<std::ptrdiff_t>(0)));
		REQUIRE(intermediate_read == output_written);
		REQUIRE(input_output_written == input_read);
		REQUIRE((initial_intermediate + intermediate_read) == intermediate);
		REQUIRE((initial_intermediate_size - intermediate_read) == intermediate_size);
		REQUIRE((initial_input_output + input_output_written) == input_output);
		REQUIRE((initial_input_output_size - input_output_written) == input_output_size);
		REQUIRE(std::equal(initial_input + 0, initial_input + input_read,
		     initial_input_output + 0, initial_input_output + input_output_written));

		{
			// validate input
			const output_char* initial_validate_input = initial_intermediate;
			const output_char* validate_input         = initial_validate_input;
			size_t validate_input_size                = initial_intermediate_size;
			const cnc_mcerror validate_input_err      = cnc_cxntocyn(nullptr,
			          static_cast<input_char**>(nullptr), &validate_input_size, &validate_input);
			REQUIRE(validate_input_err == CNC_MCERROR_OK);
			REQUIRE((validate_input - initial_validate_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
		}
		{
			// count input
			const output_char* initial_count_input = initial_intermediate;
			const output_char* count_input         = initial_count_input;
			size_t count_input_size                = initial_intermediate_size;
			size_t count_output_size               = initial_output_size;
			const cnc_mcerror count_input_err      = cnc_cxntocyn(&count_output_size,
			          static_cast<input_char**>(nullptr), &count_input_size, &count_input);
			const size_t count_output_written      = initial_output_size - count_output_size;
			REQUIRE(count_input_err == CNC_MCERROR_OK);
			REQUIRE((count_input - initial_count_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE(count_output_written == input_output_written);
		}
		{
			// unbounded output
			const output_char* initial_unbounded_input = initial_intermediate;
			const output_char* unbounded_input         = initial_unbounded_input;
			size_t unbounded_input_size                = initial_input_output_size;
			input_char unbounded_output_buffer[64];
			input_char* initial_unbounded_output  = unbounded_output_buffer;
			input_char* unbounded_output          = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxntocyn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(input_output_written)));
			REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
			REQUIRE(std::equal(initial_unbounded_output + 0,
			     initial_unbounded_output + input_output_written, initial_input_output + 0,
			     initial_input_output + input_output_written));
		}
	}

	template <typename Char, bool IsUnicode = true>
	void compare_roundtrip_utf32() {
		for (ztd_char32_t expected_c = 0; expected_c < __ztd_idk_detail_last_unicode_code_point;
		     ++expected_c) {
			if constexpr (IsUnicode) {
				if (__ztd_idk_detail_is_surrogate(expected_c)) {
					continue;
				}
			}
			else {
				if (ztd::tests::u32_basic_source_character_set.find(expected_c)
				     == std::u32string_view::npos) {
					continue;
				}
			}
			using output_char = Char;
			using input_char  = ztd_char32_t;
			input_char input_buffer[64] { expected_c };
			const input_char* initial_input = input_buffer;
			const input_char* input         = initial_input;
			const size_t initial_input_size = 64;
			size_t input_size               = initial_input_size;
			output_char output_buffer[64];
			output_char* initial_output      = output_buffer;
			output_char* output              = initial_output;
			const size_t initial_output_size = 64;
			size_t output_size               = initial_output_size;
			REQUIRE(input_buffer[0] == expected_c);

			const cnc_mcerror output_err
			     = cnc_cxntocyn(&output_size, &output, &input_size, &input);
			const size_t input_read     = initial_input_size - input_size;
			const size_t output_written = initial_output_size - output_size;
			REQUIRE(output_err == CNC_MCERROR_OK);
			REQUIRE((input - initial_input) == (static_cast<std::ptrdiff_t>(1)));
			REQUIRE(input_read == 1);
			REQUIRE((output - initial_output) >= (static_cast<std::ptrdiff_t>(0)));

			{
				// validate input
				const input_char* initial_validate_input = initial_input;
				const input_char* validate_input         = initial_validate_input;
				size_t validate_input_size               = initial_input_size;
				const cnc_mcerror validate_input_err
				     = cnc_cxntocyn(nullptr, static_cast<output_char**>(nullptr),
				          &validate_input_size, &validate_input);
				REQUIRE(validate_input_err == CNC_MCERROR_OK);
				REQUIRE((validate_input - initial_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
			}
			{
				// count input
				const input_char* initial_count_input = initial_input;
				const input_char* count_input         = initial_count_input;
				size_t count_input_size               = initial_input_size;
				size_t count_output_size              = initial_output_size;
				const cnc_mcerror count_input_err     = cnc_cxntocyn(&count_output_size,
				         static_cast<output_char**>(nullptr), &count_input_size, &count_input);
				const size_t count_output_written     = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE(count_output_written == output_written);
			}
			{
				// unbounded output
				const input_char* initial_unbounded_input = initial_input;
				const input_char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size               = initial_input_size;
				output_char unbounded_output_buffer[64];
				output_char* initial_unbounded_output = unbounded_output_buffer;
				output_char* unbounded_output         = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err = cnc_cxntocyn(
				     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(output_written)));
				REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + output_written, initial_output + 0,
				     initial_output + output_written));
			}

			compare_roundtrip_X<ztd_char_t>(output_buffer, output_written);
			compare_roundtrip_X<ztd_wchar_t>(output_buffer, output_written);
			compare_roundtrip_X<ztd_char8_t>(output_buffer, output_written);
			compare_roundtrip_X<ztd_char16_t>(output_buffer, output_written);
			compare_roundtrip_X<ztd_char32_t>(output_buffer, output_written);

			const output_char* initial_intermediate = output_buffer;
			const output_char* intermediate         = initial_intermediate;
			const size_t initial_intermediate_size  = output_written;
			size_t intermediate_size                = initial_intermediate_size;
			input_char input_output_buffer[64];
			input_char* initial_input_output       = input_output_buffer;
			input_char* input_output               = initial_input_output;
			const size_t initial_input_output_size = 64;
			size_t input_output_size               = initial_input_output_size;
			const cnc_mcerror input_output_err     = cnc_cxntocyn(
                    &input_output_size, &input_output, &intermediate_size, &intermediate);
			const size_t intermediate_read    = initial_intermediate_size - intermediate_size;
			const size_t input_output_written = initial_input_output_size - input_output_size;
			REQUIRE(input_output_err == CNC_MCERROR_OK);
			REQUIRE((intermediate - initial_intermediate)
			     == (static_cast<std::ptrdiff_t>(output_written)));
			REQUIRE(intermediate_size == 0);
			REQUIRE(intermediate_read == output_written);
			REQUIRE((input_output - initial_input_output) == (static_cast<std::ptrdiff_t>(1)));
			REQUIRE(input_output_written == 1);
			REQUIRE(input_output_written == input_read);
			REQUIRE((initial_intermediate + intermediate_read) == intermediate);
			REQUIRE((initial_intermediate_size - intermediate_read) == intermediate_size);
			REQUIRE((initial_input_output + input_output_written) == input_output);
			REQUIRE((initial_input_output_size - input_output_written) == input_output_size);
			REQUIRE(input_output_buffer[0] == expected_c);

			{
				// validate input
				const output_char* initial_validate_input = initial_intermediate;
				const output_char* validate_input         = initial_validate_input;
				size_t validate_input_size                = initial_intermediate_size;
				const cnc_mcerror validate_input_err      = cnc_cxntocyn(nullptr,
				          static_cast<input_char**>(nullptr), &validate_input_size, &validate_input);
				REQUIRE(validate_input_err == CNC_MCERROR_OK);
				REQUIRE((validate_input - initial_validate_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			}
			{
				// count input
				const output_char* initial_count_input = initial_intermediate;
				const output_char* count_input         = initial_count_input;
				size_t count_input_size                = initial_intermediate_size;
				size_t count_output_size               = initial_output_size;
				const cnc_mcerror count_input_err      = cnc_cxntocyn(&count_output_size,
				          static_cast<input_char**>(nullptr), &count_input_size, &count_input);
				const size_t count_output_written = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE(count_output_written == input_output_written);
			}
			{
				// unbounded output
				const output_char* initial_unbounded_input = initial_intermediate;
				const output_char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size                = initial_input_output_size;
				input_char unbounded_output_buffer[64];
				input_char* initial_unbounded_output  = unbounded_output_buffer;
				input_char* unbounded_output          = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err = cnc_cxntocyn(
				     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(input_output_written)));
				REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + input_output_written, initial_input_output + 0,
				     initial_input_output + input_output_written));
			}
		}
	}
} // namespace

TEST_CASE(
     "check single conversion from one of the typical typed encodings to UTF-32 and back to the "
     "typed encoding, using direct functions",
     "[cuneicode][direct][roundtrip-c32][single]") {
	SECTION("mc") {
		compare_roundtrip_utf32<ztd_char_t, false>();
		if (cnc_is_execution_encoding_unicode()) {
			compare_roundtrip_utf32<ztd_char_t, true>();
		}
	}
	SECTION("mwc") {
		compare_roundtrip_utf32<ztd_wchar_t, false>();
		if (cnc_is_wide_execution_encoding_unicode()) {
			compare_roundtrip_utf32<ztd_wchar_t, true>();
		}
	}
	SECTION("c8") {
		compare_roundtrip_utf32<ztd_char8_t, false>();
		compare_roundtrip_utf32<ztd_char8_t, true>();
	}
	SECTION("c16") {
		compare_roundtrip_utf32<ztd_char16_t, false>();
		compare_roundtrip_utf32<ztd_char16_t, true>();
	}
	SECTION("c32") {
		compare_roundtrip_utf32<ztd_char32_t, false>();
		compare_roundtrip_utf32<ztd_char32_t, true>();
	}
}
