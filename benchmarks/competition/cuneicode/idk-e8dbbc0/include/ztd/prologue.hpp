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

// clang-format off

#if defined(ZTD_PROLOGUE_I_)
	#error "[ztd] Library Prologue was already included in translation unit and not properly ended with an epilogue."
#endif

#define ZTD_PROLOGUE_I_ 1

#if ZTD_IS_ON(ZTD_CXX)

	#define _FWD(...) static_cast<decltype( __VA_ARGS__ )&&>( __VA_ARGS__ )

	#if ZTD_IS_ON(ZTD_COMPILER_GCC) || ZTD_IS_ON(ZTD_COMPILER_CLANG)
		#define _MOVE(...) static_cast<__typeof( __VA_ARGS__ )&&>( __VA_ARGS__ )
	#else
		#include <type_traits>
		
		#define _MOVE(...) static_cast<::std::remove_reference_t<( __VA_ARGS__ )>&&>( __VA_OPT__(,) )
	#endif
#endif

// clang-format on
