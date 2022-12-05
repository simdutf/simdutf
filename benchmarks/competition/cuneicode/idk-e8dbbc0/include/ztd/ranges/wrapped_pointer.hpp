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

#ifndef ZTD_RANGES_WRAPPED_POINTER_HPP
#define ZTD_RANGES_WRAPPED_POINTER_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/to_address.hpp>
#include <ztd/idk/contiguous_iterator_tag.hpp>
#include <ztd/idk/unwrap.hpp>

#include <utility>
#include <iterator>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	template <typename _Type>
	class wrapped_pointer {
	private:
		template <typename>
		friend class ::ztd::ranges::wrapped_pointer;

		template <typename _RightType>
		inline static constexpr bool __is_non_const_other_v
			= !::std::is_same_v<wrapped_pointer<_Type>,
			       wrapped_pointer<_RightType>> && ::std::is_const_v<_Type> && !::std::is_const_v<_RightType>;

		using __unwrapped_type = decltype(ztd::unwrap(::std::declval<_Type&>()));

	public:
		using iterator_category = ::ztd::contiguous_iterator_tag;
		using value_type        = ::ztd::remove_cvref_t<__unwrapped_type>;
		using element_type      = ::std::remove_reference_t<__unwrapped_type>;
		using reference         = ::std::add_lvalue_reference_t<__unwrapped_type>;
		using pointer           = ::std::add_pointer_t<element_type>;
		using difference_type   = decltype(::std::declval<const pointer&>() - ::std::declval<const pointer&>());

		constexpr wrapped_pointer() : wrapped_pointer(nullptr) {
		}
		constexpr wrapped_pointer(pointer __ptr) : _M_ptr(__ptr) {
		}
		template <typename _RightType,
			::std::enable_if_t<__is_non_const_other_v<_RightType>, ::std::nullptr_t> = nullptr>
		constexpr wrapped_pointer(const wrapped_pointer<_RightType>& __right) noexcept : _M_ptr(__right._M_ptr) {
		}
		template <typename _RightType,
			::std::enable_if_t<::std::is_const_v<_Type> && !::std::is_const_v<_RightType>,
			     ::std::nullptr_t> = nullptr>
		constexpr wrapped_pointer(wrapped_pointer<_RightType>&& __right) noexcept
		: _M_ptr(::std::move(__right._M_ptr)) {
		}
		constexpr wrapped_pointer(const wrapped_pointer&)            = default;
		constexpr wrapped_pointer(wrapped_pointer&&)                 = default;
		constexpr wrapped_pointer& operator=(const wrapped_pointer&) = default;
		constexpr wrapped_pointer& operator=(wrapped_pointer&&)      = default;

		constexpr explicit operator bool() const noexcept {
			return this->_M_ptr != nullptr;
		}

		constexpr pointer base() const noexcept {
			return this->_M_ptr;
		}

		constexpr reference operator[](difference_type __index) const noexcept {
			pointer __ptr = this->_M_ptr + __index;
			return ztd::unwrap(*__ptr);
		}

		constexpr reference operator*() const noexcept {
			return ztd::unwrap(*this->_M_ptr);
		}

		constexpr pointer operator->() const noexcept {
			return ::std::addressof(ztd::unwrap(*this->_M_ptr));
		}

		constexpr wrapped_pointer& operator++() noexcept {
			++(this->_M_ptr);
			return *this;
		}

		constexpr wrapped_pointer operator++(int) noexcept {
			auto __copy = *this;
			++__copy;
			return __copy;
		}

		constexpr wrapped_pointer& operator--() noexcept {
			--(this->_M_ptr);
			return *this;
		}

		constexpr wrapped_pointer operator--(int) noexcept {
			auto __copy = *this;
			--__copy;
			return __copy;
		}

		constexpr wrapped_pointer& operator+=(difference_type __right) noexcept {
			this->_M_ptr += __right;
			return *this;
		}

		constexpr wrapped_pointer& operator-=(difference_type __right) noexcept {
			this->_M_ptr -= __right;
			return *this;
		}

		constexpr wrapped_pointer operator+(difference_type __right) const noexcept {
			return wrapped_pointer(this->_M_ptr + __right);
		}

		constexpr wrapped_pointer operator-(difference_type __right) const noexcept {
			return wrapped_pointer(this->_M_ptr - __right);
		}

		friend constexpr pointer to_address(const wrapped_pointer& __wrapped) noexcept {
			return ztd::unwrap_iterator(__wrapped._M_ptr);
		}

	private:
		pointer _M_ptr;
	};

	template <typename _LeftType, typename _RightType>
	constexpr bool operator==(const wrapped_pointer<_LeftType>& __left, const wrapped_pointer<_RightType>& __right) {
		return __left.base() == __right.base();
	}

	template <typename _RightType>
	constexpr bool operator==(::std::nullptr_t __left, const wrapped_pointer<_RightType>& __right) {
		return __left == __right.base();
	}

	template <typename _LeftType>
	constexpr bool operator==(const wrapped_pointer<_LeftType>& __left, ::std::nullptr_t __right) {
		return __left.base() == __right;
	}

	template <typename _LeftType, typename _RightType>
	constexpr bool operator!=(const wrapped_pointer<_LeftType>& __left, const wrapped_pointer<_RightType>& __right) {
		return __left.base() != __right.base();
	}

	template <typename _RightType>
	constexpr bool operator!=(::std::nullptr_t __left, const wrapped_pointer<_RightType>& __right) {
		return __left != __right.base();
	}

	template <typename _LeftType>
	constexpr bool operator!=(const wrapped_pointer<_LeftType>& __left, ::std::nullptr_t __right) {
		return __left.base() != __right;
	}

	template <typename _LeftType, typename _RightType>
	constexpr typename wrapped_pointer<_LeftType>::difference_type operator-(
		const wrapped_pointer<_LeftType>& __left, const wrapped_pointer<_RightType>& __right) noexcept {
		return __left.base() - __right.base();
	}

	template <typename _Type>
	constexpr auto to_mutable_iter(const wrapped_pointer<const _Type>& __value) noexcept {
		using _Ptr = typename wrapped_pointer<_Type>::pointer;
		return wrapped_pointer<_Type>(const_cast<_Ptr>(__value.base()));
	}

	template <typename _Type>
	constexpr auto to_mutable_iter(wrapped_pointer<const _Type>& __value) noexcept {
		using _Ptr = typename wrapped_pointer<_Type>::pointer;
		return wrapped_pointer<_Type>(const_cast<_Ptr>(__value.base()));
	}

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_

}} // namespace ztd::ranges

namespace std {

	template <typename _Type>
	struct pointer_traits<::ztd::ranges::wrapped_pointer<_Type>> {
		using pointer         = typename ::ztd::ranges::wrapped_pointer<_Type>::pointer;
		using element_type    = typename ::ztd::ranges::wrapped_pointer<_Type>::element_type;
		using difference_type = typename ::ztd::ranges::wrapped_pointer<_Type>::difference_type;

		static constexpr pointer pointer_to(element_type& __iter) noexcept {
			return ::std::addressof(__iter);
		}

		static constexpr pointer to_address(const ::ztd::ranges::wrapped_pointer<_Type>& __iter) noexcept(
		     noexcept(::ztd::to_address(__iter.base()))) {
			return ::ztd::to_address(__iter.base());
		}
	};

} // namespace std

#endif
