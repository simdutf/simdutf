.. ============================================================================
..
.. ztd.cuneicode
.. Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
.. Contact: opensource@soasis.org
..
.. Commercial License Usage
.. Licensees holding valid commercial ztd.cuneicode licenses may use this file in
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
.. ========================================================================= ..

Registry-Based Conversions: Resource Handles
============================================

:cpp:type:`cnc_conversion` is first and foremost a handle to a resource. It must be opened/created like one, and destroyed like one. The ``*_new`` flavor of functions perform the allocation automatically using the :doc:`associated registry's heap </api/registry>`. The ``*_open`` flavor fo functions let the user pass in an area of memory (or probe to find out the exact area of memory) to open the handle into. The ``*_open`` flavor of functions is for particularly advanced users who want maximum control over where the type is placed and allocated and is much more complicated to use: users are recommended to use :cpp:func:`cnc_conv_new` and :cpp:func:`cnc_conv_new_n` where appropriate to save on precision handling and hassle.



``cnc_conversion`` Creation
---------------------------

.. doxygentypedef:: cnc_conversion

.. doxygenfunction:: cnc_conv_open

.. doxygenfunction:: cnc_conv_open_n

.. doxygenfunction:: cnc_conv_open_select

.. doxygenfunction:: cnc_conv_open_n_select

.. doxygenfunction:: cnc_conv_new

.. doxygenfunction:: cnc_conv_new_n

.. doxygenfunction:: cnc_conv_new_select

.. doxygenfunction:: cnc_conv_new_n_select

.. doxygenfunction:: cnc_conv_open_c8

.. doxygenfunction:: cnc_conv_open_c8n

.. doxygenfunction:: cnc_conv_open_c8_select

.. doxygenfunction:: cnc_conv_open_c8n_select

.. doxygenfunction:: cnc_conv_new_c8

.. doxygenfunction:: cnc_conv_new_c8n

.. doxygenfunction:: cnc_conv_new_c8_select

.. doxygenfunction:: cnc_conv_new_c8n_select

.. doxygenfunction:: cnc_conv_close

.. doxygenfunction:: cnc_conv_delete
