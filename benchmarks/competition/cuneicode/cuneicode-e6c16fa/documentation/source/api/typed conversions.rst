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

Typed Conversions
=================

Typed conversions are of the form ``{prefix}(s?)n(r?)to{suffix}(s?)n``. The prefix/suffix mapping can be found in the :doc:`design documentation for conversions </design/naming>`.

The "prefix" represents the source data. The "suffix" represents the destination data. The `s` stands for "string", which means a bulk conversion. The `r` in the name stands for "restartable", which means the function takes an `cnc_mcstate_t` pointer. If there `s` is not present in the name, it is a single conversion function. If the `r` is not present in the name, it is the "non-restartable" version (the version that does not take the state).

Additional encodings not meant to be in the "core set" supported by a typical C or C++ implementation, and that have definitive names other than the unicode encodings, can be found in the :doc:`encodings documentation</api/encodings>`.

.. important::

	Any function which does not convert to the :term:`execution encoding` or :term:`wide execution encoding` are guaranteed not to touch the locale (as defined by ``LC_CTYPE``).


.. warning::

	If an encoding conversion goes to or from either the execution encoding or the wide execution encoding, it may touch the locale which may perform a lock or other operations. If multiple funtion calls are used and ``LC_CTYPE`` is changed between any of those function calls without properly clearing the :doc:`cnc_mcstate_t </api/mcstate_t>` object to the initial shift sequence, the behavior of the functions become unspecified.




Bulk Conversion Functions
-------------------------

.. note::

	The description for most of these functions is identical. Any relevant information is contained above.


.. doxygenfunction:: cnc_mcsntomcsn

.. doxygenfunction:: cnc_mcsnrtomcsn

.. doxygenfunction:: cnc_mcsntomwcsn

.. doxygenfunction:: cnc_mcsnrtomwcsn

.. doxygenfunction:: cnc_mcsntoc8sn

.. doxygenfunction:: cnc_mcsnrtoc8sn

.. doxygenfunction:: cnc_mcsntoc16sn

.. doxygenfunction:: cnc_mcsnrtoc16sn

.. doxygenfunction:: cnc_mcsntoc32sn

.. doxygenfunction:: cnc_mcsnrtoc32sn


.. doxygenfunction:: cnc_mwcsntomcsn

.. doxygenfunction:: cnc_mwcsnrtomcsn

.. doxygenfunction:: cnc_mwcsntomwcsn

.. doxygenfunction:: cnc_mwcsnrtomwcsn

.. doxygenfunction:: cnc_mwcsntoc8sn

.. doxygenfunction:: cnc_mwcsnrtoc8sn

.. doxygenfunction:: cnc_mwcsntoc16sn

.. doxygenfunction:: cnc_mwcsnrtoc16sn

.. doxygenfunction:: cnc_mwcsntoc32sn

.. doxygenfunction:: cnc_mwcsnrtoc32sn


.. doxygenfunction:: cnc_c8sntomcsn

.. doxygenfunction:: cnc_c8snrtomcsn

.. doxygenfunction:: cnc_c8sntomwcsn

.. doxygenfunction:: cnc_c8snrtomwcsn

.. doxygenfunction:: cnc_c8sntoc8sn

.. doxygenfunction:: cnc_c8snrtoc8sn

.. doxygenfunction:: cnc_c8sntoc16sn

.. doxygenfunction:: cnc_c8snrtoc16sn

.. doxygenfunction:: cnc_c8sntoc32sn

.. doxygenfunction:: cnc_c8snrtoc32sn


.. doxygenfunction:: cnc_c16sntomcsn

.. doxygenfunction:: cnc_c16snrtomcsn

.. doxygenfunction:: cnc_c16sntomwcsn

.. doxygenfunction:: cnc_c16snrtomwcsn

.. doxygenfunction:: cnc_c16sntoc8sn

.. doxygenfunction:: cnc_c16snrtoc8sn

.. doxygenfunction:: cnc_c16sntoc16sn

