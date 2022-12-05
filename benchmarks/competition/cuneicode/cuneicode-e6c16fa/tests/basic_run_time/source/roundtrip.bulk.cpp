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

#include <ztd/cuneicode/shared/unicode_range.hpp>

#include <iterator>
#include <algorithm>
#include <cstring>

namespace {
	template <typename Char, typename InputChar>
	void compare_bulk_roundtrip_X(const InputChar* initial_input, size_t initial_input_size) {
		using output_char       = Char;
		using input_char        = InputChar;
		const input_char* input = initial_input;
		size_t input_size       = initial_input_size;
		std::vector<output_char> output_buffer(initial_input_size * 5);
		output_char* initial_output      = output_buffer.data();
		output_char* output              = initial_output;
		const size_t initial_output_size = output_buffer.size();
		size_t output_size               = initial_output_size;
		const cnc_mcerror output_err = cnc_cxsntocysn(&output_size, &output, &input_size, &input);
		const size_t input_read      = initial_input_size - input_size;
		const size_t output_written  = initial_output_size - output_size;
		REQUIRE(output_err == CNC_MCERROR_OK);
		REQUIRE(input_read == initial_input_size);
		REQUIRE((initial_input + input_read) == input);
		REQUIRE((initial_input_size - input_read) == input_size);
		REQUIRE((initial_output + output_written) == output);
		REQUIRE((initial_output_size - output_written) == output_size);
		output_buffer.resize(output_written);


		{
			// validate input
			const input_char* initial_validate_input = initial_input;
			const input_char* validate_input         = initial_validate_input;
			size_t validate_input_size               = initial_input_size;
			const cnc_mcerror validate_input_err     = cnc_cxsntocysn(nullptr,
			         static_cast<output_char**>(nullptr), &validate_input_size, &validate_input);
			REQUIRE(validate_input_err == CNC_MCERROR_OK);
			REQUIRE((validate_input - initial_validate_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
		}
		{
			// count input
			const input_char* initial_count_input = initial_input;
			const input_char* count_input         = initial_count_input;
			size_t count_input_size               = initial_input_size;
			size_t count_output_size              = initial_output_size;
			const cnc_mcerror count_input_err     = cnc_cxsntocysn(&count_output_size,
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
			std::vector<output_char> unbounded_output_buffer(output_written);
			output_char* initial_unbounded_output = unbounded_output_buffer.data();
			output_char* unbounded_output         = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxsntocysn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(output_written)));
			REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
			REQUIRE(std::equal(std::cbegin(unbounded_output_buffer),
			     std::cend(unbounded_output_buffer), std::cbegin(output_buffer),
			     std::cend(output_buffer)));
		}

		const output_char* initial_intermediate = output_buffer.data();
		const output_char* intermediate         = initial_intermediate;
		const size_t initial_intermediate_size  = output_written;
		size_t intermediate_size                = initial_intermediate_size;
		std::vector<input_char> input_output_buffer(initial_input_size);
		input_char* initial_input_output       = input_output_buffer.data();
		input_char* input_output               = initial_input_output;
		const size_t initial_input_output_size = input_output_buffer.size();
		size_t input_output_size               = initial_input_output_size;
		const cnc_mcerror input_output_err     = cnc_cxsntocysn(
               &input_output_size, &input_output, &intermediate_size, &intermediate);
		const size_t intermediate_read    = initial_intermediate_size - intermediate_size;
		const size_t input_output_written = initial_input_output_size - input_output_size;
		REQUIRE(input_output_err == CNC_MCERROR_OK);
		REQUIRE(intermediate_size == 0);
		REQUIRE(intermediate_read == output_written);
		REQUIRE(input_output_written == input_read);
		REQUIRE((initial_intermediate + intermediate_read) == intermediate);
		REQUIRE((initial_intermediate_size - intermediate_read) == intermediate_size);
		REQUIRE((initial_input_output + input_output_written) == input_output);
		REQUIRE((initial_input_output_size - input_output_written) == input_output_size);
		input_output_buffer.resize(input_output_written);
		REQUIRE(std::equal(initial_input + 0, initial_input + initial_input_size,
		     std::cbegin(input_output_buffer), std::cend(input_output_buffer)));
		{
			// validate intermediate
			const output_char* initial_validate_input = initial_intermediate;
			const output_char* validate_input         = initial_validate_input;
			size_t validate_input_size                = initial_intermediate_size;
			const cnc_mcerror validate_input_err      = cnc_cxsntocysn(nullptr,
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
			size_t count_output_size               = initial_input_output_size;
			const cnc_mcerror count_input_err      = cnc_cxsntocysn(&count_output_size,
			          static_cast<input_char**>(nullptr), &count_input_size, &count_input);
			const size_t count_output_written = initial_input_output_size - count_output_size;
			REQUIRE(count_input_err == CNC_MCERROR_OK);
			REQUIRE((count_input - initial_count_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE(count_output_written == input_output_written);
		}
		{
			// unbounded output
			const output_char* initial_unbounded_input = initial_intermediate;
			const output_char* unbounded_input         = initial_unbounded_input;
			size_t unbounded_input_size                = initial_intermediate_size;
			std::vector<input_char> unbounded_output_buffer(input_output_written);
			input_char* initial_unbounded_output  = unbounded_output_buffer.data();
			input_char* unbounded_output          = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxsntocysn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(input_output_written)));
			REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
			REQUIRE(std::equal(std::cbegin(unbounded_output_buffer),
			     std::cend(unbounded_output_buffer), std::cbegin(input_output_buffer),
			     std::cend(input_output_buffer)));
		}
	}

	template <typename Char, typename Input>
	void compare_bulk_roundtrip_utf32(Input& input_buffer) {
		using output_char = Char;
		using input_char  = std::remove_cv_t<std::remove_reference_t<decltype(input_buffer[0])>>;
		const auto* initial_input       = input_buffer.data();
		const auto* input               = initial_input;
		const size_t initial_input_size = input_buffer.size();
		size_t input_size               = initial_input_size;
		std::vector<output_char> output_buffer(initial_input_size * 5);
		output_char* output              = output_buffer.data();
		const size_t initial_output_size = output_buffer.size();
		size_t output_size               = initial_output_size;

		const cnc_mcerror output_err = cnc_cxsntocysn(&output_size, &output, &input_size, &input);
		const size_t input_read      = initial_input_size - input_size;
		const size_t output_written  = initial_output_size - output_size;
		REQUIRE(output_err == CNC_MCERROR_OK);
		REQUIRE(input_read == initial_input_size);
		REQUIRE(input_size == 0);
		output_buffer.resize(output_written);

		{
			// validate input
			const input_char* initial_validate_input = initial_input;
			const input_char* validate_input         = initial_validate_input;
			size_t validate_input_size               = initial_input_size;
			const cnc_mcerror validate_input_err     = cnc_cxsntocysn(nullptr,
			         static_cast<output_char**>(nullptr), &validate_input_size, &validate_input);
			REQUIRE(validate_input_err == CNC_MCERROR_OK);
			REQUIRE((validate_input - initial_validate_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
		}
		{
			// count input
			const input_char* initial_count_input = initial_input;
			const input_char* count_input         = initial_count_input;
			size_t count_input_size               = initial_input_size;
			size_t count_output_size              = initial_output_size;
			const cnc_mcerror count_input_err     = cnc_cxsntocysn(&count_output_size,
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
			std::vector<output_char> unbounded_output_buffer(output_written);
			output_char* initial_unbounded_output = unbounded_output_buffer.data();
			output_char* unbounded_output         = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxsntocysn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(input_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(output_written)));
			REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
			REQUIRE(std::equal(std::cbegin(unbounded_output_buffer),
			     std::cend(unbounded_output_buffer), std::cbegin(output_buffer),
			     std::cend(output_buffer)));
		}

		compare_bulk_roundtrip_X<ztd_char_t>(output_buffer.data(), output_buffer.size());
		compare_bulk_roundtrip_X<ztd_wchar_t>(output_buffer.data(), output_buffer.size());
		compare_bulk_roundtrip_X<ztd_char8_t>(output_buffer.data(), output_buffer.size());
		compare_bulk_roundtrip_X<ztd_char16_t>(output_buffer.data(), output_buffer.size());
		compare_bulk_roundtrip_X<ztd_char32_t>(output_buffer.data(), output_buffer.size());

		const output_char* initial_intermediate = output_buffer.data();
		const output_char* intermediate         = initial_intermediate;
		const size_t initial_intermediate_size  = output_written;
		size_t intermediate_size                = initial_intermediate_size;
		std::vector<input_char> input_output_buffer(initial_input_size);
		input_char* initial_input_output       = input_output_buffer.data();
		input_char* input_output               = initial_input_output;
		const size_t initial_input_output_size = input_output_buffer.size();
		size_t input_output_size               = initial_input_output_size;
		const cnc_mcerror input_output_err     = cnc_cxsntocysn(
               &input_output_size, &input_output, &intermediate_size, &intermediate);
		const size_t intermediate_read    = initial_intermediate_size - intermediate_size;
		const size_t input_output_written = initial_input_output_size - input_output_size;
		REQUIRE(input_output_err == CNC_MCERROR_OK);
		REQUIRE(intermediate_size == 0);
		REQUIRE(intermediate_read == output_written);
		REQUIRE(input_output_written == initial_input_size);
		REQUIRE(input_output_written == input_read);
		REQUIRE((initial_intermediate + intermediate_read) == intermediate);
		REQUIRE((initial_intermediate_size - intermediate_read) == intermediate_size);
		REQUIRE((initial_input_output + input_output_written) == input_output);
		REQUIRE((initial_input_output_size - input_output_written) == input_output_size);
		// resize only after doing all the pointer-testing shenanigans
		input_output_buffer.resize(input_output_written);
		REQUIRE(std::equal(std::cbegin(input_buffer), std::cend(input_buffer),
		     std::cbegin(input_output_buffer), std::cend(input_output_buffer)));

		{
			// validate intermediate
			const output_char* initial_validate_input = initial_intermediate;
			const output_char* validate_input         = initial_validate_input;
			size_t validate_input_size                = initial_intermediate_size;
			const cnc_mcerror validate_input_err      = cnc_cxsntocysn(nullptr,
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
			size_t count_output_size               = initial_input_output_size;
			const cnc_mcerror count_input_err      = cnc_cxsntocysn(&count_output_size,
			          static_cast<input_char**>(nullptr), &count_input_size, &count_input);
			const size_t count_output_written = initial_input_output_size - count_output_size;
			REQUIRE(count_input_err == CNC_MCERROR_OK);
			REQUIRE((count_input - initial_count_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE(count_output_written == input_output_written);
		}
		{
			// unbounded output
			const output_char* initial_unbounded_input = initial_intermediate;
			const output_char* unbounded_input         = initial_unbounded_input;
			size_t unbounded_input_size                = initial_intermediate_size;
			std::vector<input_char> unbounded_output_buffer(input_output_written);
			input_char* initial_unbounded_output  = unbounded_output_buffer.data();
			input_char* unbounded_output          = initial_unbounded_output;
			const cnc_mcerror unbounded_input_err = cnc_cxsntocysn(
			     nullptr, &unbounded_output, &unbounded_input_size, &unbounded_input);
			REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
			REQUIRE((unbounded_input - initial_unbounded_input)
			     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			REQUIRE((unbounded_output - initial_unbounded_output)
			     == (static_cast<std::ptrdiff_t>(input_output_written)));
			REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
			REQUIRE(std::equal(std::cbegin(unbounded_output_buffer),
			     std::cend(unbounded_output_buffer), std::cbegin(input_output_buffer),
			     std::cend(input_output_buffer)));
		}
	}

} // namespace

TEST_CASE(
     "check bulk conversion from one of the typical typed encodings to UTF-32 and back to the "
     "typed encoding, using direct functions",
     "[cuneicode][direct][roundtrip-c32][bulk]") {
	const auto& unicode_input      = full_unicode_range();
	const auto& basic_source_input = basic_source_range();
	SECTION("mc") {
		compare_bulk_roundtrip_utf32<ztd_char_t>(basic_source_input);
		if (cnc_is_execution_encoding_unicode()) {
			compare_bulk_roundtrip_utf32<ztd_char_t>(unicode_input);
		}
	}
	SECTION("mwc") {
		compare_bulk_roundtrip_utf32<ztd_wchar_t>(basic_source_input);
		if (cnc_is_wide_execution_encoding_unicode()) {
			compare_bulk_roundtrip_utf32<ztd_wchar_t>(unicode_input);
		}
	}
	SECTION("c8") {
		compare_bulk_roundtrip_utf32<ztd_char8_t>(basic_source_input);
		compare_bulk_roundtrip_utf32<ztd_char8_t>(unicode_input);
	}
	SECTION("c16") {
		compare_bulk_roundtrip_utf32<ztd_char16_t>(basic_source_input);
		compare_bulk_roundtrip_utf32<ztd_char16_t>(unicode_input);
	}
	SECTION("c32") {
		compare_bulk_roundtrip_utf32<ztd_char32_t>(basic_source_input);
		compare_bulk_roundtrip_utf32<ztd_char32_t>(unicode_input);
	}
}
