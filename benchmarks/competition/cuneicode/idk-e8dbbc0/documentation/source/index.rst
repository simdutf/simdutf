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
.. =============================================================================

``ztd.idk``
===========

This is the IDK (Industrial Development Kit) library, part of the ZTD collection. The IDK is a small, useful toolbox of supplementary things, including

- The ztd.idk core library:

	- A small collection of type traits, optimizations, and other semi-niche utilities for accelerating development.
	- Small, header-only.
	- CMake: ``ztd::idk`` (also pulls in ``ztd::tag_invoke`` and ``ztd::version``)

- The ztd.tag_invoke customization point library:

	- Modeled after `C++ proposal p1895 <https://wg21.link/p1895>`_.
	- Makes for a single extension point to be written, ``tag_invoke(...)``, whose first argument is the name of the extension point to be hooking into. E.g., ``tag_invoke(tag_t<lua_push>, ...)``.
	- Tiny, header-only.
	- CMake: ``ztd::tag_invoke`` (also pulls in ``ztd::version``)

- The ztd.version configuration macro library:

	- A formalization of the principles found in this post and this post.
	- Mistake-resistant configuration and default-on/off vs. deliberate on/off detection.
	- Infinitesimally tiny, header-only.
	- CMake: ``ztd::version``




Who Is This Library For?
------------------------

Ideally, no one.

.. toctree::
	:maxdepth: 1
	:caption: Contents:

	in the wild
	definitions
	config
	api
	future
	benchmarks
	license
	bibliography




Indices & Search
================

.. toctree::

	genindex
