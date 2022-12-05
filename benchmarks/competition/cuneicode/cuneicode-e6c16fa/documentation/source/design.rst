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

Design Goals and Philosophy
===========================

The goal of this library are to:

- enable people to write new code that can properly handle encoded information, specifically text;
- give them effective means to convert that information in various ways for their own encodings;
- do so without using hidden allocations or other conversions that are not controllable;
- and, allow them to not need to provide pairwise conversions for every encoding pair they care about.

To this end, there are 2 sets of functionality: typed and static conversions which utilize proper input and output buffer types, and untyped conversions which go through a level of indirection and explicitly work on byte-based (``unsigned char``) buffers. Each of the 2 sets of functionality are further subdivided into 2 use cases that users are about:

- Singular Conversions: where a user wants to encode, decode, or transcode one complete unit of information at a time and receive an error when that conversion fails.
- Multiple (bulk) Conversions: where a user wants to encode, decode, or transcode the maximum amount of information possible in a single given call, for speed or throughput reasons.

Finally, to make sure this library is capable of scaling and does not have users hobbling together pairwise function calls for each encoding pair they care about, we provide **automatic** translation through an indirect layer that leverages one of the popular omni-encodings (e.g. Unicode) so that users do not have to provide conversion routines to every possible other encoding: just the ones they care about.

.. toctree::
	:maxdepth: 1

	design/indivisible
	design/function
	design/naming
	design/single
	design/bulk
	design/registry
