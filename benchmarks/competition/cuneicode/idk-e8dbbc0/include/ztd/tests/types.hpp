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

#ifndef ZTD_IDK_TESTS_TYPES_HPP
#define ZTD_IDK_TESTS_TYPES_HPP

#include <ztd/version.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <string>
#if ZTD_IS_ON(ZTD_STD_SPACESHIP_COMPARE)
#include <compare>
#endif

namespace ztd::tests {

	template <typename...>
	class type_list;

	struct empty_struct { };
	union empty_union { };
	enum empty_enum {};
	enum class empty_enum_class {};

	template <typename>
	struct some_template { };

	struct some_non_template { };

	struct regular_struct {
		regular_struct() : a(), b() {
		}
		regular_struct(int arg_a) noexcept : regular_struct(arg_a, arg_a) {
		}
		regular_struct(int arg_a, int arg_b) : a(arg_a), b(arg_b) {
		}
		regular_struct(const regular_struct&)            = default;
		regular_struct(regular_struct&&)                 = default;
		regular_struct& operator=(const regular_struct&) = default;
		regular_struct& operator=(regular_struct&&)      = default;
#if ZTD_IS_ON(ZTD_STD_SPACESHIP_COMPARE)
		friend auto operator<=>(const regular_struct&, const regular_struct&) = default;
#endif
		int a;
		int b;
	};

	struct aggressive_aggregate {
		int a;
		int b;
	};

	struct alias_smacker {
		std::string s;
		alias_smacker(const std::string& s) : s(s) {
		}
	};

	struct self_referential_detection {
		self_referential_detection() : a(0), b(0) {
		}
		self_referential_detection(int arg_a) : a(arg_a), b(arg_a + 1) {
		}
		self_referential_detection(const self_referential_detection& right) : a(right.a), b(right.b) {
			self = this;
		}
		self_referential_detection(self_referential_detection&& right) : a(right.a), b(right.b) {
			self = this;
		}
		self_referential_detection& operator=(const self_referential_detection& right) {
			a    = right.a;
			b    = right.b;
			self = this;
			return *this;
		}
		self_referential_detection& operator=(self_referential_detection&& right) {
			a    = right.a;
			b    = right.b;
			self = this;
			return *this;
		}

#if ZTD_IS_ON(ZTD_STD_SPACESHIP_COMPARE)
		friend auto operator<=>(const self_referential_detection&, const self_referential_detection&) = default;
#endif
		int a;
		int b;
		self_referential_detection* self;

		~self_referential_detection() {
			a = 0xDDDDDDDD;
			b = 0xCCCCCCCC;
		}
	};

	inline constexpr int int_holder_i_default = 5;

	struct int_holder {
		int i = int_holder_i_default;
	};

	template <unsigned long long = 0, bool DefaultConstructThrow = false, bool CopyConstructThrow = true,
	     bool CopyAssignThrow = true, bool MoveConstructThrow = false, bool MoveAssignThrow = false>
	struct op_counter {
		static std::size_t default_construct_count;
		static std::size_t copy_count;
		static std::size_t move_count;
		static std::size_t copy_construct_count;
		static std::size_t move_construct_count;
		static std::size_t copy_assign_count;
		static std::size_t move_assign_count;
		static std::size_t destruct_count;
		static std::size_t default_construct_throw_on;
		static std::size_t copy_construct_throw_on;
		static std::size_t copy_assign_throw_on;
		static std::size_t move_construct_throw_on;
		static std::size_t move_assign_throw_on;


		op_counter() noexcept(!DefaultConstructThrow) {
			if constexpr (DefaultConstructThrow) {
				if (default_construct_count == default_construct_throw_on) {
					throw 1;
				}
			}
			++default_construct_count;
		}
		op_counter(const op_counter&) noexcept(!CopyConstructThrow) {
			if constexpr (CopyConstructThrow) {
				if (copy_construct_count == copy_construct_throw_on) {
					throw 1;
				}
			}
			++copy_construct_count;
			++copy_count;
		}
		op_counter(op_counter&&) noexcept(!MoveConstructThrow) {
			if constexpr (MoveConstructThrow) {
				if (move_construct_count == move_construct_throw_on) {
					throw 1;
				}
			}
			++move_construct_count;
			++move_count;
		}
		op_counter& operator=(const op_counter&) noexcept(!CopyAssignThrow) {
			if constexpr (CopyAssignThrow) {
				if (copy_assign_count == copy_assign_throw_on) {
					throw 1;
				}
			}
			++copy_assign_count;
			++copy_count;
			return *this;
		}
		op_counter& operator=(op_counter&&) noexcept(!MoveAssignThrow) {
			if constexpr (MoveAssignThrow) {
				if (move_assign_count == move_assign_throw_on) {
					throw 1;
				}
			}
			++move_assign_count;
			++move_count;
			return *this;
		}
		~op_counter() noexcept {
			++destruct_count;
		}

