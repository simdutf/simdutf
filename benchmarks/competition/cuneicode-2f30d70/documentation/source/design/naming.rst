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

Naming Design and Mapping
=========================

The names of the functions are written in a compressed C-style, which makes them not the most friendly in terms of readability. But, thankfully, all the function names for the strongly-typed single and bulk conversion functions follow the same convention, composed of a number of parts.

The mapping of the prefix/suffix and the name is listed below:

.. list-table:: Relationship Table For Prefix/Suffix
	:widths: auto
	:header-rows: 1
	:stub-columns: 1
	:name: design-naming-encoding.table

	* - Pre\Suffix Name
	  - Type
	  - Encoding Used
	  - Max Output
	* - ``mc``
	  - ``char``
	  - :term:`execution encoding`
	  - ``CNC_MC_MAX``
	* - ``mwc``
	  - ``wchar_t``
	  - :term:`wide execution encoding`
	  - ``CNC_MWC_MAX``
	* - ``c8``
	  - ``ztd_char8_t``
	  - :term:`UTF-8`
	  - ``CNC_C8_MAX``
	* - ``c16``
	  - ``ztd_char16_t``
	  - :term:`UTF-16`
	  - ``CNC_C16_MAX``
	* - ``c32``
	  - ``ztd_char32_t``
	  - :term:`UTF-32`
	  - ``CNC_C32_MAX``
	* - ``cx`` or ``xy``
	  - ``input``/``output``-deduced
	  - ``input``/``output``-deduced
	  - ``output``-dependent

Those parts are as follows:

- ``{prefix}`` - the prefix identifying what character set the function will be converting from.
- ``s`` - if present, this denotes that this is a :doc:`bulk conversion </design/bulk>`; otherwise, it is a :doc:`single conversion </design/single>`.
- ``n`` - if present, this is a function which will optionally take a count value to denote how much space, in number of **elements** (not bytes), is present in the source (input) data.
- ``r`` - if present, this denotes that this is a "restartable" function; i.e., that this function takes a state parameter and operates on no invisible state; otherwise, it is "non-restartable" and creates an automatic storage duration state object internally that will be discarded after the function completes.
- ``to`` - present in all names, simply signifies the start of the next portion of the function name and is help as the English "to", as in "A to B" or "Beef to Buns".
- ``{suffix}`` - the suffix identifying what character set the function will be converting to.
- ``s`` - similar to above, if present, this denotes that the name is a :doc:`bulk conversion </design/bulk>`; otherwise, it is a :doc:`single conversion </design/single>`.
- ``n`` - if present, this is a function which will optionally take a count value to denote how much space, in number of **elements** (not bytes), is present in the destination (output) data.

For example, a function which is named ``c8ntomcn`` effectively reads "UTF-8, with count, converted to Locale-Based Execution Encoding, with count, non-restartable".

The :doc:`maximum output macros </api/constants>` are part of a series of macros used with a function with the appropriately associated suffix / output type. These allow a caller to always have a suitably-sized output buffer for a single complete output operation.
