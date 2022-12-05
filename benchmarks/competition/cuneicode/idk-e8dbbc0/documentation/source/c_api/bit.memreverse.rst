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

8-bit Memory Reverse
====================

The 8-bit memory reverse swaps 8-bit bytes, regardless of the size of ``CHAR_BIT`` on the given platform. In order to achieve this in a platform-agnostic manner, it requires that ``CHAR_BIT % 8`` is ``0``. When ``CHAR_BIT`` is larger than 8 (16, 24, 32, 64, and other values that are multiples of 8), each 8-bit byte within an ``unsigned char`` is masked off with ``0xFF << (8 * byte_index)``, and then serialized for storing/loading. ``byte_index`` is a value from [0, ``CHAR_BIT / 8``) and it is swapped with the reverse 8-bit byte, which is computed with ``0xFF << (8 * ((CHAR_BIT / 8) - 1 - byte_index))``.


.. doxygenfunction:: ztdc_memreverse8
	
.. doxygenfunction:: ztdc_memreverse8uN