		static void reset() {
			default_construct_count    = 0;
			copy_count                 = 0;
			move_count                 = 0;
			copy_construct_count       = 0;
			move_construct_count       = 0;
			copy_assign_count          = 0;
			move_assign_count          = 0;
			destruct_count             = 0;
			default_construct_throw_on = std::numeric_limits<std::size_t>::max();
			copy_construct_throw_on    = std::numeric_limits<std::size_t>::max();
			copy_assign_throw_on       = std::numeric_limits<std::size_t>::max();
			move_construct_throw_on    = std::numeric_limits<std::size_t>::max();
			move_assign_throw_on       = std::numeric_limits<std::size_t>::max();
		}
	};

	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::default_construct_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::copy_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::copy_construct_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::copy_assign_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::move_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::move_construct_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::move_assign_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::destruct_count
	     = 0;
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::default_construct_throw_on
	     = std::numeric_limits<std::size_t>::max();
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::copy_construct_throw_on
	     = std::numeric_limits<std::size_t>::max();
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::copy_assign_throw_on
	     = std::numeric_limits<std::size_t>::max();
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::move_construct_throw_on
	     = std::numeric_limits<std::size_t>::max();
	template <unsigned long long id, bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow,
	     bool MoveConstructThrow, bool MoveAssignThrow>
	std::size_t op_counter<id, DefaultConstructThrow, CopyConstructThrow, CopyAssignThrow, MoveConstructThrow,
	     MoveAssignThrow>::move_assign_throw_on
	     = std::numeric_limits<std::size_t>::max();

	template <bool DefaultConstructThrow, bool CopyConstructThrow, bool CopyAssignThrow, bool MoveConstructThrow,
	     bool MoveAssignThrow, bool DestructorThrow>
	struct thrower {
		thrower() noexcept(!DefaultConstructThrow) {
			if constexpr (DefaultConstructThrow) {
				throw "fek - default construct";
			}
		}
		thrower(const thrower&) noexcept(!CopyConstructThrow) {
			if constexpr (CopyConstructThrow) {
				throw "fek - copy construct";
			}
		}
		thrower(thrower&&) noexcept(!MoveConstructThrow) {
			if constexpr (MoveConstructThrow) {
				throw "fek - move construct";
			}
		}
		thrower& operator=(const thrower&) noexcept(!CopyAssignThrow) {
			if constexpr (CopyAssignThrow) {
				throw "fek - copy assign";
			}
			return *this;
		}
		thrower& operator=(thrower&&) noexcept(!MoveAssignThrow) {
			if constexpr (MoveAssignThrow) {
				throw "fek - move assign";
			}
			return *this;
		}

		~thrower() noexcept(!DestructorThrow) {
			if constexpr (DestructorThrow) {
				throw "fek - destructor";
			}
		}
	};

	using no_thrower                         = thrower<false, false, false, false, false, false>;
	using throw_copy_move                    = thrower<false, true, true, true, true, false>;
	using noexcept_copy_throw_move           = thrower<false, false, false, true, true, false>;
	using noexcept_move_throw_copy           = thrower<false, true, true, false, false, false>;
	using noexcept_copy_throw_move_construct = thrower<false, false, false, true, false, false>;
	using destroy_throw                      = thrower<false, false, false, false, false, false>;


	template <unsigned long long = 0, typename ThrowSensitiveData = std::vector<int>>
	struct FailTrigger {
		FailTrigger() : data(1) {
			if (fail)
				throw 1;
		}

		static bool fail;

		ThrowSensitiveData data;
	};

