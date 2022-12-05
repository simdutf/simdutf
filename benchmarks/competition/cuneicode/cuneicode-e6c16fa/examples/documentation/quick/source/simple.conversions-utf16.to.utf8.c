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

#include <ztd/cuneicode.h>

#include <ztd/idk/size.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {

	const ztd_char16_t utf16_text[] = u"🥺🙏";
	ztd_char8_t utf8_text[9]        = { 0 };

	// Now, actually output it
	const ztd_char16_t* p_input = utf16_text;
	ztd_char8_t* p_output       = utf8_text;
	// ztd_c_array_size INCLUDES the null terminator in the size!
	size_t input_size   = ztd_c_array_size(utf16_text);
	size_t output_size  = ztd_c_array_size(utf8_text);
	cnc_mcstate_t state = { 0 };
	// call the function with the right parameters!
	cnc_mcerror err = cnc_c16snrtoc8sn( // formatting
	     &output_size, &p_output,       // output first
	     &input_size, &p_input,         // input second
	     &state);                       // state parameter
	if (err != CNC_MCERROR_OK) {
		const char* err_str = cnc_mcerror_to_str(err);
		printf(
		     "An (unexpected) error occurred and the conversion could not "
		     "happen! Error string: %s\n",
		     err_str);
		return 1;
	}

	// requires a capable terminal / output, but will be
	// UTF-8 text!
	printf("Converted UTF-8 text: %s\n", (const char*)utf8_text);

	return 0;
}
