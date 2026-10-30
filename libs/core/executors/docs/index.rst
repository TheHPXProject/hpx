..
    Copyright (c) 2020 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_executors:

=========
executors
=========

The executors module exposes executors and execution policies. Most importantly,
it exposes the following classes and constants:

* :hpx:class:`hpx::execution::sequenced_executor`
* :hpx:class:`hpx::execution::parallel_executor`
* :hpx:class:`hpx::execution::sequenced_policy`
* :hpx:class:`hpx::execution::parallel_policy`
* :hpx:class:`hpx::execution::parallel_unsequenced_policy`
* :hpx:class:`hpx::execution::sequenced_task_policy`
* :hpx:class:`hpx::execution::parallel_task_policy`
* :hpx:var:`hpx::execution::seq`
* :hpx:var:`hpx::execution::par`
* :hpx:var:`hpx::execution::par_unseq`
* :hpx:var:`hpx::execution::task`

When HPX is configured with data-parallel support (``HPX_WITH_DATAPAR``), the
module additionally provides vectorizing policies. These are not part of the
generated API reference.

* ``hpx::execution::simd_policy``, ``hpx::execution::simd_task_policy`` and
  the object ``hpx::execution::simd``
* ``hpx::execution::par_simd_policy``, ``hpx::execution::par_simd_task_policy``
  and the object ``hpx::execution::par_simd``
* ``hpx::execution::fixed_size_simd_policy<N>``,
  ``hpx::execution::fixed_size_simd_task_policy<N>`` and the variable template
  ``hpx::execution::fixed_size_simd<N>``
* ``hpx::execution::par_fixed_size_simd_policy<N>``,
  ``hpx::execution::par_fixed_size_simd_task_policy<N>`` and the variable
  template ``hpx::execution::par_fixed_size_simd<N>``

The ``simd`` and ``par_simd`` policies vectorize using the native number of
vector lanes. The ``fixed_size_simd<N>`` and ``par_fixed_size_simd<N>``
policies vectorize using ``N`` lanes. Rebinding with ``(task)``, executors
or parameters preserves ``N``:

.. code-block:: c++

   std::vector<float> v(1000, 1.0f);

   hpx::for_each(hpx::execution::fixed_size_simd<4>, v.begin(), v.end(),
       [](auto& x) { x *= 2; });

   auto f = hpx::for_each(hpx::execution::par_fixed_size_simd<8>(hpx::execution::task),
       v.begin(), v.end(), [](auto& x) { x += 1; });
   f.get();

With a fixed lane count the vector loop processes only complete packs of ``N``
elements. Alignment checks use the selected pack type.

A lane count of ``0`` selects the native number of lanes for the element
type, which is equivalent to ``simd`` and ``par_simd``.

A lane count of ``1`` is semantically scalar. The algorithms execute such a
policy with scalar semantics, without alignment peeling or a remainder loop,
and the user-provided function is only invoked with scalar arguments. Use a
lane count greater than ``1`` if the function must be invoked with vector
packs.

See the :ref:`API reference <modules_executors_api>` of this module for more
details.