.. doxygenfunction:: cnc_c16snrtoc16sn

.. doxygenfunction:: cnc_c16sntoc32sn

.. doxygenfunction:: cnc_c16snrtoc32sn


.. doxygenfunction:: cnc_c32sntomcsn

.. doxygenfunction:: cnc_c32snrtomcsn

.. doxygenfunction:: cnc_c32sntomwcsn

.. doxygenfunction:: cnc_c32snrtomwcsn

.. doxygenfunction:: cnc_c32sntoc8sn

.. doxygenfunction:: cnc_c32snrtoc8sn

.. doxygenfunction:: cnc_c32sntoc16sn

.. doxygenfunction:: cnc_c32snrtoc16sn

.. doxygenfunction:: cnc_c32sntoc32sn

.. doxygenfunction:: cnc_c32snrtoc32sn



Single Conversion Functions
---------------------------

.. note::

	The description for most of these functions is identical. Any relevant information is contained above.


.. doxygenfunction:: cnc_mcntomcn

.. doxygenfunction:: cnc_mcnrtomcn

.. doxygenfunction:: cnc_mcntomwcn

.. doxygenfunction:: cnc_mcnrtomwcn

.. doxygenfunction:: cnc_mcntoc8n

.. doxygenfunction:: cnc_mcnrtoc8n

.. doxygenfunction:: cnc_mcntoc16n

.. doxygenfunction:: cnc_mcnrtoc16n

.. doxygenfunction:: cnc_mcntoc32n

.. doxygenfunction:: cnc_mcnrtoc32n


.. doxygenfunction:: cnc_mwcntomcn

.. doxygenfunction:: cnc_mwcnrtomcn

.. doxygenfunction:: cnc_mwcntomwcn

.. doxygenfunction:: cnc_mwcnrtomwcn

.. doxygenfunction:: cnc_mwcntoc8n

.. doxygenfunction:: cnc_mwcnrtoc8n

.. doxygenfunction:: cnc_mwcntoc16n

.. doxygenfunction:: cnc_mwcnrtoc16n

.. doxygenfunction:: cnc_mwcntoc32n

.. doxygenfunction:: cnc_mwcnrtoc32n


.. doxygenfunction:: cnc_c8ntomcn

.. doxygenfunction:: cnc_c8nrtomcn

.. doxygenfunction:: cnc_c8ntomwcn

.. doxygenfunction:: cnc_c8nrtomwcn

.. doxygenfunction:: cnc_c8ntoc8n

.. doxygenfunction:: cnc_c8nrtoc8n

.. doxygenfunction:: cnc_c8ntoc16n

.. doxygenfunction:: cnc_c8nrtoc16n

.. doxygenfunction:: cnc_c8ntoc32n

.. doxygenfunction:: cnc_c8nrtoc32n


.. doxygenfunction:: cnc_c16ntomcn

.. doxygenfunction:: cnc_c16nrtomcn

.. doxygenfunction:: cnc_c16ntomwcn

.. doxygenfunction:: cnc_c16nrtomwcn

.. doxygenfunction:: cnc_c16ntoc8n

.. doxygenfunction:: cnc_c16nrtoc8n

.. doxygenfunction:: cnc_c16ntoc16n

.. doxygenfunction:: cnc_c16nrtoc16n

.. doxygenfunction:: cnc_c16ntoc32n

.. doxygenfunction:: cnc_c16nrtoc32n


.. doxygenfunction:: cnc_c32ntomcn

.. doxygenfunction:: cnc_c32nrtomcn

.. doxygenfunction:: cnc_c32ntomwcn

.. doxygenfunction:: cnc_c32nrtomwcn

.. doxygenfunction:: cnc_c32ntoc8n

.. doxygenfunction:: cnc_c32nrtoc8n

.. doxygenfunction:: cnc_c32ntoc16n

.. doxygenfunction:: cnc_c32nrtoc16n

.. doxygenfunction:: cnc_c32ntoc32n

.. doxygenfunction:: cnc_c32nrtoc32n
