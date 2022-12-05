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

#ifndef ZTD_IDK_TO_ADDRESS_HPP
#define ZTD_IDK_TO_ADDRESS_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/type_traits.hpp>

#include <ztd/idk/detail/mark_contiguous.hpp>

#include <memory>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {

		template <typename _Type, typename = void>
		struct __operator_arrow { };

		template <typename _Type>
		struct __operator_arrow<_Type, ::std::void_t<decltype(::std::declval<_Type&>().operator->())>> {
			using type = typename __operator_arrow<decltype(::std::declval<_Type&>().operator->())>::type;
		};

		template <typename _Type>
		struct __extract_first_parameter { };

		template <template <class, class...> typename _Ty, typename _First, typename... _Args>
		struct __extract_first_parameter<_Ty<_First, _Args...>> {
			using type = _First;
		};

		template <typename _Type, typename = void>
		struct __extract_element_type : public __extract_first_parameter<_Type> { };

		template <typename _Type>
		struct __extract_element_type<_Type, ::std::void_t<typename remove_cvref_t<_Type>::element_type>> {
			using type = typename remove_cvref_t<_Type>::element_type;
		};

		template <typename _Type, typename = void>
		struct __is_maybe_std_pointer_traitable : ::std::false_type { };

		template <typename _Type>
		struct __is_maybe_std_pointer_traitable<_Type,
		     ::std::void_t<typename __extract_element_type<remove_cvref_t<_Type>>::type>> : ::std::true_type { };

		template <typename _Type>
		inline constexpr bool __is_maybe_std_pointer_traitable_v = __is_maybe_std_pointer_traitable<_Type>::value;

		template <typename _Type>
		using __detect_std_pointer_traits_to_address
		     = decltype(::std::pointer_traits<_Type>::to_address(::std::declval<_Type&>()));
	} // namespace __idk_detail

	//////
	/// @brief The type of the operator arrow function.
	///
	/// @tparam _Type  The pointer or iterator to attempt to operator arrow type within.
	template <typename _Type>
	using operator_arrow_t = typename __idk_detail::__operator_arrow<_Type>::type;

	//////
	/// @brief Whether or not the given type can have the operator arrow used on it.
	///
	/// @tparam _Type The type to check.
	template <typename _Type, typename = void>
	class is_operator_arrowable : public ::std::integral_constant<bool, ::std::is_pointer_v<_Type>> { };

	//////
	/// @brief Whether or not the given type can have the operator arrow used on it. This is a partial specialized for
	/// overloaded class types.
	///
	/// @tparam _Type The type to check.
	template <typename _Type>
	class is_operator_arrowable<_Type, ::std::void_t<decltype(::std::declval<_Type&>().operator->())>>
	: public ::std::integral_constant<bool,
	       is_operator_arrowable<decltype(::std::declval<_Type&>().operator->())>::value> { };

	//////
	/// @brief An alias of the inner `value` for ztd::is_operator_arrowable.
	template <typename _Type>
	inline constexpr bool is_operator_arrowable_v = is_operator_arrowable<_Type>::value;

	//////
	/// @brief Whether or not the given type is can have to_address (std::pointer_traits<Type>::to_address) called on
	/// it.
	template <typename _Type, typename = void>
	class is_to_addressable
	: public ::std::integral_constant<bool,
	       (::std::is_pointer_v<
	             _Type> && !::std::is_function_v<::std::remove_reference_t<::std::remove_pointer_t<_Type>>>)
	            || is_operator_arrowable_v<::std::remove_reference_t<_Type>>> { };

	//////
	/// @brief Whether or not the given type is can have to_address (std::pointer_traits<Type>::to_address) called on
	/// it.
	template <typename _Type>
	class is_to_addressable<_Type, ::std::enable_if_t<__idk_detail::__is_maybe_std_pointer_traitable_v<_Type>>>
	: public ::std::integral_constant<bool,
	       is_detected_v<__idk_detail::__detect_std_pointer_traits_to_address,
	            _Type> || (!::std::is_function_v<::std::remove_reference_t<_Type>> && is_operator_arrowable_v<::std::remove_reference_t<_Type>>)> {
	};

	//////
	/// @brief A @c _v alias for ztd::is_to_addressable.
	template <typename _Type>
	inline constexpr bool is_to_addressable_v = is_to_addressable<_Type>::value;

	namespace __idk_detail {

		struct __to_address_fn {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_TO_ADDRESS)
			template <typename _Type>
			constexpr auto operator()(_Type&& __ptr_like) const
			     noexcept(noexcept(::std::to_address(::std::forward<_Type>(__ptr_like))))
			          -> decltype(::std::to_address(::std::forward<_Type>(__ptr_like))) {
				return ::std::to_address(::std::forward<_Type>(__ptr_like));
			}
#else
			//////
			/// @brief Identity: returns the pointer value that was input.
			template <typename _Type>
			constexpr _Type* operator()(_Type* __ptr) const noexcept {
				static_assert(!::std::is_function_v<_Type>, "the pointer shall not be function pointer type");
				return __ptr;
			}

			//////
			/// @brief Calls to_address if it's available, or falls back to other means for pointers and other
			/// potentially-contiguous types.
			template <typename _Pointer, ::std::enable_if_t<!::std::is_pointer_v<_Pointer>>* = nullptr>
			constexpr auto operator()(_Pointer& p) const noexcept {
				if constexpr (::ztd::__idk_detail::__mark_contiguous_v<_Pointer>) {
#if ZTD_IS_ON(ZTD_LIBVCXX)
					return (*this)(p._Unwrapped());
#else
					return (*this)(p.operator->());
#endif
				}
				else {
					if constexpr (is_detected_v<__idk_detail::__detect_std_pointer_traits_to_address, _Pointer>) {
						return ::std::pointer_traits<_Pointer>::to_address(p);
					}
					else {
						return (*this)(p.operator->());
					}
				}
			}
#endif
		};

	} // namespace __idk_detail

	inline namespace __fn {
		//////
		/// @brief Calls to_address if it's available, or falls back to other means for pointers and other
		/// potentially-contiguous types. This is an identity function for pointer types.
		///
		/// @returns A pointer type representing the pointer or iterator passed in, if at all possible.
		inline constexpr __idk_detail::__to_address_fn to_address = {};
	} // namespace __fn

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
