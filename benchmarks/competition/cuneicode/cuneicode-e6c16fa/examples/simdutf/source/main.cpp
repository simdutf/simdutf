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

#include <ztd/cuneicode/shared/simdutf/registry.hpp>
#include <ztd/cuneicode/shared/stream_helpers.hpp>

#include <ztd/cuneicode.h>

#include <ztd/idk/size.h>
#include <ztd/idk/assert.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct cnc_conversion_deleter {
	void operator()(cnc_conversion* handle) const {
		cnc_conv_delete(handle);
	}
};

struct cnc_registry_deleter {
	void operator()(cnc_conversion_registry* handle) const {
		cnc_registry_delete(handle);
	}
};

int main(int, char*[]) {
	// open the registry
	const utf8string_view from_code = (const ztd_char8_t*)"utf8";
	const utf8string_view to_code   = (const ztd_char8_t*)"utf16";

	std::unique_ptr<cnc_conversion_registry, cnc_registry_deleter> registry
	     = nullptr;
	{
		cnc_conversion_registry* raw_registry = registry.get();
		cnc_registry_options registry_options = CNC_REGISTRY_OPTIONS_DEFAULT;
		cnc_open_error err = cnc_registry_new(&raw_registry, registry_options);
		if (err != CNC_OPEN_ERROR_OK) {
			std::cerr << "[error] Could not open a conversion "
			             "registry (error code: "
			          << err << ")." << std::endl;
			return 1;
		}
		registry.reset(raw_registry);
	}

	if (!cnc_shared_add_simdutf_to_registry(registry.get())) {
		std::cerr << "[error] Could not add simdutf to "
		             "the conversion registry."
		          << std::endl;
		return 1;
	}
	// if we are just listing encodings, do so and then stop here.
	std::cout << "[info] ";
	print_encoding_list(std::cout, registry.get());

	// open up the conversion specifier
	std::unique_ptr<cnc_conversion, cnc_conversion_deleter> conversion = nullptr;
	cnc_conversion_info info                                           = {};
	{
		cnc_conversion* raw_conversion = conversion.get();
		std::size_t from_size          = from_code.size();
		const ztd_char8_t* from_data   = from_code.data();
		std::size_t to_size            = to_code.size();
		const ztd_char8_t* to_data     = to_code.data();
		cnc_open_error err = cnc_conv_new_c8n(registry.get(), from_size,
		     from_data, to_size, to_data, &raw_conversion, &info);
		if (err != CNC_OPEN_ERROR_OK) {
			std::cerr << "[error] Could not open a conversion from \""
			          << from_code << "\" to \"" << to_code << "\"."
			          << std::endl;
			return 1;
		}
		conversion.reset(raw_conversion);
		print_conversion_info(std::cout, info);
	}

	const ztd_char8_t utf8_text[]
	     = u8R"(We don't know where Arua came from, nor how she came to be. We do know, however, that nothing existed before Arua. The Goddess of life, souls, and all existence created the universe as she saw fit… a calm and beautiful place where souls could dwell peacefully. The deep matters of the universe cradled the waves of life, on which all beings would sway soothingly. This perfect universe was called the Sea of Souls.

Life would bring shine to the Sea of Souls, in the form of seven planets. Arua created these out of love, and dedicated each of them to the inhabitants that she breathed life into.

"The planets, as your lives, dear ones, are all yours. As am I. As are your souls."

These are the words of Arua that ring in everyone's hearts when they are born.

"To you, I give the Gods, to watch over you. Praise them well, for no love can ever match the one you will receive from them."

And thus, the Gods were created, to watch over all life that Arua had created among the seven planets. And Arua named the planets after each of the Gods and Goddesses that would look after them: Junon, Lunar, Eldeon, Orlo, Karkia, Skaaj, and Hebarn.

Each planet's inhabitants lived in happiness, idolizing their respective God or Goddess, and reveling in the pleasures that living under their protection and rule brought.

But all was not harmonious for long. The Dark God Hebarn disbanded from the other planets, and his plots to destroy Arua's universe began.

Soon, the gossips about strange visitors started. The Visitors protect life, and make right from all that is wrong. They are divine beings; they seldom suffer from hunger or thirst, they do not need as much sleep as most other creatures, but they can hurt, they can love. And they can help.)";

	std::size_t buffer_byte_size = 0;
	{
		// get the necessary buffer size for the conversion
		const unsigned char* input = (const unsigned char*)utf8_text;
		size_t input_size          = ztd_c_string_array_size(utf8_text);
		const size_t initial_output_byte_size = SIZE_MAX;
		size_t output_byte_size               = initial_output_byte_size;
		cnc_mcerror err                       = cnc_conv_count(
		                           conversion.get(), &output_byte_size, &input_size, &input);
		if (err != CNC_MCERROR_OK) {
			std::cerr << "[error] Could not perform counting operation on "
			             "input data from \""
			          << from_code << "\" to \"" << to_code
			          << "\". Error reason:" << cnc_mcerror_to_str(err)
			          << std::endl;
			return 1;
		}
		buffer_byte_size = (initial_output_byte_size - output_byte_size);
	}
	std::vector<unsigned char> output_buffer(buffer_byte_size);
	size_t output_size         = output_buffer.size();
	unsigned char* output      = (unsigned char*)output_buffer.data();
	size_t input_size          = ztd_c_string_array_size(utf8_text);
	const unsigned char* input = (const unsigned char*)utf8_text;
	cnc_mcerror err
	     = cnc_conv(conversion.get(), &output_size, &output, &input_size, &input);
	ZTD_ASSERT(err == CNC_MCERROR_OK);
	ZTD_ASSERT(output_size == 0);
	// write out as a hex sequence
	const char16_t* it = (const char16_t*)output_buffer.data();
	const char16_t* last
	     = (const char16_t*)(output_buffer.data() + output_buffer.size());
	std::cout << "[info] UTF-16 Result as a Hexadecimal Sequence:" << std::endl;
	for (; it != last;) {
		std::cout << "0x" << std::hex << static_cast<int>(*it);
		++it;
		if (it != last) {
			std::cout << " ";
		}
	}
	std::cout << std::endl;
	return 0;
}
