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

#ifndef ZTD_IDK_EBCO_HPP
#define ZTD_IDK_EBCO_HPP

#include <ztd/idk/version.hpp>

#include <utility>
#include <type_traits>
#include <memory>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief A class for optimizing the amount of space a certain member of type `_Type` might use.
	///
	/// @tparam _Type The type of the member.
	/// @tparam _Tag A differentiating tag to separate this member from others when there are multiple bases of the
	/// same `_Type`.
	///
	/// @remarks The only reason this class continues to be necessary is because of Microsoft Visual C++. Every other
	/// compiler respects the new C++20 attribute [[no_unique_address]] - it is only Microsoft that explicitly decided
	/// that our opt-in indication that we care more about the object's size is not important.
	template <typename _Type, ::std::size_t _Tag = 0, typename = void>
	class alignas(_Type) ebco {
	private:
		_Type _M_value;

	public:
		//////
		/// @brief Default construction.
		///
		ebco() = default;
		//////
		/// @brief Copy construction.
		///
		ebco(const ebco&) = default;
		//////
		/// @brief Move construction.
		///
		ebco(ebco&&) = default;
		//////
		/// @brief Copy assignment operator.
		///
		ebco& operator=(const ebco&) = default;
		//////
		/// @brief Move assignment operator.
		///
		ebco& operator=(ebco&&) = default;
		//////
		/// @brief Copies the object into storage.
		///
		constexpr ebco(const _Type& __value) noexcept(::std::is_nothrow_copy_constructible_v<_Type>)
		: _M_value(__value) {};
		//////
		/// @brief Moves the object into storage.
		///
		constexpr ebco(_Type&& __value) noexcept(::std::is_nothrow_move_constructible_v<_Type>)
		: _M_value(::std::move(__value)) {};
		//////
		/// @brief Copy assigns into the previous object into storage.
		///
		constexpr ebco& operator=(const _Type& __value) noexcept(::std::is_nothrow_copy_assignable_v<_Type>) {
			this->_M_value = __value;
			return *this;
		}
		//////
		/// @brief Move assigns into the previous object into storage.
		///
		constexpr ebco& operator=(_Type&& __value) noexcept(::std::is_nothrow_move_assignable_v<_Type>) {
			this->_M_value = ::std::move(__value);
			return *this;
		};
		//////
		/// @brief Constructs the object in storage from the given arguments.
		///
		template <typename _Arg, typename... _Args,
		     typename = ::std::enable_if_t<
		          !::std::is_same_v<::std::remove_reference_t<::std::remove_cv_t<_Arg>>,
		               ebco> && !::std::is_same_v<::std::remove_reference_t<::std::remove_cv_t<_Arg>>, _Type>>>
		constexpr ebco(_Arg&& __arg, _Args&&... __args) noexcept(
		     ::std::is_nothrow_constructible_v<_Type, _Arg, _Args...>)
		: _M_value(::std::forward<_Arg>(__arg), ::std::forward<_Args>(__args)...) {
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type& get_value() & noexcept {
			return static_cast<_Type&>(this->_M_value);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type const& get_value() const& noexcept {
			return static_cast<_Type const&>(this->_M_value);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type&& get_value() && noexcept {
			return static_cast<_Type&&>(this->_M_value);
		}
	};

	//////
	/// @brief A partial template specialization for types which can be stored as a base class, enabling more of the
	/// optimization potential.
	template <typename _Type, ::std::size_t _Tag>
	class alignas(_Type) ebco<_Type, _Tag,
	     ::std::enable_if_t<::std::is_class_v<_Type> && !::std::is_final_v<_Type> && !::std::is_reference_v<_Type>>>
	: private _Type {
	public:
		//////
		/// @brief Default construction.
		///
		ebco() = default;
		//////
		/// @brief Copy construction.
		///
		ebco(const ebco&) = default;
		//////
		/// @brief Move construction.
		///
		ebco(ebco&&) = default;
		//////
		/// @brief Copy constructs the object in storage.
		///
		constexpr ebco(const _Type& __value) noexcept(::std::is_nothrow_copy_constructible_v<_Type>)
		: _Type(__value) {};
		//////
		/// @brief Move constructs the object in storage.
		///
		constexpr ebco(_Type&& __value) noexcept(::std::is_nothrow_move_constructible_v<_Type>)
		: _Type(::std::move(__value)) {};
		//////
		/// @brief Constructs the object in storage from the given arguments.
		///
		template <typename _Arg, typename... _Args,
		     typename = ::std::enable_if_t<
		          !::std::is_same_v<::std::remove_reference_t<::std::remove_cv_t<_Arg>>,
		               ebco> && !::std::is_same_v<::std::remove_reference_t<::std::remove_cv_t<_Arg>>, _Type>>>
		constexpr ebco(_Arg&& __arg, _Args&&... __args) noexcept(
		     ::std::is_nothrow_constructible_v<_Type, _Arg, _Args...>)
		: _Type(::std::forward<_Arg>(__arg), ::std::forward<_Args>(__args)...) {
		}

		//////
		/// @brief Copy assignment operator.
		///
		ebco& operator=(const ebco&) = default;
		//////
		/// @brief Move assignment operator.
		///
		ebco& operator=(ebco&&) = default;
		//////
		/// @brief Copy assigns into the previous object into storage.
		///
		constexpr ebco& operator=(const _Type& __value) noexcept(::std::is_nothrow_copy_assignable_v<_Type>) {
			static_cast<_Type&>(*this) = __value;
			return *this;
		}
		//////
		/// @brief Move assigns into the previous object into storage.
		///
		constexpr ebco& operator=(_Type&& __value) noexcept(::std::is_nothrow_move_assignable_v<_Type>) {
			static_cast<_Type&>(*this) = ::std::move(__value);
			return *this;
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type& get_value() & noexcept {
			return static_cast<_Type&>(*this);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type const& get_value() const& noexcept {
			return static_cast<_Type const&>(*this);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type&& get_value() && noexcept {
			return static_cast<_Type&&>(*this);
		}
	};

	//////
	/// @brief A partial specialization for l-value reference types.
	///
	template <typename _Type, ::std::size_t _Tag>
	class alignas(_Type*) ebco<_Type&, _Tag> {
	private:
		_Type* _M_p_value;

	public:
		//////
		/// @brief Default construction.
		///
		ebco() = default;
		//////
		/// @brief Default construction.
		///
		ebco(const ebco&) = default;
		//////
		/// @brief Default construction.
		///
		ebco(ebco&&) = default;
		//////
		/// @brief Copy assignment.
		///
		constexpr ebco& operator=(const ebco& __value) noexcept {
			*(this->_M_p_value) = *(__value._M_p_value);
			return *this;
		}
		//////
		/// @brief Move assignment.
		///
		constexpr ebco& operator=(ebco&& __value) noexcept {
			*(this->_M_p_value) = ::std::move(*(__value._M_p_value));
			return *this;
		}
		//////
		/// @brief Holds onto @p __value 's reference
		///
		constexpr ebco(_Type& __value) noexcept : _M_p_value(::std::addressof(__value)) {};
		//////
		/// @brief Assigns into the underlying stored reference.
		///
		constexpr ebco& operator=(_Type& __value) noexcept {
			*(this->_M_p_value) = __value;
			return *this;
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type& get_value() & noexcept {
			return *(this->_M_p_value);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type const& get_value() const& noexcept {
			return *(this->_M_p_value);
		}

		//////
		/// @brief Gets the wrapped value.
		///
		constexpr _Type&& get_value() && noexcept {
			return ::std::move(*(this->_M_p_value));
		}
	};

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