	template <unsigned long long id, typename ThrowSensitiveData>
	bool FailTrigger<id, ThrowSensitiveData>::fail = false;

	struct hell_driver0 {
		constexpr regular_struct* operator&() const {
			return nullptr;
		}
	};

	namespace greedy {
		inline constexpr int X_v_default = 4;

		struct X {
			int v = X_v_default;
		};

		template <typename T>
		X operator==(T, T) {
			return X();
		}

		template <typename T>
		X operator!=(T, T) {
			return X();
		}

		template <typename T>
		X operator<(T, T) {
			return X();
		}

		template <typename T>
		X operator<=(T, T) {
			return X();
		}

		template <typename T>
		X operator>(T, T) {
			return X();
		}

		template <typename T>
		X operator>=(T, T) {
			return X();
		}

		template <typename T>
		X operator-(T, T) {
			return X();
		}

		template <typename T>
		T operator+(T, std::size_t) {
			return T();
		}
	} // namespace greedy


	// This type is CopyInsertable into std::vector<Bomb> so push_back should
	// have the strong exception-safety guarantee.
	struct Bomb {
		Bomb() = default;

		Bomb(const Bomb& b) : armed(b.armed) {
			tick();
		}

		Bomb(Bomb& b) : Bomb(const_cast<const Bomb&>(b)) {
		}

		Bomb(Bomb&& b) noexcept(false) : armed(b.armed) {
			tick();
			b.moved_from = true;
		}

		// std::vector in GCC 4.x tries to use this constructor
		// MSVC is also too dumb, and picks this constructor
		// with T = Bomb because it's v. smart...
		template <typename T>
		Bomb(T&) = delete;

		bool moved_from = false;
		bool armed      = true;

	private:
		void tick() {
			if (armed && ticks++)
				throw 1;
		}

		static int ticks;
	};

	inline int Bomb::ticks = 0;

	struct UnusualCopy {
		UnusualCopy(UnusualCopy&);
	};

	struct AnyAssign {
		template <class T>
		void operator=(T&&);
	};

	struct DelAnyAssign {
		template <class T>
		void operator=(T&&) = delete;
	};

	struct DelCopyAssign {
		DelCopyAssign& operator=(const DelCopyAssign&) = delete;
		DelCopyAssign& operator=(DelCopyAssign&&)      = default;
	};

	struct MO {
		MO(MO&&)            = default;
		MO& operator=(MO&&) = default;
	};

	struct CopyConsOnlyType {
		CopyConsOnlyType(int) {
		}
		CopyConsOnlyType(CopyConsOnlyType&&)                 = delete;
		CopyConsOnlyType(const CopyConsOnlyType&)            = default;
		CopyConsOnlyType& operator=(const CopyConsOnlyType&) = delete;
		CopyConsOnlyType& operator=(CopyConsOnlyType&&)      = delete;
	};

	struct MoveConsOnlyType {
		MoveConsOnlyType(int) {
		}
		MoveConsOnlyType(const MoveConsOnlyType&)            = delete;
		MoveConsOnlyType(MoveConsOnlyType&&)                 = default;
		MoveConsOnlyType& operator=(const MoveConsOnlyType&) = delete;
		MoveConsOnlyType& operator=(MoveConsOnlyType&&)      = delete;
	};

	using unsigned_integer_types_list
	     = type_list<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long
#if ZTD_IS_ON(ZTD___UINT128_T)
	          ,
	          __uint128_t
#endif
#if ZTD_IS_ON(ZTD___UINT256_T)
	          ,
	          __uint256_t
#endif
	          >;

	using character_types_list = type_list<char, signed char, unsigned char,
#if ZTD_IS_ON(ZTD_NATIVE_CHAR8_T)
	     char8_t,
#endif
	     char16_t, char32_t, wchar_t>;

	using scalar_types_list = type_list<char, signed char, unsigned char, // character types
	     short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, // integer types
	     float, double, long double,                                              // floating types
	     bool,                                                                    // boolean type
	     empty_enum, empty_enum_class,                                            // enumerations
	     empty_struct, empty_union,                                               // structures/unions
	     void*, int*, empty_struct*, empty_union*, double*, unsigned char*, char* // pointers
	     >;

} // namespace ztd::tests

#endif
