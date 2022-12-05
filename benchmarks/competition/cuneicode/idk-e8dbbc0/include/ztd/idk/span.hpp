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

#ifndef ZTD_IDK_SPAN_HPP
#define ZTD_IDK_SPAN_HPP

#include <ztd/idk/version.hpp>

#include <ztd/ranges/adl.hpp>

#include <type_traits>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_SPAN)

#include <span>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief dynamic extent copycat
	///
	inline constexpr decltype(::std::dynamic_extent) dynamic_extent = ::std::dynamic_extent;

	using ::std::as_bytes;
	using ::std::as_writable_bytes;
	using ::std::span;

	template <class T>
	inline constexpr span<T> make_span(T* ptr, size_t count) noexcept {
		return ::std::span<T>(ptr, count);
	}

	template <class T>
	inline constexpr span<T> make_span(T* first, T* last) noexcept {
		return ::std::span<T>(first, last);
	}

	template <class T, std::size_t N>
	inline constexpr span<T, N> make_span(T (&arr)[N]) noexcept {
		return ::std::span<T, N>(&arr[0], N);
	}

	template <class Container, class EP = decltype(std::data(std::declval<Container&>()))>
	inline constexpr auto make_span(Container& cont) noexcept -> ::std::span<typename std::remove_pointer<EP>::type> {
		return ::std::span<typename std::remove_pointer<EP>::type>(cont);
	}

	template <class Container, class EP = decltype(std::data(std::declval<Container&>()))>
	inline constexpr auto make_span(Container const& cont) noexcept
	     -> ::std::span<const typename std::remove_pointer<EP>::type> {
		return ::std::span<const typename std::remove_pointer<EP>::type>(cont);
	}

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

namespace std {
	template <typename _Ty, ::std::size_t _LeftExtent, ::std::size_t _RightExtent>
	constexpr bool operator==(
	     const ::std::span<_Ty, _LeftExtent>& __left, const ::std::span<_Ty, _RightExtent>& __right) noexcept {
		auto __left_size  = __left.size();
		auto __right_size = __right.size();
		if (__left_size < __right_size) {
			return ::std::equal(__left.begin(), __left.end(), __right.begin());
		}
		else {
			return ::std::equal(__right.begin(), __right.end(), __left.begin());
		}
	}

	template <typename _Ty, ::std::size_t _LeftExtent, ::std::size_t _RightExtent>
	constexpr bool operator!=(
	     const ::std::span<_Ty, _LeftExtent>& __left, const ::std::span<_Ty, _RightExtent>& __right) noexcept {
		return !(__left == __right);
	}
} // namespace std

#else

// Use home-grown span from Martin Moene
#define span_FEATURE_MAKE_SPAN 1
#include <ztd/idk/detail/span.implementation.hpp>

#include <ztd/prologue.hpp>

namespace nonstd { namespace span_lite {
	template <typename _Ty, ::std::size_t _LeftExtent, ::std::size_t _RightExtent>
	constexpr bool operator==(const ::nonstd::span_lite::span<_Ty, _LeftExtent>& __left,
		const ::nonstd::span_lite::span<_Ty, _RightExtent>& __right) noexcept {
		auto __left_size  = __left.size();
		auto __right_size = __right.size();
		if (__left_size < __right_size) {
			return ::std::equal(__left.cbegin(), __left.cend(), __right.cbegin());
		}
		else {
			return ::std::equal(__right.cbegin(), __right.cend(), __left.cbegin());
		}
	}

	template <typename _Ty, ::std::size_t _LeftExtent, ::std::size_t _RightExtent>
	constexpr bool operator!=(const ::nonstd::span_lite::span<_Ty, _LeftExtent>& __left,
		const ::nonstd::span_lite::span<_Ty, _RightExtent>& __right) noexcept {
		return !(__left == __right);
	}
}} // namespace nonstd::span_lite

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief dynamic extent copycat
	///
	inline constexpr decltype(::nonstd::dynamic_extent) dynamic_extent = ::nonstd::dynamic_extent;

	using ::nonstd::as_bytes;
	using ::nonstd::as_writable_bytes;
	using ::nonstd::make_span;
	using ::nonstd::span;

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#if ZTD_IS_ON(ZTD_STD_LIBRARY_BORROWED_RANGE)

namespace std {

	template <typename _Ty, decltype(::ztd::dynamic_extent) _Extent>
	inline constexpr bool enable_borrowed_range<::ztd::span<_Ty, _Extent>> = true;

} // namespace std

#else

namespace ztd { namespace ranges {

	template <typename _Ty, decltype(::ztd::dynamic_extent) _Extent>
	inline constexpr bool enable_borrowed_range<::ztd::span<_Ty, _Extent>> = true;

}} // namespace ztd::ranges

#endif

#endif

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief Determines whether or not a given type is a @c span of some kind.
	template <typename _Ty>
	class is_span : public ::std::false_type { };

	template <typename _Ty, decltype(::ztd::dynamic_extent) _N>
	class is_span<::ztd::span<_Ty, _N>> : public ::std::true_type { };

	//////
	/// @brief A @c _v alias for ztd::is_span.
	template <typename _Ty>
	inline constexpr bool is_span_v = is_span<_Ty>::value;

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
