..
    Copyright (C) 2007-2026 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_2_0_0:

===========================
|hpx| V2.0.0 (Oct 31, 2026)
===========================

|hpx| V2.0.0 is one of the largest releases in the project's history, touching
the language baseline, the execution model, the networking layer, the build
system, and observability tooling. The inventories below cover over 700 pull
requests with merge commits in the release comparison, two additional PRs
incorporated through transplanted changes, and more than 150 closed milestone
issues. The sections below expand on each major theme.

|hpx| V2.0.0 is the result of a broad community effort. Since V1.11.0,
roughly 100 contributors have authored more than 4,000 commits, and over
80 of them made their first contribution to |hpx| during this release cycle.
We are grateful to everyone who contributed code, documentation, tests,
bug reports, and reviews. Your work made this release possible.

Language and Standard Baseline
==============================

- C++20 is now the *minimum* required standard, up from C++17. V1.11.0 was
  the last release to support C++17 (:hpx-issue:`5497`).
- Compared with the 1.11 release branch, the minimum supported GCC
  version rises from 11 to 12, and Visual Studio from 2019 to 2022. The
  minimum Clang version remains 16.
- Base builds require CMake 3.18 or newer. The prerequisite table has been
  corrected to match the enforced minimum; C++20 module builds require a
  newer CMake version as described below.
- A large volume of obsolete ``#if``/feature-test guards that existed only to
  support pre-C++20 compilers were removed (:hpx-issue:`6941`), simplifying
  the codebase now that C++20 is guaranteed.

C++20 Modules
=============

Native C++20 module support was added across the |hpx| module hierarchy
(:hpx-issue:`6014`). It is optional: ``HPX_WITH_CXX_MODULES`` defaults to
``OFF``. Enabling it requires CMake 3.28 or newer, a Ninja or Visual Studio
generator, and GCC 14+, LLVM Clang 16+, or Visual Studio 17.4+. AppleClang
is rejected by the module configuration checks.

C++26 Adoption (Ahead of Standardization)
=========================================

Adopted early via reflection and experimental compiler support:

|hpx| adopted several not-yet-finalized C++26 features early, using
experimental compiler support, well ahead of formal standardization:

- **Reflection-based serialization** - arbitrary user types can now be
  serialized without hand-written ``serialize()`` member functions, using
  compile-time reflection to enumerate members automatically.
- **Reflection-driven actions and components** - a large family of new
  reflection APIs replaced macro-heavy boilerplate:

  - ``reflect_action`` integrated with ``basic_action``
    (:hpx-pr:`7281`), including direct actions (:hpx-pr:`7349`),
    component actions (:hpx-pr:`7311`), and auto-registration
    (:hpx-pr:`7298`).
  - ``reflect_client`` and the ``HPX_REFLECT_CLIENT`` macro for
    reflection-based client types (:hpx-pr:`7385`), with
    ``hpx::launch::sync`` overload support (:hpx-pr:`7417`).
  - ``hpx::async<^^func>(target, ...)``-style reflection overloads
    (:hpx-pr:`7376`), plus reflection overloads threaded through
    ``async_distributed`` (sync, post, dataflow, async_continue,
    launch-policy variants: :hpx-pr:`7509`, :hpx-pr:`7515`,
    :hpx-pr:`7523`, :hpx-pr:`7462`).
  - Example programs migrated to demonstrate the new API:
    ``factorial_reflection`` (:hpx-pr:`7508`), ``fibonacci_reflection``
    (:hpx-pr:`7455`).
  - A migration guide (:hpx-pr:`7348`) and compile-time overhead
    benchmarks comparing macros vs. ``reflect_action``
    (:hpx-pr:`7332`, :hpx-pr:`7459`) were added to help users and
    maintainers evaluate the transition.

- **Early contract support** - |hpx| API functions have begun to be annotated
  with C++26 contract preconditions/postconditions, starting with sorting
  and heap algorithms (:hpx-pr:`7434`).

Senders/Receivers Overhaul
==========================

