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

#ifndef ZTD_RANGES_SUBRANGE_HPP
#define ZTD_RANGES_SUBRANGE_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/range.hpp>
#include <ztd/ranges/iterator.hpp>
#include <ztd/ranges/adl.hpp>

#include <ztd/idk/type_traits.hpp>

#include <iterator>
#include <utility>

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
#include <ranges>
#endif

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @addtogroup ztd_ranges_support Support Classes
	///
	/// @{

	namespace __rng_detail {
		struct __size_mark { };

		//////
		/// @brief An enumeration that helps determine whether a subrange has size information or not.
		enum class __subrange_kind : bool {
			//////
			/// @brief Does not have a size (or does not have a size that can be computed in O(1)).
			unsized,
			//////
			/// @brief Has a size that can be computed in O(1).
			sized
		};

		template <typename _SizeType, bool _Store>
		struct __subrange_size {
			constexpr __subrange_size() noexcept {
			}
		};

		template <typename _SizeType>
		struct __subrange_size<_SizeType, true> {
			_SizeType _M_size;

			constexpr __subrange_size() noexcept : _M_size(static_cast<_SizeType>(0)) {
			}
			constexpr __subrange_size(_SizeType&& __size) noexcept(::std::is_nothrow_move_constructible_v<_SizeType>)
			: _M_size(::std::move(__size)) {
			}
			constexpr __subrange_size(const _SizeType& __size) noexcept(
				::std::is_nothrow_copy_constructible_v<_SizeType>)
			: _M_size(__size) {
			}
		};

		//////
		/// @brief A utility class to aid in trafficking iterator pairs (or, possibly, and iterator and sentinel pair)
		/// through the API to provide a generic, basic "range" type. Attempts to mimic `std::ranges::subrange` on
		/// platforms where it is not available.
		///
		/// @tparam _It The iterator type.
		/// @tparam _Sen The sentinel type, defaulted to `_It`.
		/// @tparam _Kind Whether or not this is a "Sized Subrange": that is, that a calculation for the size of the
		/// subrange can be done in O(1) time and is available.
		template <typename _It, typename _Sen = _It,
			__subrange_kind _Kind
			= is_sized_sentinel_for_v<_It, _Sen> ? __subrange_kind::sized : __subrange_kind::unsized>
		class __subrange : __subrange_size<iterator_size_type_t<_It>, !is_sized_sentinel_for_v<_It, _Sen>> {
		private:
			inline static constexpr bool _SizeRequired
				= _Kind == __subrange_kind::sized && !is_sized_sentinel_for_v<_It, _Sen>;
			using _SizeType     = ranges::iterator_size_type_t<_It>;
			using __base_size_t = __subrange_size<_SizeType, _SizeRequired>;

		public:
			//////
			/// @brief The `iterator` type for this subrange, dictated by the template parameter `_It`.
			using iterator = _It;
			//////
			/// @brief The `const_iterator` type for this subrange, dictated by the template parameter `_It`.
			using const_iterator = iterator;
			//////
			/// @brief The `sentinel` type for this subrange, dictated by the template parameter `_Sen`.
			using sentinel = _Sen;
			//////
			/// @brief The `const_sentinel` type for this subrange, dictated by the template parameter `_Sen`.
			using const_sentinel = sentinel;
			//////
			/// @brief The iterator category. Same as the iterator category for `_It`.
			using iterator_category = ::std::conditional_t<is_iterator_contiguous_iterator_v<iterator>,
				contiguous_iterator_tag, ranges::iterator_category_t<iterator>>;
			//////
			/// @brief The iterator concept. Same as the iterator concept for `_It`.
			using iterator_concept = ::std::conditional_t<is_iterator_contiguous_iterator_v<iterator>,
				contiguous_iterator_tag, ranges::iterator_concept_t<iterator>>;
			//////
			/// @brief The `pointer` type. Same as the `pointer` type for `_It`.
			using pointer = ranges::iterator_pointer_t<iterator>;
			//////
			/// @brief The `const_pointer` type. Same as the `const_pointer` type for `_It`.
			using const_pointer = pointer;
			//////
			/// @brief The `reference` type. Same as the `reference` type for `_It`.
			using reference = ranges::iterator_reference_t<iterator>;
			//////
			/// @brief The `const_reference` type. Same as the `const_reference` type for `_It`.
			using const_reference = reference;
			//////
			/// @brief The `value_type.` Same as the `value_type` for `_It`.
			using value_type = ranges::iterator_value_type_t<iterator>;
			//////
			/// @brief The `difference_type.` Same as the `difference_type` for `_It`.
			using difference_type = ranges::iterator_difference_type_t<iterator>;
			//////
			/// @brief The `size_type.` Same as the `size_type` for `_It`.
			using size_type = _SizeType;

			//////
			/// @brief Constructs a ztd::ranges::subrange containing a defaulted iterator and a defaulted sentinel.
			constexpr __subrange() = default;

			//////
			/// @brief Constructs a ztd::ranges::subrange with its begin and end constructed by `__range`'s `begin()`
			/// and
			/// `end()` values.
			///
			/// @param[in] __range The Range to get the `begin()` and `end()` out of to initialize the subrange's
			/// iterators.
			template <typename _Range,
				::std::enable_if_t<!::std::is_same_v<remove_cvref_t<_Range>, __subrange>>* = nullptr>
			constexpr __subrange(_Range&& __range) noexcept(::std::is_nothrow_constructible_v<__subrange,
				range_iterator_t<remove_cvref_t<_Range>>, range_sentinel_t<remove_cvref_t<_Range>>>)
			: __subrange(::ztd::ranges::begin(__range), ::ztd::ranges::end(__range)) {
			}

			//////
			/// @brief Constructs a ztd::ranges::subrange with its begin and end constructed by `__range`'s `begin()`
			/// and
			/// `end()` values.
			///
			/// @param[in] __range The Range to get the `begin()` and `end()` out of to initialize the subrange's
			/// iterators.
			/// @param[in] __size The size to construct with.
			template <typename _Range, __subrange_kind _StrawmanKind = _Kind,
				::std::enable_if_t<(_StrawmanKind == __subrange_kind::sized)>* = nullptr>
			constexpr __subrange(_Range&& __range, size_type __size) noexcept(noexcept(
				__subrange(::ztd::ranges::begin(__range), ::ztd::ranges::end(__range), ::std::move(__size))))
			: __subrange(::ztd::ranges::begin(__range), ::ztd::ranges::end(__range), ::std::move(__size)) {
			}

			//////
			/// @brief Constructs a ztd::ranges::subrange with its begin and end constructed by `__range`'s `begin()`
			/// and
			/// `end()` values.
			///
			/// @param[in] __it An iterator value to `std::move` in.
			/// @param[in] __sen A sentinel value to `std::move` in.
			constexpr __subrange(iterator __it, sentinel __sen) noexcept(
				::std::is_nothrow_move_constructible_v<iterator>&& ::std::is_nothrow_move_constructible_v<sentinel>)
			: __subrange(__size_mark {}, ::std::integral_constant<bool, _SizeRequired>(), ::std::move(__it),
				::std::move(__sen)) {
			}

			//////
			/// @brief Constructs a ztd::ranges::subrange with its begin and end constructed by `__range`'s `begin()`
			/// and
			/// `end()` values.
			///
			/// @param[in] __it An iterator value to construct with.
			/// @param[in] __sen A sentinel value to construct with.
			/// @param[in] __size The size to construct with.
			template <__subrange_kind _StrawmanKind                           = _Kind,
				::std::enable_if_t<_StrawmanKind == __subrange_kind::sized>* = nullptr>
			constexpr __subrange(iterator __it, sentinel __sen, size_type __size) noexcept(
				::std::is_nothrow_move_constructible_v<iterator>&& ::std::is_nothrow_constructible_v<
				     sentinel>&& ::std::is_nothrow_move_constructible_v<size_type>)
			: __base_size_t(::std::move(__size)), _M_it(::std::move(__it)), _M_sen(::std::move(__sen)) {
			}

			//////
			/// @brief The stored begin iterator.
			constexpr iterator begin() & noexcept {
				if constexpr (::std::is_copy_constructible_v<iterator>) {
					return this->_M_it;
				}
				else {
					return ::std::move(this->_M_it);
				}
			}

			//////
			/// @brief The stored begin iterator.
			constexpr iterator begin() const& noexcept {
				return this->_M_it;
			}

			//////
			/// @brief The stored begin iterator.
			constexpr iterator begin() && noexcept {
				return ::std::move(this->_M_it);
			}

			//////
			/// @brief The stored end iterator.
			constexpr const sentinel& end() const& noexcept {
				return this->_M_sen;
			}

			//////
			/// @brief The stored end iterator.
			constexpr sentinel& end() & noexcept {
				return this->_M_sen;
			}

			//////
			/// @brief The stored end iterator.
			constexpr sentinel&& end() && noexcept {
				return ::std::move(this->_M_sen);
			}

			//////
			/// @brief Whether or not this range is empty.
			///
			/// @returns `begin()` == `end()`
			constexpr bool empty() const noexcept {
				return this->_M_it == this->_M_sen;
			}

			//////
			/// @brief The stored begin iterator, const-ified.
			///
			/// @remarks This must be reimplemetned at some point.
			/////
			constexpr iterator cbegin() const noexcept {
				return this->_M_it;
			}

			//////
			/// @brief The stored end iterator.
			///
			/// @remarks This must be reimplemetned at some point.
			/////
			constexpr sentinel cend() const noexcept {
				return this->_M_sen;
			}

			//////
			/// @brief The size of the range.
			///
			/// @returns @code std::distance(begin(), end()) @endcode
			///
			/// @remarks This function call only works if the `_Kind` of this subrange is
			/// ztd::ranges::subrange_kind::sized.
			template <__subrange_kind _Strawman                           = _Kind,
				::std::enable_if_t<_Strawman == __subrange_kind::sized>* = nullptr>
			constexpr size_type size() const noexcept {
				if constexpr (_SizeRequired) {
					return this->__base_size_t::_M_size;
				}
				else {
					return ::std::distance(this->_M_it, this->_M_sen);
				}
			}

			//////
			/// @brief A `pointer` to the range of elements.
			///
			/// @returns `std::addressof(`*begin()).
			///
			/// @remarks This function call only works if the `iterator_concept` is a `contiguous_iterator_tag` or
			/// better.
			template <typename _Strawman                                           = _It,
				::std::enable_if_t<is_iterator_contiguous_iterator_v<_Strawman>>* = nullptr>
			constexpr pointer data() const noexcept {
				return ::ztd::to_address(this->_M_it);
			}

			//////
			/// @brief Produces a copy of the subrange and advances the `begin()` iterator by 1.
			///
			/// @remarks This function call only works if the underlying iterator and sentinal types are copyable.
			[[nodiscard]] constexpr __subrange next() const& noexcept(
				(::std::is_nothrow_copy_constructible_v<
				      iterator> && ::std::is_nothrow_copy_constructible_v<sentinel>)&& noexcept(::ztd::ranges::
				          iter_advance(::std::declval<iterator&>()))) {
				auto __it = this->_M_it;
				::ztd::ranges::iter_advance(__it);
				return __subrange(::std::move(__it), this->_M_sen);
			}

			//////
			/// @brief Produces a copy of the subrange and advances the `begin()` iterator by 1.
			///
			/// @remarks This function call can be more efficient and allows working with move-only iterators. This
			/// function call will move the iterators underlying this object.
			[[nodiscard]] constexpr __subrange next() && noexcept(
				(::std::is_nothrow_move_constructible_v<
				      iterator> && ::std::is_nothrow_move_constructible_v<sentinel>)&& noexcept(::ztd::ranges::
				          iter_advance(::std::declval<iterator&>()))) {
				iterator __it = ::std::move(this->_M_it);
				::ztd::ranges::iter_advance(__it);
				return __subrange(::std::move(__it), ::std::move(this->_M_sen));
			}

			//////
			/// @brief Produces a copy of the subrange and advances the `begin()` iterator by `__diff`.
			///
			/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
			///
			/// @remarks This function call only works if the underlying iterator and sentinal types are copyable.
			[[nodiscard]] constexpr __subrange next(difference_type __diff) const& noexcept(
				(::std::is_nothrow_copy_constructible_v<
				      iterator> && ::std::is_nothrow_copy_constructible_v<sentinel>)&& noexcept(::ztd::ranges::
				          iter_advance(::std::declval<iterator&>(), ::std::declval<difference_type>()))) {
				auto __it = this->_M_it;
				::ztd::ranges::iter_advance(__it, __diff);
				return __subrange(::std::move(__it), this->_M_sen);
			}

			//////
			/// @brief Produces a copy of the subrange and advances the `begin()` iterator by `__diff`.
			///
			/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
			///
			/// @remarks This function call can be more efficient and allows working with move-only iterators. This
			/// function call will move the iterators underlying this object.
			[[nodiscard]] constexpr __subrange next(difference_type __diff) && noexcept(
				(::std::is_nothrow_move_constructible_v<
				      iterator> && ::std::is_nothrow_move_constructible_v<sentinel>)&& noexcept(::ztd::ranges::
				          iter_advance(::std::declval<iterator&>(), ::std::declval<difference_type>()))) {
				iterator __it = ::std::move(this->_M_it);
				::ztd::ranges::iter_advance(__it, __diff);
				return __subrange(::std::move(__it), ::std::move(this->_M_sen));
			}

			//////
			/// @brief Produces a copy of the subrange and recedes the `begin()` iterator by `__diff`.
			///
			/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
			///
			/// @remarks This function call requires that the underlying iterator are bidirectional.
			[[nodiscard]] constexpr __subrange prev(difference_type __diff = 1) const
				noexcept((::std::is_nothrow_copy_constructible_v<
				               iterator> && ::std::is_nothrow_copy_constructible_v<sentinel>)&& noexcept(::ztd::
				          ranges::iter_recede(::std::declval<iterator&>(), ::std::declval<difference_type>()))) {
				auto __it = this->_M_it;
				::ztd::ranges::iter_recede(__it, __diff);
				return __subrange(::std::move(__it), this->_M_sen);
			}

			//////
			/// @brief Advances the `begin()` iterator of this ztd::ranges::subrange by `__diff` or just `1` if the
			/// argument is not specified.
			///
			/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
			constexpr __subrange& advance(difference_type __diff = 1) noexcept(noexcept(
				::ztd::ranges::iter_advance(::std::declval<iterator&>(), ::std::declval<difference_type>()))) {
				::ztd::ranges::iter_advance(this->_M_it, __diff);
				return *this;
			}

			//////
			/// @brief Recedes the `begin()` iterator of this ztd::ranges::subrange by `__diff` or just `1` if the
			/// argument is not specified.
			///
			/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
			///
			/// @remarks This function call requires that the underlying iterator are bidirectional.
			constexpr __subrange& recede(difference_type __diff = 1) noexcept(noexcept(
				::ztd::ranges::iter_recede(std::declval<iterator&>(), ::std::declval<difference_type>()))) {
				::ztd::ranges::iter_recede(this->_M_it, __diff);
				return *this;
			}

		private:
			template <typename _ArgIterator, typename _ArgSentinel>
			constexpr __subrange(__size_mark, ::std::true_type, _ArgIterator&& __it, _ArgSentinel&& __sen) noexcept(
				::std::is_nothrow_constructible_v<iterator, _ArgIterator>&& ::std::is_nothrow_constructible_v<
				     sentinel, _ArgSentinel>&& ::std::is_nothrow_constructible_v<__base_size_t, _SizeType>)
			: __base_size_t(static_cast<_SizeType>(__sen - __it))
			, _M_it(::std::forward<_ArgIterator>(__it))
			, _M_sen(::std::forward<_ArgSentinel>(__sen)) {
			}

			template <typename _ArgIterator, typename _ArgSentinel>
			constexpr __subrange(__size_mark, ::std::false_type, _ArgIterator&& __it, _ArgSentinel&& __sen) noexcept(
				::std::is_nothrow_constructible_v<iterator,
				     _ArgIterator>&& ::std::is_nothrow_constructible_v<sentinel, _ArgSentinel>)
			: _M_it(::std::forward<_ArgIterator>(__it)), _M_sen(::std::forward<_ArgSentinel>(__sen)) {
			}

			iterator _M_it;
			sentinel _M_sen;
		};
	} // namespace __rng_detail

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)

	// std::subrange is busted and I'm not interested in digging into why it's busted
	using ::std::ranges::subrange;
	using ::std::ranges::subrange_kind;

