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

#include <ztd/idk/version.h>

#include <ztd/idk/bit.h>

#include <ztd/idk/endian.h>
#include <ztd/idk/extent.h>
#include <ztd/idk/static_assert.h>
#include <ztd/idk/assume_aligned.h>

#include <ztd/idk/detail/bit.load_store.impl.h>

#if ZTD_IS_ON(ZTD_C)
#include <string.h>
#else
#include <cstring>
#endif


#if ZTD_IS_ON(ZTD_COMPILER_GCC) || ZTD_IS_ON(ZTD_COMPILER_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#if ((CHAR_BIT % 8) == 0)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(8);
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(16);
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(32);
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(64);

#if defined(UINT_LEAST24_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(24);
#endif
#if defined(UINT_LEAST48_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(48);
#endif
#if defined(UINT_LEAST56_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(56);
#endif
#if defined(UINT_LEAST72_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(72);
#endif
#if defined(UINT_LEAST80_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(80);
#endif
#if defined(UINT_LEAST88_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(88);
#endif
#if defined(UINT_LEAST96_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(96);
#endif
#if defined(UINT_LEAST104_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(104);
#endif
#if defined(UINT_LEAST112_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(112);
#endif
#if defined(UINT_LEAST120_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(120);
#endif
#if defined(UINT_LEAST128_WIDTH)
ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS(128);
#endif
#endif

#if ZTD_IS_ON(ZTD_COMPILER_GCC) || ZTD_IS_ON(ZTD_COMPILER_CLANG)
#pragma GCC diagnostic pop
#endif


#undef ZTDC_GENERATE_LOAD8_STORE8_DEFINITIONS
