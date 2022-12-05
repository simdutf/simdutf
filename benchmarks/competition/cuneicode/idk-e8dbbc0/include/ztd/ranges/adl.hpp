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

#ifndef ZTD_RANGES_ADL_HPP
#define ZTD_RANGES_ADL_HPP

#include <ztd/ranges/version.hpp>

#include <ztd/idk/type_traits.hpp>
#include <ztd/idk/unwrap.hpp>

#include <iterator>
#include <limits>

#include <ztd/prologue.hpp>

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	namespace __rng_detail {
		template <typename _It, typename = void>
		struct __iterator_reference_or_fallback {
			//////
			/// @brief Deliberately undocumented.
			using type = decltype(*::std::declval<_It&>());
		};

		template <typename _It>
		struct __iterator_reference_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::reference>> {
			//////
			/// @brief Deliberately undocumented.
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::reference;
		};
	} // namespace __rng_detail

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
	template <typename _It>
	using iterator_reference_t = ::std::iter_reference_t<_It>;
#else
	template <typename _It>
	using iterator_reference_t =
		typename __rng_detail::__iterator_reference_or_fallback<::std::remove_reference_t<_It>>::type;
#endif

	namespace __rng_detail {

		template <typename _It, typename = void>
		struct __iterator_value_type_from_ref_or_void {
			//////
			/// @brief Deliberately undocumented.
			using type = void;
		};

		template <typename _It>
		struct __iterator_value_type_from_ref_or_void<_It, ::std::void_t<decltype(*::std::declval<_It&>())>> {
			//////
			/// @brief Deliberately undocumented.
			using type = ::std::remove_reference_t<decltype(*::std::declval<_It&>())>;
		};

		template <typename _It, typename = void>
		struct __iterator_value_type_or_fallback {
			//////
			/// @brief Deliberately undocumented.
			using type = typename __iterator_value_type_from_ref_or_void<_It>::type;
		};

		template <typename _It>
		struct __iterator_value_type_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::value_type>> {
			//////
			/// @brief Deliberately undocumented.
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::value_type;
		};

		template <typename _It, typename = void>
		struct iterator_difference_type_or_fallback {
			//////
			/// @brief Deliberately undocumented.
			using type = ::std::ptrdiff_t;
		};

		template <typename _It>
		struct iterator_difference_type_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::difference_type>> {
		private:
			//////
			/// @brief Deliberately undocumented.
			using __maybe_void_type =
				typename ::std::iterator_traits<::std::remove_reference_t<_It>>::difference_type;

		public:
			using type
				= ::std::conditional_t<::std::is_void_v<__maybe_void_type>, ::std::ptrdiff_t, __maybe_void_type>;
		};

		template <typename _It, typename = void>
		struct __iterator_value_type_interception {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
			//////
			/// @brief Deliberately undocumented.
			using type = ::std::iter_value_t<_It>;
#else
			//////
			/// @brief Deliberately undocumented.
			using type = typename __iterator_value_type_or_fallback<::std::remove_reference_t<_It>>::type;
#endif
		};

		template <typename _Container>
		struct __iterator_value_type_interception<::std::back_insert_iterator<_Container>> {
			//////
			/// @brief Deliberately undocumented.
			using type = typename remove_cvref_t<unwrap_t<_Container>>::value_type;
		};

		template <typename _It, typename = void>
		struct __iterator_category_failure {
			using type = ::std::conditional_t<::std::is_pointer_v<remove_cvref_t<_It>>, contiguous_iterator_tag,
				::std::output_iterator_tag>;
		};

		template <typename _It>
		struct __iterator_category_failure<_It,
			::std::void_t<typename ::std::remove_reference_t<_It>::iterator_category>> {
			using type = typename ::std::remove_reference_t<_It>::iterator_category;
		};

		template <typename _It, typename = void>
		struct __iterator_category_or_fallback {
			using type = typename __iterator_category_failure<_It>::type;
		};

		template <typename _It>
		struct __iterator_category_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_category>> {
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_category;
		};

		template <typename _It, typename = void>
		struct __iterator_concept_failure {
			using type = ::std::conditional_t<::std::is_pointer_v<remove_cvref_t<_It>>, contiguous_iterator_tag,
				::std::output_iterator_tag>;
		};

		template <typename _It>
		struct __iterator_concept_failure<_It,
			::std::void_t<typename ::std::remove_reference_t<_It>::iterator_concept>> {
			using type = typename ::std::remove_reference_t<_It>::iterator_concept;
		};

		template <typename _It, typename = void>
		struct __iterator_concept_or_fallback {
			using type = typename __iterator_concept_failure<_It>::type;
		};

		template <typename _It>
		struct __iterator_concept_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_concept>> {
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_concept;
		};

		template <typename _It, typename = void>
		struct __iterator_category_or_concept_or_fallback {
		private:
			using _MaybeType = typename __iterator_category_or_fallback<_It>::type;

		public:
			using type = ::std::conditional_t<::std::is_same_v<_MaybeType, ::std::output_iterator_tag>,
				typename __iterator_concept_or_fallback<remove_cvref_t<_It>>::type, _MaybeType>;
		};

		template <typename _It>
		struct __iterator_category_or_concept_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_category>> {
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_category;
		};

		template <typename _It, typename = void>
		struct __iterator_concept_or_category_or_fallback {
		private:
			using _MaybeType = typename __iterator_concept_or_fallback<_It>::type;

		public:
			using type = ::std::conditional_t<::std::is_same_v<_MaybeType, ::std::output_iterator_tag>,
				typename __iterator_category_or_fallback<remove_cvref_t<_It>>::type, _MaybeType>;
		};

		template <typename _It>
		struct __iterator_concept_or_category_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_concept>> {
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::iterator_concept;
		};

		template <typename _It, typename = void>
		struct __iterator_pointer_or_fallback {
		private:
			using _Reference = iterator_reference_t<::std::remove_reference_t<_It>>;

		public:
			using type = ::std::conditional_t<::std::is_reference_v<_Reference>,
				::std::add_pointer_t<::std::remove_reference_t<_Reference>>, void>;
		};

		template <typename _It>
		struct __iterator_pointer_or_fallback<_It,
			::std::void_t<typename ::std::iterator_traits<::std::remove_reference_t<_It>>::pointer>> {
			using type = typename ::std::iterator_traits<::std::remove_reference_t<_It>>::pointer;
		};

		template <typename _It>
		using __iterator_concept_or_fallback_t =
			typename __iterator_concept_or_category_or_fallback<::std::remove_reference_t<_It>>::type;
	} // namespace __rng_detail

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
	template <typename _It>
	using iterator_difference_type_t = ::std::iter_difference_t<_It>;