#else
	//////
	/// @brief The type of subrange, sized or unsized.
	using subrange_kind = __rng_detail::__subrange_kind;

	//////
	/// @brief A general-purpose iterator-sentinel (or iterator-sentinel-size) container.
	template <typename _It, typename _Sen = _It,
		subrange_kind _Kind = is_sized_sentinel_for_v<_It, _Sen> ? subrange_kind::sized : subrange_kind::unsized>
	using subrange = ::ztd::ranges::__rng_detail::__subrange<_It, _Sen, _Kind>;
#endif

	namespace __rng_detail {
		template <typename _Range>
		using __subrange_for_t = subrange<range_iterator_t<_Range>, range_sentinel_t<_Range>>;
	} // namespace __rng_detail

	//////
	/// @brief Decomposes a range into its two iterators and returns it as a ztd::ranges::subrange.
	template <typename _Range>
	constexpr __rng_detail::__subrange_for_t<_Range> make_subrange(_Range&& __range) noexcept(
		::std::is_nothrow_constructible_v<_Range, __rng_detail::__subrange_for_t<_Range>>) {
		return { ::ztd::ranges::begin(__range), ::ztd::ranges::end(__range) };
	}

	//////
	/// @brief Takes two iterators and returns them as a ztd::ranges::subrange.
	template <typename _It, typename _Sen>
	constexpr subrange<remove_cvref_t<_It>, remove_cvref_t<_Sen>> make_subrange(_It&& __it, _Sen&& __sen) noexcept(
		::std::is_nothrow_constructible_v<subrange<remove_cvref_t<_It>, remove_cvref_t<_Sen>>, _It, _Sen>) {
		return { ::std::forward<_It>(__it), ::std::forward<_Sen>(__sen) };
	}

	//////
	/// @}

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#if ZTD_IS_ON(ZTD_STD_LIBRARY_BORROWED_RANGE)

namespace std { namespace ranges {

	template <typename _It, typename _Sen, ::ztd::ranges::__rng_detail::__subrange_kind _Kind>
	inline constexpr bool enable_borrowed_range<::ztd::ranges::__rng_detail::__subrange<_It, _Sen, _Kind>> = true;

}} // namespace std::ranges

#else

namespace ztd { namespace ranges {

	//////
	/// @brief Mark subranges as appropriately borrowed ranges.
	template <typename _It, typename _Sen, ::ztd::ranges::__rng_detail::__subrange_kind _Kind>
	inline constexpr bool enable_borrowed_range<::ztd::ranges::__rng_detail::__subrange<_It, _Sen, _Kind>> = true;

}} // namespace ztd::ranges

#endif

#include <ztd/epilogue.hpp>

#endif
