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

#ifndef ZTD_RANGES_WORD_ITERATOR_HPP
#define ZTD_RANGES_WORD_ITERATOR_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/range.hpp>
#include <ztd/ranges/reconstruct.hpp>
#include <ztd/ranges/algorithm.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/ebco.hpp>
#include <ztd/idk/to_address.hpp>
#include <ztd/idk/to_underlying.hpp>
#include <ztd/idk/endian.hpp>
#include <ztd/idk/reference_wrapper.hpp>
#include <ztd/idk/detail/math.hpp>
#include <ztd/idk/to_underlying.hpp>

#include <cstddef>
#include <limits>
#include <climits>
#include <cstring>
#include <memory>
#include <optional>
#include <iostream>

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {

		template <typename _Word, typename _Range, bool>
		class __word_iterator_storage : private ebco<_Range> {
		private:
			using __base_t = ebco<_Range>;

		public:
			constexpr __word_iterator_storage() noexcept(::std::is_nothrow_default_constructible_v<_Range>)
			: __base_t() {
			}
			constexpr __word_iterator_storage(_Range&& __range) noexcept(
				::std::is_nothrow_move_constructible_v<_Range>)
			: __base_t(::std::move(__range)) {
			}
			constexpr __word_iterator_storage(const _Range& __range) noexcept(
				::std::is_nothrow_copy_constructible_v<_Range>)
			: __base_t(__range) {
			}

			using __base_t::get_value;
		};

		template <typename _Word, typename _Range>
		class __word_iterator_storage<_Word, _Range, true> : private ebco<_Range> {
		private:
			using __base_t = ebco<_Range>;

		public:
			::std::optional<_Word> _M_val;

			constexpr __word_iterator_storage() noexcept(::std::is_nothrow_default_constructible_v<_Range>)
			: __base_t() {
			}
			constexpr __word_iterator_storage(_Range&& __range) noexcept(
				::std::is_nothrow_move_constructible_v<_Range>&& ::std::is_nothrow_default_constructible_v<_Word>)
			: __base_t(::std::move(__range)), _M_val(::std::nullopt) {
			}
			constexpr __word_iterator_storage(const _Range& __range) noexcept(
				::std::is_nothrow_copy_constructible_v<_Range>&& ::std::is_nothrow_default_constructible_v<_Word>)
			: __base_t(__range), _M_val(::std::nullopt) {
			}

			using __base_t::get_value;
		};

	} // namespace __rng_detail

	//////
	///@brief The sentinel type to be paired with a ztd::ranges::word_iterator
	using word_sentinel = ranges::default_sentinel_t;

	//////
	///@brief An iterator that composes words out of the bits of a provided underlying stored range.
	template <typename _Word, typename _Range, endian _Endian>
	class word_iterator
	: private __rng_detail::__word_iterator_storage<_Word, range_reconstruct_t<remove_cvref_t<_Range>>,
		  is_iterator_input_iterator_v<ranges::range_iterator_t<range_reconstruct_t<remove_cvref_t<_Range>>>>> {
	private:
		using _URange                      = range_reconstruct_t<remove_cvref_t<_Range>>;
		using __base_iterator              = ranges::range_iterator_t<_URange>;
		using __base_sentinel              = range_sentinel_t<_URange>;
		using __base_reference             = iterator_reference_t<__base_iterator>;
		using __maybe_void_base_value_type = iterator_value_type_t<__base_iterator>;
		using __base_value_type            = ::std::conditional_t<::std::is_void_v<__maybe_void_base_value_type> // cf
                    || (!::std::is_arithmetic_v<__maybe_void_base_value_type>                           // cf
                         && !::std::is_same_v<__maybe_void_base_value_type, ::std::byte>),              // cf
               unsigned char, __maybe_void_base_value_type>;
		using __difference_type            = iterator_difference_type_t<__base_iterator>;
		using __size_type                  = iterator_size_type_t<__base_iterator>;
		using __value_type                 = _Word;
		inline constexpr static bool _IsInput = is_iterator_input_iterator_v<__base_iterator>;
		using __base_storage_t                = __rng_detail::__word_iterator_storage<_Word, _URange, _IsInput>;

		static_assert(sizeof(__value_type) >= sizeof(__base_value_type),
			"the 'byte' type selected for the word_iterator must not be larger than the value_type of the "
			"iterator that it is meant to view");
		static_assert((sizeof(__value_type) % sizeof(__base_value_type)) == 0,
			"the 'byte' type selected for the word_iterator must be evenly divisible by the "
			"iterator that it is meant to view");

		static inline constexpr __size_type __base_values_per_word = sizeof(__value_type) / sizeof(__base_value_type);

		template <bool _IsConst>
		class __word_reference {
		private:
			using __cv_value_type = ::std::conditional_t<_IsConst, const _Word, _Word>;
			using __underlying_base_value_type
				= decltype(::ztd::any_enum_or_char_to_underlying(__base_value_type {}));
			using __underlying_word_type = decltype(::ztd::any_enum_or_char_to_underlying(std::declval<_Word>()));
			inline static constexpr __underlying_word_type __base_bits_per_element
				= static_cast<__underlying_word_type>(sizeof(__underlying_base_value_type) * CHAR_BIT);
			inline static constexpr __underlying_word_type __base_lowest_bit_mask
				= static_cast<__underlying_word_type>(__idk_detail::__ce_ipow(2, __base_bits_per_element) - 1);

		public:
			constexpr __word_reference(_URange& __range) noexcept : _M_base_range_ref(__range) {
			}

			template <typename _Value, ::std::enable_if_t<::ztd::always_true_v<_Value> && !_IsConst>* = nullptr>
			constexpr __word_reference& operator=(_Value __maybe_val) noexcept {
				if constexpr (_Endian == endian::native
					&& (endian::native != endian::big && endian::native != endian::little)) {
					static_assert(always_false_constant_v<endian, _Endian>,
						"read value from byte stream to native endianness that is neither little nor big "
						"(byte order is impossible to infer from the standard)");
				}
				static_assert(sizeof(__value_type) <= (sizeof(__base_value_type) * __base_values_per_word),
					"the size of the value type must be less than or equal to the array size");
				__value_type __val = static_cast<__value_type>(__maybe_val);
				__base_value_type __write_storage[__base_values_per_word] {};
				auto __write_storage_first = __write_storage + 0;
				auto __write_storage_last  = __write_storage + __base_values_per_word;
#if ZTD_IS_ON(ZTD_STD_LIBRARY_IS_CONSTANT_EVALUATED)
				if (!::std::is_constant_evaluated()) {
					// just memcpy the data
					::std::memcpy(__write_storage, ::std::addressof(__val), sizeof(value_type));
				}
				else
#endif
				{
					// God's given, handwritten, bit-splittin'
					// one-way """memcpy""". 😵
					__underlying_word_type __bit_value
						= ::ztd::any_enum_or_char_to_underlying(static_cast<value_type>(__val));
					auto __write_storage_it = __write_storage + 0;
					for (::std::size_t __index = 0; __index < __base_values_per_word; ++__index) {
						__underlying_word_type __bit_position
							= static_cast<__underlying_word_type>(__index * __base_bits_per_element);
						__underlying_base_value_type __shifted_bit_value
							= static_cast<__underlying_base_value_type>(__bit_value >> __bit_position);
						*__write_storage_it
							= static_cast<__base_value_type>(__shifted_bit_value & __base_lowest_bit_mask);
						++__write_storage_it;
					}
				}
				if constexpr (_Endian != endian::native) {
					if constexpr (_Endian == endian::big) {
						__rng_detail::__reverse(__write_storage_first, __write_storage_last);
					}
					else {
						// What about middle endian or some such??
						// No way to detect in "constexpr" properly: just cry.
						static_assert(always_false_constant_v<endian, _Endian>);
					}
				}
				auto& __base_range = this->_M_base_range();
				if constexpr (_IsInput) {
					auto __result         = __rng_detail::__copy(__write_storage_first, __write_storage_last,
						        ::ztd::ranges::begin(::std::move(__base_range)),
						        ::ztd::ranges::end(::std::move(__base_range)));
					this->_M_base_range() = reconstruct(::std::in_place_type<_URange>, ::std::move(__result.out));
				}
				else {
					__rng_detail::__copy(__write_storage_first, __write_storage_last,
						::ztd::ranges::begin(__base_range), ::ztd::ranges::end(__base_range));
				}
				return *this;
			}

			constexpr __value_type value() const noexcept {
				if constexpr (_Endian == endian::native
					&& (endian::native != endian::big && endian::native != endian::little)) {
					static_assert(always_false_constant_v<endian, _Endian>,
						"read value from byte stream to native endianness that is neither little nor big "
						"(byte order is impossible to infer from the standard)");
				}
				__base_value_type __read_storage[__base_values_per_word] {};
				__base_value_type* __read_storage_first = __read_storage + 0;
				::std::size_t __read_storage_size       = ::ztd::ranges::size(__read_storage);
				__value_type __val {};
				if constexpr (_IsInput) {
					// input iterator here (output iterstors cannot be used)
					// to do this kind of work
					// use iterator directly, re-update it when we are done
					// to prevent failure
					auto& __base_range = this->_M_base_range();
					auto __result = __rng_detail::__copy_n_unsafe(::ztd::ranges::begin(::std::move(__base_range)),
						__read_storage_size, __read_storage_first);
					this->_M_base_range() = ranges::reconstruct(::std::in_place_type<_URange>,
						::std::move(__result.in).begin().base(), ::std::move(__base_range).end());
				}
				else {
					// prevent feed-updating iterator through usage here
					// just copy-and-use
					auto __base_it_copy            = ::ztd::ranges::begin(this->_M_base_range());
					[[maybe_unused]] auto __result = __rng_detail::__copy_n_unsafe(
						::std::move(__base_it_copy), __read_storage_size, __read_storage_first);
				}
				if constexpr (_Endian == endian::big) {
					if constexpr ((sizeof(value_type) * CHAR_BIT) > 8) {
						__base_value_type* __read_storage_last = __read_storage + __base_values_per_word;
						__rng_detail::__reverse(__read_storage_first, __read_storage_last);
					}
				}
#if ZTD_IS_ON(ZTD_STD_LIBRARY_IS_CONSTANT_EVALUATED)
				if (!::std::is_constant_evaluated())
#else
				if (false)
#endif
				{
					::std::size_t __read_memory_storage_size = __read_storage_size * sizeof(__base_value_type);
					::std::memcpy(::std::addressof(__val), __read_storage_first, __read_memory_storage_size);
				}
				else {
					// God's given, handwritten, bit-fusin'
					// one-way """memcpy""". 😵
					for (::std::size_t __index = 0; __index < __base_values_per_word; ++__index) {
						__underlying_word_type __bit_value = static_cast<__underlying_word_type>(
							::ztd::any_enum_or_char_to_underlying(__read_storage[__index]));
						__underlying_word_type __bit_position
							= static_cast<__underlying_word_type>(__index * __base_bits_per_element);
						__underlying_word_type __shifted_bit_value = (__bit_value << __bit_position);
						__val |= __shifted_bit_value;
					}
				}
				return __val;
			}

			constexpr operator __value_type() const noexcept {
				return this->value();
			}

		private:
			constexpr _URange& _M_base_range() const noexcept {
				return this->_M_base_range_ref.get();
			}

			::ztd::reference_wrapper<_URange> _M_base_range_ref;
		};

	public:
		//////
		///@brief The underlying range type.
		using range_type = _URange;
		//////
		///@brief The underlying iterator type.
		using iterator = __base_iterator;
		//////
		///@brief The underlying sentinel type.
		using sentinel = __base_sentinel;
		//////
		///@brief The advertised iterator concept.
		using iterator_category
			= ::std::conditional_t<::ztd::ranges::is_concept_or_better_v<::std::random_access_iterator_tag,
			                            ::ztd::ranges::iterator_category_t<__base_iterator>>,
			     ::std::random_access_iterator_tag, ::ztd::ranges::iterator_category_t<__base_iterator>>;
		//////
		///@brief The advertised iterator category.
		using iterator_concept
			= ::std::conditional_t<::ztd::ranges::is_concept_or_better_v<::std::random_access_iterator_tag,
			                            ::ztd::ranges::iterator_concept_t<__base_iterator>>,
			     ::std::random_access_iterator_tag, ::ztd::ranges::iterator_concept_t<__base_iterator>>;
		//////
		///@brief The difference_type for iterator distances.
		using difference_type = __difference_type;
		//////
		///@brief The value_type.
		using value_type = __value_type;
		//////
		///@brief The non-const-qualified reference type.
		using reference = ::std::conditional_t<_IsInput, value_type&, __word_reference<false>>;
		//////
		///@brief The const-qualified reference type.
		using const_reference = ::std::conditional_t<_IsInput, const value_type&, __word_reference<true>>;

	private:
		static constexpr bool _S_deref_noexcept() noexcept {
			if constexpr (_IsInput) {
				return true;
			}
			else {
				return noexcept(reference(::std::declval<range_type&>()));
			}
		}

		static constexpr bool _S_const_deref_noexcept() noexcept {
			if constexpr (_IsInput) {
				return true;
			}
			else {
				return noexcept(const_reference(::std::declval<range_type&>()));
			}
		}

		static constexpr bool _S_copy_noexcept() noexcept {
			return ::std::is_nothrow_copy_constructible_v<iterator>;
		}

		static constexpr bool _S_recede_noexcept() noexcept {
			return noexcept(--::std::declval<iterator&>());
		}

		static constexpr bool _S_advance_noexcept() noexcept {
			return noexcept(++::std::declval<iterator&>());
		}

	public:
		//////
		///@brief Default default constructor.
		constexpr word_iterator() = default;

		//////
		/// @brief Creates a word_iterator that will walk over the specified rage values.
		///
		/// @param[in] __base_range The range to use for iteration.
		constexpr word_iterator(const range_type& __base_range) noexcept(
			::std::is_nothrow_constructible_v<__base_storage_t, const range_type&>)
		: __base_storage_t(__base_range) {
		}
		//////
		/// @brief Creates a word_iterator that will walk over the specified rage values.
		///
		/// @param[in] __base_range The range to use for iteration.
		constexpr word_iterator(range_type&& __base_range) noexcept(
			::std::is_nothrow_constructible_v<__base_storage_t, range_type&&>)
		: __base_storage_t(::std::move(__base_range)) {
		}

		//////
		///@brief Default copy constructor.
		word_iterator(const word_iterator&) = default;
		//////
		///@brief Default move constructor.
		word_iterator(word_iterator&&) = default;
		//////
		///@brief Default copy assignment.
		word_iterator& operator=(const word_iterator&) = default;
		//////
		///@brief Default move assignment.
		word_iterator& operator=(word_iterator&&) = default;

		//////
		///@brief Retrieves the underlying range.
		constexpr range_type range() & noexcept(::std::is_copy_constructible_v<range_type>
			     ? ::std::is_nothrow_copy_constructible_v<range_type>
			     : ::std::is_nothrow_move_constructible_v<range_type>) {
			if constexpr (::std::is_copy_constructible_v<range_type>) {
				return this->__base_storage_t::get_value();
			}
			else {
				return ::std::move(this->__base_storage_t::get_value());
			}
		}

		//////
		///@brief Retrieves the underlying range.
		constexpr range_type range() const& noexcept(::std::is_nothrow_copy_constructible_v<range_type>) {
			return this->__base_storage_t::get_value();
		}

		//////
		///@brief Retrieves the underlying range.
		constexpr range_type range() && noexcept(::std::is_nothrow_move_constructible_v<range_type>) {
			return ::std::move(this->__base_storage_t::get_value());
		}

		//////
		///@brief Shifts the iterator over by +1.
		constexpr word_iterator operator++(int) noexcept(_S_copy_noexcept() && _S_advance_noexcept()) {
			auto __copy = *this;
			++(*this);
			return __copy;
		}

		//////
		///@brief Shifts the iterator over by +1.
		constexpr word_iterator& operator++() noexcept(_S_advance_noexcept()) {
			if constexpr (_IsInput) {
				// force read on next dereference
				this->__base_storage_t::_M_val = ::std::nullopt;
			}
			else {
				auto __first_it = ::ztd::ranges::begin(::std::move(this->__base_storage_t::get_value()));
				auto __last_it  = ::ztd::ranges::end(::std::move(this->__base_storage_t::get_value()));
				::ztd::ranges::iter_advance(__first_it, __base_values_per_word);
				this->__base_storage_t::get_value() = ranges::reconstruct(
					::std::in_place_type<_URange>, ::std::move(__first_it), ::std::move(__last_it));
			}
			return *this;
		}

		//////
		///@brief Shifts an iterator by -1.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::bidirectional_iterator_tag, _Strawman>, word_iterator>
		operator--(int) const noexcept(_S_copy_noexcept() && _S_recede_noexcept()) {
			auto __copy = *this;
			--(*this);
			return __copy;
		}

		//////
		///@brief Shifts an iterator over by -1.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::bidirectional_iterator_tag, _Strawman>, word_iterator&>
		operator--() noexcept {
			__recede(this->__base_storage_t::get_value(), __base_values_per_word);
			return *this;
		}

		//////
		/// @brief Returns an iterator whose positioned is shifted over by `__by` .
		///
		/// @param[in] __by The amount to shift the iterator's position by.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, word_iterator>
		operator+(difference_type __by) const noexcept(_S_copy_noexcept() && _S_advance_noexcept()) {
			auto __copy = *this;
			__copy += __by;
			return __copy;
		}

		//////
		/// @brief Shifts the iterator's position over by `__by` .
		///
		/// @param[in] __by The amount to shift the iterator's position by.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, word_iterator&>
		operator+=(difference_type __by) noexcept(_S_advance_noexcept()) {
			if (__by < static_cast<difference_type>(0)) {
				return this->operator+=(-__by);
			}
			auto __first_it = ::ztd::ranges::begin(::std::move(this->__base_storage_t::get_value()));
			auto __last_it  = ::ztd::ranges::end(::std::move(this->__base_storage_t::get_value()));
			::ztd::ranges::iter_advance(__first_it, __base_values_per_word * __by);
			this->__base_storage_t::get_value() = ranges::reconstruct(
				::std::in_place_type<_URange>, ::std::move(__first_it), ::std::move(__last_it));
			return *this;
		}

		//////
		/// @brief Computes the distance between two iterators.
		///
		/// @param[in] __right The iterator at the right hand side of the subtraction operation.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, difference_type>
		operator-(const word_iterator& __right) const noexcept {
			difference_type __dist = this->__base_storage_t::get_value() - __right.__base_storage_t::get_value();
			return static_cast<difference_type>(__dist * __base_values_per_word);
		}

		//////
		/// @brief Returns an iterator whose positioned is shifted over by `__by` .
		///
		/// @param[in] __by The amount to decrement the iterator's position.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, word_iterator>
		operator-(difference_type __by) const noexcept(_S_copy_noexcept() && _S_recede_noexcept()) {
			auto __copy = *this;
			__copy -= __by;
			return __copy;
		}

		//////
		/// @brief Shifts the position of the iterator over by `__by` .
		///
		/// @param[in] __by The amount to decrement the iterator's position.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, word_iterator&>
		operator-=(difference_type __by) noexcept(_S_recede_noexcept()) {
			if (__by < static_cast<difference_type>(0)) {
				return this->operator+=(-__by);
			}
			auto __first_it = ::ztd::ranges::begin(::std::move(this->__base_storage_t::get_value()));
			auto __last_it  = ::ztd::ranges::end(::std::move(this->__base_storage_t::get_value()));
			__recede(__first_it, __base_values_per_word * __by);
			this->__base_storage_t::get_value() = ranges::reconstruct(
				::std::in_place_type<_URange>, ::std::move(__first_it), ::std::move(__last_it));
			return *this;
		}

		//////
		/// @brief References the value at the offset `__index.`
		///
		/// @param[in] __index The offset to index into.
		///
		/// @remarks If this is an input range, the referenced value comes from internal storage.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, reference>
		operator[](difference_type __index) noexcept(_S_copy_noexcept() && _S_advance_noexcept()) {
			auto __copy = *this;
			__copy += __index;
			return *__copy;
		}

		//////
		/// @brief References the value at the offset `__index.`
		///
		/// @param[in] __index The offset to index into.
		///
		/// @remarks If this is an input range, the referenced value comes from internal storage.
		template <typename _Strawman = range_type>
		constexpr ::std::enable_if_t<
			is_range_iterator_concept_or_better_v<::std::random_access_iterator_tag, _Strawman>, const_reference>
		operator[](difference_type __index) const noexcept(_S_copy_noexcept() && _S_advance_noexcept()) {
			auto __copy = *this;
			__copy += __index;
			return *__copy;
		}

		//////
		/// @brief References to the current value.
		///
		/// @remarks If this is an input range, the value comes from internal storage.
		constexpr reference operator*() noexcept(_S_deref_noexcept()) {
			if constexpr (_IsInput) {
				if (this->__base_storage_t::_M_val == ::std::nullopt) {
					this->_M_read_one();
				}
				return *this->__base_storage_t::_M_val;
			}
			else {
				return reference(this->__base_storage_t::get_value());
			}
		}

		//////
		/// @brief References to the current value.
		///
		/// @remarks If this is an input range, the value comes from internal storage.
		constexpr const_reference operator*() const noexcept(_S_const_deref_noexcept()) {
			if constexpr (_IsInput) {
				if (this->__base_storage_t::_M_val == ::std::nullopt) {
					const_cast<word_iterator*>(this)->_M_read_one();
				}
				return *this->__base_storage_t::_M_val;
			}
			else {
				return const_reference(this->__base_storage_t::get_value());
			}
		}

		//////
		/// @brief Checks if the iterator has not reached the sentinel (the end of the range).
		///
		/// @param[in] __left The iterator to check.
		friend constexpr bool operator==(const word_iterator& __left, const word_sentinel&) noexcept(
			noexcept(__left._M_base_is_empty())) {
			return __left._M_base_is_empty();
		}

		//////
		/// @brief Checks if the iterator has not reached the sentinel (the end of the range).
		///
		/// @param[in] __left The iterator to check.
		friend constexpr bool operator!=(const word_iterator& __left, const word_sentinel&) noexcept(
			noexcept(!__left._M_base_is_empty())) {
			return !__left._M_base_is_empty();
		}

		//////
		/// @brief Checks if the iterator has reached the sentinel (the end of the range).
		///
		/// @param[in] __left The iterator to check.
		/// @param[in] __sen The sentinel.
		friend constexpr bool operator==(const word_sentinel& __sen, const word_iterator& __left) noexcept(
			noexcept(__left == __sen)) {
			return __left == __sen;
		}

		//////
		/// @brief Checks if the iterator has not reached the sentinel (the end of the range).
		///
		/// @param[in] __left The iterator to check.
		/// @param[in] __sen The sentinel.
		friend constexpr bool operator!=(const word_sentinel& __sen, const word_iterator& __left) noexcept(
			noexcept(__left != __sen)) {
			return __left != __sen;
		}

	private:
		constexpr void _M_read_one() noexcept(_S_deref_noexcept()) {
			if constexpr (_IsInput) {
				_Word __read_word              = __word_reference<true>(this->__base_storage_t::get_value());
				this->__base_storage_t::_M_val = ::std::optional<_Word>(__read_word);
			}
		}

		constexpr bool _M_base_is_empty() const noexcept {
			if constexpr (is_detected_v<ranges::detect_adl_empty, range_type>) {
				return ::ztd::ranges::empty(this->__base_storage_t::get_value());
			}
			else {
				return ::ztd::ranges::begin(this->__base_storage_t::get_value())
					== ::ztd::ranges::end(this->__base_storage_t::get_value());
			}
		}
	};

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
