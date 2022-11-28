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

``cnc_mcstate_t``
=================

The state object is used during conversions to provide a place for the function to write any temporary data into. This is useful for encodings such as IBM or Microsoft's rendition of SHIFT-JIS, where specific shift sequences are used to provide additional sequences or information for a given input or output string.

.. note::

	For the ``c8``, ``c16``, and ``c32`` prefixed/suffixed functions, it may **not** use the state objects to store "partial writes" or "partial reads" of the data. Any encoding defined as UTF-8, UTF-16, and UTF-32 used through the ``mc`` (:term:`execution encoding`-related) or ``mwc`` (:term:`wide execution encoding`-related) shall also not be used to store partial pieces of the input or partial pieces of the output in order to accumulate information before reading in more data or writing out. If there is insufficient space to do a write to the output, :cpp:enumerator:`CNC_MCERROR_INSUFFICIENT_OUTPUT` must be returned. Similarly, if there is insufficient data and the data is at the very end, then :cpp:enumerator:`CNC_MCERROR_INCOMPLETE_INPUT` must be returned.

	An implementation may define encodings which are not UTF-8, UTF-16, or UTF-32 that **does** perform partial writes, such as a ``"UTF-8-partial"`` or ``"UTF-32-partial"``. But it shall not have the same ``LC_TYPE`` identifier as the UTF-8, UTF-16, or UTF-32 encodings.

.. doxygenunion:: cnc_mcstate_t

.. doxygenfunction:: cnc_mcstate_is_complete

.. doxygenfunction:: cnc_mcstate_set_assume_valid

.. doxygenfunction:: cnc_mcstate_get_assume_valid