#else
	template <typename _It>
	using iterator_difference_type_t =
		typename __rng_detail::iterator_difference_type_or_fallback<::std::remove_reference_t<_It>>::type;
#endif

	template <typename _It>
	using iterator_value_type_t = typename __rng_detail::__iterator_value_type_interception<_It>::type;

	template <typename _It>
	using iterator_size_type_t = ::std::make_signed_t<iterator_difference_type_t<::std::remove_reference_t<_It>>>;

	namespace __rng_detail {

		template <typename _Range>
		using __ssize_diff_type = ::std::conditional_t <                       // cf
			::std::numeric_limits<iterator_difference_type_t<_Range>>::digits // cf
			<::std::numeric_limits<::std::ptrdiff_t>::digits, ::std::ptrdiff_t, iterator_difference_type_t<_Range>>;

#if ZTD_IS_OFF(ZTD_STD_LIBRARY_RANGES)
		namespace __adl {
			using ::std::data;
			using ::std::empty;
			using ::std::size;

			using ::std::begin;
			using ::std::cbegin;
			using ::std::crbegin;
			using ::std::rbegin;

			using ::std::cend;
			using ::std::crend;
			using ::std::end;
			using ::std::rend;

			using ::std::iter_swap;
			using ::std::swap;

			template <typename _Range>
			using __detect_begin = decltype(begin(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_cbegin = decltype(rbegin(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_rbegin = decltype(cbegin(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_crbegin = decltype(crbegin(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_end = decltype(end(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_cend = decltype(rend(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_rend = decltype(cend(::std::declval<_Range>()));

			template <typename _Range>
			using __detect_crend = decltype(crend(::std::declval<_Range>()));

			template <typename _Range>
			constexpr bool __begin_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_begin, _Range>) {
					return noexcept(begin(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().begin());
				}
			}

			template <typename _Range>
			constexpr bool __cbegin_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_cbegin, _Range>) {
					return noexcept(cbegin(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().cbegin());
				}
			}


			template <typename _Range>
			constexpr bool __rbegin_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_rbegin, _Range>) {
					return noexcept(rbegin(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().rbegin());
				}
			}


			template <typename _Range>
			constexpr bool __crbegin_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_crbegin, _Range>) {
					return noexcept(crbegin(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().crbegin());
				}
			}

			template <typename _Range>
			constexpr bool __end_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_end, _Range>) {
					return noexcept(end(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().end());
				}
			}

			template <typename _Range>
			constexpr bool __cend_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_cend, _Range>) {
					return noexcept(cend(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().cend());
				}
			}


			template <typename _Range>
			constexpr bool __rend_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_rend, _Range>) {
					return noexcept(rend(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().rend());
				}
			}


			template <typename _Range>
			constexpr bool __crend_noexcept() noexcept {
				if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
					return true;
				}
				else if constexpr (is_detected_v<__detect_crend, _Range>) {
					return noexcept(crend(::std::declval<_Range>()));
				}
				else {
					return noexcept(::std::declval<_Range>().crend());
				}
			}

			template <typename _It>
			constexpr bool __iter_move_noexcept() noexcept {
				if constexpr (::std::is_lvalue_reference_v<decltype(*::std::declval<_It>())>) {
					return noexcept(::std::move(*::std::declval<_It>()));
				}
				else {
					return noexcept(*::std::declval<_It>());
				}
			}

			class __iter_move_fn {
			public:
				template <typename _It>
				constexpr auto operator()(_It&& __it) const noexcept(__iter_move_noexcept<_It>())
					-> ::std::conditional_t<::std::is_lvalue_reference_v<decltype(*::std::forward<_It>(__it))>,
					     decltype(::std::move(*::std::forward<_It>(__it))), decltype(*::std::forward<_It>(__it))> {
					if constexpr (::std::is_lvalue_reference_v<decltype(*::std::forward<_It>(__it))>) {
						return ::std::move(*::std::forward<_It>(__it));
					}
					else {
						return *::std::forward<_It>(__it);
					}
				}
			};

			class __begin_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__begin_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return (__range + 0);
					}
					else if constexpr (is_detected_v<__detect_begin, _Range>) {
						return begin(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).begin();
					}
				}
			};

			class __cbegin_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__cbegin_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return (__range + 0);
					}
					else if constexpr (is_detected_v<__detect_cbegin, _Range>) {
						return cbegin(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).cbegin();
					}
				}
			};

			class __rbegin_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__rbegin_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return ::std::make_reverse_iterator(__range + ::std::extent_v<remove_cvref_t<_Range>>);
					}
					else if constexpr (is_detected_v<__detect_rbegin, _Range>) {
						return rbegin(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).rbegin();
					}
				}
			};

			class __crbegin_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__crbegin_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return ::std::make_reverse_iterator(__range + ::std::extent_v<remove_cvref_t<_Range>>);
					}
					else if constexpr (is_detected_v<__detect_crbegin, _Range>) {
						return crbegin(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).crbegin();
					}
				}
			};

			class __end_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__end_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return (__range + ::std::extent_v<remove_cvref_t<_Range>>);
					}
					else if constexpr (is_detected_v<__detect_end, _Range>) {
						return end(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).end();
					}
				}
			};

			class __cend_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__cend_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return (__range + ::std::extent_v<remove_cvref_t<_Range>>);
					}
					else if constexpr (is_detected_v<__detect_cend, _Range>) {
						return cend(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).cend();
					}
				}
			};

			class __rend_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__rend_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return ::std::make_reverse_iterator(__range + 0);
					}
					else if constexpr (is_detected_v<__detect_rend, _Range>) {
						return rend(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).rend();
					}
				}
			};

			class __crend_fn {
			public:
				template <typename _Range>
				constexpr decltype(auto) operator()(_Range&& __range) const noexcept(__crend_noexcept<_Range>()) {
					if constexpr (::std::is_array_v<remove_cvref_t<_Range>>) {
						return ::std::make_reverse_iterator(__range + 0);
					}
					else if constexpr (is_detected_v<__detect_crend, _Range>) {
						return crend(::std::forward<_Range>(__range));
					}
					else {
						return ::std::forward<_Range>(__range).crend();
					}
				}
			};

			class __data_fn {
			public:
				template <typename _Range>
				constexpr auto operator()(_Range&& __range) const
					noexcept(noexcept(data(::std::forward<_Range>(__range))))
					     -> decltype(data(::std::forward<_Range>(__range))) {
					return data(::std::forward<_Range>(__range));
				}
			};

			class __size_fn {
			public:
				template <typename _Range>
				constexpr auto operator()(_Range&& __range) const
					noexcept(noexcept(size(::std::forward<_Range>(__range))))
					     -> decltype(size(::std::forward<_Range>(__range))) {
					return size(::std::forward<_Range>(__range));
				}
			};

			class __empty_fn {
			public:
				template <typename _Range>
				constexpr auto operator()(_Range&& __range) const
					noexcept(noexcept(empty(::std::forward<_Range>(__range))))
					     -> decltype(empty(::std::forward<_Range>(__range))) {
					return empty(::std::forward<_Range>(__range));
				}
			};

			class __iter_swap_fn {
			public:
				template <typename _ItLeft, typename _ItRight>
				constexpr auto operator()(_ItLeft&& __left, _ItRight&& __right) const noexcept(
					noexcept(iter_swap(::std::forward<_ItLeft>(__left), ::std::forward<_ItRight>(__right))))
					-> decltype(iter_swap(::std::forward<_ItLeft>(__left), ::std::forward<_ItRight>(__right))) {
					iter_swap(::std::forward<_ItLeft>(__left), ::std::forward<_ItRight>(__right));
				}
			};

			class __swap_fn {
			public:
				template <typename _Left, typename _Right>
				constexpr auto operator()(_Left&& __left, _Right&& __right) const
					noexcept(noexcept(swap(::std::forward<_Left>(__left), ::std::forward<_Right>(__right))))
					     -> decltype(swap(::std::forward<_Left>(__left), ::std::forward<_Right>(__right))) {
					swap(::std::forward<_Left>(__left), ::std::forward<_Right>(__right));
				}
			};
		} // namespace __adl
