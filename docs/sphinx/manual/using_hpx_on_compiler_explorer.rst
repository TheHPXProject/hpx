..
    Copyright (c) 2026 Jatin Sharma

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _using_hpx_on_compiler_explorer:

================================
Using |hpx| on Compiler Explorer
================================

`Compiler Explorer <https://godbolt.org>`_ (CE) is a browser-based tool that
compiles C++ code in real time and shows assembly, program output, and
optimization results side by side. It is widely used for quick experiments,
conference demos, and sharing reproducible snippets. This page explains how to
build |hpx| for CE, how to link against its static libraries from a raw compiler
command, and how to use the :ref:`sandbox header <using_hpx_ce_sandbox>` that
|hpx| ships specifically for constrained environments.

.. _using_hpx_ce_constraints:

Sandbox environment constraints
================================

CE executes compiled programs inside a Linux container (nsjail) with no network
access and limited resources. The constraints that matter most for |hpx| are:

* No network namespace — sockets cannot be opened. Any attempt to initialise
  the distributed runtime will fail with ``EACCES`` at startup.
* A small PID budget (typically 32 PIDs) — |hpx| starts cleanly within this
  limit when the worker thread count is kept low.
* A short execution timeout — CE's public instance allows roughly 10 seconds.
  An |hpx| fork or self-hosted instance can relax this to 30 seconds, which is
  enough for benchmarks on problem sizes of one million elements or more.
* Static linking is required — the sandbox does not augment ``LD_LIBRARY_PATH``,
  so shared |hpx| libraries are not found at runtime.

.. _using_hpx_ce_build:

Building |hpx| for Compiler Explorer
======================================

|hpx| ships a ``godbolt-minimal`` configure preset in ``CMakePresets.json``
that encodes all the flags needed for a CE-compatible build:

.. code-block:: shell-session

   $ cmake --preset godbolt-minimal
   $ cmake --build --preset godbolt-minimal

This preset enables:

* ``HPX_WITH_STATIC_LINKING=ON`` — bundles everything into the ``.a`` archives
  that CE links against.
* ``HPX_WITH_NETWORKING=OFF`` — disables the parcelset so |hpx| never attempts
  to open a network socket.
* ``HPX_WITH_FETCH_ASIO=ON`` — downloads Asio via FetchContent, removing the
  need for a system-level Asio installation.
* ``HPX_WITH_MALLOC=system`` — uses the system allocator; avoids a jemalloc or
  tcmalloc dependency.
* ``HPX_WITH_TESTS=OFF``, ``HPX_WITH_EXAMPLES=OFF``,
  ``HPX_WITH_DOCUMENTATION=OFF`` — skips everything that CE does not need.

The preset produces four static libraries under ``build/godbolt-minimal/lib/``:

.. code-block:: text

   libhpx_wrap.a
   libhpx_init.a
   libhpx.a
   libhpx_core.a

These are the only artifacts that CE consumes.

.. note::

   The ``HPX_WITH_CXX_STANDARD`` variable pins the C++ standard for the build
   (e.g. ``-DHPX_WITH_CXX_STANDARD=20``). This is |hpx|'s own cache variable
   and is distinct from ``CMAKE_CXX_STANDARD``, which |hpx| rejects unless
   ``HPX_USE_CMAKE_CXX_STANDARD`` is also set.

.. _using_hpx_ce_linking:

Linking without CMake
======================

CE's backend compiles user code with a raw ``g++`` or ``clang++`` invocation.
The complete set of flags needed is:

.. code-block:: shell-session

   $ g++ -std=c++20 -O2 -o my_program my_program.cpp          \
       -isystem /path/to/hpx/include                           \
       -isystem /path/to/boost/include                         \
       -L/path/to/hpx/lib                                      \
       -DHPX_APPLICATION_EXPORTS                               \
       -Wl,-wrap=main                                          \
       -lhpx_wrap -lhpx_init -lhpx -lhpx_core                 \
       -lpthread -ldl -lrt

Two details here are easy to get wrong:

**Library link order.** The order ``hpx_wrap → hpx_init → hpx → hpx_core``
must be preserved. Reversing it produces undefined-reference errors because
``hpx_wrap`` depends on symbols in ``hpx_init``, which depends on the full
runtime in ``hpx``, which in turn depends on the core library.

**The** ``-Wl,-wrap=main`` **flag.** Including ``hpx/hpx_main.hpp`` (see
:ref:`minimal`) works by re-routing control through |hpx|'s own entry point
before the user's ``main`` is called. On Linux this is implemented via the
linker's ``--wrap`` option; the flag must therefore appear on the *linker*
command line, not merely in the compile flags. Without it, the |hpx| runtime is
never initialised and all API calls crash at startup. See
:ref:`hpx_main_implementation_linux` for a detailed explanation of the
mechanism.

.. important::

   ``-DHPX_APPLICATION_EXPORTS`` must be passed as a preprocessor definition
   when compiling application code against the static libraries. Omitting it
   causes link failures related to |hpx|'s symbol visibility macros.

.. _using_hpx_ce_writing_code:

Writing |hpx| code for Compiler Explorer
==========================================

