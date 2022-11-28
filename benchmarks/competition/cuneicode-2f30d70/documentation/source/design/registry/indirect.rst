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

Indirect Conversions
====================

When a registry :doc:`conversion rountine</design/registry/conversion>` provides an encoding path to a common encoding, but not to each other, it can be difficult to get data in one shape to another. For example, if you only register a conversion routine from SHIFT-JIS to UTF-8, and then from UTF-8 to UTF-32, you have provided no direct path between SHIFT-JIS and UTF-32. But, what if it was possible for the the library to realize that UTF-8 is a possible substract between the two libraries? What if it could automatically detect certain "Universal Encodings", like the Unicode Encodings, and use those as a bridge between 2 disparate encodings?

This is a technique that has been in use for glibc, musl libc, ICU, libiconv, and many encoders for over a decade now. They provide a conversion to a common substrate — generally, Unicode in the form of UTF-8, UTF-16, or UTF-32 (mostly this last one) — and then use it to convert for well over a decade now.


how the :doc:`conversion registry </api/registry>` will bridge the two encodings together without a developer needing to specifically write the encodings through an encoding pair. This is done by utilizing UTF-32 as a go-between for the two functions. This is a technique that is common among text transcoding engines, albeit the process of doing so for other libraries is generally explicit, involved, and sometimes painful.

When :doc:`opening a new conversion routine </api/registry conversion handles>`, use the ``is_indirect`` member and related information on the :cpp:class:`cnc_conversion_info <::cnc_conversion_info>` structure to find out if the conversion has been opened through an intermediate. Note that this is the only time the routines will tell you this: this information may not be accessible later.



Indirect Liaisons
-----------------

Indirect encoding paths will not link together arbitrarily long encoding conversion steps to get from one encoding to another: it does not attempt to create a connectivity graph between all encodings (though, wouldn't that be a fun project?). Remember that each intermediate encoding that the data must travel through imposes overhead! So, only one encoding is allowed to be the go-between for encodings.

Unfortunately, not all encodings are recognized as liaison encodings. For example, writing an encoding conversion from ``"UTF-8"`` to ``SHIFT-JIS`` and then from ``SHIFT-JIS`` to ``EUC-JP`` is not an indirect path the library will string together. Right now, is simply will check if you encoding to a Unicode Encoding like ``UTF-32``, and then see if there is a conversion from that Unicode Encoding to your desired destination. The full list of Indirect Liaison encodings (in order of preference) is:

1. UTF-32
2. UTF-32 Unchecked
3. UTF-16
4. UTF-16 Unchecked
5. UTF-8
6. UTF-8 Unchecked