#endif
	} // namespace __rng_detail

	inline namespace __fn {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
		inline constexpr auto& begin     = ::std::ranges::begin;
		inline constexpr auto& rbegin    = ::std::ranges::rbegin;
		inline constexpr auto& cbegin    = ::std::ranges::cbegin;
		inline constexpr auto& crbegin   = ::std::ranges::crbegin;
		inline constexpr auto& end       = ::std::ranges::end;
		inline constexpr auto& rend      = ::std::ranges::rend;
		inline constexpr auto& cend      = ::std::ranges::cend;
		inline constexpr auto& crend     = ::std::ranges::crend;
		inline constexpr auto& size      = ::std::ranges::size;
		inline constexpr auto& data      = ::std::ranges::data;
		inline constexpr auto& empty     = ::std::ranges::empty;
		inline constexpr auto& swap      = ::std::ranges::swap;
		inline constexpr auto& iter_swap = ::std::ranges::iter_swap;
		inline constexpr auto& iter_move = ::std::ranges::iter_move;
#else
		inline constexpr __rng_detail::__adl::__begin_fn begin {};
		inline constexpr __rng_detail::__adl::__rbegin_fn rbegin {};
		inline constexpr __rng_detail::__adl::__cbegin_fn cbegin {};
		inline constexpr __rng_detail::__adl::__crbegin_fn crbegin {};
		inline constexpr __rng_detail::__adl::__end_fn end {};
		inline constexpr __rng_detail::__adl::__rend_fn rend {};
		inline constexpr __rng_detail::__adl::__cend_fn cend {};
		inline constexpr __rng_detail::__adl::__crend_fn crend {};
		inline constexpr __rng_detail::__adl::__size_fn size {};
		inline constexpr __rng_detail::__adl::__data_fn data {};
		inline constexpr __rng_detail::__adl::__empty_fn empty {};
		inline constexpr __rng_detail::__adl::__swap_fn swap {};
		inline constexpr __rng_detail::__adl::__iter_swap_fn iter_swap {};
		inline constexpr __rng_detail::__adl::__iter_move_fn iter_move {};
#endif
	} // namespace __fn

	template <typename _Range>
	using detect_adl_size = decltype(::ztd::ranges::size(::std::declval<::std::add_lvalue_reference_t<_Range>>()));

	template <typename _Range>
	using detect_adl_empty = decltype(::ztd::ranges::empty(::std::declval<::std::add_lvalue_reference_t<_Range>>()));

	template <typename _Range>
	using detect_adl_begin = decltype(::ztd::ranges::begin(::std::declval<_Range>()));

	template <typename _Range>
	using detect_adl_end = decltype(::ztd::ranges::end(::std::declval<_Range>()));

	template <typename _Ty>
	struct is_range
	: ::std::integral_constant<bool, is_detected_v<detect_adl_begin, _Ty> && is_detected_v<detect_adl_end, _Ty>> { };

	template <typename _Ty>
	inline constexpr bool is_range_v = is_range<_Ty>::value;

	template <typename _It>
	using iterator_rvalue_reference_t = decltype(::ztd::ranges::iter_move(::std::declval<_It&>()));

