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

Generic Typed Conversions
=========================

Generic typed conversions rely on the types being put in to determine what encodings and conversions should be done. They are more flexibile when the input is generic, or the user has well-defined source and destination pointers to use with the API. The type-to-prefix/encoding mapping for this function is described in the :doc:`naming documentation </design/naming>`. When using these functions, using ``nullptr`` is ambiguous because the macro/template cannot understand what the to / from pointers should be. In those cases, cast the ``nullptr`` value with ``(CharTypeHere**)nullptr``.


Bulk Conversion Functions
-------------------------

.. doxygendefine:: cnc_cxsntocysn

.. doxygendefine:: cnc_cxsnrtocysn



Single Conversion Functions
---------------------------

.. doxygendefine:: cnc_cxntocyn

.. doxygendefine:: cnc_cxnrtocyn
