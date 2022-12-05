.. =============================================================================
..
.. ztd.cuneicode
.. Copyright © 2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
.. Contact: opensource@soasis.org
..
.. Commercial License Usage
.. Licensees holding valid commercial ztd.text licenses may use this file in
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

Known Unicode Encodings
=======================

The list of known Unicode encodings is identical to the one found in a consuming project for C++ called ztd.text. That list can be `found here <https://ztdtext.readthedocs.io/en/latest/known%20unicode%20encodings.html>`_.

The known Unicode encodings are important because they are evaluated before all other candidates as an intermediate in the service of performing an indirect encoding conversion; not every Unicode encoding is given such an elevated status, though. Only the encodings listed on the :doc:`indirect conversion desing documentation</design/registry/indirect>` are given the elevated encoding status and checked before all else: otherwise, the order of finding and the priority of picking such an indirect conversion is unspecified. A stronger guarantee can be made by using the select-based functions when opening a :doc:`cuneicode conversion routine in the registry </api/registry conversion handles>`.
