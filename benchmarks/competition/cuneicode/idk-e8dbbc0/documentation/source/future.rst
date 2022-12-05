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

Progress & Future Work
======================

This is where the status and progress of the library will be kept. You can also check the `Issue Tracker <https://github.com/soasis/idk/issues>`_ for specific issues and things being worked on!



Containers
----------

We should work on some spicy containers. Probably.

- ☐ ``fixed_vector`` (``noexcept``-throughout)
- ☐ ``small_vector`` (``noexcept``-throughout)
- ☐ ``vector`` (``noexcept``-throughout)


Allocators
----------

We should release some spicy allocators. Maybe wrap a few of the existing ones.

- ☐ That shiny new Linux allocator everyone was talking about early in the Pandemic
- ☐ ``mimalloc`` (eww)
- ☐ ``jemalloc`` (requires fixing their godawful build system)
