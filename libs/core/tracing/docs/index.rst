..
    Copyright (c) 2026 Vansh Dobhal

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_tracing:

=======
tracing
=======

This module provides the |hpx| tracing abstraction: a compile-time-dispatched
API that annotates |hpx| runtime events (task lifecycle, futures, parcels, work
stealing, worker sleep, and more) and routes those annotations to a single
profiler backend selected at build time.

Three profiler backends are supported, with a no-op fallback when none is
enabled. ``HPX_SetupTracing.cmake`` errors out at configure time if more
than one of :option:`HPX_WITH_APEX`, :option:`HPX_WITH_ITTNOTIFY` or
:option:`HPX_WITH_TRACY` is enabled.

* :ref:`modules_tracy` — the |tracy|_ profiler, enabled with
  :option:`HPX_WITH_TRACY`.
* |ittnotify|_, enabled with :option:`HPX_WITH_ITTNOTIFY`.
* |apex|_, enabled with :option:`HPX_WITH_APEX`.
* an empty backend when none of the above is enabled — the C++ hooks are
  ``constexpr`` no-ops and the tracing macros expand to nothing, so the
  hooks compile away when tracing is disabled.

Include ``hpx/modules/tracing.hpp`` to get the public API; it dispatches
at compile time to one of the per-backend headers under
``hpx/tracing/backends/``. Entries are declared in the ``hpx::tracing``
namespace and the dispatch macros (``HPX_TRACING_MARK_EVENT``,
``HPX_TRACING_ZONE``, ``HPX_TRACING_PAUSE``, ``HPX_TRACING_RESUME``) live
in ``hpx/tracing/macros.hpp``.

See :ref:`optimizing_with_tracy` for how to build |hpx| against Tracy and
attach the profiler.