|hpx|'s original, home-grown implementation of P0443/P1897/P2300
(:hpx-issue:`5045`) has been removed entirely and replaced by |stdexec|_
(|nvidia|_'s reference implementation) as the sole senders/receivers
foundation, usable from both C++20 and C++26:

- Multiple correctness/compliance passes: fixing P2300 compliance and
  ``set_stopped`` handling for ``any_sender`` (:hpx-pr:`7453`),
  ``get_allocator`` query support for schedulers (:hpx-pr:`7320`),
  allocator support for ``when_all_vector`` (:hpx-pr:`7346`), and
  ``get_completion_domain`` advertisement on ``run_loop`` (:hpx-pr:`7276`).
- Distributed and accelerator adaptors were updated to integrate cleanly
  with |stdexec|_: ``async_mpi`` (:hpx-pr:`7341`, :hpx-pr:`7318`,
  :hpx-pr:`7277`), ``async_cuda`` (:hpx-pr:`7330`, :hpx-pr:`7374`), and the
  fork-join executor (fixing truncation bugs on large ranges,
  :hpx-issue:`6922`).
  Distributed sender/receiver facilities were also moved into their own
  module (:hpx-pr:`7396`) with dedicated distributed test coverage
  (:hpx-pr:`7402`, :hpx-pr:`7354`).
- Algorithm-level sender/receiver support was extended to more algorithms,
  e.g. ``set_intersection`` (:hpx-pr:`7365`), ``set_difference``
  (:hpx-pr:`7344`), and ``shift_left``/``shift_right`` (:hpx-pr:`7296`).
- Miscellaneous but important bug fixes: a lifetime race in
  ``sync_wait``'s notify path (:hpx-pr:`7394`), a silent hang in
  ``make_future`` with ``run_loop_scheduler`` (:hpx-pr:`7437`), and a
  future/promise bridge redesign (:hpx-pr:`7355`).

Runtime and Execution APIs
==========================

- The P2079 parallel scheduler integrates sender work with HPX thread pools
  (:hpx-pr:`6655`). ``hpx::execution::experimental::get_parallel_scheduler()``
  uses the default pool; the overload taking a pool reference supports named
  pools obtained through ``hpx::resource::get_thread_pool`` (:hpx-pr:`7579`).
- Execution-policy traits and the independent
  ``rebind_policy_executor_t`` / ``rebind_policy_parameters_t`` utilities
  support inspecting and adapting policies (:hpx-pr:`7550`, :hpx-pr:`7551`).
- Thrust integration adds HPX execution policies for Thrust algorithms in
  CUDA builds (:hpx-pr:`6744`), controlled by ``HPX_WITH_THRUST``.
- New concurrent containers include vector, queue, unordered map, and
  unordered set (:hpx-pr:`6897`).
- ``hpx::local::termination_detection`` waits for local work to finish,
  with timeout, deadline, and stop-token cancellation overloads
  (:hpx-pr:`6886`, :hpx-pr:`6890`).
- ``hpx::threads::set_thread_affinity`` and
  ``hpx::this_thread::set_affinity`` expose thread-affinity control
  (:hpx-pr:`6861`).
- ``finalize`` and ``disconnect`` accept ``std::chrono`` durations through
  new overloads (:hpx-pr:`7612`).
- ``hpx::agas::set_rpc_timeout`` now rejects positive durations shorter
  than one millisecond: it returns ``false`` and leaves the stored timeout
  unchanged instead of storing a zero-millisecond timeout.
- ``HPX::init`` supports runtime initialization from libraries, and
  ``HPX::auto_wrap_main`` starts the runtime around ``main()`` without
  requiring the wrapping header (:hpx-pr:`6775`, :hpx-pr:`6804`).
  ``HPX::auto_wrap_main`` is also supported with MSVC
  (:hpx-issue:`7662`, :hpx-pr:`7664`).
  Local initialization was decoupled from the distributed runtime
  (:hpx-pr:`7103`).

``tag_invoke`` Removal
======================

A systematic, multi-PR effort removed ``tag_invoke``-based customization
across nearly every layer of the library, replacing it with plain member
functions - this is one of the more consequential breaking changes for
downstream code that customized |hpx| behavior via ADL/``tag_invoke``:

- Core execution layer (:hpx-pr:`7368`), executor CPOs
  (:hpx-pr:`7319`), container algorithms (:hpx-pr:`7395`), and parallel
  algorithms broadly (:hpx-pr:`7380`, :hpx-pr:`7301`).
- Specific customization points converted to members, e.g.
  ``connect_t`` (:hpx-pr:`7292`).
- Cleanup of now-stale ``hpx_tag_invoke`` module dependencies
  (:hpx-pr:`7451`) and a final sweep
  (:hpx-pr:`7443`).

Networking / Parcelport Work
============================

- A new, optimized MPI parcelport, with orderly teardown that quiesces
  before ``MPI_Finalize`` (:hpx-pr:`7404`).
- An optimized |lci|_ parcelport.
- A separate LCW/MPIx parcelport was added (:hpx-pr:`6815`), enabled with
  ``HPX_WITH_PARCELPORT_LCW=ON`` (default ``OFF``).
- Substantial AGAS/parcelset hardening:

  - ``force_disconnect`` support for removing crashed/unreachable
    localities, including races between disconnect completion and AGAS
    resolve visibility (:hpx-issue:`7483`, :hpx-pr:`7490`,
    :hpx-pr:`7491`, :hpx-pr:`7447`).
  - Dijkstra termination detection improvements, including probe retry
    bounding and cursor restart per probe (:hpx-pr:`7496`,
    :hpx-pr:`7516`, :hpx-pr:`7495`).
  - Connection/communicator caching fixes, e.g. avoiding communicator
    reuse between collectives/scan test phases (:hpx-pr:`7526`,
    :hpx-pr:`7364`) and caching channel communicators per site
    (:hpx-pr:`7481`).
  - Disconnected-locality handling, including a dedicated dispatch guard
    (:hpx-pr:`7536`) and correct error handling in
    ``resolve_locality`` (:hpx-pr:`7388`).
  - A fix for the shared-pointer serialization type-confusion /
    deserialization vulnerability reported in :hpx-issue:`7333`
    (:hpx-pr:`7334`).

Observability / Tracing
=======================

- A new, minimal tracing abstraction layer, ``hpx::tracing``
  (:hpx-issue:`7049`), unifying the |ittnotify|_ (Intel VTune) and |tracy|_
  backends behind one API, with |apex|_ also unified under it (:hpx-pr:`7293`).
- |tracy|_ support was added as a first-class profiler backend, including
  causal tracing (:hpx-pr:`7423`), fiber support activation
  (:hpx-issue:`6989`), and per-subsystem event gating for overhead control
  (:hpx-pr:`7529`).
- Instrumentation was added broadly: schedulers and work-stealing
  (:hpx-pr:`7450`), background work and OS-thread sleep events
  (:hpx-pr:`7484`), parcel send/receive (:hpx-pr:`7511`), future
  continuations (:hpx-pr:`7403`), task lifecycle events
  (:hpx-pr:`7347`), suspend/resume hooks (:hpx-pr:`7372`), data-parallel
  workloads (:hpx-pr:`7392`), and general runtime instrumentation
  (:hpx-pr:`7357`).
- LIKWID profiling support was added (:hpx-pr:`6053`), enabled with
  ``HPX_LIKWID_WITH_LIKWID=ON``.
- New tooling built on top of this infrastructure: **hpx-top**, an
  interactive TUI performance monitor (:hpx-issue:`7001`), and
  **hpx_stat_viewer**; both scripts are now installed (:hpx-issue:`7656`,
  :hpx-pr:`7658`).
- A distributed-observability prototype ("HPX-Vision") was proposed and
  discussed (:hpx-issue:`7225`) as a longer-term direction beyond this
  release.
- The |ittnotify|_ tracing backend now provides a shared domain, regions,
  and markers (:hpx-pr:`7636`). The Tracy targets are exported so that
  consumers can find them through HPX (:hpx-pr:`7654`), and Tracy export
  annotations were moved off class heads to build with GCC 11 and 12
  (:hpx-pr:`7669`).

Collectives Infrastructure
==========================

Hierarchical collectives were built out and hardened substantially this
cycle:

- New hierarchical primitives: ``all_reduce`` (with the reduction seed now
  moved instead of copied, :hpx-pr:`7398`), ``all_gather``/``scatter``
  (flattened payloads, :hpx-pr:`7375`), ``all_to_all`` (flattened
  exchange payloads :hpx-pr:`7377`, hierarchical implementation
  :hpx-pr:`7307`, size-based pairwise dispatch :hpx-pr:`7431`,
  and a ~66x performance regression fix for the flat variant,
  :hpx-issue:`7411`), and a hierarchical **scan** collective
  (:hpx-pr:`7343`).
- Validation and robustness hardening across collectives, including
  rejecting explicit generations mixed with auto-generation
  (:hpx-pr:`7369`), unifying generation handling across collective types
  (:hpx-pr:`7326`), and broad input-validation/tree-coverage hardening
  (:hpx-pr:`7321`, :hpx-pr:`7359`).
- ``hpx::distributed::barrier`` was reimplemented on top of the
  collectives infrastructure rather than its own ad hoc protocol
  (:hpx-issue:`7203`).

Supervision Module
==================

A new lifecycle/observability subsystem, "supervision," was introduced for
managing dispatch of distributed work under crash/partial-failure
conditions:

- Core infrastructure for lifecycle events and observers
  (:hpx-pr:`7399`), a dispatch component (:hpx-pr:`7413`), and
  admission control (:hpx-pr:`7409`).
- Fenced dispatch support (:hpx-pr:`7425`), dispatch locality and join
  epoch handling (:hpx-pr:`7438`), and target resolution
  (:hpx-pr:`7427`), plus general dispatch fixes and cleanup
  (:hpx-pr:`7457`).
- A reflection overload for ``dispatch_work`` was added consistent with
  the broader C++26 reflection push (:hpx-pr:`7432`).
- The module can be disabled at build time (``HPX_WITH_SUPERVISION=OFF``);
  a related abort-on-disable regression was fixed
  (:hpx-issue:`7513`).

Parallel Algorithms
===================

A large, ongoing wave of standard-conformance and correctness work across
the parallel algorithms library:

- **C++20/23 conformance**: fixing sentinel return-type mismatches in
  ``hpx::ranges`` container CPOs (:hpx-pr:`7322`), unifying projection
  support across ``starts_with``/``ends_with`` (:hpx-pr:`7314`),
  and adding projection coverage for ``unique``/``unique_copy``
  (:hpx-pr:`7393`) and ``minmax_element`` (:hpx-pr:`7230`).
- **New algorithms**: ``hpx::experimental::for_each_index`` per P4150
  (:hpx-issue:`7214`), ``hpx::ranges::iota`` (:hpx-issue:`6969`),
  ``find_last``/``find_last_if``/``find_last_if_not``
  (:hpx-issue:`6892`), and the C++23 fold family: ``fold_left``,
  ``fold_right``, ``fold_left_first``, ``fold_right_last``,
  ``fold_left_with_iter``, and ``fold_left_first_with_iter``
  (:hpx-pr:`6884`, :hpx-pr:`6910`).
- **Correctness fixes**: ``max_element`` returning the last, rather than
  first, greatest element (:hpx-pr:`7304`), two bugs in ``chunk_size.hpp``
  involving swapped arguments (:hpx-issue:`6931`), an incorrect
  single-segment subrange handling bug in segmented ``scan``
  (:hpx-issue:`7052`), and non-preserved sequenced semantics in parallel
  ``uninitialized_relocate`` for overlapping ranges (:hpx-issue:`6878`).
- **Proxy/iterator support**: proxy support for ``find`` algorithms
  (:hpx-issue:`6719`), unwrapping contiguous iterators
  (:hpx-issue:`6729`), and conditional special member functions for
  ``zip_iterator`` (:hpx-issue:`6752`).
- **Distributed segmented merge**: added ``merge`` support for segmented
  input and output ranges distributed across localities, with sequential,
  parallel, and task execution policies (:hpx-pr:`7545`).
- **Datapar/SIMD**: fixing |eve|_/simd unaligned load/store broadcast bugs
  (:hpx-pr:`7435`, :hpx-issue:`7287`), and guarding a mismatched SIMD
  path on ``zip_iterator`` compatibility (:hpx-pr:`7328`).
- **Fixed-size SIMD policies**: added ``hpx::execution::fixed_size_simd<N>``
  and ``hpx::execution::par_fixed_size_simd<N>`` (plus their ``task``
  variants) to select vector lane counts explicitly. Datapar loops and
  algorithms use the policy's lane count, exposed through
  ``hpx::execution::policy_traits``. Existing SIMD policies retain the
  native lane count (:hpx-pr:`7674`).

Scheduler / Threading Improvements
==================================

- **NUMA-aware work stealing**: a distance-aware scheduling feature
  (:hpx-issue:`7091`), plus fixes for suspended-processing-unit handling
  in NUMA-aware scheduling (:hpx-issue:`6963`) and reduced lock
  contention via randomized victim selection (:hpx-issue:`6904`).
- **Queue performance**: SPSC queue performance work for the
  work-requesting scheduler (:hpx-issue:`7112`), and comparative
  benchmarking / hardening of the MPMC queue (:hpx-issue:`7167`), plus a
  fix for a race condition in ``lockfree::deque::empty()``
  (:hpx-issue:`7158`).
- **Memory/lifetime fixes**: memory accumulating while iterating with a
  sliding semaphore (:hpx-issue:`7185`) and while creating tasks
  (:hpx-issue:`7150`); better lifetime management for background/pool
  threads (:hpx-pr:`7350`); thread-data size reductions
  (:hpx-pr:`7379`); and batched atomic decrements in
  ``thread_queue::cleanup_terminated_locked()`` (:hpx-issue:`7085`).
- **Fork-join executor**: fixed silent truncation for ranges larger than
  2^32 (:hpx-issue:`6922`).
- **Shared priority scheduler**: the scheduler now creates a new thread
  when one is requested (:hpx-issue:`5189`, :hpx-pr:`7682`).
- **Fork dispatch**: fixed ``hpx::launch::fork`` tasks remaining
  unscheduled when launched from a plain OS thread or an HPX thread in
  another pool. Both ``post`` and ``async`` now schedule the task directly
  when an immediate handoff is unavailable (:hpx-pr:`7687`).

Build / CI Modernization
========================

- A ``CMakePresets.json`` file was added for simplified configuration
  (:hpx-issue:`6798`), along with support for the Ninja Multi-Config
  generator (:hpx-issue:`6710`).
- GitHub Actions workflows were consolidated into composite actions
  (:hpx-issue:`7087`), and a slash-command-driven CI workflow was
  introduced (:hpx-issue:`7075`).
- Dynamic build-and-test discovery replaced some hard-coded exclusion
  lists, though this briefly regressed test coverage on Windows
  (:hpx-issue:`7151`, :hpx-issue:`7199`) before being fixed.
- CircleCI usage was reduced/removed in favor of GitHub Actions.
- Numerous obsolete C++20 feature-test guards were deleted now that C++20
  is the guaranteed baseline (:hpx-issue:`6941`).
- macOS/vcpkg and hidden-visibility build issues were fixed
  (:hpx-issue:`6772`, :hpx-issue:`6653`), and Fedora-specific macro
  collisions (``BLOCK_SIZE``) were resolved (:hpx-issue:`6961`).
- Conan builds now alias lowercase targets for Asio and Hwloc
  (:hpx-issue:`7659`, :hpx-pr:`7661`).
- MSVC and MinGW fixes: ``dbghelp.lib`` is named from ``backtrace.cpp``
  (:hpx-pr:`7665`), and the static empty vtable fallback in
  ``execution_base`` is used on Windows builds
  (:hpx-issue:`7650`, :hpx-pr:`7652`). The raw MSVC ``hpx_main`` path of
  ``godbolt-minimal`` is now CI-tested and documented, and passes
  ``/SUBSYSTEM:CONSOLE`` (:hpx-pr:`7644`, :hpx-pr:`7683`).
- A Linux CI workflow with ``_GLIBCXX_DEBUG`` enabled was added
  (:hpx-pr:`7673`), and the parallel scheduler tests were fixed on
  Clang 16 (:hpx-pr:`7679`).

Security / Robustness Fixes
===========================

- **Shared-pointer serialization vulnerability**: an insecure
  deserialization / type-confusion issue in shared-pointer serialization
  was fixed (:hpx-issue:`7333`, :hpx-pr:`7334`).
- **Data races**: a race in ``hostname_print_helper::get_hostname()`` on
  worker-thread startup (:hpx-issue:`7520`, :hpx-pr:`7525`, with a regression
  test added in :hpx-pr:`7546`).
- **Synchronization**: corrected timed semaphore waits (:hpx-pr:`7617`),
  a deadlock and race in ``latch::reset`` (:hpx-pr:`7555`),
  and handling of spurious wakeups in ``hpx::barrier`` (:hpx-pr:`7611`).
  The memory order used after a condition-variable wakeup in future
  ``wait()`` was upgraded from relaxed to acquire (:hpx-pr:`7677`).
- **AGAS garbage collection**: fixed background collection when networking
  is disabled (:hpx-pr:`7627`).
- **Exception safety**: a memory-leak risk in
  ``options_description_easy_init`` under exceptions was fixed
  (:hpx-issue:`7040`), and unsafe ``std::strcat`` usage in ``print.cpp``
  was replaced with safer string operations (:hpx-issue:`7046`).
- **Serialization correctness**: a double-read/type-mismatch bug in
  ``exception_ptr::load()`` was fixed (:hpx-pr:`7278`), along with a
  semantic mismatch in serialization archive flags
  (:hpx-issue:`7034`). Collection sizes read from archives are now checked
  before use (:hpx-pr:`7594`).

Breaking changes
================

The items below require changes in user code. Each is covered in more detail
in the sections above.

- C++20 is now the minimum required standard. Compiling |hpx| or applications
  using it with C++17 is no longer supported.
- The minimum supported compiler versions have changed. Please see our
  :ref:`prerequisites` page for the current support matrix.
- The use of ``tag_invoke`` for API customization has been removed where
  possible. Code that relied on overloading |hpx| functionality through
  ``tag_invoke`` will have to be adapted.
- Our own, by now outdated, implementation of ``std::execution`` (senders and
  receivers) has been removed. It is replaced by the reference implementation
  (|stdexec|_).

Closed Issues and Pull Requests
==================================

The lists below were reconciled against release-2.0.X at
``5d466501ae29d98b655cff5b7fa27d63d067af9a``, using release-1.11.X at
``4823cb5b41cc6ec5ab3a49741541927f24704bf3`` as the baseline (including its
maintenance backports, rather than only the v1.11.0 tag).

There are 697 merged PRs whose merge commits are reachable from the release
and absent from that baseline, including PRs outside milestone 2.0.0. The
140 closed issues are the milestone inventory; closure does not necessarily
mean an implementation was delivered. Closed, unmerged PRs are excluded.
Two further PRs with verified transplanted changes are recorded separately
below the generated list.

Closed issues
=============

* :hpx-issue:`7680` - shared_future::get() documentation incorrectly states that it invalidates the future
* :hpx-issue:`7662` - Add auto-wrapping for main for MSVC
* :hpx-issue:`7659` - Conan requires CMake aliases for Asio and Hwloc
* :hpx-issue:`7656` - `hpx-top` and `hpx_stat_viewer` are not installed
* :hpx-issue:`7650` - any_sender.hpp: constexpr empty_vtable fails to compile in MinGW shared (DLL) builds
* :hpx-issue:`7647` - Error using HPX built with `HPX_WITH_TRACY=TRUE`
* :hpx-issue:`7613` - `HPX_REGISTER_CHANNEL(type, name)` 2-arg form still fails for written-out template types
* :hpx-issue:`7608` - debugging: refresh DbgHelp module list after SymInitialize
* :hpx-issue:`7592` - Re-enable auto-linking on Windows for static builds
* :hpx-issue:`7591` - Windows CI: Unsynchronized `SymInitialize` calls between HPX and Tracy cause intermittent `DbgHelp` failures and heap corruption
* :hpx-issue:`7589` - Replace raw double timeouts with std::chrono in finalize.hpp
* :hpx-issue:`7569` - godbolt-minimal does not install hpx/experimental/sandbox.hpp
* :hpx-issue:`7568` - Add HPX to Compiler Explorer
* :hpx-issue:`7557` - examples/quickstart/sort_by_key_demo fails to compile with Apple clang 21 (libc++ __sift_down vs compare_projected)
* :hpx-issue:`7539` - libhpx_wrap has an unpropagated oneTBB dependency
* :hpx-issue:`7520` - Data race in hostname_print_helper::get_hostname() on worker thread startup
* :hpx-issue:`7517` - HPX_FORWARD does not forward under nvcc: cudafe strips && from static_cast<decltype(x)&&>
* :hpx-issue:`7513` - dijkstra_termination_disconnected_*_7474 abort on any HPX_WITH_SUPERVISION=OFF build
* :hpx-issue:`7483` - force_disconnect: race between disconnect completion and AGAS resolve visibility
* :hpx-issue:`7480` - Apply AGAS RPC timeout during hosted locality namespace bootstrap polling
* :hpx-issue:`7470` - late_component_launcher: demonstrate crash detection -> force_disconnect purge -> relaunch
* :hpx-issue:`7461` - check-circular-deps CI failing on master (pre-existing, unrelated to PR #base 4f5b0d6b)
* :hpx-issue:`7428` - v1.11.0 fails to build with asio-1.38.2
* :hpx-issue:`7411` - Regression: flat all_to_all ~66x slower since 6df14adf09 (PU offset suppressed for all localities)
* :hpx-issue:`7389` - `waittime` vs `wait_time` name mismatch when `HPX_HAVE_THREAD_QUEUE_WAITTIME` is ON
* :hpx-issue:`7383` - HPX affinity binds introduce randomised offsets for multiple localities in single node
* :hpx-issue:`7333` - Insecure Deserialization - Type Confusion in Shared Pointers - HPX v1.11.0
* :hpx-issue:`7287` - vector_pack_load::unaligned broadcasts scalar instead of loading pack
* :hpx-issue:`7251` - stdexec warning about transform_completion_signatures
* :hpx-issue:`7245` - CMake developer warning regarding fetching Stdexec
* :hpx-issue:`7225` - Proposal / Feedback Request: HPX-Vision (Distributed Observability Prototype for HPX)
* :hpx-issue:`7214` - Implement `hpx::experimental::for_each_index` as described in P4150
* :hpx-issue:`7212` - Make inspect report links use the scanned tree's actual Git commit
* :hpx-issue:`7203` - Reimplement the distributed barrier on top of collectives infrastructure
* :hpx-issue:`7199` - Deduplicate tests.examples exclusion regex generation in Windows workflows
* :hpx-issue:`7197` - Fix external builds with C++ modules enabled
* :hpx-issue:`7185` - Memory usage accumulates while iterating, using a sliding semaphore
* :hpx-issue:`7184` - Memory leak in overlapping relocation test functions
* :hpx-issue:`7167` - Compare performance of HPX's MPMC queue with other implementations
* :hpx-issue:`7158` - Potential Inconsistent State and Race Condition in hpx::lockfree::deque::empty()
* :hpx-issue:`7151` - Dynamic build-and-test workflow lost several previously excluded tests
* :hpx-issue:`7150` - Memory usage accumulates while creating tasks
* :hpx-issue:`7131` - Fix Sender Algorithm Customization
* :hpx-issue:`7124` - 1d_stencil_5 uses non-owning serialization for partition_data buffer
* :hpx-issue:`7117` - CircleCI container_algorithms shard uses partial ctest regex and runs unbuilt test target
* :hpx-issue:`7112` - Improve performance of SPSC queue used in work-requesting scheduler
* :hpx-issue:`7099` - [wrap] Enable hpx_main wrapping for local-only (non-distributed) builds
* :hpx-issue:`7091` - [Feature] NUMA-Distance-Aware Work Stealing in Schedulers
* :hpx-issue:`7087` - Unify and Restructure GitHub Actions Workflows via Composite Actions
* :hpx-issue:`7085` - thread_queue: batch the atomic decrement in cleanup_terminated_locked()
* :hpx-issue:`7077` - par policy silently falls back to sequential for non-contiguous iterators in all three uninitialized_relocate CPOs
* :hpx-issue:`7075` - [FEA] Implement CI Workflow using Slash Commands
* :hpx-issue:`7062` - Remove usage of deprecated asio::ip::tcp::resolver::query
* :hpx-issue:`7059` - HPX Build Configuration Error: Missing Asio Dependency
* :hpx-issue:`7052` - Segmented algorithms: Incorrect single-segment subrange handling + Few other bugs in scan.hpp
* :hpx-issue:`7050` - thread_queue: per-task heap allocation in staged queue could be further optimised
* :hpx-issue:`7049` - Introduce minimal tracing abstraction layer (hpx::tracing) for Tracy
* :hpx-issue:`7046` - Replace unsafe `std::strcat` with safer string operations in `print.cpp`
* :hpx-issue:`7040` - exception Safety Risk in `options_description_easy_init` Could Lead to Memory Leaks
* :hpx-issue:`7037` - `is_sorted` / `is_sorted_until` force class level template parameters in methods
* :hpx-issue:`7035` - hpx::compute::vector is missing standard std::vector methods (assign, at, const iterators)
* :hpx-issue:`7034` - Semantic Mismatch in Serialization Archive Flags
* :hpx-issue:`7030` - mpi::experimental::detail::async ignores MPI error codes from MPI_Ixxx calls, leading to potential hangs
* :hpx-issue:`7028` - async_replay_validate incorrectly throws abort_replay_exception when the last attempt succeeds
* :hpx-issue:`7027` - Fix failing test related to numa_allocator
* :hpx-issue:`7025` - Incorrect Return Type Usage in `get_next_threads`
* :hpx-issue:`7022` - scheduler_executor performance regression with stdexec integration
* :hpx-issue:`7001` - Feature Proposal: HPX-Top (Interactive TUI Performance Monitor)
* :hpx-issue:`7000` - tests: 7 sender test files declare std::mt19937 gen but hpx_main calls std::srand(seed) instead of gen.seed(seed), making --seed silently ineffective
* :hpx-issue:`6989` - Tracy fiber support present but currently unused in HPX runtime
* :hpx-issue:`6982` - Missing validation of thread_schedule_hint.hint in shared_priority_queue_scheduler
* :hpx-issue:`6978` - shared_priority_queue_scheduler: possible stale (domain_num, q_index) after select_active_pu() in schedule_thread() `none` path
* :hpx-issue:`6975` - Fix failing unit tests related to move algorithm
* :hpx-issue:`6974` - Fix failing unit tests related to sort algorithms
* :hpx-issue:`6969` - Implementation of `hpx::ranges::iota`
* :hpx-issue:`6963` - shared_priority_queue_scheduler: NUMA scheduling does not handle suspended processing units
* :hpx-issue:`6961` - BLOCK_SIZE is defined on Fedora
* :hpx-issue:`6953` - base_component::get_unmanaged_id incorrectly returns management_type::managed ID
* :hpx-issue:`6941` - Remove obsolete feature tests
* :hpx-issue:`6931` - Two correctness bugs in chunk_size.hpp : swapped next_or_subrange args and mismatched add_ready_future_idx parameter order
* :hpx-issue:`6922` - fork_join_executor silently truncates ranges > 2^32
* :hpx-issue:`6908` - Implement missing C++23 range algorithms: fold, chunk_by, slide, stride, cartesian_product
* :hpx-issue:`6906` - Add missing subtraction unit tests for gid_type
* :hpx-issue:`6905` - Split fast path ignores receiver's scheduler leading to mismatched completion thread
* :hpx-issue:`6904` - reduce lock contention in local_priority_queue_scheduler via randomized victim selection
* :hpx-issue:`6896` - Fix incomplete sentence in README.rst
* :hpx-issue:`6892` - Add find_last, find_last_if, and find_last_if_not algorithms
* :hpx-issue:`6888` - Enhancement: Add timeout and cancellation support to hpx::local::termination_detection()
* :hpx-issue:`6878` - Parallel uninitialized_relocate does not preserve sequenced semantics for overlapping ranges
* :hpx-issue:`6874` - tests.examples.1d_stencil.1d_stencil_5 is failing in some CI runs
* :hpx-issue:`6867` - v1.11.0 fails to build on F44
* :hpx-issue:`6854` - hpx::shared_mutex deadlock
* :hpx-issue:`6842` - Errors when building tests relying on stdexec
* :hpx-issue:`6798` - Provide a CMakePresets.json file
* :hpx-issue:`6786` - Problem : Building hpx [ have boost installed ]
* :hpx-issue:`6776` - Bug with hpx::collectives::set on macOS (arm64)
* :hpx-issue:`6772` - Broken bundle HPX + vcpkg + macOS since v1.10
* :hpx-issue:`6754` - Incorrect is_known_contiguous_iterator support for string
* :hpx-issue:`6752` - zip_iterator should have conditional special member functions
* :hpx-issue:`6745` - Forward `stdexec::bulk` in HPX to support full S/R NVIDIA's integration implementation
* :hpx-issue:`6735` - Failure when calling a parallel algorithm from a Python extension
* :hpx-issue:`6729` - Consider unwrapping contiguous iterators in parallel algorithms
* :hpx-issue:`6728` - Building on ubuntu-24.04-arm
* :hpx-issue:`6722` - Unreachable code warning in advance_and_get_distance
* :hpx-issue:`6721` - Unreachable code warning in merge algorithm
* :hpx-issue:`6719` - Proxy support for find algorithms
* :hpx-issue:`6718` - Bad user experience with algorithms interface
* :hpx-issue:`6710` - Support Ninja Multi-Config generator
* :hpx-issue:`6653` - Passing `HPX_WITH_HIDDEN_VISIBILITY=TRUE` on MacOS results in library which can't be used
* :hpx-issue:`6627` - Extrema algorithms with repeated values
* :hpx-issue:`6625` - Update relocation semantics to match P2786R13
* :hpx-issue:`6624` - Remove type alias sender completions
* :hpx-issue:`6541` - Add HPX to Godbolt Compiler Explorer
* :hpx-issue:`6536` - Non-Standard Conforming Parallel Algorithms
* :hpx-issue:`6534` - Addressing remaining Stdexec issues
* :hpx-issue:`6528` - Wrong version recommendation of shpinx when building documentation
* :hpx-issue:`6506` - Create API similar to pthread_setaffinity_np for HPX threads
* :hpx-issue:`6504` - `FindTBB.cmake` cannot find correct TBB library.
* :hpx-issue:`6502` - The test executable numa_allocator_test fails with an assertion
* :hpx-issue:`6500` - The test partitioned_vector\_ doesn't finish in a very long time
* :hpx-issue:`6347` - Failed Linking CXX executable due to undefined references
* :hpx-issue:`6345` - Initialization hangs when only setting --hpx:cores
* :hpx-issue:`6240` - HPX does not compile with G++ version prior to 9.3.
* :hpx-issue:`6232` - Par performance of hpx::reverse
* :hpx-issue:`6163` - Expose global termination detection through a new API
* :hpx-issue:`6014` - Support C++20 modules
* :hpx-issue:`5918` - HpxChannels Registration fails for written out template types
* :hpx-issue:`5907` - HPX_DEBUG/CMAKE_BUILD_TYPE not correctly decoupled/consistently used.
* :hpx-issue:`5903` - Support for Windows HPC Pack
* :hpx-issue:`5497` - Start using C++17 features unconditionally
* :hpx-issue:`5045` - Implement P0443/P1897/P2300
* :hpx-issue:`4702` - Prefer enum class over unscoped enums
* :hpx-issue:`4697` - get_num_localities and get_locality_id should return size_t
* :hpx-issue:`4672` - Consistently use hpx::function_* instead of std::function
* :hpx-issue:`4657` - HEP 1 implementation tracker
* :hpx-issue:`4367` - Rewrite history to remove unneeded binary files
* :hpx-issue:`4329` - HPX 2
* :hpx-issue:`4074` - HPX module "configurations"
* :hpx-issue:`2235` - Concurrent data structures support

Merged pull requests
====================

* :hpx-pr:`7689` - build(deps): bump lukka/get-cmake from 4.4.2 to 4.4.4
* :hpx-pr:`7688` - Make replicate vote tests deterministic
* :hpx-pr:`7687` - Fix launch::fork dropping tasks from OS threads
* :hpx-pr:`7684` - docs: use correct HPX::iostreams_component target name in quickstart
* :hpx-pr:`7683` - godbolt-minimal: pass /SUBSYSTEM:CONSOLE in the raw MSVC path
* :hpx-pr:`7682` - Ensure shared priority scheduler creates thread when requested
* :hpx-pr:`7681` - futures: fix shared_future::get() postcondition in docs
* :hpx-pr:`7679` - Fix parallel scheduler tests on Clang 16
* :hpx-pr:`7677` - fix(futures): upgrade memory_order_relaxed to acquire after cond_var wakeup in wait()
* :hpx-pr:`7674` - Enable specifying the number of vector lanes to use with SIMD execution policies
* :hpx-pr:`7673` - Add Linux CI workflow with _GLIBCXX_DEBUG enabled
* :hpx-pr:`7669` - tracing: move export off Tracy class heads for GCC 11 and 12
* :hpx-pr:`7668` - Bump dawidd6/action-download-artifact from 26 to 27
* :hpx-pr:`7665` - debugging: name dbghelp.lib from backtrace.cpp on MSVC
* :hpx-pr:`7664` - Support HPX::auto_wrap_main on MSVC
* :hpx-pr:`7661` - Alias lowercase Conan targets for Asio and Hwloc
* :hpx-pr:`7660` - Bump dawidd6/action-download-artifact from 25 to 26
* :hpx-pr:`7658` - Install hpx-top.py and hpx_stat_viewer.py
* :hpx-pr:`7657` - Execution/complete p2079
* :hpx-pr:`7654` - cmake: export the Tracy targets so consumers can find HPX
* :hpx-pr:`7653` - Make partitioned vector iterators model random access
* :hpx-pr:`7652` - execution_base: use the static empty vtable fallback on all Windows builds
* :hpx-pr:`7644` - godbolt-minimal: CI-test and document the raw MSVC hpx_main path
* :hpx-pr:`7643` - lcos_local: cancel pending get/set requests when closing a one_element_channel
* :hpx-pr:`7642` - Bump dawidd6/action-download-artifact from 24 to 25
* :hpx-pr:`7641` - Bump github/codeql-action from 4.38.1 to 4.38.2
* :hpx-pr:`7640` - Fix/coroutine asan stack recycling
* :hpx-pr:`7637` - Update Collective Benchmark
* :hpx-pr:`7636` - tracing: fill in the ITT backend ? shared domain, regions, markers
* :hpx-pr:`7635` - components_base: fix wrapper heap reclamation
* :hpx-pr:`7634` - Mark zero-copy save chunks as const and harden vector chunk tests
* :hpx-pr:`7633` - Add companion concepts for futures, execution policies, and iterators
* :hpx-pr:`7632` - Re-enable auto-linking on Windows (dynamic linking, C++20 modules)
* :hpx-pr:`7631` - debugging: suppress C4251 warning in dbghelp_symbol_cache.hpp
* :hpx-pr:`7637` - Update Collective Benchmark
* :hpx-pr:`7631` - debugging: suppress C4251 warning in dbghelp_symbol_cache.hpp
* :hpx-pr:`7630` - Clarify nostack fork_join leaf use and harden UINT32 range / exception paths.
* :hpx-pr:`7629` - Recycle terminated thread_data before malloc on recursive async spawn
* :hpx-pr:`7628` - Wire abp-priority queues to lockfree_abp_{fifo,lifo} backends
* :hpx-pr:`7627` - fix AGAS background GC when networking is disabled
* :hpx-pr:`7626` - Prefer MADV_FREE when recycling mmap'd coroutine stacks
* :hpx-pr:`7625` - Re-enable auto-linking on Windows(dynamic linking, no C++20 modules)
* :hpx-pr:`7624` - Re-enable auto-linking on Windows(static linking, including C++20 modules)
* :hpx-pr:`7621` - performance_counters: use public counter-type API in registry_thread_safety_7563 test
* :hpx-pr:`7620` - tracing: fill two small gaps in the Tracy backend`
* :hpx-pr:`7619` - fix node-specific HPX command-line option parsing
* :hpx-pr:`7618` - docs: document HPX_REGISTER_CHANNEL and template type restriction
* :hpx-pr:`7617` - synchronization: fix incorrect restart-state comparison in counting_semaphore::wait_until
* :hpx-pr:`7615` - Fix std::get on types derived from std::pair with HPX_DATASTRUCTURES\_...
* :hpx-pr:`7614` - Fix HPX_REGISTER_CHANNEL 2-arg form still pasting type into identifiers
* :hpx-pr:`7612` - core: add std::chrono overloads of finalize and disconnect
* :hpx-pr:`7611` - Core: Fix spurious wakeup vulnerability in hpx::barrier
* :hpx-pr:`7609` - debugging: refresh DbgHelp module list on stale symbol lookup
* :hpx-pr:`7607` - Get the Rostam Jenkins lanes passing again
* :hpx-pr:`7606` - Re-enable auto-linking on Windows(static linking, no C++20 modules) (#7592)
* :hpx-pr:`7605` - build(deps): bump github/codeql-action from 4.38.0 to 4.38.1
* :hpx-pr:`7604` - thrust: rename policy_traits test to thrust_policy_traits to avoid ta...
* :hpx-pr:`7601` - debugging: cache DbgHelp symbol lookups
* :hpx-pr:`7600` - Use std::size_t for matrix and block sizes in the transpose examples
* :hpx-pr:`7599` - actions: emit a stub on device so the action types exist
* :hpx-pr:`7598` - Optimize: Weaken atomic memory orderings in when_all_vector
* :hpx-pr:`7597` - fix synchronous channel communicator race
* :hpx-pr:`7596` - Use std::size_t for grid and partition counts in the examples
* :hpx-pr:`7595` - Convert iterator differences and strto* results explicitly
* :hpx-pr:`7594` - Check collection sizes read from an archive before using them
* :hpx-pr:`7593` - Carry the thread heap counts as std::size_t
* :hpx-pr:`7590` - Stop cancelling running Jenkins builds
* :hpx-pr:`7588` - Carry the idle and busy loop counts as std::int64_t
* :hpx-pr:`7587` - docs: document future callback and async reference pitfalls (#2589)
* :hpx-pr:`7586` - Take a std::size_t item count in the bulk dequeue path
* :hpx-pr:`7582` - Return std::int64_t from the queue holder thread counters
* :hpx-pr:`7581` - Raise documentation build step timeouts to restore headroom
* :hpx-pr:`7579` - Add get_parallel_scheduler(pool) for named HPX thread pools
* :hpx-pr:`7577` - Make fibhash return std::size_t
* :hpx-pr:`7575` - Stop hiding errors in the PDF documentation build
* :hpx-pr:`7574` - Don't define HPX config macros while finding an installed HPX
* :hpx-pr:`7573` - Restore the default handler before aborting
* :hpx-pr:`7572` - Keep the real PAPI error and skip the test when the counter is missing
* :hpx-pr:`7571` - Give each worker its own random generator for work stealing
* :hpx-pr:`7570` - Make godbolt-minimal ship hpx/experimental/sandbox.hpp
* :hpx-pr:`7567` - Keep LSU CI results when builds are interrupted
* :hpx-pr:`7566` - Add a 32 bit Windows CI job
* :hpx-pr:`7565` - Issue #7480 fix: AGAS RPC timeout during hosted locality namespace bootstrap polling
* :hpx-pr:`7564` - performance_counters: synchronize registry access to countertypes\_
* :hpx-pr:`7562` - performance_counters: discover counters registered after startup
* :hpx-pr:`7559` - make operator_brackets_proxy transparent to hpx::get for tuple-like references
* :hpx-pr:`7558` - build(deps): bump github/codeql-action from 4.37.9 to 4.38.0
* :hpx-pr:`7556` - fix: drop redundant HPX_CORE_EXPORT on version check definitions
* :hpx-pr:`7555` - Fix: Resolve deterministic deadlock and race condition in latch::reset
* :hpx-pr:`7554` - Exclude docs from Codacy's duplication analysis
* :hpx-pr:`7553` - tracy: serialise DbgHelp calls with HPX on Windows
* :hpx-pr:`7552` - config: remove TBB example benchmarks and unused FindTBB module
* :hpx-pr:`7551` - Add orthogonal rebind_policy_executor_t / rebind_policy_parameters_t
* :hpx-pr:`7550` - Feat : policy traits 6717
* :hpx-pr:`7546` - debugging: add regression test for hostname_print_helper race
* :hpx-pr:`7545` - Segmented merge
* :hpx-pr:`7544` - tracing: sample per-task lifecycle events 1-in-N
* :hpx-pr:`7542` - Use an HPX-aware mutex in numa_binding_allocator::initialize_pages
* :hpx-pr:`7541` - Factor the duplicated AGAS instance-name formatting into a shared helper
* :hpx-pr:`7540` - Explicitly disable the use of TBB as the parallelization backend for libstdc++
* :hpx-pr:`7536` - Factor the disconnected-locality dispatch guard into a shared helper
* :hpx-pr:`7534` - Keep LSU matrix artifacts separate
* :hpx-pr:`7533` - Stop failed LSU builds from publishing installs
* :hpx-pr:`7532` - Fix LSU GitHub status reporting
* :hpx-pr:`7531` - Bound Slurm waits in LSU CI
* :hpx-pr:`7530` - Add feature build check for std::filesystem::display_string
* :hpx-pr:`7529` - Tracing: add per-subsystem gates for event classes
* :hpx-pr:`7528` - Tracy: bump to v0.14.1
* :hpx-pr:`7527` - Keep the caching allocator's thread_local lookup out of line
* :hpx-pr:`7526` - Avoid communicator reuse between collectives test phases
* :hpx-pr:`7525` - Fix data race in debug hostname printing
* :hpx-pr:`7524` - collectives, colocated, parcelset: drop whole-file device-code guards
* :hpx-pr:`7523` - async_distributed: add distributed tests for reflect sync, post, dataflow, and async_continue
* :hpx-pr:`7522` - docs: note HPX-specific names without std counterparts
* :hpx-pr:`7521` - Whiten the Dijkstra locality before the token is transmitted
* :hpx-pr:`7518` - config: skip the decltype form of HPX_FORWARD under nvcc
* :hpx-pr:`7516` - Bound the Dijkstra termination probe retry when the token cannot be delivered
* :hpx-pr:`7515` - async_distributed: add reflection overloads for async_continue, post_cb policy, and dataflow
* :hpx-pr:`7514` - Fill in documentation gaps for the tracing modules
* :hpx-pr:`7511` - parcelset,tracing: Emit Tracy events for parcel send and receive
* :hpx-pr:`7510` - Disable force_disconnect tests if not configured
* :hpx-pr:`7509` - async_distributed: add launch policy overloads for reflect sync and async_cb
* :hpx-pr:`7508` - examples: add factorial_reflection demonstrating C++26 reflection API
* :hpx-pr:`7507` - Make the nvcc CUDA configurations compile and pass again
* :hpx-pr:`7506` - Fix the two shutdown regressions from #7471 that hang the distributed CIs
* :hpx-pr:`7505` - Waiting for threads in pools to start running before continuing
* :hpx-pr:`7504` - build(deps): bump github/codeql-action from 4.37.8 to 4.37.9
* :hpx-pr:`7501` - Fixing apparent multi-line comment
* :hpx-pr:`7500` - Stop the colocated tests finalizing from inside the hpx_main loop
* :hpx-pr:`7499` - examples: rename background_work_smoke subsystems to fix HPX_WITH_CUDA build
* :hpx-pr:`7498` - Update Sphinx/Breathe version requirements in documentation contributing guide
* :hpx-pr:`7497` - tracing: skip Tracy work when no profiler is connected
* :hpx-pr:`7496` - Dijkstra termination detection
* :hpx-pr:`7495` - Restart the Dijkstra termination probe cursor for every probe
* :hpx-pr:`7494` - Remove remnants of sentinel mentions in supervision docs
* :hpx-pr:`7493` - Set SLURM_MPI_TYPE=pmix for the rostam CIs
* :hpx-pr:`7492` - Check that parcels to a killed locality report an error
* :hpx-pr:`7491` - Make force_disconnect claim a locality before removing it
* :hpx-pr:`7490` - Fixing force_disconnect issue
* :hpx-pr:`7488` - build(deps): bump dawidd6/action-download-artifact from 23 to 24
* :hpx-pr:`7487` - build(deps): bump github/codeql-action from 4.37.7 to 4.37.8
* :hpx-pr:`7486` - Used atomic work-time count to replace empty() check in thread_queue.hpp
* :hpx-pr:`7484` - tracing: Instrument background work zones and OS worker thread sleep events
* :hpx-pr:`7482` - Enable missing use cases for the late launcher example
* :hpx-pr:`7481` - Cache channel communicators separately for each site
* :hpx-pr:`7479` - Fix cross-references in documentation
* :hpx-pr:`7478` - Use dedicated AGAS-specific RPC timeout for hosted namespaces
* :hpx-pr:`7477` - Implement P3149
* :hpx-pr:`7476` - Fix C++23 fold_left_first and fold_right_last return type deduction
* :hpx-pr:`7472` - build(deps): bump dawidd6/action-download-artifact from 21 to 23
* :hpx-pr:`7471` - Implementing aborting dynamic worker for late component launcher example
* :hpx-pr:`7466` - build(deps): bump github/codeql-action from 4.37.6 to 4.37.7
* :hpx-pr:`7465` - Updating weblinks to new hpx.dev website
* :hpx-pr:`7464` - Fix cpp-dependencies parser crash by removing quotes in comments
* :hpx-pr:`7462` - async_distributed: add C++26 reflection overloads for sync, post, async_cb, post_cb
* :hpx-pr:`7460` - fix(async_distributed): add missing Enable=void to promise_lco dtor policy specialization
* :hpx-pr:`7459` - actions_base: add runtime dispatch overhead benchmark
* :hpx-pr:`7458` - Adding helper simplifying to launch a connecting locality
* :hpx-pr:`7457` - Supervision module and dispatch fixes and cleanup
* :hpx-pr:`7456` - tests: expand hpx::search_n unit tests to match HPX algorithm test standards
* :hpx-pr:`7455` - examples: add fibonacci_reflection demonstrating C++26 reflection API
* :hpx-pr:`7453` - fix(execution_base): complete P2300 compliance and fix set_stopped crash for any_sender
* :hpx-pr:`7451` - Remove stale hpx_tag_invoke module dependencies
* :hpx-pr:`7450` - tracing: Add work-stealing instrumentation for Tracy profiler
* :hpx-pr:`7449` - Fix pairwise all_to_all build with Clang 17 and OpenMP
* :hpx-pr:`7448` - components_base: add reflection-based default for get_component_name
* :hpx-pr:`7447` - Adding hpx::force_disconnect to support the use case described in #7441
* :hpx-pr:`7446` - Documentation fixes
* :hpx-pr:`7445` - Add collectives API documentation metadata
* :hpx-pr:`7443` - final tag_invoke cleanup
* :hpx-pr:`7442` - build(deps): bump github/codeql-action from 4.37.4 to 4.37.6
* :hpx-pr:`7438` - Supervision dispatch locality and join epochs
* :hpx-pr:`7437` - fix(execution): prevent silent hang in make_future with run_loop_scheduler
* :hpx-pr:`7436` - runtime_components: add distributed integration test for reflect_action
* :hpx-pr:`7435` - Fix EVE/simd unaligned load and store broadcast
* :hpx-pr:`7434` - Add contract preconditions to sorting/heap algorithms
* :hpx-pr:`7433` - Bump lukka/get-cmake from 4.4.1 to 4.4.2
* :hpx-pr:`7432` - supervision_dispatch: add C++26 reflection overload for dispatch_work
* :hpx-pr:`7431` - Add size-based pairwise dispatch to all_to_all
* :hpx-pr:`7429` - Bump github/codeql-action from 4.37.3 to 4.37.4
* :hpx-pr:`7427` - Supervision dispatch and target resolution
* :hpx-pr:`7426` - Fix the build with HPX_WITH_THREAD_QUEUE_WAITTIME=ON
* :hpx-pr:`7425` - Adding support for fenced dispatch to supervision component
* :hpx-pr:`7423` - tracing: add causal tracing API for Tracy
* :hpx-pr:`7422` - Bump github/codeql-action from 4 to 4.37.3
* :hpx-pr:`7420` - CodeRabbit CI Fix: Fix ARM SVE CI build failures
* :hpx-pr:`7419` - Advertise the parcelport port that was actually bound
* :hpx-pr:`7418` - actions_base: annotation-driven action registration via C++26 reflection
* :hpx-pr:`7417` - components: add hpx::launch::sync overload to reflect_client
* :hpx-pr:`7416` - Bump lukka/get-cmake from 4.3.4 to 4.4.1
* :hpx-pr:`7415` - Bump actions/stale from 10 to 11
* :hpx-pr:`7414` - Don't disabled first core placement if --hpx:bind=none
* :hpx-pr:`7413` - Adding supervision dispatch component
* :hpx-pr:`7412` - Give the launched locality the endpoint the probe actually bound
* :hpx-pr:`7410` - tag_invoke cleanup
* :hpx-pr:`7409` - Implement check_admission in supervision module
* :hpx-pr:`7408` - Adding a set of tests for pool_timer
* :hpx-pr:`7407` - Adding await_terminal support to supervision module
* :hpx-pr:`7405` - Let the OS pick the parcelport port in the departed locality test
* :hpx-pr:`7404` - Make MPI parcelport teardown orderly: quiesce before MPI_Finalize
* :hpx-pr:`7403` - Tracing inline future continuations
* :hpx-pr:`7402` - Feat/distributed adaptors
* :hpx-pr:`7401` - Cache the first collective finalizer failure and rethrow it for every site
* :hpx-pr:`7400` - remove tag_invoke from algorithm loop
* :hpx-pr:`7399` - Add supervision infrastructure for lifecycle events and observers
* :hpx-pr:`7398` - Move the all_reduce reduction seed instead of copying it
* :hpx-pr:`7396` - Move distributed sender&receiver facilities to separate module
* :hpx-pr:`7395` - remove tag_invoke from container algorithms
* :hpx-pr:`7394` - execution: fix sync_wait notify lifetime race
* :hpx-pr:`7393` - Add projection tests for ranges::unique algorithms
* :hpx-pr:`7392` - Tracing data parallel workloads
* :hpx-pr:`7391` - Fix doxygen warnings when building docs
* :hpx-pr:`7388` - Fix resolve_locality error handling for departed localities
* :hpx-pr:`7387` - Don't apply PU offset for localities that explicitly use core bindings
* :hpx-pr:`7385` - components: add reflect_client<^^Server> and HPX_REFLECT_CLIENT macro
* :hpx-pr:`7382` - Fixing issue with the latch_notification test
* :hpx-pr:`7380` - remove tag invoke from parallel algorithms
* :hpx-pr:`7379` - Thread data size reductions
* :hpx-pr:`7378` - Move communicator basenames into owned storage
* :hpx-pr:`7377` - Flatten hierarchical all-to-all exchange payloads
* :hpx-pr:`7376` - async: add hpx::async<^^func>(target, ...) reflection overload
* :hpx-pr:`7375` - Flatten hierarchical gather and scatter payloads
* :hpx-pr:`7374` - async_cuda: preserve transform_stream environment
* :hpx-pr:`7372` - Tracing suspend resume hooks
* :hpx-pr:`7371` - Working around Clang ICE happening when compiling the algorithm sender tests
* :hpx-pr:`7369` - Reject explicit generations after auto generation on the same communicator
* :hpx-pr:`7368` - remove tag_invoke from core execution
* :hpx-pr:`7367` - Fixing future::wait_until (and wait_for) to return once future was made ready
* :hpx-pr:`7366` - Adding unit tests for and_gate
* :hpx-pr:`7365` - add sender/receiver support to set_intersection
* :hpx-pr:`7364` - Avoid communicator reuse between scan test phases
* :hpx-pr:`7363` - Implement minmax_element to use less comparisons
* :hpx-pr:`7361` - add pre commit hooks
* :hpx-pr:`7360` - Duplication of the thread queues in the schedulers
* :hpx-pr:`7359` - Harden collectives validation paths
* :hpx-pr:`7357` - Tracing runtime instrumentation
* :hpx-pr:`7356` - Allowing for a test to fail only for a given configuration
* :hpx-pr:`7355` - keep future bridge redesign
* :hpx-pr:`7354` - Feat/distributed transfer
* :hpx-pr:`7353` - remove tag_invoke usage
* :hpx-pr:`7352` - ci: add actions_base reflection tests to CI workflows
* :hpx-pr:`7350` - Introduces better lifetime management for background/pool threads
* :hpx-pr:`7349` - actions_base: add reflect_direct_action and direct action macro reflection support
* :hpx-pr:`7348` - docs: add C++26 reflection-based action migration guide
* :hpx-pr:`7347` - Add tracing support for task lifecycle events
* :hpx-pr:`7346` - feat(algorithms): implement P2300 allocator support for when_all_vector
* :hpx-pr:`7344` - add sender/receiver support to set_difference
* :hpx-pr:`7343` - Add hierarchical scan collectives
* :hpx-pr:`7342` - actions_base: fix reflect_action for noexcept functions, add arity
* :hpx-pr:`7341` - async_mpi: modernize transform_mpi sender participation
* :hpx-pr:`7340` - Harden collectives validation paths
* :hpx-pr:`7339` - Fixing various test failures.
* :hpx-pr:`7337` - tracing: remove legacy ITT hooks from full components
* :hpx-pr:`7336` - removed unnecessary thread id info in merge
* :hpx-pr:`7335` - feat(executors): implement domain-optimized continues_on for thread_p...
* :hpx-pr:`7334` - Fixing shared_ptr serialization vulnerabilities
* :hpx-pr:`7332` - actions_base: add compile-time benchmark for macro vs reflect_action
* :hpx-pr:`7331` - async_mpi: use member async_execute on mpi executor
* :hpx-pr:`7330` - async_cuda: use member dispatch for executor entry points
* :hpx-pr:`7329` - build(deps): bump lukka/get-cmake from 4.3.3 to 4.3.4
* :hpx-pr:`7328` - fix(datapar): guard mismatch SIMD path on zip_iterator compatibility
* :hpx-pr:`7327` - build(deps): bump actions/checkout from 6 to 7
* :hpx-pr:`7326` - Hierarchical collectives: unify the per-call generation step for cross-collective sharing
* :hpx-pr:`7324` - Improve collective benchmark
* :hpx-pr:`7322` - Fix sentinel return type mismatch in hpx::ranges container algorithm CPOs
* :hpx-pr:`7321` - Harden hierarchical collectives: flat fallback unification, input validation, tree test coverage
* :hpx-pr:`7320` - feat(execution): implement get_allocator query for P2300 schedulers
* :hpx-pr:`7319` - Remove tag_invoke from executor layer CPOs
* :hpx-pr:`7318` - async_mpi: preserve sender environment
* :hpx-pr:`7317` - Decouple ITTNotify synchronization via hpx::tracing
* :hpx-pr:`7316` - test(execution): add comprehensive cooperative tests for let_stopped and upon_stopped
* :hpx-pr:`7315` - make_future: use run_loop scheduler accessor
* :hpx-pr:`7314` - parallel: Unify projection support and fix constraints for starts_with and ends_with
* :hpx-pr:`7311` - actions_base: add reflection-based component action (reflect_component_action)
* :hpx-pr:`7309` - keep_future: expose empty env
* :hpx-pr:`7307` - Add hierarchical all_to_all collective
* :hpx-pr:`7306` - Fix warnings and errors when building docs
* :hpx-pr:`7304` - Fix max_element returning last greatest element instead of first
* :hpx-pr:`7303` - Patching recently introduced issues on Windows/MSVC
* :hpx-pr:`7302` - Disable logging on the github CI to avoid out of disk space errors
* :hpx-pr:`7301` - Remove `tag_invoke` and use member functions
* :hpx-pr:`7300` - build(deps): bump lukka/get-cmake from 4.3.2 to 4.3.3
* :hpx-pr:`7299` - Fixing various smaller issues in build system
* :hpx-pr:`7298` - actions_base: add auto-registration to reflect_action
* :hpx-pr:`7297` - Fixing execution parameters test
* :hpx-pr:`7296` - add S/R support for shift_left and shift_right
* :hpx-pr:`7295` - Execution parameter num_chunks fix in CPO
* :hpx-pr:`7294` - algorithms: fix operator!= inconsistency in prefetching_iterator
* :hpx-pr:`7293` - Unify APEX tracing backend under hpx::tracing API
* :hpx-pr:`7292` - replace tag_invoke(connect_t) friends with member connect()
* :hpx-pr:`7291` - fix return type
* :hpx-pr:`7290` - build(deps): bump lukka/get-cmake from 4.3.2 to 4.3.3
* :hpx-pr:`7289` - Adapt remaining HPX Modules to C++20 modules
* :hpx-pr:`7286` - Adapting modules of level 39/40 to C++20 modules
* :hpx-pr:`7285` - Avoid forcing inline on debug builds
* :hpx-pr:`7284` - tracing: suppress MSVC C4251 warning, remove stale comment
* :hpx-pr:`7283` - ci: fix ittnotify ref-collector build in CI
* :hpx-pr:`7282` - make_future: drop static_assert from detail::make_future
* :hpx-pr:`7281` - actions_base: integrate reflect_action with basic_action
* :hpx-pr:`7280` - actions_base: reimplement HPX_DEFINE_PLAIN_ACTION_2 using C++26 reflection
* :hpx-pr:`7279` - Adapting HPX modules from level 37 and 38 to C++20 modules
* :hpx-pr:`7278` - serialization: fix double-read and type mismatch in exception_ptr load()
* :hpx-pr:`7277` - async_mpi: harden transform_mpi callback helpers with static_asserts on receiver shape
* :hpx-pr:`7276` - run_loop: advertise sync_wait_domain via get_completion_domain on env_t
* :hpx-pr:`7275` - Excluding tests from Codacy analysis
* :hpx-pr:`7274` - Improve performance of is_heap and is_heap_until
* :hpx-pr:`7273` - Level36 modules
* :hpx-pr:`7272` - execution: Add scheduler-aware overload to as_sender() for P2300 pipe...
* :hpx-pr:`7271` - Adapting async_distributed module to C++20 modules
* :hpx-pr:`7270` - Adapting parcelset module to C++20 modules
* :hpx-pr:`7269` - algorithms: fix predicate constraint in hpx::unique_copy CPO
* :hpx-pr:`7268` - Adapting components module to C++20 modules
* :hpx-pr:`7267` - Add DCO to check if commits are signed
* :hpx-pr:`7266` - actions_base: add reflection-based action definition (reflect_action)
* :hpx-pr:`7264` - tracing: add lock_context and migrate synchronization primitives to unified tracing
* :hpx-pr:`7263` - tracing: add hpx::tracing::rename_region abstraction
* :hpx-pr:`7261` - Removing export declarations from non-primary templates
* :hpx-pr:`7260` - Feat/fork join executor p2300 bridge
* :hpx-pr:`7259` - [test] Add projection tests for hpx::ranges::{unique_copy, partition_copy, remove_copy, remove_copy_if}
* :hpx-pr:`7258` - Add ITTNotify CI workflow and smoke test
* :hpx-pr:`7257` - Migrate to stdexec & Fix deadlock on HPX worker threads
* :hpx-pr:`7256` - Implement HPX Future-Sender Bridge (P2300 interoperability)
* :hpx-pr:`7255` - Re-enable automatically closing stale issues
* :hpx-pr:`7253` - Add parallel distributed algorithms for segmented copy
* :hpx-pr:`7250` - Unified tracing API: merge ITT/Tracy instrumentation
* :hpx-pr:`7249` - ci: pin VS 2022 workflows to windows-2022 runner
* :hpx-pr:`7248` - make_future: static_assert against silent-hang run_loop_scheduler sender
* :hpx-pr:`7247` - make_future: isolate __loop\_ private-member access in single detail helper
* :hpx-pr:`7246` - fix #7245: switch Stdexec FetchContent_Populate to FetchContent_MakeA vailable
* :hpx-pr:`7244` - drop dead trait, dead test guards, and migrate async_mpi receiver to native P2300
* :hpx-pr:`7242` - Update HPX URL for perftest commenting
* :hpx-pr:`7240` - Implement P2300 bulk adapter for HPX executors
* :hpx-pr:`7239` - Implement P2300 get_scheduler bridge for parallel_executor
* :hpx-pr:`7238` - Implement P2300 get_scheduler bridge for executors
* :hpx-pr:`7237` - Adapting HPX actions module to C++20 modules
* :hpx-pr:`7235` - Silence Codacy warnings
* :hpx-pr:`7234` - Bump lukka/get-cmake from 4.3.0 to 4.3.2
* :hpx-pr:`7233` - Suppress MSVC linker warnings in C++ 20 module mode
* :hpx-pr:`7232` - ci: switch to lukka/get-cmake to resolve GitHub API rate limits
* :hpx-pr:`7231` - Bump dawidd6/action-download-artifact from 20 to 21
* :hpx-pr:`7230` - algorithms: expose Proj parameter in hpx::min/max/minmax_element CPOs
* :hpx-pr:`7227` - Adding const version of some of the looping constructs to support datapar
* :hpx-pr:`7224` - Making sure chunking iterators are detected as random access
* :hpx-pr:`7223` - Fix missing invoke header in for_each_index algorithm
* :hpx-pr:`7220` - [parallel] Add projection support to hpx::is_sorted, hpx::is_sorted_until, and hpx::is_partitioned CPOs
* :hpx-pr:`7219` - Fix iteration mismatch in collective tests local timing
* :hpx-pr:`7218` - Multi line warning fix
* :hpx-pr:`7217` - Allow using counting_iterator with datapar policies
* :hpx-pr:`7216` - tracing: add hpx::tracing::set_thread_name abstraction
* :hpx-pr:`7215` - Implement hpx::experimental::for_each_index (P4150R0)
* :hpx-pr:`7213` - Use scanned tree commit in inspect links
* :hpx-pr:`7211` - [parallel] Fix incorrect return types in max_element and minmax_element parallel reduction
* :hpx-pr:`7210` - Reimplement distributed::barrier on top of collectives infrastructure
* :hpx-pr:`7209` - executors: fix performance regressions
* :hpx-pr:`7208` - [parallel] Fix element_type deduction in minmax algorithms
* :hpx-pr:`7206` - Fix C++ modules BMI installation and re-enable external build tests
* :hpx-pr:`7205` - CI: Add manual "Cancel All Workflows" automation via label
* :hpx-pr:`7204` - serialization: add reflection tests for static member function name e...
* :hpx-pr:`7201` - Deduplicate Windows tests.examples exclusions
* :hpx-pr:`7198` - Add non-power-of-arity tests for hierarchical all_reduce and all_gather
* :hpx-pr:`7196` - [tests] Add return-iterator compliance and edge-case regression tests for hpx::partial_sort
* :hpx-pr:`7195` - docs: update .github markdown links to new TheHPXProject organization
* :hpx-pr:`7194` - Enable static component registration in application executables
* :hpx-pr:`7193` - Add flat-collective fallback to hierarchical_communicator for small site counts
* :hpx-pr:`7192` - Adding parameters hook function for partition management
* :hpx-pr:`7191` - [parallel] Fix hpx::nth_element to correctly support C++20 projections and sentinels
* :hpx-pr:`7190` - Adapting HPX to C++ modules
* :hpx-pr:`7189` - Add unit tests for hierarchical all_reduce and all_gather
* :hpx-pr:`7188` - fix(algorithms): fix hpx::rotate return-iterator compliance and add regression tests
* :hpx-pr:`7187` - Fix memory leak in overlapping relocation test functions (#7184)
* :hpx-pr:`7186` - Implement segmented version of mismatch algorithm
* :hpx-pr:`7183` - tests(algorithms): fix return-iterator C++20 compliance for hpx::shift_left and hpx::shift_right
* :hpx-pr:`7182` - fix: add runtime precondition checks to connection_cache public methods
* :hpx-pr:`7181` - Making cached_allocator thread safe
* :hpx-pr:`7180` - Add static plugin loading support for HPX plugins
* :hpx-pr:`7179` - Adding datapar emulation layer for platforms that don't support it
* :hpx-pr:`7178` - Remove leftover Boost headers
* :hpx-pr:`7177` - Fixing size of HPX Logo in generated reports
* :hpx-pr:`7176` - Fixing more links to new repository
* :hpx-pr:`7175` - tests(algorithms): ensure cross-policy consistency and add missing edge cases
* :hpx-pr:`7174` - Remove CircleCI testing
* :hpx-pr:`7173` - Update README.rst
* :hpx-pr:`7172` - Update dependency report and inspect to generate links to new repository
* :hpx-pr:`7171` - Add optional Orbit MPMC comparison benchmark
* :hpx-pr:`7170` - tests(adjacent_find): Add missing edge-case tests for empty and boundary ranges
* :hpx-pr:`7169` - Fix parallel is_sorted_until off by one cross boundary check
* :hpx-pr:`7168` - Co-locate SPSC channel cached indices with their atomics
* :hpx-pr:`7166` - parcelset: add connection cache saturation counters and exhaustion alerting
* :hpx-pr:`7164` - pass executor to hpx::dataflow in sort and partial_sort
* :hpx-pr:`7163` - Fix missing early termination in parallel `find_first_of` inner loop
* :hpx-pr:`7162` - partial ordering tests for merge
* :hpx-pr:`7161` - fix(find_first_of): add missing early return and fix datapar iterator dependency
* :hpx-pr:`7160` - Add hierarchical all_reduce and all_gather via reduce+broadcast composition
* :hpx-pr:`7159` - concurrency: harden anchor consistency checks in lockfree::deque
* :hpx-pr:`7156` - Add segmented is_partitioned
* :hpx-pr:`7155` - docs: add 'Using HPX on Compiler Explorer' manual page
* :hpx-pr:`7154` - ci: add CI workflow validating godbolt-minimal preset
* :hpx-pr:`7153` - Optimize exception safety in zip_iterator by adding missing noexcept
* :hpx-pr:`7152` - ci: restore dropped dynamic workflow exclusions
* :hpx-pr:`7149` - Allow for fork_join_executor to work with stackless HPX threads
* :hpx-pr:`7147` - uninitialized_relocate: use pointer arithmetic for overlap distance calculation
* :hpx-pr:`7146` - enable auto-cancellation for all CI workflows
* :hpx-pr:`7145` - Tracing suspend abstraction
* :hpx-pr:`7144` - feat(tools): add hpx_stat_viewer with Advanced Heuristic Anomaly Detection & Sparkline Trends
* :hpx-pr:`7143` - Separate template parameters for parallel is_partitioned, fill_n, uninitialized_fill
* :hpx-pr:`7142` - Optimize datapar SIMD loop bounds and resolve C++20 sentinel bugs
* :hpx-pr:`7141` - Use is_default() consistently in hierarchical gather_here
* :hpx-pr:`7140` - serialization: add reflection tests for function name extraction
* :hpx-pr:`7139` - Limiting the amount of objects that are being cached in memory pools
* :hpx-pr:`7138` - cmake: improve godbolt-minimal preset for Compiler Explorer
* :hpx-pr:`7137` - chore(deps): bump dawidd6/action-download-artifact from 19 to 20
* :hpx-pr:`7136` - Fix copy-paste errors in collectives Doxygen comments
* :hpx-pr:`7135` - datapar: resolve non-deterministic flaky tests caused by SIMD cancellation UB
* :hpx-pr:`7134` - improve parallelism in ci
* :hpx-pr:`7133` - FIx wrap_main for local-runtime builds
* :hpx-pr:`7132` - Fix Sender Algorithm Customization
* :hpx-pr:`7130` - Add local-runtime fallback for hpx/iostream.hpp
* :hpx-pr:`7129` - contracts: add violation handler infrastructure and annotated hpx::future
* :hpx-pr:`7128` - Adapting components_base HPX module to C++ modules
* :hpx-pr:`7127` - bounded versions of segmented generate and fill
* :hpx-pr:`7125` - Fix 1d_stencil_5 partition buffer ownership
* :hpx-pr:`7123` - Clean up stdexec-only paths and update affected tests
* :hpx-pr:`7122` - fix use after move errors in replace_copy
* :hpx-pr:`7121` - Switching from test-branch to master for docs_push
* :hpx-pr:`7120` - Fix exact test matching in container algorithm CI shards
* :hpx-pr:`7116` - Refactor build-and-test workflow into matrix + composite action
* :hpx-pr:`7115` - Cache read/write indices in channel_spsc
* :hpx-pr:`7114` - Fix heap corruption in partial_sort filter() function
* :hpx-pr:`7110` - Implement segmented version of equal algorithm
* :hpx-pr:`7108` - Update reflection_qualified_name_of tests with server/client request ...
* :hpx-pr:`7106` - feat(futures): implement C++23 monadic operations
* :hpx-pr:`7104` - Removing concurrency group from documentation build and push
* :hpx-pr:`7103` - feat: enable transparent hpx::init dispatch for local-only builds
* :hpx-pr:`7102` - Disable pushing generated docs from CircleCI
* :hpx-pr:`7101` - schedulers: add NUMA-distance-aware victim list and NUMA hint routing
* :hpx-pr:`7100` - Removed unused lambda captures
* :hpx-pr:`7097` - Fix scheduler_executor regression
* :hpx-pr:`7096` - Migrate documentation build and deploy to GitHub Actions
* :hpx-pr:`7095` - refactor: replace sizeof==0 hack with always_false in sort_by_key
* :hpx-pr:`7094` - Adding const where possible to tracing API
* :hpx-pr:`7093` - cmake: remove HPX_WITH_CXX20_STD_ENDIAN feature test
* :hpx-pr:`7092` - fix: improve static_assert messages in futures traits
* :hpx-pr:`7090` - Allowing to create fork_join_executor from parallel_executor
* :hpx-pr:`7089` - note sequential fallback for non-contiguous iterators in parallel uninitialized_relocate CPOs
* :hpx-pr:`7088` - revert slash commands
* :hpx-pr:`7086` - Added delta local accumulation inside thread cleanup loop
* :hpx-pr:`7084` - trigger workflows
* :hpx-pr:`7083` - Fix critical collectives correctness issues (hierarchical partitioning, scatter/all_to_all validation, barrier release re-entry)
* :hpx-pr:`7082` - Add Conan package support
* :hpx-pr:`7081` - test: add right-overlap tests for forward uninitialized_relocate CPOs
* :hpx-pr:`7079` - Feat/sandbox laboratory
* :hpx-pr:`7078` - examples: add sender_diamond quickstart example
* :hpx-pr:`7076` - add slash command
* :hpx-pr:`7072` - Add segmented is_sorted_until , is_sorted
* :hpx-pr:`7069` - Improve hpx_wrap error message with explicit linker flags for non-CMa...
* :hpx-pr:`7068` - cmake: remove HPX_WITH_CXX20_STD_EXECUTION_POLICES feature test
* :hpx-pr:`7067` - cmake: remove HPX_WITH_CXX20_LAMBDA_CAPTURE feature test
* :hpx-pr:`7066` - cmake: remove HPX_WITH_CXX20_STD_CONSTRUCT_AT feature test
* :hpx-pr:`7065` - cmake: Improve error messages for missing optional dependencies Improve dependency errors
* :hpx-pr:`7063` - Add godbolt-minimal CMake preset for browser-based compiler environments
* :hpx-pr:`7061` - build(deps): bump jwlawson/actions-setup-cmake from 2.1 to 2.2
* :hpx-pr:`7058` - Tracing unified interface
* :hpx-pr:`7057` - Add inspect checks to pre-commit and DCO commit-msg hook
* :hpx-pr:`7056` - Feat: Replaced the old lockfree queue with the moodycamel ConcurrentQ...
* :hpx-pr:`7055` - Modernized HPX algorithm headers
* :hpx-pr:`7054` - [Algorithms] Fix C++20 sentinel compatibility in find, mismatch, and search
* :hpx-pr:`7053` - Fix segmented subrange handling and scan bugs
* :hpx-pr:`7051` - cmake: remove HPX_WITH_CXX20_STD_RANGES_ITER_SWAP feature test
* :hpx-pr:`7048` - Fix formatting issue #7046 in print.cpp
* :hpx-pr:`7047` - Fix four critical bugs in sheneos distributed interpolation example
* :hpx-pr:`7045` - Fix copyright and HPSF link
* :hpx-pr:`7044` - [datapar] Fix sentinel compatibility in SIMD algorithms
* :hpx-pr:`7043` - build(deps): bump actions/download-artifact from 7 to 8
* :hpx-pr:`7042` - `is_sorted`: add independent iterator template parameters to support segmented algorithms
* :hpx-pr:`7041` - Fix exception safety risk in options_description_easy_init
* :hpx-pr:`7039` - fix compiler inconsistencies for fundamental types in reflecting names
* :hpx-pr:`7038` - [Core] Use hpx::parallel::detail::distance in unsequenced algorithms
* :hpx-pr:`7036` - implement missing standard methods in hpx::compute::vector
* :hpx-pr:`7033` - Replace actions/cache to actions/artifacts
* :hpx-pr:`7032` - Fix numa allocator test 7027
* :hpx-pr:`7031` - fix: handle immediate failures in mpi::experimental::detail::async
* :hpx-pr:`7029` - fix logic bug in async_replay exhaustion handling
* :hpx-pr:`7026` - replace 'return false' with 'return 0'
* :hpx-pr:`7024` - fix: validate thread_schedule_hint.hint bounds in shared_priority_queue_scheduler
* :hpx-pr:`7023` - Fix adaptation of the Tracy module to C++ modules
* :hpx-pr:`7021` - fix scheduler_executor bulk
* :hpx-pr:`7020` - Fix incorrect return iterators in buffer_memcpy relocation path
* :hpx-pr:`7018` - Fix heap corruption and out-of-bounds access in sort_thread partitioning
* :hpx-pr:`7017` - Enable Tracy fiber-based task tracing for HPX scheduler
* :hpx-pr:`7016` - support GCC trunk/C++26 reflection in build, tests and CI
* :hpx-pr:`7015` - Fix critical edge cases in reduce_by_key and improve sentinel support
* :hpx-pr:`7014` - Fix std::distance usage in is_partitioned and find_first_of
* :hpx-pr:`7012` - Added Dark mode to the website
* :hpx-pr:`7011` - Cleaning up minor issues that slipped into master
* :hpx-pr:`7010` - use optimised image for reflection ci
* :hpx-pr:`7008` - build(deps): bump actions/checkout from 5 to 6
* :hpx-pr:`7007` - performance: Optimize shared_mutex and fix C++20 modular build errors
* :hpx-pr:`7006` - Fix inaccurate comments in 1d_stencil_5 example
* :hpx-pr:`7005` - Fix clang-format issues across parallel algorithms and core libs
* :hpx-pr:`7004` - Updating inspect #include checker to look for more symbols from <type_traits>
* :hpx-pr:`7003` - tests: fix RNG seeding in several sender tests
* :hpx-pr:`7002` - Add HPX-Top: Real-time Terminal Performance Dashboard
* :hpx-pr:`6999` - Add distributed versions of replace, replace_if, replace_copy and replace_copy_if algorithms
* :hpx-pr:`6998` - Fix incorrect Python 3 print usage in hpxrun.py.in
* :hpx-pr:`6997` - tests: fix self-copy UB in move algorithm tests causing occasional segfault
* :hpx-pr:`6996` - cmake: remove HPX_WITH_CXX20_SOURCE_LOCATION feature test
* :hpx-pr:`6995` - datapar support for hpx::search and search_n
* :hpx-pr:`6994` - Small Code Quality Boost: Addressing long-standing FIXMEs and TODOs
* :hpx-pr:`6993` - Extending the index_queue_spawning facility to support returning values
* :hpx-pr:`6992` - build(deps): bump actions/upload-artifact from 6 to 7
* :hpx-pr:`6991` - Algorithm: Implementing hpx::iota and hpx::ranges::iota (#6969)
* :hpx-pr:`6990` - schedulers: fix stale domain_num/q_index after select_active_pu() in schedule_thread() none path
* :hpx-pr:`6988` - Fix segmented_algorithms CI and fail-on-cache-miss, Implement depreport workflow
* :hpx-pr:`6986` - Fixing unused variable warning/errors.
* :hpx-pr:`6985` - Fix critical race conditions in AGAS and improve component pinning robustness
* :hpx-pr:`6984` - Use rebind_parameters for the rotate algorithm implementation
* :hpx-pr:`6983` - cmake: remove HPX_WITH_CXX20_PAREN_INITIALIZATION_OF_AGGREGATES feature test
* :hpx-pr:`6981` - Fix bound_queue cleanup in work-requesting scheduler
* :hpx-pr:`6980` - cmake: remove HPX_WITH_CXX20_CONSTEXPR_DESTRUCTOR feature test
* :hpx-pr:`6979` - Fix set_error called on moved-from receiver in thread_pool_scheduler
* :hpx-pr:`6977` - cmake: remove HPX_WITH_CXX20_STD_DEFAULT_SENTINEL feature test
* :hpx-pr:`6976` - cmake: remove HPX_WITH_CXX20_STD_BIT_CAST feature test
* :hpx-pr:`6973` - cmake: remove HPX_WITH_CXX20_STD_IDENTITY feature test
* :hpx-pr:`6971` - Fix get_num_items logic in default_distribution_policy
* :hpx-pr:`6968` - schedulers: fix NUMA hint not validating suspended PUs
* :hpx-pr:`6967` - async_cuda, async_mpi: replace tuple-based keep_alive capture with C+...
* :hpx-pr:`6966` - cmake: remove HPX_WITH_CXX20_STD_ENDIAN feature test
* :hpx-pr:`6965` - executors: replace tuple-based lambda capture with C++20 pack capture
* :hpx-pr:`6964` - cmake: Remove obsolete feature test for CXX20_PERFECT_PACK_CAPTURE
* :hpx-pr:`6962` - Replace BLOCK_SIZE with different name to avoid clashes with Linux-defined macro
* :hpx-pr:`6959` - cmake: Remove obsolete feature test for CXX20_TRIVIAL_VIRTUAL_DESTRUCTOR
* :hpx-pr:`6958` - Re-add iterator traits
* :hpx-pr:`6956` - docs: add lifetime warning for create_channel_communicator
* :hpx-pr:`6955` - fix: correct argument order for `next_or_subrange` and `add_ready_future_idx` functions
* :hpx-pr:`6951` - Add zero-validation for with_processing_units_count
* :hpx-pr:`6950` - Merge Path Algorithm
* :hpx-pr:`6949` - Bump actions/upload-artifact from 6 to 7
* :hpx-pr:`6947` - executors: fix uint32_t truncation in fork_join static scheduling (Phase 1)
* :hpx-pr:`6946` - dynamic test discovery in CI
* :hpx-pr:`6944` - nit: remove linux specific header in test utils
* :hpx-pr:`6943` - Fix thread safety bugs in concurrent containers and a few algorithm issues
* :hpx-pr:`6942` - Add threshold validation to limiting_executor
* :hpx-pr:`6940` - fix clang tidy errors in hpx
* :hpx-pr:`6939` - Add zero-thread validation in fork_join_executor PU-mask constructor
* :hpx-pr:`6938` - fix dangling reference errors in iterator tests
* :hpx-pr:`6937` - enable fold range tests in gh actions
* :hpx-pr:`6936` - Working on fixing recent performance regressions
* :hpx-pr:`6935` - Experimenting with rebinding parameters objects
* :hpx-pr:`6934` - Fix parallel performance of hpx::reverse by using index-based dispatch
* :hpx-pr:`6933` - Misc fixes for documentation
* :hpx-pr:`6930` - Adjusting build system to HPX conventions
* :hpx-pr:`6928` - Replace scan implementation with explicit functions to avoid triggering concept checking
* :hpx-pr:`6927` - Attempt to work around Github API limitations
* :hpx-pr:`6926` - Disable async_cuda test as nvcc fails
* :hpx-pr:`6925` - implement c++20 modules for core libraries
* :hpx-pr:`6924` - shared_mutex : add ignore_while_checking around upgrade_cond notify in unlock_shared()
* :hpx-pr:`6923` - Convert enum message_buffer_append_state to enum class
* :hpx-pr:`6921` - Add workflow_dispatch for local testing and fix PR triggers
* :hpx-pr:`6920` - Fixing various compilation regressions
* :hpx-pr:`6918` - Bump actions/checkout from 4 to 6
* :hpx-pr:`6917` - Bump actions/upload-artifact from 4 to 6
* :hpx-pr:`6916` - Add pull_request triggers to GHA workflows and fixed check-formatting jobs
* :hpx-pr:`6915` - Adding missing module dependencies to the HPX algorithm module
* :hpx-pr:`6914` - Cleaning up recently added debug suffix configuration
* :hpx-pr:`6913` - Add comprehensive subtraction unit tests for gid_type
* :hpx-pr:`6910` - Implement C++23 Algorithms: fold_left, fold_right.
* :hpx-pr:`6909` - Refactoring local_priority_queue_scheduler for better cache behavior
* :hpx-pr:`6907` - reduce lock contention in local_priority_queue_scheduler via randomized victim selection
* :hpx-pr:`6903` - Migrate HPX CI from CircleCI to GitHub Actions
* :hpx-pr:`6902` - using standard traits present in iterator library
* :hpx-pr:`6900` - shared_mutex: follow-up fix and add regression test
* :hpx-pr:`6899` - Implemented forwarding_sender_query for sender adaptors
* :hpx-pr:`6898` - Removed the incomplete line in Readme
* :hpx-pr:`6897` - Implement concurrent data structures for Issue #2235
* :hpx-pr:`6894` - Implement find_last, find_last_if, and find_last_if_not algorithms as CPOs
* :hpx-pr:`6893` - fix: decouple hpx_debug from cmake_build_type and allow custom debug postfix
* :hpx-pr:`6891` - Refine unordered_map unit tests and add operator[] size stability check
* :hpx-pr:`6890` - Feature/termination detection timeout
* :hpx-pr:`6889` - Fixing set_affinity_test
* :hpx-pr:`6886` - Expose global termination detection through new API (#6163)
* :hpx-pr:`6885` - refactor execution parameters and steady clock usage
* :hpx-pr:`6884` - implement fold algorithms acc. to c++26
* :hpx-pr:`6883` - Fix initialization hang when --hpx:threads exceeds --hpx:cores
* :hpx-pr:`6882` - Adjust filewriter for collective benchmark
* :hpx-pr:`6880` - Added SPDX License Identifier
* :hpx-pr:`6879` - Implement parallel hpx::uninitialized_relocate_* algorithms for overlapping ranges
* :hpx-pr:`6876` - Feature/task bench support
* :hpx-pr:`6875` - implement serialization for partition_data in 1d_stencil_5
* :hpx-pr:`6873` - Fix runtime shutdown ordering for collectives on macOS Debug - Issue #6776
* :hpx-pr:`6872` - Added random repeating numbers in tests and fixed sentinel tests for extrema algorithms
* :hpx-pr:`6870` - standardise hpx::generate function signature
* :hpx-pr:`6869` - update relocation definitions to track p2786r13 standard
* :hpx-pr:`6866` - Fix threads hanging indefinitely by acquiring the state mutex before checking for an exclusive lock
* :hpx-pr:`6864` - [Serialization] [Feature] Add serialization for multi containers and unordered_set
* :hpx-pr:`6863` - Fix partitioned_vector tests timeout by reducing vector dimensions #6500
* :hpx-pr:`6862` - Bump jwlawson/actions-setup-cmake from 2.0 to 2.1
* :hpx-pr:`6861` - implement thread affinity api similar to pthread_setaffinity_np
* :hpx-pr:`6859` - [Serialization] [Feature] Add experimental C++26 reflection support
* :hpx-pr:`6858` - Adding intrinsic support for the Tracy profiler
* :hpx-pr:`6857` - Bump jwlawson/actions-setup-cmake from 2.0 to 2.1
* :hpx-pr:`6856` - Godbolt Boost detection: search for static libs if shared not found
* :hpx-pr:`6853` - Remove type alias sender completions support (#6624)
* :hpx-pr:`6852` - Add HPX_EXPORT to plugin factory types to fix MacOS visibility issues
* :hpx-pr:`6851` - added CMakePresets.json with multiple build configurations for HPX development
* :hpx-pr:`6850` - Fix Ninja Multi-Config generator support for pkg-config files
* :hpx-pr:`6849` - Fix ARM build failure by automatically enabling generic context coroutines Fixes #6728
* :hpx-pr:`6848` - Forking and modernizing Boost.iostreams
* :hpx-pr:`6846` - fix(stdexec): added adaption to ensure successful build with stdexec
* :hpx-pr:`6845` - Starting to adapt HPX full modules to C++ modules
* :hpx-pr:`6844` - Fixing cyclic dependency between memory and serialization module
* :hpx-pr:`6841` - rewrite hpx::search_n with standard-conforming API and add searcher-based overloads to hpx::search
* :hpx-pr:`6840` - Fix worker_timed.hpp: type-safe delay using chrono::nanoseconds
* :hpx-pr:`6839` - Use c++20 concepts where possible
* :hpx-pr:`6837` - Removing HPX_HAVE_CXX11_STD_SHARED_PTR_LWG3018 option
* :hpx-pr:`6836` - Removing boost.shared_array
* :hpx-pr:`6835` - Enforce clang-format east-const
* :hpx-pr:`6834` - Feature/hierarchical collectives
* :hpx-pr:`6832` - Applying minor optimizations and fixes in various places
* :hpx-pr:`6831` - Fix linker error for print_dec<unsigned long> on ARM64
* :hpx-pr:`6830` - Fix numa_allocator test failure on macOS
* :hpx-pr:`6826` - Apply partitioner optimizations to all code paths
* :hpx-pr:`6825` - Cleaning up C++ module configuration
* :hpx-pr:`6824` - Use constexpr and hpx::wait_each in future_reduce test
* :hpx-pr:`6823` - More work on exposing C++ modules
* :hpx-pr:`6821` - Switch to executing a parallel algorithm to synchronous execution
* :hpx-pr:`6820` - Fix expected failure in contract tests
* :hpx-pr:`6818` - Adapt last HPX modules of the HPX core library to C++ modules
* :hpx-pr:`6817` - Adapting HPX CUDA modules to C++ modules
* :hpx-pr:`6815` - The new LCW (or MPIx) parcelport
* :hpx-pr:`6813` - Simplify find_package CMake modules
* :hpx-pr:`6812` - Adapting HPX networking base modules to C++ modules
* :hpx-pr:`6811` - Adapting HPX modules of levels 24, 25 and 26 to C++ modules
* :hpx-pr:`6809` - Adapting HPX modules of levels 19, 20, 21, and 22 to C++ modules
* :hpx-pr:`6808` - Bump actions/checkout from 4 to 6
* :hpx-pr:`6807` - Adapting HPX modules of levels 16, 17, and 18 to C++ modules
* :hpx-pr:`6806` - Add CMakePresets.json for better IDE and build system integration
* :hpx-pr:`6805` - Adapting HPX modules of level 15 to C++ modules
* :hpx-pr:`6804` - Added HPX::auto_wrap_main option
* :hpx-pr:`6803` - Adapting HPX modules of levels 13 and 14 to C++ modules
* :hpx-pr:`6802` - Adapting HPX modules from level 12 to C++ modules
* :hpx-pr:`6801` - Removing exports from detail namespaces
* :hpx-pr:`6800` - Adapting HPX modules from level 11 to C++ modules
* :hpx-pr:`6799` - ci: fix exclude regex handling in linux sanitizer workflow
* :hpx-pr:`6797` - Fix documentation use of literalinclude
* :hpx-pr:`6796` - tests: add proxy-iterator regression test for find
* :hpx-pr:`6795` - Fixing problems with scheduler fast-idle mode
* :hpx-pr:`6794` - Adapt HPX module iterator_support to C++ modules
* :hpx-pr:`6792` - Adjust memory order constraints on reference counting
* :hpx-pr:`6790` - Make stackless threads usable with parallel algorithms
* :hpx-pr:`6789` - contracts module
* :hpx-pr:`6787` - Update the LCI parcelport to LCI v2
* :hpx-pr:`6785` - Bump github/codeql-action from 3 to 4
* :hpx-pr:`6784` - Add executors documentation
* :hpx-pr:`6782` - Adapting module serialization
* :hpx-pr:`6781` - Converting HPX modules to C++ modules
* :hpx-pr:`6780` - Exposing HPX type_support module as a C++ module
* :hpx-pr:`6779` - Fixing MacOS CI issues
* :hpx-pr:`6778` - Add missed linker flags for hwloc on Apple platforms
* :hpx-pr:`6777` - Exposing HPX assertion module as a C++ module
* :hpx-pr:`6775` - Expose an explicit CMake target HPX::init to be used by non-executables
* :hpx-pr:`6774` - Cleaning up macro names and usage required in the context of C++ modules
* :hpx-pr:`6773` - Expose format module using C++20 modules
* :hpx-pr:`6771` - Update CI performance test reference measurements
* :hpx-pr:`6770` - Generate the list of module HPX headers that expose C++ module exports
* :hpx-pr:`6769` - CI/Jenkins: enable modules on GCC 15
* :hpx-pr:`6768` - Extend inspect checks to cover .ixx module interface files
* :hpx-pr:`6767` - Fixing dependencies for CircleCI workflow
* :hpx-pr:`6766` - Fixing url for Inspect logo
* :hpx-pr:`6763` - Fixing Github builders
* :hpx-pr:`6762` - Bump actions/checkout from 4 to 5
* :hpx-pr:`6761` - Expose version module using C++20 modules
* :hpx-pr:`6760` - Optimizing collectives for num_sites equal to one
* :hpx-pr:`6759` - Fixing is_known_contiguous_iterator support for string
* :hpx-pr:`6758` - More fork_join executor tests invoked from the main thread
* :hpx-pr:`6757` - Adding explicit default special functions to zip_iterator
* :hpx-pr:`6756` - Suppress more clang-tidy warning
* :hpx-pr:`6755` - Adding test for version API
* :hpx-pr:`6753` - Improve proxy references support in algorithms
* :hpx-pr:`6751` - Disable test on sanitizer CI
* :hpx-pr:`6749` - Reducing the size of CirclCI tasks
* :hpx-pr:`6748` - Allowing for the fork_join_executor to be used on the main thread
* :hpx-pr:`6746` - Integrate NVIDIA's S/R Bulk implementation into HPX
* :hpx-pr:`6744` - Integrating thrust into HPX
* :hpx-pr:`6741` - Algorithm tests using partitioner with cleanup
* :hpx-pr:`6738` - Add aclocal-1.16 symlink
* :hpx-pr:`6737` - Allowing to invoke parallel algorithms from main thread
* :hpx-pr:`6734` - Resolves the redundant use of `resolver_client` in `agas/agas_fwd.hpp`
* :hpx-pr:`6732` - Removing support for C++17
* :hpx-pr:`6731` - Removing deprecated Asio features
* :hpx-pr:`6727` - Fixed the S/R version of partial_sort with unit test added
* :hpx-pr:`6725` - Adding Boost 1.88
* :hpx-pr:`6723` - Optimize `hpx::merge`
* :hpx-pr:`6720` - Fixed the S/R version of nth_element with unit test added
* :hpx-pr:`6716` - Fixing some of the reported linker warnings
* :hpx-pr:`6705` - Adding gcc/15 to jenkins
* :hpx-pr:`6700` - Update parallel algorithms to use cpp20 concepts
* :hpx-pr:`6655` - Implement parallel_scheduler in HPX
* :hpx-pr:`6515` - Adding process example
* :hpx-pr:`6465` - Always return outermost thread id
* :hpx-pr:`6428` - Enable testing of new scheduler in CI
* :hpx-pr:`6053` - Support for new performance tool suite : LIKWID

Additional incorporated pull requests
=====================================

These PRs were merged into development branches. Their GitHub merge commits
are not ancestors of either comparison revision, but their changes were
incorporated into this release through other commits:

* :hpx-pr:`7666` - ci: pin ittapi to v3.27.0 on the release branch
* :hpx-pr:`7610` - Regenerate the closed issue and pull request lists for V2.0.0
* :hpx-pr:`7584` - Regenerate V2.0.0 release notes Closed issues and pull requests lists
* :hpx-pr:`7583` - docs: fix script name in release procedure
* :hpx-pr:`7549` - Add a first draft of the V2.0.0 release notes
* :hpx-pr:`7548` - Port generate_issue_pr_list.sh from hub to the GitHub CLI
* :hpx-pr:`7351` - CodeRabbit Generated Unit Tests: Add unit tests for PR changes
* :hpx-pr:`6702` - When_all_vector update
