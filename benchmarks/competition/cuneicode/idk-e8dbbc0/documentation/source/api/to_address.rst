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

to_address
==========

Utility extension point to get the address of either a pointer (which is already an address) or an iterator type (which must be a contiguous iterator).

.. note:: This does not actually check if the iterator is contiguous. It just checks if the input type is either a pointer or something which can be ``operator->()``, which implies it must yield an in-language pointer type eventually.

.. doxygenclass:: ztd::is_operator_arrowable

.. doxygenvariable:: ztd::is_operator_arrowable_v

.. doxygenclass:: ztd::is_to_addressable

.. doxygenvariable:: ztd::is_to_addressable_v

.. doxygenvariable:: ztd::to_address
