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

Glossary of Terms & Definitions
===============================

Occasionally, we may need to use precise language to describe what we want. This contains a list of definitions that can be linked to from the documentation to help describe key concepts that are useful for the explication of the concepts and ideas found in this documentation.


.. glossary::
	:sorted:

	complete unit of work
		A complete unit of work is when as little as is possible to form a complete set of output operations is consumed. This can result in 1 or more output :term:`code units <code unit>` or :term:`code points <code point>`, or a transition in shift state (which consumes some of the inpuut but may output nothing). It guarantees forward progress in some fashion through either an output or a state change.

	code unit
		A single unit of encoded information. This is typically, 8-, 16-, or 32-bit entites arranged in some sequential fashion that, when read or treated in a certain manner, end up composing higher-level units which make up readable text. Much of the world's most useful encodings that encode text use multiple code units in sequence to give a specific meaning to something, which makes most encodings variable length encodings.

	code point
		A single unit of decoded information. Most typically associated with :term:`unicode code points <unicode code point>`, but they can be other things such as :term:`unicode scalar values <unicode scalar value>` or even a 13-bit value.

		Note that a single code point does not imply a :term:`"character" <character>`, as that is a complex entity in human language and writing that cannot be mapped easily to a single unit of decoded information.

	character
		This word carries with it 2 meanings, thanks to C-style languages and their predecessors. Sometimes, ``char``\ s, ``wchar_t``\ s, ``char8_t``\ s, and similar are called "narrow character"s, "wide character"s, "UTF-8 characters" and similar. This is the result of a poor legacy in software and hardware nomenclature. These are not character types, but rather types that _may_ represent the abstract notion of a character but frequently, and often, do not. After all, you wouldn't be here reading this if it did and non-English wasn't busted in your application, now would you?

		The other definition is just an abstract unit of information in human languages and writing. The closest approximation that Unicode has for the human language/writing character is a :term:`Grapheme Cluster <grapheme cluster>`.

	unicode code point
		A single unit of decoded information for Unicode. It represents the smallest, non-encoded, and indivisible piece of information that can be used to talk about higher level algorithms, properties, and more from the Unicode Standard.

		A unicode code point has been reserved to take at most 21 bits of space to identify itself.

		A single unicode code point is NOT equivalent to a :term:`character <character>`, and multiple of them can be put together or taken apart and still have their sequence form a :term:`"character" <character>`. For a more holistic, human-like interpretation of code points or other data, see :term:`grapheme clusters <grapheme cluster>`.

	unicode scalar value
		A single unit of decoded information for Unicode. It's definition is identical to that of :term:`unicode code points <unicode code point>`, with the additional constraint that every unicode scalar value may not be a "Surrogate Value". Surrogate values are non-characters used exclusively for the purpose of encoding and decoding specific sequences of code units, and therefore carry no useful meaning in general interchange. They may appear in text streams in certain encodings.

	indivisible unit of work
		A single unit of transcoding effort when going from one encoding to another that consumes the smallest possible input to produce an output, to change the state, to both produce an output and change the state, or to produce an error. Unlike :term:`unicode code points <unicode code point>` or :term:`unicode scalar values <unicode scalar value>`, indivisible units of work do not have a fixed width or fixed definition and are dependent on the two encodings involved in the transcoding operation being performed.

	grapheme cluster
		The closest the Unicode Standard gets to recognizing a :term:`human-readable and writable character <character>`, grapheme cluster's are arbitrarily sized bundles of :term:`unicode code points <unicode code point>` that compose of a single concept that might match what a :term:`"character" <character>` is in any given human language.

	encoding
		A set of functionality that includes an encode process or a decode process (or both). The encode process takes in a stream of code points and puts out a stream of code units. The decode process takes in a stream of code units and puts out a stream of code points.

	encode
		Converting from a stream of input, typically code points, to a stream of output, typically code units. The output may be less suitable for general interchange or consumption, or is in a specific interchange format for the interoperation. Frequently, this library expects and works with the goal that any decoding process is producing :term:`unicode code points <unicode code point>` or :term:`unicode scalar values <unicode scalar value>` from some set of :term:`code units <code unit>`.

	decode
		Converting from a stream of input, typically code units, to a stream of output, typically code points. The output is generally in a form that is more widely consummable or easier to process than when it started. Frequently, this library expects and works with the goal that any decoding process is producing :term:`unicode code points <unicode code point>` or :term:`unicode scalar values <unicode scalar value>` from some set of :term:`code units <code unit>`.

	transcode
		Converting from one form of encoded information to another form of encoded information. In the context of this library, it means going from an input in one :term:`encoding <encoding>`'s code units to an output of another encoding's code units. Typically, this is done by invoking the :term:`decode <decode>` of the original encoding to reach a common interchange format (such as :term:`unicode code points <unicode code point>`) before taking that intermediate output and piping it through the :term:`encode <encode>` step of the other encoding. Different transcode operations may not need to go through a common interchange, and may transcode "directly", as a way to improve space utilization, time spent, or both.

	mojibake
		(Japanese: 文字化け Pronunciation: [modʑibake] "unintelligible sequence of characters".) From Japanese 文字 (moji), meaning "character" and 化け (bake), meaning change, is an occurence of incorrect unreadable characters displayed when computer software fails to render text correctly to its associated character encoding.

	execution encoding
		The locale-based encoding related to "multibyte characters" (C and C++ magic words) processed during program evaluation/execution. It is directly related to the ``std::set_locale(LC_CTYPE, ...)`` calls. Note that this is different from :term:`literal encoding`, which is the encoding of string literals. The two may not be (and many times, are not) the same.

	wide execution encoding
		The locale-based encoding related to "wide characters" (C and C++ magic words) processing during program evaluation/execution. It is directly related to the ``std::set_locale(LC_CTYPE, ...)`` calls. Note that this is different from the :term:`wide literal encoding`, which is the encoding of wide string literals. The two may not be (and many times, are not) the same. Nominally, wide string literals are usually not like this, but there are a handful of compilers were they use neither UTF-16 or UTF-32 as the wide execution encoding, and instead use, for example, `EUC-TW <https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-TW>`_.

	literal encoding
		The encoding of string literals (``"👋"``) during constant evaluation. This is usually controlled by command line arguments (MSVC and GCC) or fixed during compilation (Clang as UTF-8, `though that may change <https://reviews.llvm.org/D88741#2352203>`_). Typically defaults to the system's "locale" setting.

	wide literal encoding
		The encoding of wide string literals (``L"👋"``) during constant evaluation. This is usually controlled by command line arguments (GCC) or fixed during compilation (Clang as UTF-32, `though that may change <https://reviews.llvm.org/D88741#2352203>`_). Typically defaults to the system's "locale" setting.

	UTF-8
		The Unicode Transformation Format-8 encoding. It is the encoding of `u8` (Lowercase Latin-``u`` and ``8``) string literals (``u8"👋"``).

	UTF-16
		The Unicode Transformation Format-16 encoding. It is the encoding of ``u`` (Lowecase Latin-``u``) string literals (``u"👋"``).

	UTF-32
		The Unicode Transformation Format-32 encoding. It is the encoding of ``U`` (Upperase Latin-``U``) string literals (``U"👋"``).
