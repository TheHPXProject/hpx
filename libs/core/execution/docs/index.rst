..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_execution:

=========
execution
=========

This library implements executors and execution policies for use with parallel
algorithms and other facilities related to managing the execution of tasks.

See the :ref:`API reference <modules_execution_api>` of the module for more
details.

.. _policy_traits:

Describing execution policies with ``policy_traits``
====================================================

``hpx::execution::policy_traits<Policy>`` reports the properties of an
execution policy: ``is_policy``, ``is_rebound``, ``is_parallel``,
``is_sequenced``, ``is_unsequenced``, ``is_async`` and ``is_vectorpack``. With
datapar enabled it also reports ``num_lanes``. The ``is_*_execution_policy``
traits are defined in terms of it.

A policy can provide these properties in either of two ways.

* Declare ``static constexpr bool`` members with these names on the policy
  type. Declare only the true ones; missing members are ``false``.
* Specialize ``policy_traits<Policy>`` for a type you cannot modify, deriving
  from ``hpx::execution::detail::policy_traits_default``.

.. code-block:: c++

   struct my_policy
   {
       static constexpr bool is_policy = true;
       static constexpr bool is_parallel = true;
   };

   static_assert(hpx::execution::policy_traits<my_policy>::is_parallel);
   static_assert(!hpx::execution::policy_traits<my_policy>::is_async);

Fixed-size SIMD policies (for example ``simd_fixed_size<N>``) set
``num_lanes = N``. A value of ``0`` selects the native pack width.
