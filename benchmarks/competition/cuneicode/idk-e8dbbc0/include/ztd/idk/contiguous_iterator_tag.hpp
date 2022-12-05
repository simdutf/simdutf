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

#ifndef ZTD_IDK_CONTIGUOUS_ITERATOR_TAG_HPP
#define ZTD_IDK_CONTIGUOUS_ITERATOR_TAG_HPP

#include <ztd/idk/version.hpp>

#include <iterator>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		class __contiguous_iterator_tag : public ::std::random_access_iterator_tag { };
	} // namespace __idk_detail

	//////
	/// @brief Either a typedef or a polyfill of the contiguous iterator tag, only standardized in C++20.
	///
	using contiguous_iterator_tag =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_CONTIGUOUS_ITERATOR_TAG)
	     ::std::contiguous_iterator_tag
#else
	     __idk_detail::__contiguous_iterator_tag
#endif
	     ;

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
