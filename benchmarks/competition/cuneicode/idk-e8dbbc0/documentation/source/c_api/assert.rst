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

Assertions
==========

This API defines 2 assertion macros. One is named ``ZTD_ASSERT``, and the other is named ``ZTD_ASSERT_MESSAGE``. The first takes only one or more conditional tokens, the second takes a mandatory message token as the first parameter, and then one or more conditional parameters.

The user can override the behavior of each of these by defining both of ``ZTD_ASSERT_USER`` and ``ZTD_ASSERT_MESSAGE_USER``.

When :ref:`debug mode is detected<config-ZTD_DEBUG>` and user-defined assertions are not macro-defined, then a default implementation is used. Typically, these:

- check the condition, and if it is true:

  - print (``std::cerr`` or ``fprintf(stderr, ...)``, depending on the language) a message including line, file, etc.; and,
  - exit the program cleanly (``std::terminate`` or ``exit``, depending on the language)

Note that no side-effects should ever go into assertions, because assertions can be compiled to do nothing.

.. doxygendefine:: ZTD_ASSERT

.. doxygendefine:: ZTD_ASSERT_MESSAGE
