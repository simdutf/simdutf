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

Bit Function Benchmarks
=======================

.. note::

	|benchmark_warning|

The below benchmarks are done on a machine with the following relevant compiler and architecture details:

* *Compiler*: Clang 13.0.0 x86_64-pc-windows-msvc
* *Standard Library*: Microsoft Visual C++ Standard Library, Visual Studio 2022 (Version 17.0)
* *Operating System*: Windows 10 64-bit
* *CPU*: Intel Core i7: 8 X 2592 MHz CPUs
* *CPU Caches*:

  * L1 Data 32 KiB (x4)
  * L1 Instruction 32 KiB (x4)
  * L2 Unified 256 KiB (x4)
  * L3 Unified 6144 KiB (x1)

There are 4 benchmarks, and about 7 kinds of categories for each. Each one represents a way of doing work being measured.

- **naive**: Writing a loop over a ``std::array`` of ``bool`` objects.
- **naive_packed**: Writing a loop over a ``std::array`` of ``std::size_t`` objects and using masking / OR / AND operations to achieve the desired effect.
- **ztdc_packed** (this library): Writing a loop over a ``std::array`` of ``std::size_t`` objects and using bit operations to search for the bit.
- **cpp_std_array_bool**: Using the analogous ``std::`` algorithm (such as ``std::find``) on a ``std::array`` of ``bool`` objects.
- **cpp_std_vector_bool**: Using the analogous ``std::`` algorithm (such as ``std::find``) on a ``std::vector<bool>``, or one of its custom methods to perform the desired operation.
- **cpp_std_bitset**: Using the analogous ``std::`` algorithm (such as ``std::find``) on a ``std::bitset<...>`` or one of its custom methods to perform the desired operation.

Each individual bar on the graph includes an error bar demonstrating the standard deviation of that measurement. The transparent circles around each bar display individual samples, so the spread can be accurately seen. Each sample can have anything from ten thousand to a million iterations in it, and for these graphs there's 50 samples, resulting in anywhere from hundreds of thousands to tens of millions of iterations.


Details
-------

As of December 5th, 2021, many standard libraries (including the one tested) use 32-bit integers for their ``bitset`` and ``vector<bool>`` implementations. This means that, or many of these, we can beat out their implementations (even if they employ the exact same bit manipulation operations we do) by virtue of using larger integer types.

For example, we are faster for the ``count`` operation despite Michael Schellenberger Costa optimizing MSVC's ``std::vector<bool>`` iterators in conjunction with its ``count`` operation, simply because we work on 64-bit integers (and roughly, the graph shows us as twice as fast).

.. note::

	This is a consequence of having a permanently fixed ABI for standard library types, meaning that even if theoretically MSVC could be faster, a person can always beat out the standard library every single time **if** that standard library has long-lasting ABI compatibility requirements.


There are further optimizations that can be done in quite a few algorithms when comparisons are involved. For example, ``std::find`` can be implemented in terms of ``memchr`` for pointers to fundamental types: this is what makes the "find" for ``cpp_std_array_bool`` so fast compared to even the bit-intrinsic-improved ``ztdc_packed``.


.. note::
	
	Therefore, despite the last note, standard libraries still perform more optimizations than what a regular user or librarian can do! The Standard Library is not all depressing. 😁


Benchmarks
----------

.. image:: /images/benchmarks/bit/count.png
	:alt: Benchmarks for each of the tested algorithms for searching, counting, and checking the sorting status of bits.

.. image:: /images/benchmarks/bit/find.png
	:alt: Benchmarks for each of the tested algorithms for searching, counting, and checking the sorting status of bits.

.. image:: /images/benchmarks/bit/is_sorted.png
	:alt: Benchmarks for each of the tested algorithms for searching, counting, and checking the sorting status of bits.

.. image:: /images/benchmarks/bit/is_sorted_until.png
	:alt: Benchmarks for each of the tested algorithms for searching, counting, and checking the sorting status of bits.
