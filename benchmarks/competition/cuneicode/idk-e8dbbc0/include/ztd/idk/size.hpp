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

#ifndef ZTD_IDK_SIZE_HPP
#define ZTD_IDK_SIZE_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/size.h>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	template <typename _Type, typename _Sentinel>
	size_t c_string_ptr_size(_Type* __ptr, const _Sentinel& __sen) {
		if (__ptr == nullptr) {
			return 0;
		}
		size_t __len = 0;
		while (__ptr[__len] != __sen) {
			++__len;
		}
		return __len;
	}

	template <typename _Type>
	size_t c_string_ptr_size(_Type* __ptr) {
		constexpr _Type __sentinel {};
		return c_string_ptr_size(__ptr, __sentinel);
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#endif