#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
	template <typename _Range>
	using range_iterator_t = ::std::ranges::iterator_t<_Range>;

	template <typename _Range>
	using range_sentinel_t = ::std::ranges::sentinel_t<_Range>;

	template <typename _Range>
	using range_value_type_t = ::std::ranges::range_value_t<_Range>;

	template <typename _Range>
	using range_rvalue_reference_t = ::std::ranges::range_rvalue_reference_t<_Range>;

	template <typename _Range>
	using range_difference_type_t = ::std::ranges::range_difference_t<_Range>;

	template <typename _Range>
	using range_size_type_t = ::std::ranges::range_size_t<_Range>;
#else
	template <typename _Range>
	using range_iterator_t = ::std::remove_reference_t<decltype(::ztd::ranges::begin(
		::std::declval<::std::add_lvalue_reference_t<_Range>>()))>;

	template <typename _Range>
	using range_sentinel_t = ::std::remove_reference_t<decltype(::ztd::ranges::end(
		::std::declval<::std::add_lvalue_reference_t<_Range>>()))>;

	template <typename _Range>
	using range_value_type_t = iterator_value_type_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_reference_t = iterator_reference_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_rvalue_reference_t = iterator_rvalue_reference_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_difference_type_t = iterator_difference_type_t<range_iterator_t<_Range>>;

	template <typename _Range>
	using range_size_type_t = iterator_size_type_t<range_iterator_t<_Range>>;
