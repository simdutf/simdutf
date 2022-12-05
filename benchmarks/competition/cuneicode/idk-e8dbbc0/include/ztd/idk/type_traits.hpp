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

#ifndef ZTD_IDK_TYPE_TRAITS_HPP
#define ZTD_IDK_TYPE_TRAITS_HPP

#include <ztd/idk/version.hpp>

#include <ztd/idk/charN_t.hpp>

#include <type_traits>

#include <ztd/prologue.hpp>

namespace ztd {
	ZTD_IDK_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __idk_detail {
		template <typename T, template <typename...> class Templ>
		struct __is_specialization_of_impl : ::std::false_type { };
		template <typename... T, template <typename...> class Templ>
		struct __is_specialization_of_impl<Templ<T...>, Templ> : ::std::true_type { };

		template <typename _It>
		using __weakly_incrementable_test = decltype(++::std::declval<_It&>());

		template <typename _It>
		using __weakly_decrementable_test = decltype(--::std::declval<_It&>());

	} // namespace __idk_detail

	//////
	/// @brief A type that accepts any template type and produces a @c std::false_type derived class.
	template <typename>
	class always_false : public ::std::integral_constant<bool, false> { };

	//////
	/// @brief A type that accepts any template type and an object and produces a @c std::false_type derived class.
	template <typename _Type, _Type>
	class always_false_constant : public always_false<_Type> { };

	//////
	/// @brief A @c _v alias for the value of ztd::always_false_constant.
	template <typename _Type, _Type _Val>
	inline constexpr bool always_false_constant_v = always_false_constant<_Type, _Val>::value;

	//////
	/// @brief An alias for a @c std::size_t -based ztd::always_false_constant.
	template <::std::size_t _Val>
	using always_false_index = always_false_constant<::std::size_t, _Val>;

	//////
	/// @brief A @c _v alias for the value of ztd::always_false_index.
	template <::std::size_t _Val>
	inline constexpr bool always_false_index_v = always_false_index<_Val>::value;

	//////
	/// @brief A @c _v alias for the value of ztd::always_false.
	template <typename _Type>
	inline constexpr bool always_false_v = always_false<_Type>::value;

	//////
	/// @brief A type that accepts any template type and produces a @c std::true_type derived class.
	template <typename>
	class always_true : public ::std::integral_constant<bool, true> { };

	//////
	/// @brief A @c _v alias for ztd::always_true.
	template <typename _Type>
	inline constexpr bool always_true_v = always_true<_Type>::value;

	//////
	/// @brief Removes const, volatile, and references (both l-value and r-value references) from the given type.
	template <typename _Type>
	using remove_cvref = ::std::remove_cv<::std::remove_reference_t<_Type>>;

	//////
	/// @brief A typename `_t` alias for ztd::remove_cvref.
	template <typename _Type>
	using remove_cvref_t = typename remove_cvref<_Type>::type;

	//////
	/// @brief Removes volatile, and references (both l-value and r-value references) from the given type.
	template <typename _Type>
	using remove_vref = ::std::remove_volatile_t<::std::remove_reference_t<_Type>>;

	//////
	/// @brief A typename `_t` alias for ztd::remove_vref.
	template <typename _Type>
	using remove_vref_t = typename remove_vref<_Type>::type;

	// clang-format off

	//////
	/// @brief Checks if the given type is one of the plain character types.
	template <typename _Type>
	class is_character : public ::std::integral_constant<bool,
		::std::is_same_v<_Type, char> || ::std::is_same_v<_Type, wchar_t> ||
#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
		::std::is_same_v<_Type, char8_t> ||
#endif
		::std::is_same_v<_Type, unsigned char> ||
		::std::is_same_v<_Type, signed char> ||
		::std::is_same_v<_Type, char16_t> ||
		::std::is_same_v<_Type, char32_t>
	> {};

	//////
	/// @brief Checks if the given type is one of the types that is used as a code unit (unnamed char, wchar_t, char8_t, char16_t, and char32_t).
	template <typename _Type>
	class is_code_unit : public ::std::integral_constant<bool,
		::std::is_same_v<_Type, char> ||
		::std::is_same_v<_Type, wchar_t> ||
#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
		::std::is_same_v<_Type, char8_t> ||
#endif
		::std::is_same_v<_Type, char16_t> ||
		::std::is_same_v<_Type, char32_t>
	> {};
	// clang-format on

	//////
	/// @brief An @c _v alias for ztd::is_code_unit.
	template <typename _Type>
	inline constexpr bool is_code_unit_v = is_code_unit<_Type>::value;

	//////
	/// @brief Checks if the given type is one of the types that is usable in the standard with the @c std::char_traits
	/// traits type that's used for @c std::string_view , @c std::string and others.
	template <typename _Type>
	class is_char_traitable : public ::std::integral_constant<bool,
	                               ::std::is_same_v<_Type, char> || ::std::is_same_v<_Type, wchar_t> ||
#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
	                                    ::std::is_same_v<_Type, char8_t> ||
#endif
	                                    ::std::is_same_v<_Type, char16_t> || ::std::is_same_v<_Type, char32_t>> {
	};
	// clang-format on

	//////
	/// @brief An @c _v alias for ztd::is_character.
	template <typename _Type>
	inline constexpr bool is_character_v = is_character<_Type>::value;

	//////
	/// @brief An @c _v alias for ztd::is_char_traitable.
	template <typename _Type>
	inline constexpr bool is_char_traitable_v = is_char_traitable<_Type>::value;

	//////
	/// @brief Checks if the given type is a pointer, and that is pointers to a character type.
	template <typename _Type>
	class is_character_pointer : public ::std::integral_constant<bool,
	                                  ::std::is_pointer_v<::ztd::remove_cvref_t<_Type>>&& // cf
	                                  ::ztd::is_character_v<::std::remove_pointer_t<::ztd::remove_cvref_t<_Type>>>> {
	};

