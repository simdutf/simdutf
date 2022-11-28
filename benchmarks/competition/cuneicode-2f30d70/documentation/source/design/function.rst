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

Function Design and Shape
=========================

It is surprisingly hard to provide the right kind of function to C and C++ that covers all use cases for text. The kinds of things people do with text are varied, but the core of the actions that conversion should cover are:

- Convert as much as as possible, or simply fail (where a conversion either happens or it does not, and if it does not it fails).
	- Most text converion APIs behave in this way, especially when they are responsible for allocating the memory.
	- Converting text "in bulk" is a heavily researched area and text validation "in under 1 instruction per byte" is a thing that has had time heavily invested into it. There are large archives of government data and similar not stored in Unicode, for one reason or another: converting potentially terabytes or petabytes of data may be required.
- Convert, skip over bad input, then resume converting, alternating between skipping bad text and converting text until the input is exhausted.
	- Input sanitizers for non-critical input text may behave this way, although it is noted that this loses information (e.g., some kind of understanding that bad text happened by using explicit `�`.)
- Convert, substitute bad input in the output, then resume converting, alternating between subsitutions and conversions until the input is exhausted.
	- This is a variant of the above style of usage and is used in most places, especially user-facing places, that attempt to handle text failures including Web Browsers such as Apple Safari, Google Chrome, Microsoft Edge, Mozilla Firefox, and more.
- Convert some indivisible unit of text, one at a time, to a known text encoding and feed it to a specific rendering engine.
	- Useful for the guts of a rendering engine that converts either a single code point or a small piece of text into a small buffer and uses it to layout their text.
	- Useful to delineate each code point boundary without having to convert the full text in-memory.
	- Useful for a well-specified form of round-tripping.
- Convert between text encodings that may or may not have a direct code path between them.
	- For example, it is not feasible for someone to need to write a conversion directly from EUC-KR to UTF-8, or TASCII to UTF-16. It should be easy to go from TASCII or EUC-JP or Latin-1 to any other encoding, where reasonable, without needing to write a new encoding path. There are well over 100 encodings in the world: writing by-hand, pairwise conversions between all of them is an infeasible task.

The API must have a general shape and usage to it that enables all of these use cases without causing deep undue burden to software engineers utilizing the library. It also needs to combat some serious issues with preexisting C APIs of the day.

- The C Standard Library only takes one :term:`code unit` at a time. This means they deeply restrict themselves to only a very limited set of encodings.
- libiconv, as defined by POSIX, allows too wide a variety of implementation techniques. Insertion of Byte Order Marks, even when not asked for, is common for certain Unicode types and there exists no agreement between implementations whether to treat data as Big Endian or Little Endian. Furthermore, insertion of invalid characters without the request or approval of the software engineer (``?`` on some GNU derivatives, ``*`` on musl libc) is horrible for cross-platform expansion.
- Platform encoding functions often do not provide adequate error information (Microsoft Windows's ``MultibyteToWideChar`` function does not report how many characters it successfully converted when it returns with an error, leaving the end-user to discard the all output and input if they do not have intimate knowledge of the input data already).

It is a lot of issues we have to fix in one API. We need APIs that:

- allows for custom error handling (to cover the replacement use case and the "skip over bad input" use case);
- allows for fast, bulk conversion (to cover the "convert terabytes of text as fast as possible" archival use case);
- and, allows for a way to convert disparate encodings that may not have a hand-crafted encoding path (to prevent needing to write encodings for `100^2` one-way encoding functions).

The good news is that, while libiconv's specification under POSIX is terrible and too permissive of a wide variety of implementation strategies and failures
