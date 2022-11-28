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

Progress & Future Work
======================

This is where the status and progress of the library will be kept up to date. You can also check the `Issue Tracker <https://github.com/soasis/cuneicode/issues>`_ for specific issues and things being worked on!



More Encodings
--------------

More encodings should be supported by this library, to make development for others easier and easier. A good start with be targeting all of :term:`iconv`'s encodings first and foremost. Then, integrating new encodings as individuals voice their need for them.



Arbitrary Indirections
----------------------

Right now, encodings only traffic through the 3 well-known Unicode Encoding forms (UTF-8, UTF-16, UTF-32). It would be far more beneficial to allow connectivity through **any** encoding pair (but with preference shown to any Unicode encoding before taking the first go-between encoding that matches).
