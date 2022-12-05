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

tag_invoke
==========

``tag_invoke`` is a way of doing customization points in Modern C++ that is meant to be easier to work with and less hassle for end-users. It follows the paper `P1895 <https://wg21.link/p1895>`_. A presentation for ``tag_invoke`` that covers its uses and its improvements over the status quo by `Gašper Ažman can be found here <https://www.youtube.com/watch?v=T_bijOA1jts>`_.


.. doxygenvariable:: ztd::tag_invoke


.. doxygenclass:: ztd::is_tag_invocable


.. doxygenvariable:: ztd::is_tag_invocable_v


.. doxygenclass:: ztd::is_nothrow_tag_invocable


.. doxygenvariable:: ztd::is_nothrow_tag_invocable_v


.. doxygentypedef:: ztd::tag_invoke_result


.. doxygentypedef:: ztd::tag_invoke_result_t
