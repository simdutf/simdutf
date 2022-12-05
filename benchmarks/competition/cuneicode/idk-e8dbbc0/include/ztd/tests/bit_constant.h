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

#pragma once

#ifndef ZTD_IDK_TESTS_BIT_CONSTANT_H
#define ZTD_IDK_TESTS_BIT_CONSTANT_H

#include <ztd/version.hpp>

#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_8(_TYPE) ((_TYPE)0x10)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_16(_TYPE) ((_TYPE)0x1023)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_24(_TYPE) ((_TYPE)0x102345)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_32(_TYPE) ((_TYPE)0x10234567)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_40(_TYPE) ((_TYPE)0x1023456789)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_48(_TYPE) ((_TYPE)0x1023456789AB)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_56(_TYPE) ((_TYPE)0x1023456789ABCD)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_64(_TYPE) ((_TYPE)0x1023456789ABCDEF)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_8(_TYPE) ((_TYPE)0x10)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_16(_TYPE) ((_TYPE)0x2310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_24(_TYPE) ((_TYPE)0x452310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_32(_TYPE) ((_TYPE)0x67452310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_40(_TYPE) ((_TYPE)0x8967452310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_48(_TYPE) ((_TYPE)0xAB8967452310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_56(_TYPE) ((_TYPE)0xCDAB8967452310)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_64(_TYPE) ((_TYPE)0xEFCDAB8967452310)


#define ZTD_TESTS_DISTINCT_BIT_CONSTANT(_TYPE, _N) ZTD_CONCAT_TOKENS_I_(ZTD_TESTS_DISTINCT_BIT_CONSTANT_, _N)(_TYPE)
#define ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE(_TYPE, _N) \
	ZTD_CONCAT_TOKENS_I_(ZTD_TESTS_DISTINCT_BIT_CONSTANT_REVERSE_, _N)(_TYPE)

#endif
