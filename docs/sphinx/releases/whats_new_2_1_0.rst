..
    Copyright (C) 2007-2026 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_2_1_0:

==================
|hpx| V2.1.0 (TBD)
==================

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
- The ``count`` and ``contains`` task-policy overloads store an owned copy of
  copyable search values in their asynchronous operation state. Noncopyable
  lvalues, including arrays, are borrowed and must remain valid until the
  returned future becomes ready, even if the future is discarded.
- The task size facilities (``hpx::threads::task_size``,
  ``hpx::task_size_duration``, ``hpx::bulk_task_size``,
  ``hpx::traits::get_task_size`` and ``hpx::traits::get_task_size_for``) moved
  from the ``functional`` module to the ``thread_support`` module. Their names
  are unchanged. Code that included
  ``hpx/functional/traits/get_task_size.hpp`` directly has to include
  ``hpx/modules/thread_support.hpp`` instead; ``hpx/functional.hpp`` and
  ``hpx/modules/functional.hpp`` still provide them.

Supervision Module
==================

- A peer fenced by failure detection is now removed from the registry, so
  ``snapshot_peers()`` no longer lists it; its fence is kept
  (:hpx-pr:`7922`). That removal no longer drops a peer that came back with a
  new epoch while it was being removed, which left the peer unmonitored, and
  the eviction a terminal lifecycle notification schedules no longer removes
  a peer that started a newer epoch meanwhile (:hpx-issue:`7924`,
  :hpx-pr:`8027`).