	//////
	/// @brief An @c _v alias for ztd::is_character_pointer.
	template <typename _Type>
	inline constexpr bool is_character_pointer_v = is_character_pointer<_Type>::value;

	//////
	/// @brief Checks whether the given full, complete type from the first argument is related to the raw template name
	/// provided in the second.
	template <typename T, template <typename...> class Templ>
	using is_specialization_of = __idk_detail::__is_specialization_of_impl<remove_cvref_t<T>, Templ>;

	//////
	/// @brief A @c _v alias for ztd::is_specialization_of.
	template <typename T, template <typename...> class Templ>
	inline constexpr bool is_specialization_of_v = is_specialization_of<T, Templ>::value;

	//////
	/// @brief A class to be used for the "detection idiom". Provides @c value_t for the true_type/false_type
	/// dichotomy and provides @c type for the detected type.
	///
	/// @remarks This is more efficient and useful at the member declarations level, especially when needing to
	/// dispatch to functionality that may or may not exist in wrapped or base classes.
	template <typename _Default, typename _Void, template <typename...> typename _Op, typename... _Args>
	class detector {
	public:
		//////
		/// @brief The type that provides the @c value static member variable.
		using value_t = ::std::false_type;
		//////
		/// @brief The type chosen from the detection operation.
		using type = _Default;
	};

	//////
	/// @brief A partial template specialization wfor when the detection operation applied to the given argments is
	/// sucucessful.
	template <typename _Default, template <typename...> typename _Op, typename... _Args>
	class detector<_Default, ::std::void_t<_Op<_Args...>>, _Op, _Args...> {
	public:
		//////
		/// @brief The type that provides the @c value static member variable.
		using value_t = ::std::true_type;
		//////
		/// @brief The type chosen from the detection operation.
		using type = _Op<_Args...>;
	};

	//////
	/// @brief A class specifically for the case where the detection idiom cannot detect the requirements.
	class nonesuch {
	public:
		//////
		/// @brief No destruction allowed.
		///
		~nonesuch() = delete;
		//////
		/// @brief No construction allowed.
		///
		nonesuch(nonesuch const&) = delete;
		//////
		/// @brief No assignment allowed.
		///
		nonesuch& operator=(nonesuch const&) = delete;
	};

	//////
	/// @brief A commonly-used alias for getting a @c true_type or @c false_type indicating whether the
	/// operation was successful.
	template <template <typename...> typename _Op, typename... _Args>
	using is_detected = typename detector<nonesuch, void, _Op, _Args...>::value_t;

	//////
	/// @brief A @c _v shortcut for ztd::is_detected.
	template <template <typename...> typename _Op, typename... _Args>
	inline constexpr bool is_detected_v = is_detected<_Op, _Args...>::value;

	//////
	/// @brief A `_t` shortcut for using the ztd::detector to provide either ztd::nonsuch or the given type as yielded
	/// by the operation applied to the arguments.
	template <template <typename...> typename _Op, typename... _Args>
	using detected_t = typename detector<nonesuch, void, _Op, _Args...>::type;

	//////
	/// @brief A shortcut for using the ztd::detector to provide either @c _Default or the given type as yielded
	/// by the operation applied to the arguments.
	template <typename _Default, template <typename...> typename _Op, typename... _Args>
	using detected_or = detector<_Default, void, _Op, _Args...>;

	//////
	/// @brief A type for giving the exact same type out as was put in.
	template <typename _Type>
	class type_identity {
	public:
		//////
		/// The provided type.
		using type = _Type;
	};

	//////
	/// @brief A `_t` typename alias for ztd::type_identity.
	template <typename _Type>
	using type_identity_t = typename type_identity<_Type>::type;

	template <typename _From, typename _To>
	using is_nothrow_convertible =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_IS_NOTHROW_CONVERTIBLE)
	     ::std::is_nothrow_convertible<_From, _To>;
#else
	     ::std::integral_constant<bool, noexcept(static_cast<_To>(::std::declval<_From>()))>;
#endif

	template <typename _From, typename _To>
	inline constexpr bool is_nothrow_convertible_v =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_IS_NOTHROW_CONVERTIBLE)
	     ::std::is_nothrow_convertible_v<_From, _To>;
#else
	     is_nothrow_convertible<_From, _To>::value;
#endif

	//////
	/// @brief A detection decltype for use with ztd::is_detected and similar. Checks if the given left-hand type has
	/// an equality operator for the given right-hand type. Const/volatile/reference qualifications do apply if they
	/// are passed in.
	template <typename _Left, typename _Right>
	using detect_equality_comparable = decltype(::std::declval<_Left>() == ::std::declval<_Right>());

	//////
	/// @brief Detects whether a type has a pre-increment operation.
	template <typename _It>
	inline constexpr bool weakly_incrementable_v
	     = ::ztd::is_detected_v<__idk_detail::__weakly_incrementable_test, _It>;

	//////
	/// @brief Detects whether a type has a post-increment operation.
	template <typename _It>
	inline constexpr bool weakly_decrementable_v
	     = ::ztd::is_detected_v<__idk_detail::__weakly_decrementable_test, _It>;

	//////
	/// @brief Detects whether two types have the same sizeof() and alignof() values.
	template <typename _Left, typename _Right>
	inline constexpr bool is_same_sizeof_alignof_v
	     = (sizeof(_Left) == sizeof(_Right)) && (alignof(_Left) == alignof(_Right));

	ZTD_IDK_INLINE_ABI_NAMESPACE_CLOSE_I_
} // namespace ztd

#include <ztd/epilogue.hpp>

#endif
