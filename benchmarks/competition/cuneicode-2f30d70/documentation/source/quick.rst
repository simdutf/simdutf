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

🔨 Quick 'n' Dirty Tutorial (In Progress)
=========================================

.. warning::

	|unfinished_warning|


cuneicode is a C library whose headers work in both C and C++. Its implementation is currently done in C++. To use it, use:

- one of the many CMake methods (`add_subdirectory`, `FetchContent`, or similar)
- directly add and build all the sources to your project

.. warning::

	Adding sources directly to your project is not guaranteed to work in future major revisions, as certain build steps might generate code in the future.

Once the library is appropriately included, you can start using cuneicode.


Simple Conversions
------------------

To convert from UTF-16 to UTF-8, use the appropriately `c8` and `c16`-marked free functions in the library:

.. literalinclude:: ../../examples/documentation/quick/source/simple.conversions-utf16.to.utf8.c
	:language: c
	:start-after: // ========================================================================= //
	:linenos:

We use raw ``printf`` to print the UTF-8 text. It may not appear correctly on a terminal whose encoding which is not UTF-8, which may be the case for older Microsoft terminals, some Linux kernel configurations, and deliberately misconfigured Mac OSX terminals. There are also some other properties that can be gained from the use of the function:

- the amount of data read (using ``initial_input_size`` - ``input_size``);
- the amount of data written out (using ``initial_output_size`` - ``output_size``);
- a pointer to any extra input after the operation (``p_input``);
- and, a pointer to any extra output that was not written to after the operation (``p_output``).

One can convert from other forms of UTF-8/16/32 encodings, and from the :term:`wide execution encodings <wide execution encoding>`/:term:`execution encoding <execution encoding>` (encodings used by default for ``const char[]`` and ``const wchar_t[]`` strings) using the various different :ref:`prefixed-based <design-naming-encoding.table>` functions.