#endif

	template <typename _It>
	using iterator_category_t =
		typename __rng_detail::__iterator_category_or_concept_or_fallback<::std::remove_reference_t<_It>>::type;

	template <typename _It>
	using iterator_concept_t = __rng_detail::__iterator_concept_or_fallback_t<_It>;

	template <typename _Tag, typename _ActualTag>
	inline constexpr bool is_concept_or_better_v = ::std::is_base_of_v<_Tag, _ActualTag>;

	template <typename _Tag, typename _It>
	inline constexpr bool is_iterator_concept_or_better_v = is_concept_or_better_v<_Tag, iterator_concept_t<_It>>;


	namespace __rng_detail {

		template <typename _Type, typename... _Args>
		using __detect_iter_advance = decltype(::std::declval<_Type>().advance(::std::declval<_Args>()...));

		template <typename _Type, typename... _Args>
		using __detect_iter_recede = decltype(::std::declval<_Type>().recede(::std::declval<_Args>()...));

		template <typename _It, typename... _Args>
		constexpr bool __advance_noexcept() noexcept {
			if constexpr (is_detected_v<__detect_iter_advance, _It, _Args...>) {
				return noexcept(::std::declval<_It>().advance(::std::declval<_Args>()...));
			}
			else {
				return noexcept(++::std::declval<::std::add_lvalue_reference_t<::std::remove_reference_t<_It>>>());
			}
		}

		template <typename _It, typename... _Args>
		constexpr bool __recede_noexcept() noexcept {
			if constexpr (is_detected_v<__detect_iter_recede, _It, _Args...>) {
				return noexcept(::std::declval<_It>().recede(::std::declval<_Args>()...));
			}
			else {
				return noexcept(--::std::declval<::std::add_lvalue_reference_t<::std::remove_reference_t<_It>>>());
			}
		}

		namespace __adl {
#if ZTD_IS_OFF(ZTD_STD_LIBRARY_RANGES)
			class __ssize_fn {
			public:
				template <typename _Range>
				constexpr auto operator()(_Range&& __range) const
					noexcept(noexcept(size(::std::forward<_Range>(__range))))
					     -> decltype(static_cast<::ztd::ranges::__rng_detail::__ssize_diff_type<_Range>>(
					          size(::std::forward<_Range>(__range)))) {
					return static_cast<::ztd::ranges::__rng_detail::__ssize_diff_type<_Range>>(
						size(::std::forward<_Range>(__range)));
				}
			};
			class __cdata_fn {
			private:
				template <typename _Range>
				using _ConstRange = ::std::conditional_t<::std::is_lvalue_reference_v<_Range>,
					const std::remove_reference_t<_Range>&, const _Range>;

			public:
				template <typename _Range>
				constexpr auto operator()(_Range&& __range) const
					noexcept(noexcept(data(static_cast<_ConstRange<_Range>&&>(__range))))
					     -> ::std::remove_reference_t<::ztd::ranges::range_reference_t<_ConstRange<_Range>>>* {
					return data(static_cast<_ConstRange<_Range>&&>(__range));
				}
			};
#endif
			class __iter_advance_fn {
			public:
				template <typename _It>
				constexpr _It&& operator()(_It&& __it) const noexcept(__advance_noexcept<_It>()) {
					if constexpr (is_detected_v<__rng_detail::__detect_iter_advance, _It>) {
						::std::forward<_It>(__it).advance();
					}
					else {
						++__it;
					}
					return ::std::forward<_It>(__it);
				}

				template <typename _It, typename _Diff>
				constexpr _It&& operator()(_It&& __it, _Diff __diff) const
					noexcept(__advance_noexcept<_It, _Diff>()) {
					if constexpr (is_detected_v<__rng_detail::__detect_iter_advance, _It, _Diff>) {
						::std::forward<_It>(__it).advance(__diff);
					}
					else {
						if constexpr (is_iterator_concept_or_better_v<::std::random_access_iterator_tag,
							              remove_cvref_t<_It>>) {
							__it += __diff;
						}
						else {
							for (; __diff > 0; --__diff) {
								++__it;
							}
						}
					}
					return ::std::forward<_It>(__it);
				}
			};

			class __iter_recede_fn {
			public:
				template <typename _It>
				constexpr _It&& operator()(_It&& __it) const noexcept(__recede_noexcept<_It>()) {
					if constexpr (is_detected_v<__detect_iter_recede, _It>) {
						::std::forward<_It>(__it).recede();
					}
					else {
						--__it;
					}
					return ::std::forward<_It>(__it);
				}

				template <typename _It, typename _Diff>
				constexpr _It&& operator()(_It&& __it, _Diff __diff) const
					noexcept(__recede_noexcept<_It, _Diff>()) {
					if constexpr (is_detected_v<__detect_iter_recede, _It, _Diff>) {
						::std::forward<_It>(__it).recede(__diff);
					}
					else {
						if constexpr (is_iterator_concept_or_better_v<::std::random_access_iterator_tag,
							              remove_cvref_t<_It>>) {
							__it -= __diff;
						}
						else {
							for (; __diff > 0; --__diff) {
								--__it;
							}
						}
						return ::std::forward<_It>(__it);
					}
				}
			};
		} // namespace __adl
	}      // namespace __rng_detail

	inline namespace __fn {
#if ZTD_IS_ON(ZTD_STD_LIBRARY_RANGES)
		inline constexpr auto& ssize = ::std::ranges::ssize;
		inline constexpr auto& cdata = ::std::ranges::cdata;
#else
		inline constexpr __rng_detail::__adl::__cdata_fn cdata {};
		inline constexpr __rng_detail::__adl::__ssize_fn ssize {};
#endif
		inline constexpr __rng_detail::__adl::__iter_advance_fn iter_advance {};
		inline constexpr __rng_detail::__adl::__iter_recede_fn iter_recede {};
	} // namespace __fn

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges


