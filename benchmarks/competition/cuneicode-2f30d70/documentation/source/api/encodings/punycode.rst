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

Punycode/Punycode (IDNA)
========================

Punycode is a Bootstring Encoding, using configuration and parameters for the Bootstring Algorithm described in `RFC3492 <https://www.rfc-editor.org/rfc/rfc3492>`_. Furthermore, there is an IDNA variant that prepends `"xn--"` to Unicode strings during encoding, and removes it during decoding (and otherwise does nothing). It uses custom states to manage the encodings.

Famously, Punycode is used for both Rust ABI identifier name mangling and in DNS for making Unicode names ASCII-only and clearly-marked as Unicode.

.. doxygenstruct:: cnc_pny_decode_state_t

.. doxygenstruct:: cnc_pny_encode_state_t

.. doxygenfunction:: cnc_mcnrtoc32n_punycode

.. doxygenfunction:: cnc_c32nrtomcn_punycode

.. doxygenfunction:: cnc_mcsnrtoc32sn_punycode

.. doxygenfunction:: cnc_c32snrtomcsn_punycode
