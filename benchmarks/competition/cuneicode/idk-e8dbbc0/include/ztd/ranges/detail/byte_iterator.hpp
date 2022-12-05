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

#ifndef ZTD_RANGES_DETAIL_BYTE_ITERATOR_HPP
#define ZTD_RANGES_DETAIL_BYTE_ITERATOR_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/ranges/range.hpp>

#include <ztd/idk/endian.hpp>
#include <ztd/idk/type_traits.hpp>

#include <cstddef>
#include <limits>
#include <climits>

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {
		template <typename _Type>
		class __un_mask_type {
		private:
			static_assert(
				::std::is_integral_v<_Type> || ::std::is_enum_v<_Type>, "is a integral or enumeration type");

		public:
			using type = _Type;
		};

		template <typename _Type>
		class __mask_type : public __un_mask_type<remove_cvref_t<_Type>> { };

		template <typename T>
		using __mask_type_t = typename __mask_type<T>::type;

		template <typename _Type, typename _Val>
		_Type __ones_mask_to_right_shift(_Val __val) {
			_Type __shift = 0;
			for (; (__val & 1) == static_cast<_Val>(0); __val >>= 1) {
				++__shift;
			}
			return __shift;
		}

		template <typename _Ref, typename _Byte, typename _Mask = __mask_type_t<_Ref>>
		class __byte_reference {
		private:
			using __base_value_type = _Byte;

		public:
			using value_type     = _Byte;
			using mask_type      = _Mask;
			using reference_type = _Ref;

			__byte_reference(reference_type __ref, mask_type __mask) : _M_ref(__ref), _M_mask(__mask) {
			}

			__byte_reference& operator=(value_type __val) {
				const __base_value_type __shift = __ones_mask_to_right_shift<__base_value_type>(this->_M_mask);
				this->M_ref |= static_cast<__base_value_type>(__val) << __shift;
				return *this;
			}

			value_type value() const {
				const mask_type __shift = __ones_mask_to_right_shift<mask_type>(this->_M_mask);
				return static_cast<value_type>((this->_M_ref & this->_M_mask) >> __shift);
			}

			operator value_type() const {
				return this->value();
			}

		private:
			reference_type _M_ref;
			mask_type _M_mask;
		};

		template <typename _It, endian _Endian = endian::native, typename _Byte = ::std::byte>
		class __byte_iterator {
		private:
			using __base_iterator   = _It;
			using __base_reference  = ranges::iterator_reference_t<__base_iterator>;
			using __base_value_type = ranges::iterator_value_type_t<__base_iterator>;
			using __difference_type = ranges::iterator_difference_type_t<__base_iterator>;
			using __size_type       = ::std::make_unsigned_t<__difference_type>;
			using __value_type      = _Byte;

			static_assert(sizeof(_Byte) <= sizeof(__base_value_type),
				"the 'byte' type selected for the __byte_iterator shall not be larger than the value_type of the "
				"iterator that it is meant to view");

			static inline constexpr __size_type __max_sizeof = sizeof(__base_value_type) / sizeof(__value_type);
			static inline constexpr __size_type __max_sizeof_index = __max_sizeof - 1;

		public:
			using iterator_type   = __base_iterator;
			using difference_type = __difference_type;
			using value_type      = __value_type;
			using reference       = __byte_reference<__base_reference, __base_value_type>;

			__byte_iterator() = default;
			__byte_iterator(iterator_type __it) : __byte_iterator(::std::move(__it), 0) {
			}
			__byte_iterator(iterator_type __it, __size_type __start_at)
			: _M_base_it(::std::move(__it)), _M_progress(__start_at) {
			}

			__byte_iterator operator++(int) const {
				auto __copy = *this;
				++(*this);
				return __copy;
			}

			__byte_iterator& operator++() {
				++this->_M_progress;
				if (this->_M_progress == __max_sizeof_index) {
					++this->_M_base_it;
					this->_M_progress = static_cast<__size_type>(0);
				}
				return *this;
			}

			__byte_iterator operator--(int) const {
				auto __copy = *this;
				--__copy;
				return __copy;
			}

			__byte_iterator& operator--() {
				if (this->_M_progress == static_cast<__size_type>(0)) {
					++this->_M_base_it;
					this->_M_progress = __max_sizeof;
				}
				--this->_M_progress;
				return *this;
			}

			__byte_iterator operator+(difference_type __by) const {
				auto __copy = *this;
				__copy += __by;
				return __copy;
			}

			__byte_iterator& operator+=(difference_type __by) {
				if (__by < static_cast<difference_type>(0)) {
					return this->operator-=(-__by);
				}
				difference_type __n_words = __by % __max_sizeof;
				return *this;
			}

			__byte_iterator operator-(difference_type __by) const {
				auto __copy = *this;
				__copy -= __by;
				return __copy;
			}

			__byte_iterator& operator-=(difference_type __by) {
				if (__by < static_cast<difference_type>(0)) {
					return this->operator+=(-__by);
				}
				return *this;
			}

			difference_type operator-(const __byte_iterator& __right) const {
				difference_type __dist = this->_M_base_it - __right._M_base_it;
				return static_cast<difference_type>(__dist * __max_sizeof);
			}

			reference operator[](difference_type __index) {
				auto __copy = *this;
				__copy += __index;
				return *__copy;
			}

			reference operator*() const {
				const __base_value_type __shift = this->_M_progress * CHAR_BIT;
				__base_value_type __mask = static_cast<__base_value_type>(::std::numeric_limits<value_type>())
					<< __shift;
				__base_reference __val = *this->_M_base_it;
				return reference(__val, __mask);
			}

		private:
			iterator_type _M_base_it;
			__size_type _M_progress;
		};

	} // namespace __rng_detail
	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#endif
