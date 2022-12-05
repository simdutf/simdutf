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

Conversion Registries
=====================

One of the many problems with ICU, libiconv, and so many other libraries is their deep inflexibility. In particular, for `iconv`, support for encodings must be enabled and then built, which means that if an Operating System's default dstribution does not provide for the conversion, it does not exist for that particular end-user. It also makes it frustrating when other applications base their changes off of whatever conversions the platform-blessed C Standard Library give, or what the platform-blessed package respositories hand out by default. Code that works on OpenBSD may fail spectacularly on Ubuntu, IBM platforms may have many many exotic encodings that do not exist on your Apple, and so on and so forth. This becomes problematic for developers who want to provide a more standard experience for their encodings, often leading them to either ship their own libiconv or engage in the platform encoding free-for-all.

This is where cuneicode's :cpp:type:`cnc_conversion_registry` comes in. The conversion registry is a structure that stores all of the information necessary to provide conversions from one encoding to another encoding in a type-agonstic way. Furthermore, it also provides a way to add conversions both (*both* single and bulk) to a registry. This means that a single registry can be infinitely extensible at runtime rather than requiring recompilation. Developers can provide their own encodings by programimng it in, or adding conversion routines based on things that hook into the registry, and any other techniques that a software engineer can come up with.

Thusly, it becomes far easier and far more portable to guarantee a set of encodings is always available to the your end-users, rather than gambling on whatever platform support or whatever offering your current POSIX or C Standard Library has at the moment. Below, you can select a more specific topic for how this works, and how the registry enables users to have the same kind of powerful conversion routines and extensibility between two options.


.. toctree::
	:maxdepth: 1

	/design/registry/conversion
	/design/registry/indirect
	/design/registry/allocation
