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

#include "c_test.h"

#include <ztd/idk/encoding_name.h>

extern int encoding_name_tests(void) {
	BEGIN_TEST("encoding name tests");
	TEST_CASE(
	     "text/is_unicode_encoding_name", "Ensure that basic usages of the is_unicode_encoding_name predicate work") {
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-8"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-16"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-32"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF8"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF16"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF32"));
		REQUIRE(ztdc_is_unicode_encoding_name("utf8"));
		REQUIRE(ztdc_is_unicode_encoding_name("utf16"));
		REQUIRE(ztdc_is_unicode_encoding_name("utf32"));
		REQUIRE(ztdc_is_unicode_encoding_name("    _-  utf8"));
		REQUIRE(ztdc_is_unicode_encoding_name("   utf16    "));
		REQUIRE(ztdc_is_unicode_encoding_name("    _--   utf32"));
		REQUIRE(ztdc_is_unicode_encoding_name("uTF8"));
		REQUIRE(ztdc_is_unicode_encoding_name("--UtF-_-16"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTf____32"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF7IMAP"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-16-BE"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-32_LE"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF-16-BE"));
		REQUIRE(ztdc_is_unicode_encoding_name("GB18030"));
		REQUIRE(ztdc_is_unicode_encoding_name("gb18030"));
		REQUIRE(ztdc_is_unicode_encoding_name("Mutf____8"));
		REQUIRE(ztdc_is_unicode_encoding_name("Mutf__8"));
		REQUIRE(ztdc_is_unicode_encoding_name("Mutf    8            "));
		REQUIRE(ztdc_is_unicode_encoding_name("gb18030"));
		REQUIRE(ztdc_is_unicode_encoding_name("utf\t\t\t\n\r\n\r\nebcdic"));
		REQUIRE(ztdc_is_unicode_encoding_name("gb18030"));
		REQUIRE(ztdc_is_unicode_encoding_name("g    B     18  ____0 30"));
		REQUIRE(ztdc_is_unicode_encoding_name("cesu8"));
		REQUIRE(ztdc_is_unicode_encoding_name("UTF---------1"));
		REQUIRE(!ztdc_is_unicode_encoding_name("ascii"));
		REQUIRE(!ztdc_is_unicode_encoding_name("tatar"));
		REQUIRE(!ztdc_is_unicode_encoding_name("latin-1"));
		REQUIRE(!ztdc_is_unicode_encoding_name("UTF-8-not-really"));
		REQUIRE(!ztdc_is_unicode_encoding_name("UTF-7-not-really"));
		REQUIRE(!ztdc_is_unicode_encoding_name("UTF69"));
	}
	TEST_CASE("text/is_encoding_name_equals", "Ensure that basic usages of the encoding_name comparison works") {
		REQUIRE(ztdc_is_encoding_name_equal("UTF-8", "UTF-8"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF-16", "UTF-16"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF-32", "UTF-32"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF8", "UTF8"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF16", "UTF16"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF32", "UTF32"));
		REQUIRE(ztdc_is_encoding_name_equal("utf8", "utf8"));
		REQUIRE(ztdc_is_encoding_name_equal("utf16", "utf16"));
		REQUIRE(ztdc_is_encoding_name_equal("utf32", "utf32"));
		REQUIRE(ztdc_is_encoding_name_equal("utf--ebcdic", "utf\t\t\t\n\r\n\r\nebcdic"));
		REQUIRE(ztdc_is_encoding_name_equal("GB18030", "gb18030"));
		REQUIRE(ztdc_is_encoding_name_equal("GB18_0-30", "g    B     18  ____0 30"));
		REQUIRE(ztdc_is_encoding_name_equal("CESU-8", "cesu8"));
		REQUIRE(ztdc_is_encoding_name_equal("UTF-1", "UTF---------1"));

		REQUIRE(!ztdc_is_encoding_name_equal("xD", "UTF-8"));
		REQUIRE(!ztdc_is_encoding_name_equal("UTF-16", "UTF-16meow"));
		REQUIRE(!ztdc_is_encoding_name_equal("UTF-32", "UTF-329405"));
		REQUIRE(!ztdc_is_encoding_name_equal("UTF8  1", "UTF8"));
		REQUIRE(!ztdc_is_encoding_name_equal("sdasdUTF16", "UTF16"));
		REQUIRE(!ztdc_is_encoding_name_equal("UTF32  :P", "UTF32"));
		REQUIRE(!ztdc_is_encoding_name_equal("utf8", "utf8tyy"));
		REQUIRE(!ztdc_is_encoding_name_equal("utf16", "utf16g"));
		REQUIRE(!ztdc_is_encoding_name_equal("utf32", "utf32s"));
	}
	END_TEST();
}
