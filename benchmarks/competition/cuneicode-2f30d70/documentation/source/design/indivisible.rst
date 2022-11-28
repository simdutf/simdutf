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

Indivisible Unit of Work
========================

When doing transcoding, in order to properly develop an algorithm that scales across all encodings and similar, one needs to be able to define certain things about the input, how it is consumed, how any related state is managed, and what outputs - if any - are created. The central way to describe this is with the concept of an *indivisible Unit of work*.

An *indivisible unit* is the smallest possible input, as defined by the input encoding, that:

- can produce one or more outputs;
- and/or, perform a transformation of any internal state.

The conversion of these indivisible units is called an *indivisible unit of work*, and they are used to complete all encoding operations. One or more of the following options must hold if an indivisible unit of work is attempted:

- enough input is consumed to perform an output or change the internal state; 
- enough input is consumed to output is written from consuming input, or from the internal state which causes the internal state to change;
- or, an error occurs and both the input and output do not change relative to the last completed indivisible unit of work, if any.

If the third condition happens, then neither the first or the second condition may happen. The state - managed through the mbstate_t pointer - may or may not change during any of these operations, and may be left in an indeterminate state after an error occurs. For the multi unit functions, the process acts as if it completes one indivisible unit of work repeatedly. When an error occurs, only the input successfully consumed and the output successfully written according to the last indivisible unit of work are reflected in the output values: no other values are written.
