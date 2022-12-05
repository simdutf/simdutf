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

8-bit Endian Load/Store
=======================

The 8-bit loads and stores put values in a format suitable for bit-by-bit transition over the network or to the filesystem. Because it will serialize exactly enough bytes to memory so that it is suitable for transition over the network, it has the general requirement that when it tries to load *N* bit integers it expects exactly *N* bits to be present in the array. Therefore, ``CHAR_BIT % 8`` must be ``0`` and ``N % 8`` must be ``0``.

When ``CHAR_BIT`` is larger than 8 (16, 24, 32, 64, and other values that are multiples of 8), each 8-bit byte within an ``unsigned char`` is masked off with ``0xFF << (8 * byte_index)``, and then serialized for storing/loading.



Unsigned Variants
-----------------

.. doxygenfunction:: ztdc_store8_leuN

.. doxygenfunction:: ztdc_store8_beuN

.. doxygenfunction:: ztdc_load8_leuN

.. doxygenfunction:: ztdc_load8_beuN

.. doxygenfunction:: ztdc_store8_aligned_leuN

.. doxygenfunction:: ztdc_store8_aligned_beuN

.. doxygenfunction:: ztdc_load8_aligned_leuN

.. doxygenfunction:: ztdc_load8_aligned_beuN




Signed Variants
---------------

.. doxygenfunction:: ztdc_store8_lesN

.. doxygenfunction:: ztdc_store8_besN

.. doxygenfunction:: ztdc_load8_lesN

.. doxygenfunction:: ztdc_load8_besN

.. doxygenfunction:: ztdc_store8_aligned_lesN

.. doxygenfunction:: ztdc_store8_aligned_besN

.. doxygenfunction:: ztdc_load8_aligned_lesN

.. doxygenfunction:: ztdc_load8_aligned_besN