The simplest way to write an |hpx| program for CE is to include
``hpx/hpx_main.hpp``. This header arranges for the |hpx| runtime to be
initialised before ``main`` runs, so the body of ``main`` can call any |hpx|
API function directly:

.. code-block:: c++

   #include <hpx/hpx_main.hpp>
   #include <hpx/algorithm.hpp>
   #include <hpx/execution.hpp>

   #include <iostream>
   #include <numeric>
   #include <vector>

   int main()
   {
       std::vector<int> v(1'000'000);
       std::iota(v.begin(), v.end(), 0);

       long long sum = hpx::transform_reduce(
           hpx::execution::par, v.begin(), v.end(), 0LL,
           std::plus<>{}, [](int x) { return static_cast<long long>(x); });

       std::cout << "sum = " << sum << "\n";
   }

.. caution::

   Include ``hpx/hpx_main.hpp`` in exactly one translation unit — the file that
   contains ``main``. Including it in more than one file causes a multiple
   definition error for the ``include_libhpx_wrap`` variable that controls
   runtime initialisation.

.. _using_hpx_ce_sandbox:

The ``hpx/experimental/sandbox.hpp`` header
============================================

|hpx| ships a header-only toolkit at ``hpx/experimental/sandbox.hpp`` designed
specifically for code running in constrained environments. It provides:

* **Environment introspection** — ``hpx::experimental::sandbox::detect_environment()``
  returns an ``environment_info`` struct describing the number of physical cores,
  processing units, NUMA domains, and active |hpx| worker threads. The
  ``is_sandbox`` flag is set when the ``COMPILER_EXPLORER`` or ``HPX_SANDBOX``
  environment variable is present, letting code adapt its behaviour
  automatically.

* **Timing** — ``hpx::experimental::sandbox::measure(fn, iterations)`` runs
  a callable ``fn`` for ``iterations`` repetitions (with one warmup pass) and
  returns the mean execution time in milliseconds.

* **Comparative benchmarking** — ``hpx::experimental::sandbox::benchmark(label,
  seq_fn, par_fn, iterations)`` measures both a sequential and a parallel
  version of the same computation, computes speedup and parallel efficiency, and
  returns a ``benchmark_report`` struct. Calling ``report.print(std::cout)``
  produces a fixed-width table that renders cleanly in CE's output pane.

A typical usage pattern looks like this:

.. code-block:: c++

   #include <hpx/hpx_main.hpp>
   #include <hpx/algorithm.hpp>
   #include <hpx/execution.hpp>
   #include <hpx/experimental/sandbox.hpp>

   #include <algorithm>
   #include <iostream>
   #include <numeric>
   #include <vector>

   int main()
   {
       namespace sb = hpx::experimental::sandbox;

       sb::describe_environment(std::cout);

       std::vector<int> data(500'000);
       std::iota(data.begin(), data.end(), 0);

       auto report = sb::benchmark(
           "transform_reduce",
           [&]() {
               hpx::transform_reduce(
                   hpx::execution::seq,
                   data.begin(), data.end(), 0LL,
                   std::plus<>{}, [](int x) { return (long long)x; });
           },
           [&]() {
               hpx::transform_reduce(
                   hpx::execution::par,
                   data.begin(), data.end(), 0LL,
                   std::plus<>{}, [](int x) { return (long long)x; });
           });

       report.print(std::cout);
   }

The output includes sequential and parallel mean times, speedup, parallel
efficiency, and a verdict (``Excellent scaling``, ``Good scaling``,
``Moderate scaling``, ``Limited scaling``, or ``No speedup``).

.. note::

   All functions in ``hpx/experimental/sandbox.hpp`` must be called from within
   a running |hpx| runtime, i.e., from an |hpx| thread. Calling them before
   :cpp:func:`hpx::init` or after :cpp:func:`hpx::finalize` is undefined
   behaviour. When using ``hpx/hpx_main.hpp``, the body of ``main`` satisfies
   this requirement automatically.

.. _using_hpx_ce_limitations:

Known limitations in sandboxed environments
=============================================

* **Single locality only.** The distributed runtime can be compiled in
  (``HPX_WITH_DISTRIBUTED_RUNTIME=ON``) and actions on locality 0 work
  normally, but there is no way to launch a second locality from within CE's
  sandbox. Code that calls ``hpx::find_all_localities()`` or
  ``hpx::get_num_localities()`` will always see exactly one locality.

* **Networking is disabled.** ``HPX_WITH_NETWORKING=OFF`` means all
  parcelport-dependent functionality (remote actions, distributed data
  structures) is unavailable regardless of what the code requests.

* **Thread count is limited by the sandbox's CPU allocation.** CE's public
  instance typically exposes two cores. Use
  ``--hpx:threads=N`` on the command line or
  ``hpx::init_params::cfg`` in code to set the worker count explicitly
  rather than relying on hardware detection. See :ref:`launching_and_configuring`
  for details.

* **macOS link flag differs.** The ``-Wl,-wrap=main`` flag is Linux-specific.
  On macOS the linker uses ``-Wl,-e,_initialize_main`` instead. CE runs Linux
  containers, so this only matters when building the CE integration locally on
  macOS for testing.
