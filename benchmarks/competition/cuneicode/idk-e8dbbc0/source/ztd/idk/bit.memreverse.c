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

#include <ztd/idk/extent.h>

#if ZTD_IS_ON(ZTD_C)
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#else
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <climits>
#endif

#include <ztd/idk/detail/bit.memreverse.impl.h>

#if (CHAR_BIT % 8 == 0)

void ztdc_memreverse8(size_t __n, unsigned char __ptr[ZTD_PTR_EXTENT(__n)]) ZTD_CXX_NOEXCEPT_I_ {
	__ZTDC_MEMREVERSE8_IMPL(unsigned char, __n, __ptr);
}

#if defined(UINT8_MAX)
uint8_t ztdc_memreverse8u8(uint8_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 8 bits
#if defined(UINT16_MAX)
uint16_t ztdc_memreverse8u16(uint16_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 16 bits
#if defined(UINT24_MAX)
uint24_t ztdc_memreverse824(uint24_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 24 bits
#if defined(UINT32_MAX)
uint32_t ztdc_memreverse8u32(uint32_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 32 bits
#if defined(UINT40_MAX)
uint40_t ztdc_memreverse8u40(uint40_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 40 bits
#if defined(UINT48_MAX)
uint48_t ztdc_memreverse8u48(uint48_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 48 bits
#if defined(UINT56_MAX)
uint56_t ztdc_memreverse8u56(uint56_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 48 bits
#if defined(UINT64_MAX)
uint64_t ztdc_memreverse8u64(uint64_t __value) {
	ztdc_memreverse8(sizeof(__value), (unsigned char*)&__value) ZTD_CXX_NOEXCEPT_I_;
	return __value;
}
#endif // 64 bits
#endif
