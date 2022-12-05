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

#ifndef ZTD_IDK_DETAIL_BIT_INTRINSIC_H
#define ZTD_IDK_DETAIL_BIT_INTRINSIC_H

#include <ztd/idk/version.h>

#include <ztd/idk/static_assert.h>

#if ZTD_IS_ON(ZTD_C)
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <cstddef>
#include <climits>
#include <cstdint>
#endif

ZTD_EXTERN_C_OPEN_I_

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_onesuc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_onesus(unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_onesui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_onesul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_onesull(unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_zerosuc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_zerosus(unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_zerosui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_zerosul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_zerosull(unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_zerosuc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_zerosus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_zerosui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_zerosul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_zerosull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_onesuc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_onesus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_onesui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_onesul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_leading_onesull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_zerosuc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_zerosus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_zerosui(
     unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_zerosul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_zerosull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_onesuc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_onesus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_onesui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_onesul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_count_trailing_onesull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_oneuc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_oneus(unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_oneui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_oneul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_oneull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_oneuc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_oneus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_oneui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_oneul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_oneull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_zerouc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_zerous(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_zeroui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_zeroul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_leading_zeroull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_zerouc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_zerous(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_zeroui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_zeroul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ int ztdc_first_trailing_zeroull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned char ztdc_rotate_leftuc(
     unsigned char value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned short ztdc_rotate_leftus(
     unsigned short value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned int ztdc_rotate_leftui(
     unsigned int value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long ztdc_rotate_leftul(
     unsigned long value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long long ztdc_rotate_leftull(
     unsigned long long value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned char ztdc_rotate_rightuc(
     unsigned char value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned short ztdc_rotate_rightus(
     unsigned short value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned int ztdc_rotate_rightui(
     unsigned int value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long ztdc_rotate_rightul(
     unsigned long value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long long ztdc_rotate_rightull(
     unsigned long long value, unsigned int __count) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztdc_has_single_bituc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztdc_has_single_bitus(unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztdc_has_single_bitui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztdc_has_single_bitul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ bool ztdc_has_single_bitull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned char ztdc_bit_flooruc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned short ztdc_bit_floorus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned int ztdc_bit_floorui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long ztdc_bit_floorul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long long ztdc_bit_floorull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned char ztdc_bit_ceiluc(unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned short ztdc_bit_ceilus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned int ztdc_bit_ceilui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long ztdc_bit_ceilul(unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long long ztdc_bit_ceilull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned char ztdc_bit_widthuc(
     unsigned char value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned short ztdc_bit_widthus(
     unsigned short value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned int ztdc_bit_widthui(unsigned int value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long ztdc_bit_widthul(
     unsigned long value) ZTD_CXX_NOEXCEPT_I_;
ZTD_C_LANGUAGE_LINKAGE_I_ ZTD_IDK_API_LINKAGE_I_ unsigned long long ztdc_bit_widthull(
     unsigned long long value) ZTD_CXX_NOEXCEPT_I_;

ZTD_EXTERN_C_CLOSE_I_

#endif
