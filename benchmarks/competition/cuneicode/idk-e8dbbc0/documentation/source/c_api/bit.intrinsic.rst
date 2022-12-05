.. =============================================================================
..
.. ztd.idk
.. Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
.. Contact: opensource@soasis.org
..
.. Commercial License Usage
.. Licensees holding valid commercial ztd.idk licenses may use this file in
.. accordance with the commercial license agreement provided with the
.. Software or, alternatively, in accordance with the terms contained in
.. a written agreement between you and Shepherd's Oasis, LLC.
.. For licensing terms and conditions see your agreement. For
.. further information contact opensource@soasis.org.
..
.. Apache License Version 2 Usage
.. Alternatively, this file may be used under the terms of Apache License
.. Version 2.0 (the "License") for non-commercial use; you may not use this
.. file except in compliance with the License. You may obtain a copy of the
.. License at
..
.. 		https://www.apache.org/licenses/LICENSE-2.0
..
.. Unless required by applicable law or agreed to in writing, software
.. distributed under the License is distributed on an "AS IS" BASIS,
.. WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
.. See the License for the specific language governing permissions and
.. limitations under the License.
..
.. =============================================================================>

Bit Intrinsics
==============

Bit intrinsics are functions that map as closely as possible to behavior and functionality in ISAs without needing to deal with the undefined behavior and non-portability of said architectures. It provides vital functionality that can greatly speed up work on specific kinds of bit operations. The provided intrinsics here are a large subset of the most efficient operations, offered in various flavors for ease-of-use.

"Leading" refers to the most significant bit in a given value. This is the "left side" of an integer when writing source code, such that ``0b10`` has a most significant bit of ``1``. "Trailing" refers to the least significant bit in a given value. This is the "left side" of an integer when writing source code, such that ``0b10`` has a least significant bit of ``0``.

.. doxygendefine:: ztdc_count_ones

.. doxygendefine:: ztdc_count_zeros

.. doxygendefine:: ztdc_count_leading_zeros

.. doxygendefine:: ztdc_count_trailing_zeros

.. doxygendefine:: ztdc_count_leading_ones

.. doxygendefine:: ztdc_count_trailing_ones

.. doxygendefine:: ztdc_first_leading_zero

.. doxygendefine:: ztdc_first_trailing_zero

.. doxygendefine:: ztdc_first_leading_one

.. doxygendefine:: ztdc_first_trailing_one

.. doxygendefine:: ztdc_rotate_left

.. doxygendefine:: ztdc_rotate_right

.. doxygendefine:: ztdc_has_single_bit

.. doxygendefine:: ztdc_bit_width

.. doxygendefine:: ztdc_bit_ceil

.. doxygendefine:: ztdc_bit_floor
