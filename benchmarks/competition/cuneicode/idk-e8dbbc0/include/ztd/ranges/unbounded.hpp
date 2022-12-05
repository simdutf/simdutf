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

#ifndef ZTD_RANGES_UNBOUNDED_HPP
#define ZTD_RANGES_UNBOUNDED_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/reconstruct.hpp>
#include <ztd/ranges/range.hpp>
#include <ztd/ranges/unreachable_sentinel.hpp>

#include <iterator>
#include <utility>

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief A class whose iterator and sentinel denote an infinity-range that, if iterated with a traditional for
	/// range loop, will never cease.
	template <typename _It>
	class unbounded_view {
	private:
		_It _M_it;

	public:
		//////
		/// @brief The iterator type.
		using iterator = _It;
		//////
		/// @brief The iterator type that can iterate indefinitely (or some approximation thereof).
		using const_iterator = iterator;
		//////
		/// @brief The sentinel type, an infinity type that compares equal to nothing.
		using sentinel = unreachable_sentinel_t;
		//////
		/// @brief The const sentinel type.
		///
		/// @remarks It's just the sentinal type.
		using const_sentinel = sentinel;
		//////
		/// @brief The pointer type related to the iterator.
		using pointer = ranges::iterator_pointer_t<iterator>;
		//////
		/// @brief The const pointer type related to the iterator.
		///
		/// @remarks It's just the pointer type.
		using const_pointer = pointer;
		//////
		/// @brief The reference type for this range.
		using reference = ranges::iterator_reference_t<iterator>;
		//////
		/// @brief The const reference type for this range.
		///
		/// @remarks It's just the reference type.
		using const_reference = reference;
		//////
		/// @brief The value type for this range.
		using value_type = ranges::iterator_value_type_t<iterator>;
		//////
		/// @brief The difference type that results from iterator subtraction (not practical for this range).
		using difference_type = ranges::iterator_difference_type_t<iterator>;
		//////
		/// @brief The iterator concept - no matter what, this is a forward range at best.
		using iterator_concept = ::std::conditional_t<
			::ztd::ranges::is_iterator_concept_or_better_v<::std::forward_iterator_tag, iterator>,
			::std::forward_iterator_tag, ranges::iterator_concept_t<iterator>>;

		//////
		/// @brief Constructs a default-constructed iterator and an infinity sentinel as the range.
		///
		/// @remarks Not very useful for anything other than generic programming shenanigans.
		constexpr unbounded_view() = default;

		//////
		/// @brief Constructs an unbounded_view using the specified iterator value iterator and an infinity sentinel.
		constexpr unbounded_view(iterator __it) noexcept(::std::is_nothrow_move_constructible_v<iterator>)
		: _M_it(::std::move(__it)) {
		}

		//////
		/// @brief The iterator the unbounded_view was constructed with.
		///
		/// @remarks This function copies the contained iterator.
		constexpr iterator begin() & noexcept {
			if constexpr (::std::is_copy_constructible_v<iterator>) {
				return this->_M_it;
			}
			else {
				return ::std::move(this->_M_it);
			}
		}

		//////
		/// @brief The iterator the unbounded_view was constructed with.
		///
		/// @remarks This function copies the contained iterator.
		constexpr iterator begin() const& noexcept {
			return this->_M_it;
		}

		//////
		/// @brief The iterator the unbounded_view was constructed with.
		///
		/// @remarks This function moves the contained iterator out.
		constexpr iterator begin() && noexcept {
			return ::std::move(this->_M_it);
		}

		//////
		/// @brief The ending sentinel.
		///
		/// @remarks The sentinel is an infinity sentinel that never compares equal to any other thing: in short, any
		/// range composed of [iterator, unreachable_sentinel) will never cease.
		constexpr sentinel end() const noexcept {
			return sentinel {};
		}

		//////
		/// @brief The reconstruct extension point for re-creating this type from its iterator and sentinel.
		constexpr friend unbounded_view tag_invoke(ztd::tag_t<::ztd::ranges::reconstruct>,
			::std::in_place_type_t<unbounded_view>, iterator __iterator,
			sentinel) noexcept(::std::is_nothrow_move_constructible_v<iterator>) {
			return unbounded_view<_It>(::std::move(__iterator));
		}

		//////
		/// @brief Checks whether this ztd::ranges::unbounded_view is empty.
		///
		/// @remarks This can prevent needing to call `begin()` which may be beneficial for move-only iterators. This
		/// is always false for a ztd::ranges::unbounded_view.
		constexpr bool empty() const noexcept {
			return false;
		}

		//////
		/// @brief Produces a copy of the unbounded_view and advances the `begin()` iterator by 1.
		///
		/// @remarks This function call only works if the underlying iterator and sentinal types are copyable.
		[[nodiscard]] constexpr unbounded_view next() const& noexcept(
			::std::is_nothrow_copy_constructible_v<iterator>&& noexcept(
			     ::ztd::ranges::iter_advance(::std::declval<iterator&>()))) {
			auto __it = this->_M_it;
			::ztd::ranges::iter_advance(__it);
			return unbounded_view(::std::move(__it));
		}

		//////
		/// @brief Produces a copy of the unbounded_view and advances the `begin()` iterator by 1.
		///
		/// @remarks This function call can be more efficient and allows working with move-only iterators. This
		/// function call will move the iterators underlying this object.
		[[nodiscard]] constexpr unbounded_view next() && noexcept(
			::std::is_nothrow_move_constructible_v<iterator>&& noexcept(
			     ::ztd::ranges::iter_advance(::std::declval<iterator&>()))) {
			iterator __it = ::std::move(this->_M_it);
			::ztd::ranges::iter_advance(__it);
			return unbounded_view(::std::move(__it));
		}

		//////
		/// @brief Produces a copy of the unbounded_view and advances the `begin()` iterator by `__diff`.
		///
		/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
		///
		/// @remarks This function call only works if the underlying iterator and sentinal types are copyable.
		[[nodiscard]] constexpr unbounded_view next(difference_type __diff) const& noexcept(
			::std::is_nothrow_copy_constructible_v<iterator>&& noexcept(
			     ::ztd::ranges::iter_advance(::std::declval<iterator&>(), __diff))) {
			auto __it = this->_M_it;
			::ztd::ranges::iter_advance(__it, __diff);
			return unbounded_view(::std::move(__it));
		}

		//////
		/// @brief Produces a copy of the unbounded_view and advances the `begin()` iterator by `__diff`.
		///
		/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
		///
		/// @remarks This function call can be more efficient and allows working with move-only iterators. This
		/// function call will move the iterators underlying this object.
		[[nodiscard]] constexpr unbounded_view next(difference_type __diff) && noexcept(
			::std::is_nothrow_move_constructible_v<iterator>&& noexcept(
			     ::ztd::ranges::iter_advance(::std::declval<iterator&>(), __diff))) {
			iterator __it = ::std::move(this->_M_it);
			::ztd::ranges::iter_advance(__it, __diff);
			return unbounded_view(::std::move(__it));
		}

		//////
		/// @brief Produces a copy of the unbounded_view and recedes the `begin()` iterator by `__diff`.
		///
		/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
		///
		/// @remarks This function call requires that the underlying iterator are bidirectional.
		[[nodiscard]] constexpr unbounded_view prev(difference_type __diff = 1) const
			noexcept(::std::is_nothrow_copy_constructible_v<iterator>&& noexcept(
			     ::ztd::ranges::iter_recede(::std::declval<iterator&>(), __diff))) {
			auto __it = this->_M_it;
			::ztd::ranges::iter_recede(__it, __diff);
			return unbounded_view(::std::move(__it));
		}

		//////
		/// @brief Advances the `begin()` iterator of this ztd::ranges::unbounded_view by `__diff` or just `1` if
		/// the argument is not specified.
		///
		/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
		constexpr unbounded_view& advance(difference_type __diff = 1) noexcept(
			noexcept(::ztd::ranges::iter_advance(::std::declval<iterator&>(), __diff))) {
			::ztd::ranges::iter_advance(this->_M_it, __diff);
			return *this;
		}

		//////
		/// @brief Recedes the `begin()` iterator of this ztd::ranges::unbounded_view by `__diff` or just `1` if
		/// the argument is not specified.
		///
		/// @param[in] __diff The amount to move this iterator by. Can be positive or negative.
		///
		/// @remarks This function call requires that the underlying iterator are bidirectional.
		constexpr unbounded_view& recede(difference_type __diff = 1) noexcept(
			noexcept(::ztd::ranges::iter_recede(::std::declval<iterator&>(), ::std::declval<difference_type>()))) {
			::ztd::ranges::iter_recede(this->_M_it, __diff);
			return *this;
		}
	};

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#if ZTD_IS_ON(ZTD_STD_LIBRARY_BORROWED_RANGE)

namespace std { namespace ranges {

	template <typename _It>
	inline constexpr bool enable_borrowed_range<::ztd::ranges::unbounded_view<_It>> = true;

}} // namespace std::ranges

#else

namespace ztd { namespace ranges {

	//////
	/// @brief Mark subranges as appropriately borrowed ranges.
	template <typename _It>
	inline constexpr bool enable_borrowed_range<::ztd::ranges::unbounded_view<_It>> = true;

}} // namespace ztd::ranges

#endif

#include <ztd/epilogue.hpp>

#endif
