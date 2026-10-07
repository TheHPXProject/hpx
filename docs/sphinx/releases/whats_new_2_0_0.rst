..
    Copyright (C) 2007-2025 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_2_0_0:

============================
|hpx| V2.0.0 (TBD)
============================

General changes
===============

- **Fixed-size SIMD policies**: added ``hpx::execution::fixed_size_simd<N>``
  and ``hpx::execution::par_fixed_size_simd<N>`` (plus their ``task``
  variants), which select the number of vector lanes explicitly. Execution
  policy properties, including ``num_lanes``, are exposed through
  ``hpx::execution::policy_traits``.
- Fixed ``hpx::any_of`` with an execution policy to return ``false`` for an
  empty range.

Breaking changes
================

- The execution-policy overloads of ``hpx::ranges::all_of``, ``any_of``,
  ``none_of``, ``count``, ``count_if``, ``contains``, ``equal``,
  ``starts_with``, ``ends_with``, ``contains_subrange``, ``is_partitioned``,
  and ``lexicographical_compare`` now follow P3179. Range arguments must model
  ``std::ranges::random_access_range`` and ``std::ranges::sized_range``;
  iterator/sentinel arguments must model ``std::random_access_iterator`` and
  ``std::sized_sentinel_for``. Overloads without an execution policy retain
  their existing constraints. In particular, the ``contains``
  iterator/sentinel overload requires both types to model
  ``std::input_iterator``.
- The P3179-aligned overloads use standard ranges callable defaults and pass
  predicates and projections by value. Their concept-based constraints may
  produce different diagnostics than the previous iterator ``static_assert``
  checks. Code that explicitly names HPX's former default callable types may
  need to use the corresponding ``std::ranges`` callable types.

Closed issues
=============

Closed pull requests
====================
