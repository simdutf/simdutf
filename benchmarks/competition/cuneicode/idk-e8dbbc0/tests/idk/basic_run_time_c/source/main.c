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

#include <stdio.h>

extern int c_span_tests(void);
extern int bit_intrinsic_tests(void);
extern int bit_rotate_tests(void);
extern int bit_memreverse_tests(void);
extern int encoding_name_tests(void);

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	int result = 0;
	result += c_span_tests();
	result += encoding_name_tests();
	result += bit_intrinsic_tests();
	result += bit_rotate_tests();
	result += bit_memreverse_tests();
	if (result == 0) {
		fprintf(stdout, "\n\n===========================================\nAll assertions passed!");
		fflush(stdout);
	}
	else {
		fprintf(stderr, "\n\n===========================================\n%d assertions failed.", result);
		fflush(stderr);
	}
	return result;
}
