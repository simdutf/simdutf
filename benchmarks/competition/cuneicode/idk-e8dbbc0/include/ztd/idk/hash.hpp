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

#ifndef ZTD_IDK_HASH_HPP
#define ZTD_IDK_HASH_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>

#include <functional>
#include <iterator>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		struct __always_true_predicate {
			template <typename _Arg>
			constexpr bool operator()(_Arg&&) const noexcept {
				return true;
			}
		};
	} // namespace __idk_detail

	inline constexpr ::std::size_t fnv1a_offset_basis =
#if ZTD_IS_ON(ZTD_SIZE_64_BITS)
	     static_cast<std::size_t>(14695981039346656037ULL)
#else
	     static_cast<std::size_t>(2166136261UL)
#endif
	     ;


	//////
	/// @brief Computes the hash for each element of the range, if the __predicate returns true for that element.
	/// Returns final combined and mixed hash value.
	///
	/// @param[in] __initial_seed The starting seed value.
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	/// @param[in] __predicate The predicate that will be called on each element to determine whether or not it should
	/// be hashed as part of the range.
	/// @param[in] __element_hasher The hasher to use for each element of the range.
	template <typename _Range, typename _Predicate, typename _ElementHasher>
	::std::size_t fnv1a_hash_if(::std::size_t __initial_seed, _Range&& __range, _Predicate&& __predicate,
	     _ElementHasher&& __element_hasher) noexcept {
		constexpr ::std::size_t __fnv1a_prime =
#if ZTD_IS_ON(ZTD_SIZE_64_BITS)
		     static_cast<std::size_t>(1099511628211ULL)
#else
		     static_cast<std::size_t>(16777619UL)
#endif
		     ;

		::std::size_t __hash_value = __initial_seed;
		for (const auto& __element : __range) {
			if (!__predicate(__element)) {
				continue;
			}
			::std::size_t __element_hash = __element_hasher(__element);
			__hash_value                 = __hash_value ^ __element_hash;
			__hash_value                 = __hash_value * __fnv1a_prime;
		}
		return __hash_value;
	}

	//////
	/// @brief Computes the hash for each element of the range, if the __predicate returns true for that element.
	/// Returns final combined and mixed hash value.
	///
	/// @param[in] __initial_seed The starting seed value.
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	/// @param[in] __predicate The predicate that will be called on each element to determine whether or not it should
	/// be hashed as part of the range.
	template <typename _Range, typename _Predicate>
	::std::size_t fnv1a_hash_if(::std::size_t __initial_seed, _Range&& __range, _Predicate&& __predicate) noexcept {
		::std::hash<remove_cvref_t<decltype(*::std::begin(__range))>> __element_hasher {};
		return ::ztd::fnv1a_hash_if(__initial_seed, ::std::forward<_Range>(__range),
		     ::std::forward<_Predicate>(__predicate), __element_hasher);
	}

	//////
	/// @brief Computes the hash for each element of the range, if the __predicate returns true for that element.
	/// Returns final combined and mixed hash value.
	///
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	/// @param[in] __predicate The predicate that will be called on each element to determine whether or not it should
	/// be hashed as part of the range.
	template <typename _Range, typename _Predicate>
	::std::size_t fnv1a_hash_if(_Range&& __range, _Predicate&& __predicate) noexcept {
		return ::ztd::fnv1a_hash_if(
		     fnv1a_offset_basis, ::std::forward<_Range>(__range), ::std::forward<_Predicate>(__predicate));
	}

	//////
	/// @brief Computes the hash for each element of the range. Returns final combined and mixed hash value.
	///
	/// @param[in] __initial_seed The starting seed value.
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	/// @param[in] __element_hasher The hasher to use for each element of the range.
	template <typename _Range, typename _ElementHasher>
	::std::size_t fnv1a_hash(
	     ::std::size_t __initial_seed, _Range&& __range, _ElementHasher&& __element_hasher) noexcept {
		return ::ztd::fnv1a_hash_if(__initial_seed, ::std::forward<_Range>(__range),
		     __idk_detail::__always_true_predicate {}, ::std::forward<_ElementHasher>(__element_hasher));
	}

	//////
	/// @brief Computes the hash for each element of the range. Returns final combined and mixed hash value.
	///
	/// @param[in] __initial_seed The starting seed value.
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	template <typename _Range>
	::std::size_t fnv1a_hash(::std::size_t __initial_seed, _Range&& __range) noexcept {
		return ::ztd::fnv1a_hash_if(
		     __initial_seed, ::std::forward<_Range>(__range), __idk_detail::__always_true_predicate {});
	}

	//////
	/// @brief Computes the hash for each element of the range. Returns final combined and mixed hash value.
	///
	/// @param[in] __range The range to loop over from its `begin` to its `end`.
	template <typename _Range>
	::std::size_t fnv1a_hash(_Range&& __range) noexcept {
		return fnv1a_hash_if(::std::forward<_Range>(__range), __idk_detail::__always_true_predicate {});
	}


	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
