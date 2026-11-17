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
- The thread enumerations (``hpx::threads::thread_schedule_state``,
  ``thread_restart_state``, ``thread_priority``, ``thread_stacksize``, the
  scheduling hints and ``thread_schedule_hint``) moved from the ``coroutines``
  module to the ``thread_support`` module. Their names are unchanged.
  ``hpx::threads::thread_state`` stays in ``coroutines``. Code that included
  ``hpx/coroutines/thread_enums.hpp`` directly for the enumerations has to
  include ``hpx/modules/thread_support.hpp`` instead.
- The CMake option ``HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD``
  is now ``HPX_THREAD_SUPPORT_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD``, and the
  macro it defines is now
  ``HPX_THREAD_SUPPORT_HAVE_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD``. The old option
  name is still accepted with a deprecation warning.
- A component type has to be registered in exactly one module. Registering a
  component that a library already registers and has marked as such is now a
  compile time error (:hpx-issue:`8075`). Before, this went unnoticed on Linux
  and failed only at run time on Windows with ``duplicate_component_id``. In
  particular, ``partitioned_vector`` and ``coarray`` for ``double``, ``int``,
  ``long long`` and ``std::string`` are predefined by the
  ``partitioned_vector`` library when |hpx| is linked dynamically, and
  applications that register one of these types unconditionally no longer
  compile. Register them only if |hpx| is linked statically:

  .. code-block:: c++

     #if defined(HPX_HAVE_STATIC_LINKING)
     HPX_REGISTER_PARTITIONED_VECTOR(double);
     #endif

  Libraries that register components on behalf of their users can mark them
  with ``HPX_COMPONENT_REGISTERED_ELSEWHERE`` in the part of their headers
  that is visible to their users only (not while compiling the library
  itself), which turns a second registration of these components into a
  compile time error as well.
- The ``HPX_WITH_DYNAMIC_HPX_MAIN`` CMake option has been removed
  (:hpx-issue:`8071`, :hpx-pr:`8081`). On Linux and macOS the program's entry
  point is now always redirected at link time, all other platforms keep
  renaming ``main()`` with a macro. ``HPX_HAVE_DYNAMIC_HPX_MAIN`` is no longer
  defined and ``HPX_WITH_DYNAMIC_HPX_MAIN`` is no longer exported to dependent
  CMake projects. On Linux and macOS, code that includes
  ``hpx/hpx_main.hpp`` has to link ``HPX::wrap_main`` (or
  ``HPX::auto_wrap_main``), which was previously avoidable by setting the
  option to ``OFF``.

Supervision Module
==================

- A peer fenced by failure detection is now removed from the registry, so
  ``snapshot_peers()`` no longer lists it; its fence is kept
  (:hpx-pr:`7922`). That removal no longer drops a peer that came back with a
  new epoch while it was being removed, which left the peer unmonitored, and
  the eviction a terminal lifecycle notification schedules no longer removes
  a peer that started a newer epoch meanwhile (:hpx-issue:`7924`,
  :hpx-pr:`8027`).
