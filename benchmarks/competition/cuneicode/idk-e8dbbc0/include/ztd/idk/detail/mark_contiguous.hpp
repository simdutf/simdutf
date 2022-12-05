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

#ifndef ZTD_IDK_DETAIL_MARK_CONTIGUOUS_HPP
#define ZTD_IDK_DETAIL_MARK_CONTIGUOUS_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>

#include <iterator>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {

		template <typename _It, typename = void>
		struct __mark_contiguous : public ::std::integral_constant<bool, ::std::is_pointer_v<remove_cvref_t<_It>>> {
		};

#if ZTD_IS_ON(ZTD_LIBSTDCXX)
		template <typename _It, typename _Parent>
		struct __mark_contiguous<::__gnu_cxx::__normal_iterator<_It, _Parent>> : public __mark_contiguous<_It> { };
#endif

#if ZTD_IS_ON(ZTD_LIBVCXX)
		template <typename _It>
		struct __mark_contiguous<_It, ::std::void_t<decltype(::std::declval<_It>()._Unwrapped())>>
		: public __mark_contiguous<decltype(::std::declval<_It>()._Unwrapped())> { };
#endif

#if ZTD_IS_ON(ZTD_LIBCXX)
		template <typename _It>
		struct __mark_contiguous<::std::__wrap_iter<_It>> : public __mark_contiguous<_It> { };
#endif

		template <typename _It>
		inline constexpr bool __mark_contiguous_v = __mark_contiguous<_It>::value;

	} // namespace __idk_detail

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
