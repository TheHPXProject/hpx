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

- The ITTNotify backend now emits task lifecycle and work-stealing events when
  ITTNotify is enabled at runtime (``--hpx:ini=hpx.use_itt_notify=1``). This is
  ``ON`` by default for ``HPX_WITH_ITTNOTIFY=ON`` builds; pass
  ``-DHPX_WITH_TRACING_LIFECYCLE_EVENTS=OFF`` and
  ``-DHPX_WITH_TRACING_WORK_STEALING_EVENTS=OFF`` to the |cmake|_ configuration
  to get the earlier behavior.
- **Fixed-size SIMD policies**: added ``hpx::execution::fixed_size_simd<N>``
  and ``hpx::execution::par_fixed_size_simd<N>`` (plus their ``task``
  variants), which select the number of vector lanes explicitly. Execution
  policy properties, including ``num_lanes``, are exposed through
  ``hpx::execution::policy_traits``.
- ``hpx.thread_queue.min_tasks_to_steal_staged`` now only applies to other
  workers stealing staged tasks. A worker always converts its own staged
  tasks, even if there are fewer of them than the configured minimum; before,
  such tasks were never run, so any non-zero setting could hang applications.
  The default (``0``) is not affected.
- **Scheduler contention**: the local priority queue scheduler starts stealing
  staged tasks after 30 empty scheduling loop iterations instead of
  ``hpx.max_idle_loop_count / 2`` (100000 by default), but until the latter
  only from workers holding at least 10 staged tasks. Both can be changed with
  the new settings ``hpx.thread_queue.idle_loop_count_steal_staged`` and
  ``hpx.thread_queue.min_tasks_to_steal_staged_early``. New work items now
  only wake workers that are backing off, and a worker that finds work resets
  its own idle backoff.

Breaking changes
================

- ``hpx::experimental::for_loop_n_strided`` now invokes the loop body exactly
  ``n`` times, as required by N4755, 7.2.4, paragraph 2.1. It used to read
  ``n`` as the distance to cover and invoked the body ``ceil(n / stride)``
  times instead, so code that scaled ``n`` by the stride to work around this
  has to stop doing that.
- Negative strides are no longer supported by
  ``hpx::experimental::for_loop_strided``,
  ``hpx::experimental::for_loop_n_strided`` and
  ``hpx::ranges::experimental::for_loop_strided``. N4755, 7.2.4,
  paragraph 2.1 only describes a forward traversal, so the stride is now
  required to be positive.
- A live-out induction variable handed to one of the strided ``for_loop``
  algorithms is now advanced by the number of iterations instead of by the
  distance those iterations cover. The value it ended up with used to be too
  large by a factor of the stride.

Closed issues
=============

Closed pull requests
====================