#if ZTD_IS_OFF(ZTD_STD_LIBRARY_BORROWED_RANGE)

namespace ztd { namespace ranges {
	// NOTE: we do not open the ABI namespace here because this is meant to be specialized without it.

	//////
	/// @brief A trait specialized by downstream classes to determine whether or not the type is a borrowed range.
	///
	/// @tparam _Range The range type that may or may not be a borrowed range.
	template <typename _Range>
	inline constexpr bool enable_borrowed_range = false;

}} // namespace ztd::ranges

#endif

namespace ztd { namespace ranges {
	ZTD_RANGES_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @brief Whether or not a given type is a borrowed range or not. Used as a proxy over the standard's
	/// borrowed_range, if it exists.
	///
	/// @tparam _Range The range type that may or may not be a borrowed range.
	///
	/// @remarks This is placed in the low-level IDK library because it has to be used in multiple places, including
	/// the std::span shim if necessary.
	template <typename _Range>
	inline constexpr bool borrowed_range_v =
#if ZTD_IS_ON(ZTD_STD_LIBRARY_BORROWED_RANGE)
		::std::ranges::borrowed_range<_Range>
#else
		::ztd::ranges::is_range_v<_Range> // must have begin/end, at least!
		&& (::std::is_lvalue_reference_v<
		         _Range> || ::ztd::ranges::enable_borrowed_range<::ztd::remove_cvref_t<_Range>>)
#endif
		;

	ZTD_RANGES_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::ranges

#include <ztd/epilogue.hpp>

#include <ztd/epilogue.hpp>

#endif
