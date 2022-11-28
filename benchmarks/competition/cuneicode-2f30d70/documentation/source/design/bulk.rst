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

Multiple (Bulk) Conversion
==========================

Bulk conversions are a way to convert multiple complete units of input into multiple complete units of output (bytes/code units/code points written and/or state changes). Bulk conversions greedily consume input and only stop on exhaustion or error. Unlike :doc:`single conversions</design/single>`, there is no theoretical or logical upper bound for the output buffer that can be given for an encoding transformation without knowing intimate details about the encoding, especially if the input or output encoding is a variable-width (consumes or outputs 1 or more code units depending on what kind of input goes in). Therefore, it is much more probable and likely that an error will return :cpp:enumerator:`CNC_MCERROR_INSUFFICIENT_OUTPUT`, which must be handled accordingly in order to fully process the given input.

A bulk conversion can be used to simulate a single conversion by arbitrarily limiting the size of an incoming input to 1 code unit, and seeing if it successfully transcodes. If it reports that the input is incomplete, keep incrementing the size of the incoming input 1 more code unit, until it either eventually reports success (:cpp:enumerator:`CNC_MCERROR_OK`) or failure
