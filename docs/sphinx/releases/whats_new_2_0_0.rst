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
system, and observability tooling. The inventories below cover more than 800
pull requests with merge commits in the release comparison, eight additional
PRs incorporated through transplanted changes, and more than 220 closed
milestone issues. The sections below expand on each major theme.

This release is the result of a broad community effort. Since V1.11.0,
more than 90 contributors have authored more than 4,000 commits, and over
75 of them made their first contribution to |hpx| during this release cycle.
We are grateful to everyone who contributed code, documentation, tests,
bug reports, and reviews. Your work made this release possible.

Language and Standard Baseline
==============================

- C++20 is now the *minimum* required standard, up from C++17. V1.11.0 was
  the last release to support C++17 (:hpx-issue:`5497`).
- Compared with the 1.11 release branch, the minimum supported GCC
  version rises from 11 to 12, and Visual Studio from 2019 to 2022. The
  minimum Clang version remains 16. The configuration now rejects GCC
  versions older than 12 with a clear message instead of failing later with
  compile errors; ``-DHPX_WITH_GCC_VERSION_CHECK=OFF`` removes the check, at
  the user's own risk (:hpx-pr:`7886`).
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

With C++20 modules enabled, each |hpx| module is used through its generated
header ``hpx/modules/<module>.hpp``, which imports the corresponding C++
module, or through the public headers such as ``hpx/init.hpp`` and
``hpx/future.hpp``. The headers inside an |hpx| module (``hpx/<module>/...``,
for example ``hpx/futures/future.hpp``) are not part of the public API.
Including one of them in addition to the imported module defines its entities
a second time, and the compilation fails with redefinition errors (see
Breaking changes below).

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
  and heap algorithms (:hpx-pr:`7434`). ``HPX_CONTRACT_ASSERT`` falls back
  onto ``HPX_ASSERT`` if contracts are disabled (:hpx-pr:`7700`).

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
  Scheduler executors accept sized and unsized forward ranges with distinct
  sentinels as bulk shapes without traversing them repeatedly
  (:hpx-pr:`7651`).
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
  future/promise bridge redesign (:hpx-pr:`7355`). The pinned |stdexec|_ is
  patched so that ``exec::async_scope::on_empty()`` compiles with nvcc
  (:hpx-issue:`7676`, :hpx-pr:`7695`). One of the constructors of
  ``run_loop``'s operation state didn't set the function that completes the
  operation, so running such an operation jumped to an invalid address
  (:hpx-pr:`7762`). Further patches of the pinned |stdexec|_ let NVCC 12.8
  instantiate host sender chains and keep SYCL builds from picking up CUDA's
  atomic headers (:hpx-pr:`7711`). Builds without CUDA, HIP builds included,
  now always use ``std::atomic`` in |stdexec|_; they failed when an old
  libcu++ was installed in a default include directory (:hpx-issue:`7884`,
  :hpx-pr:`7885`).

Runtime and Execution APIs
==========================

- The P2079 parallel scheduler integrates sender work with HPX thread pools
  (:hpx-pr:`6655`). ``hpx::execution::experimental::get_parallel_scheduler()``
  uses the default pool; the overload taking a pool reference supports named
  pools obtained through ``hpx::resource::get_thread_pool`` (:hpx-pr:`7579`).
- Execution-policy traits and the independent
  ``rebind_policy_executor_t`` / ``rebind_policy_parameters_t`` utilities
  support inspecting and adapting policies (:hpx-pr:`7550`, :hpx-pr:`7551`).
  The customization points ``construct_rebound_policy_executor`` and
  ``construct_rebound_policy_parameters`` let policies that carry additional
  state preserve it when being rebound (:hpx-pr:`7585`).
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
- Tasks can be marked with their expected size:
  ``hpx::threads::with_task_size<Size>(f)`` and the shorthands
  ``small_task``, ``medium_task`` and ``large_task`` attach a
  ``hpx::threads::task_size`` to a function, ``hpx::traits::get_task_size``
  reads it, and ``hpx::post``, ``hpx::async``, the executors, ``.then()`` and
  dataflow pass it to the scheduler in the ``thread_schedule_hint``. The
  schedulers don't use the size yet (:hpx-issue:`7837`, :hpx-pr:`7848`).
  Only functions carry a size: a size in the hint of an executor, a launch
  policy or a scheduler has no effect, and bulk operations don't pass sizes
  on, as their tasks run the function many times. The iteration wrappers of
  the parallel algorithms forward the size of the loop body. Each size stands
  for a run time, given by ``hpx::task_size_duration``: ``nano_task`` (the new
  smallest size) about 1 ns, small about 100 ns, medium about 10 us and large
  about 1 ms or more. ``hpx::bulk_task_size`` gives the size of a task that
  calls a function of a given size many times (:hpx-pr:`7925`). A function
  keeps its size when it is stored in an ``hpx::function`` or
  ``hpx::move_only_function``, bound, or run by ``hpx::thread``, the same way
  it keeps its annotation (:hpx-pr:`7926`).
  ``hpx::traits::get_task_size_for<F, Args...>`` gives the size of a call
  with arguments of the given types: the function objects of the standard
  library, such as ``std::plus<>`` or ``std::less<>``, called with
  arithmetic, enum or pointer arguments are nano tasks. ``reduce``,
  ``transform_reduce``, ``transform`` and ``merge`` pass the size of their
  operations on (:hpx-pr:`7946`).
- ``hpx::execution::experimental::task_size_chunk_size`` are executor
  parameters that chunk the iterations of a loop based on the task size of
  its body instead of measuring an iteration. A loop runs on as many cores as
  can be given ``chunks_per_core`` (8) chunks of at least ``min_chunk_time``
  (2.5 us) each, and on the calling thread if that is only one chunk. The
  defaults can be changed with the configuration entries
  ``hpx.task_size_chunk_size.min_chunk_time_us`` and
  ``hpx.task_size_chunk_size.chunks_per_core``, and the new benchmark
  ``task_size_chunk_size_sweep`` finds the best values for a machine. The new
  CMake option ``HPX_WITH_TASK_SIZE_DEFAULT_PARAMETERS`` (``OFF`` by default)
  makes the default executor parameters use ``task_size_chunk_size`` for
  loops whose body has a size (:hpx-pr:`7948`).
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
    reuse between collectives test phases (:hpx-pr:`7526`,
    :hpx-pr:`7364`, :hpx-pr:`7781`, :hpx-pr:`7791`) and caching channel
    communicators per site (:hpx-pr:`7481`).
  - Disconnected-locality handling, including a dedicated dispatch guard
    (:hpx-pr:`7536`) and correct error handling in
    ``resolve_locality`` (:hpx-pr:`7388`).
  - A fix for the shared-pointer serialization type-confusion /
    deserialization vulnerability reported in :hpx-issue:`7333`
    (:hpx-pr:`7334`).

- Non-blocking calls on distributed clients whose id is not resolved yet
  (e.g. a client created from ``find_from_basename``) no longer block: the
  ``launch::async`` and ``launch::apply`` overloads of ``channel``,
  ``send_channel`` and ``receive_channel``, as well as ``hpx::async`` and
  ``hpx::post`` with a client target, are deferred until the id is
  available. ``hpx::post`` to such a client now returns ``false``. MPI is now
  only finalized by whoever initialized it;
  ``hpx::mpi::experimental::finalize`` used to finalize MPI underneath the
  MPI parcelport (:hpx-issue:`5919`, :hpx-pr:`7693`).
- The MPI parcelport pre-posts several receives for incoming parcel headers
  instead of one, which caused intermittent timeouts while AGAS was
  bootstrapping. The number of receives is set by the new
  ``hpx.parcel.mpi.max_header_receives`` setting, which by default depends
  on the size of the communicator (:hpx-issue:`7734`, :hpx-pr:`7739`).
- On Windows, a refused TCP connection attempt now fails immediately instead
  of after about two seconds of SYN retransmissions. As refused attempts are
  retried up to 1000 times, sending a parcel to a locality whose process is
  gone could block the sending thread for more than half an hour
  (:hpx-pr:`7741`). The TCP parcelport no longer terminates its I/O thread if
  a shutdown races with the cleanup of a send operation (:hpx-pr:`7720`).
- With Asio 1.34 and later, host names without an IPv6 address didn't
  resolve: only the IPv6 lookup was used, and the fallback from IPv4 to IPv6
  couldn't work. All address families are resolved again, as with older Asio
  versions (:hpx-pr:`7816`).
- A send to a locality that has gone away (the connection was reset or
  aborted, the pipe broke, or the peer closed it) is now always reported to
  the caller as ``locality_was_disconnected``. Before, the write handler got
  the raw Asio error, or during shutdown a success code, after which the
  caller waited for a reply that never came (:hpx-pr:`7865`).
- A parcelport that rescheduled a suspended operation on a thread never
  armed the timer that resumes it, because the result of registering the
  thread was checked the wrong way round; delayed network operations stayed
  suspended (:hpx-issue:`7893`, :hpx-pr:`7894`).
- ``hpx::force_disconnect`` can now be called on any locality, not only on the
  console. On another locality it asks the console to remove the given
  locality and returns the result; before, it failed with
  ``hpx::error::invalid_status``. Disconnecting the calling locality itself or
  the console is rejected with ``hpx::error::bad_parameter`` (:hpx-pr:`7901`).
- Removing a locality from the connection cache no longer clears its entry
  while connections to it are still checked out, which threw
  ``invalid_status`` on the locality being disconnected and hung
  ``force_disconnect`` (:hpx-pr:`7912`). Credit decrements still queued for
  a locality that was removed with ``force_disconnect`` are dropped, as they
  already were for a disconnected one, instead of escaping as a
  ``bad_parameter`` exception, and so are decrements whose target can't be
  resolved during shutdown. A locality that is being force-disconnected is
  reported as disconnected by ``resolve_locality`` until its disconnect is
  recorded (:hpx-pr:`7917`).

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
- |apex|_ can now be used with the local runtime
  (``HPX_WITH_DISTRIBUTED_RUNTIME=OFF``) (:hpx-issue:`4567`,
  :hpx-pr:`7686`).
- The |ittnotify|_ backend now emits task lifecycle and work-stealing events
  when ITTNotify is enabled at runtime (``--hpx:ini=hpx.use_itt_notify=1``):
  each task is shown as an overlapped task with its execution segments, and
  stolen tasks are marked. This is ``ON`` by default for
  ``HPX_WITH_ITTNOTIFY=ON`` builds; pass
  ``-DHPX_WITH_TRACING_LIFECYCLE_EVENTS=OFF`` and
  ``-DHPX_WITH_TRACING_WORK_STEALING_EVENTS=OFF`` to the |cmake|_
  configuration to get the earlier behavior (:hpx-pr:`7675`).

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
- Any site can now be the root of a hierarchical communicator; the tree used
  to be rooted at site 0. The communicator names of a root other than 0 get
  a ``root<N>/`` prefix, so that communicators with different roots can share
  a base name (:hpx-pr:`7829`).

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
- The supervision dispatch manual and API reference were brought in line
  with the API (:hpx-pr:`7804`, :hpx-pr:`7805`).
- Pending ``await_terminal()`` waiters are now completed with
  ``invalid_status`` before the runtime shuts down, instead of their promises
  being broken after it stopped, which aborted a locality that called
  ``supervision::finalize()`` while a failure detection sweep was in flight
  (:hpx-issue:`7825`, :hpx-pr:`7835`). ``discover_and_join`` leaves time for
  the join: a registry name that never resolves no longer uses up the whole
  budget, so that the join to the root failed even though the other peers
  were found (:hpx-pr:`7867`). At shutdown, activity agents on other
  localities are no longer asked whether they are still active: an agent
  whose locality had already left never answered, and the shutdown of the
  thread manager hung (:hpx-pr:`7866`). A notification that can't be
  delivered to an observer whose locality went away no longer marks the
  publishing target as failed; the observer is unregistered instead
  (:hpx-pr:`7897`).

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
  parallel, and task execution policies (:hpx-pr:`7545`), also with C++20
  modules enabled (:hpx-pr:`7698`, :hpx-pr:`7707`).
- **Strided loops**: ``for_loop_strided``, ``for_loop_n_strided`` and
  their range variants now follow N4755: ``for_loop_n_strided`` invokes the
  body ``n`` times, the stride must be positive, and live-out induction
  variables advance by the number of iterations (see Breaking changes
  below, :hpx-pr:`7703`).
- **Datapar/SIMD**: fixing |eve|_/simd unaligned load/store broadcast bugs
  (:hpx-pr:`7435`, :hpx-issue:`7287`), and guarding a mismatched SIMD
  path on ``zip_iterator`` compatibility (:hpx-pr:`7328`).
- **Fixed-size SIMD policies**: added ``hpx::execution::fixed_size_simd<N>``
  and ``hpx::execution::par_fixed_size_simd<N>`` (plus their ``task``
  variants) to select vector lane counts explicitly. Datapar loops and
  algorithms use the policy's lane count, exposed through
  ``hpx::execution::policy_traits``. Existing SIMD policies retain the
  native lane count (:hpx-pr:`7674`).
- **Boolean chunk results**: the partitioners no longer collect chunk
  results of type ``bool`` in a ``std::vector<bool>``, whose elements share
  memory locations, so that concurrently finishing chunks could lose a
  result. This made ``hpx::ranges::contains`` return ``false`` sporadically
  for values in the range (:hpx-pr:`7736`).
- **Chunk iterators**: decrementing ``chunk_size_iterator`` and
  ``chunk_size_idx_iterator`` over a bidirectional iterator no longer hangs
  with MSVC (:hpx-pr:`7735`).
- **find_end**: the parallel ``find_end`` compared the rest of the
  subsequence with the values following its first element (``t + 1``,
  ``t + 2``, ...) instead of with the subsequence itself, so it could miss
  matches or report false ones, and it didn't compile for element types
  without ``operator++`` (:hpx-issue:`7766`, :hpx-pr:`7767`).
- **Static chunk sizes**: with a chunk size larger than the number of
  elements, ``chunk_size_iterator`` and ``chunk_size_idx_iterator`` computed
  the last chunk and the end of the range from the unclamped chunk size. The
  range now consists of one chunk covering all elements (:hpx-issue:`7859`,
  :hpx-pr:`7860`).
- **Merge**: the parallel ``merge`` is partitioned over all of its output
  elements instead of over the longer input, so that executor parameters
  see the whole work of the merge (:hpx-pr:`7942`).
- **Bulk completion**: the bulk operations of ``parallel_executor``, and so
  the parallel algorithms, complete when their last chunk is done, instead
  of when all of their worker tasks have finished. A worker whose chunks
  were all stolen by others no longer holds up the operation, and workers
  without chunks are no longer spawned (:hpx-pr:`7949`).

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
  when one is requested (:hpx-issue:`5189`, :hpx-pr:`7682`). Destroying a
  thread from outside the pool no longer indexes the scheduler's per-worker
  data with an invalid worker number (:hpx-issue:`7908`, :hpx-pr:`7909`).
- **Fork dispatch**: fixed ``hpx::launch::fork`` tasks remaining
  unscheduled when launched from a plain OS thread or an HPX thread in
  another pool. Both ``post`` and ``async`` now schedule the task directly
  when an immediate handoff is unavailable (:hpx-pr:`7687`, :hpx-pr:`7699`).
- **Staged task stealing**: ``hpx.thread_queue.min_tasks_to_steal_staged``
  now only applies to other workers stealing staged tasks. A worker always
  converts its own staged tasks, even if there are fewer of them than the
  configured minimum; before, such tasks were never run, so any non-zero
  setting could hang applications. The default (``0``) is not affected
  (:hpx-pr:`7730`).
- **Spin-wait hints**: ``HPX_SMT_PAUSE`` now emits ``yield`` on ARM and
  AArch64 (``__yield()`` with MSVC) (:hpx-pr:`7727`).
- **Scheduler contention**: the local priority queue scheduler starts
  stealing staged tasks after 30 empty scheduling loop iterations instead of
  ``hpx.max_idle_loop_count / 2`` (100000 by default), but until the latter
  only from workers holding at least 10 staged tasks. Both can be changed
  with the new settings ``hpx.thread_queue.idle_loop_count_steal_staged`` and
  ``hpx.thread_queue.min_tasks_to_steal_staged_early``. New work items now
  only wake workers that are backing off, and a worker that finds work resets
  its own idle backoff (:hpx-pr:`7725`).
- **Early steal minima**: with the local priority queue scheduler, idle
  workers steal pending threads only from workers that have at least
  ``hpx.thread_queue.min_tasks_to_steal_pending_early`` (4) of them, which
  avoids contention on a queue that all idle workers find at the same time.
  The early minima for pending and staged tasks no longer apply during a
  window of idle scheduling loop iterations. Instead, they are lifted for a
  victim once the thieves find that its owner doesn't get to its few tasks,
  and a worker woken up because new work arrived ignores them for its first
  steal. This shortens the time until a few tasks posted by a busy worker
  run from milliseconds to microseconds (:hpx-pr:`7856`).
- **Hierarchical spawning**: the tasks posted by
  ``parallel_executor_spawn_hierarchically`` captured the launch policy of a
  loop iteration by reference and could read it after the iteration ended,
  which hung bulk execution with GCC (:hpx-issue:`7887`, :hpx-pr:`7900`).
- **Work-requesting schedulers**: these schedulers never actually stole
  tasks. They do now, which also required fixing a reference leak for every
  stolen thread with ``local-workrequesting-mc`` and a hang when suspending
  worker threads (:hpx-pr:`7726`). Idle workers now send steal requests right
  away, as the schedulers stay in fast idle mode (:hpx-issue:`7746`,
  :hpx-pr:`7755`). A worker answering a steal request no longer hands out
  tasks while it has fewer pending tasks than
  ``hpx.thread_queue.min_tasks_to_steal_pending``, as with the other
  schedulers, and stealing fewer tasks than that minimum is no longer always
  refused (:hpx-pr:`7771`).
- **Bound threads**: a thread with ``thread_priority::bound`` that yields
  with ``pending_boost``, for instance while spinning on a contended
  spinlock, now stays on its worker thread. Before, it could resume on a
  different OS thread, which aborted if it held a ``std::mutex``
  (:hpx-pr:`7740`).
- **Background threads**: a background thread that yielded in the middle of
  its work is no longer dropped when its worker thread stops; this could
  leave a parcel half sent and hang the shutdown of a locality
  (:hpx-pr:`7723`). Creating background threads is now tested with every
  scheduler (:hpx-issue:`6190`, :hpx-pr:`7749`, :hpx-pr:`7797`), and the
  background thread helpers are exported from the C++20 module
  (:hpx-issue:`7758`, :hpx-pr:`7759`). A pool with a background work
  function no longer raises ``hpx.max_background_threads`` for the pools
  created after it (:hpx-issue:`7747`, :hpx-pr:`7752`), and the background
  scheduler no longer sets every scheduler mode bit, which turned stealing
  back on (:hpx-issue:`7742`, :hpx-pr:`7750`).
- **Scheduler settings**: ``--hpx:queuing=!<name>`` selected an empty
  scheduler name and always failed, and ``--hpx:high-priority-threads`` is
  now only accepted for schedulers that have high priority queues
  (:hpx-issue:`7743`, :hpx-pr:`7749`, :hpx-pr:`7757`). The default of
  ``hpx.thread_queue.max_delete_count`` is now
  ``HPX_THREAD_QUEUE_MAX_DELETE_COUNT``, ``min_add_new_count`` is limited to
  ``max_add_new_count``, and the shared priority scheduler cleans up all
  terminated threads once ``max_terminated_threads`` is exceeded
  (:hpx-issue:`7748`, :hpx-pr:`7754`). Changing scheduler modes at runtime
  (``add_scheduler_mode`` and friends) now applies each scheduler's own
  rules, as setting them does: ``hpx.numa_sensitive`` works with the shared
  priority scheduler again, ``local-workrequesting`` keeps stealing, and the
  static schedulers keep it off (:hpx-issue:`7745`, :hpx-pr:`7753`). The
  documentation now states that the ``local`` scheduler ignores thread
  priorities, including ``thread_priority::bound`` (:hpx-issue:`7761`,
  :hpx-pr:`7770`).
- **NUMA-sensitive stealing**: ``--hpx:numa-sensitive`` and
  ``hpx.numa_sensitive`` distinguish three levels again for the local, local
  priority and ABP priority schedulers: at level 0 all workers may steal
  across NUMA domains, at level 1 only the workers at a domain boundary
  steal from the nearest domains, and at level 2 nobody steals across
  domains. The default, and the value of the bare option, is now 1; before,
  the bare option turned NUMA sensitivity off, and levels 1 and 2 behaved the
  same. ``hpx::is_scheduler_numa_sensitive()`` reports the configured
  setting. The work-requesting schedulers ignore the setting for now
  (:hpx-issue:`7744`, :hpx-pr:`7756`).
- **Running tasks as children**: ``post()`` could access a task launched
  with the ``run_as_child`` hint after another worker had already finished
  and released it, and dropped the schedule hint of such a task
  (:hpx-issue:`7724`, :hpx-pr:`7751`). A stackless task can now be run
  directly by its waiting parent; this crashed before (:hpx-issue:`7708`,
  :hpx-pr:`7718`). A stackful task run directly by its parent now ends as
  ``terminated``, like a stackless one; before, it was left in state
  ``unknown``, so the parent didn't notice that it had run
  (:hpx-issue:`7713`, :hpx-pr:`7819`). Children run directly by their parent
  are now counted in the thread and phase counts of the pool
  (:hpx-issue:`7830`, :hpx-pr:`7836`).
- **Per-worker data**: the per-worker data of the schedulers and the
  ``thread_queue`` objects are aligned to the cache line size, so that the
  data of different workers no longer shares cache lines. This removes most
  of the processes whose spawn throughput was stuck in a slow mode
  (:hpx-pr:`7845`). They are now aligned to the new
  ``hpx::threads::get_destructive_interference_size()`` instead, 128 bytes
  on x86-64, ARM64 and 64-bit POWER, as the spatial prefetcher of Intel CPUs
  fetches pairs of 64-byte lines. The new advanced CMake option
  ``HPX_WITH_DESTRUCTIVE_INTERFERENCE_SIZE`` overrides the default. The
  scheduler's per-worker data is grouped by who writes it, and the mutex of
  each ``thread_queue`` and the scheduling counters of each worker get a
  region of their own (:hpx-pr:`7872`).
- **Shutdown**: a pool no longer hangs at shutdown when an |hpx| thread
  places new work on the queue of a worker that has already left its
  scheduling loop. The last worker to leave now takes over the work left on
  any queue (:hpx-pr:`7864`). A timed suspension that is woken early no
  longer leaves its timer helper thread pending after the workers stopped,
  which hung the shutdown (:hpx-pr:`7918`).
- **Suspending workers**: resuming a worker right after it decided to
  suspend could lose the wakeup and leave the worker asleep, and a spurious
  wakeup could end the suspension early (:hpx-issue:`7783`, :hpx-pr:`7784`).
- **Queue length**: ``get_queue_length()`` for all workers, which the
  ``/threadqueue/length`` performance counter uses, threw with the shared
  priority scheduler; it now returns the sum over all queues (:hpx-pr:`7855`).
  With thread manager logging enabled, the exit message of a worker read the
  counters of the wrong worker for every pool but the first
  (:hpx-pr:`7838`).
- **Documentation**: a new manual section, "Configuring and tuning the thread
  schedulers", describes how a worker finds work, all settings that influence
  it, and which scheduler uses which setting; the configuration tables now
  list all keys of the ``[hpx]`` and ``[hpx.thread_queue]`` sections
  (:hpx-pr:`7832`).

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
- ``HPX_WITH_MALLOC`` was silently ignored with MSVC: the selected allocator
  was not linked. It is now, and the Windows release CI builds with
  |mimalloc|_ (:hpx-pr:`7732`).
- The configuration tests are no longer compiled with warnings treated as
  errors, so warnings caused by a packager's compiler flags no longer make
  feature checks such as the one for ``std::atomic`` fail
  (:hpx-issue:`7719`, :hpx-pr:`7729`).
- Tests no longer use names that the Windows SDK headers define as macros
  or types (such as ``SIZE``), and the inspect tool's ``--windows_macro``
  check now reports them (:hpx-pr:`7738`).
- ``HPX_WITH_NANOBENCH`` no longer requires networking to be enabled, and
  works with MSVC (:hpx-pr:`7800`).
- Fetching hwloc (``HPX_WITH_FETCH_HWLOC=ON``) keeps the timestamps of its
  archive, so that its build no longer tries to rerun automake, and a failing
  hwloc build now stops the configuration right away (:hpx-issue:`7820`,
  :hpx-pr:`7821`).
- The performance tests of the CI are registered with
  ``add_hpx_performance_report_test`` and report to CDash. Each benchmark
  runs 30 times, the medians of the executions are compared with reference
  results of the node the tests ran on, and pull requests get a comment only
  about measurements that changed by more than 5%. The runs of the commits
  of the master branch are recorded in the performance history at
  `hpx-performance <https://github.com/TheHPXProject/hpx-performance>`_,
  with graphs of every benchmark over time (:hpx-pr:`6514`,
  :hpx-pr:`7831`, :hpx-pr:`7839`). The graphs show the medians over the
  commits of the master branch and list the commits after which a median
  moved by more than 5% (:hpx-pr:`7899`), and runs missing from the history
  can be added by hand (:hpx-pr:`7903`). Two scheduler reports measure task
  creation, stealing and the waking of idle workers on many cores
  (:hpx-pr:`7888`).
- The ``cancel-ci`` label and the ``/cancel-ci`` comment now also cancel the
  running workflows of pull requests from forks (:hpx-pr:`7228`).
- The pkg-config files can be generated again when the MPI link flags are
  wrapped in ``$<HOST_LINK:...>`` by FindMPI, which failed the configuration
  with the modules built as object libraries (:hpx-issue:`7826`,
  :hpx-pr:`7833`).
- The Windows CI workflows build their dependencies with vcpkg and keep them
  in the GitHub Actions cache, instead of downloading a prebuilt export
  (:hpx-issue:`7857`, :hpx-pr:`7858`).

Security / Robustness Fixes
===========================

- **Shared-pointer serialization vulnerability**: an insecure
  deserialization / type-confusion issue in shared-pointer serialization
  was fixed (:hpx-issue:`7333`, :hpx-pr:`7334`).
- **Data races**: a race in ``hostname_print_helper::get_hostname()`` on
  worker-thread startup (:hpx-issue:`7520`, :hpx-pr:`7525`, with a regression
  test added in :hpx-pr:`7546`), and on the wait counter of the scheduler's
  idle backoff (:hpx-issue:`7701`, :hpx-pr:`7702`).
- **Synchronization**: corrected timed semaphore waits (:hpx-pr:`7617`),
  a deadlock and race in ``latch::reset`` (:hpx-pr:`7555`),
  and handling of spurious wakeups in ``hpx::barrier`` (:hpx-pr:`7611`).
  ``hpx::barrier::arrive_and_wait`` no longer reads the phase after the
  lock was released (:hpx-pr:`7685`), and ``timed_mutex::try_lock_until``
  now keeps waiting until the deadline instead of giving up after the first
  wakeup (:hpx-pr:`7714`). The destructor of an ``hpx::stop_callback``
  created for a token without any remaining ``hpx::stop_source`` no longer
  waits forever for a callback that never runs (:hpx-pr:`7782`). A timed
  wait on a counting semaphore that timed out no longer loses a release
  that arrived at the same time (:hpx-pr:`7802`). Move-assigning to an
  ``hpx::jthread`` that still owned a running thread terminated the program;
  it now requests that thread to stop and joins it first, as the standard
  requires (:hpx-pr:`7817`).
  The memory order used after a condition-variable wakeup in future
  ``wait()`` was upgraded from relaxed to acquire (:hpx-pr:`7677`).
  ``condition_variable::notify_all`` and ``abort_all`` left the waiting
  entries pointing to a local queue on the stack of the notifying thread,
  which a waiter that timed out or was aborted later accessed
  (:hpx-issue:`7786`, :hpx-pr:`7787`). ``condition_variable::abort_all``
  and ``notify_one`` could still wake a waiter that had already left the
  wait, which terminated the process or woke an unrelated thread;
  ``abort_all`` now rejects new waiters until it has drained the queue
  (:hpx-pr:`7879`). A waiter is now woken only once it is suspended, and one
  that left its wait is no longer woken at all, which removes the late
  wakeups behind intermittent ``future_uncompleted`` errors. This needs no
  reference counting of the waiting threads (:hpx-pr:`7904`).
  ``shared_mutex::lock`` and ``try_lock`` could take the lock while another
  thread was upgrading its shared lock to an exclusive one, so both threads
  owned it (:hpx-issue:`7873`, :hpx-pr:`7874`).
- **Stopping the runtime**: concurrent calls to ``hpx::local::stop()`` could
  take ownership of the same runtime and destroy it twice. A second call now
  fails with ``invalid_status`` (:hpx-issue:`7768`, :hpx-pr:`7769`).
- **AGAS garbage collection**: fixed background collection when networking
  is disabled (:hpx-pr:`7627`).
- **Exception safety**: a memory-leak risk in
  ``options_description_easy_init`` under exceptions was fixed
  (:hpx-issue:`7040`), and unsafe ``std::strcat`` usage in ``print.cpp``
  was replaced with safer string operations (:hpx-issue:`7046`).
  ``hpx::split_future`` on a future without a shared state, for instance a
  default-constructed or moved-from one, now reports ``no_state`` instead of
  dereferencing a null pointer (:hpx-issue:`7806`, :hpx-pr:`7809`).
- **Serialization correctness**: a double-read/type-mismatch bug in
  ``exception_ptr::load()`` was fixed (:hpx-pr:`7278`), along with a
  semantic mismatch in serialization archive flags
  (:hpx-issue:`7034`). Collection sizes read from archives are now checked
  before use (:hpx-pr:`7594`), as is the length of a checkpoint read from a
  stream (:hpx-pr:`7705`).
- **Process launching**: failures while setting up a child process, such as
  a failed change of the working directory, are now reported
  (:hpx-issue:`7646`, :hpx-pr:`7649`).
- **Caches**: ``local_cache::reserve`` evicted entries based on the
  difference between the old and the new capacity instead of the number of
  entries over the new limit, so it could empty a cache that already fit, or
  leave an unlimited cache over its new limit (:hpx-issue:`7792`,
  :hpx-pr:`7793`). ``local_cache::free_space`` skipped the entry to be
  evicted next, so the following evictions didn't follow the policy of the
  cache, and inserting an entry larger than the whole cache could crash
  (:hpx-issue:`7823`, :hpx-pr:`7824`). ``local_cache::update`` didn't
  account for the size of an entry it replaced, and accepted a replacement
  larger than the cache (:hpx-issue:`7846`, :hpx-pr:`7847`). On 64-bit
  Windows an amount of space to free of 2 GiB or more turned negative, so
  that ``local_cache`` evicted nothing and still stored entries beyond its
  capacity (:hpx-pr:`7889`). With the ``size_entry`` and ``lfu_entry``
  policies, ``local_cache`` discarded entries in the opposite of the
  documented order: the smallest entries instead of the biggest, and the most
  frequently used instead of the least frequently used (:hpx-pr:`7892`).

Breaking changes
================

The items below require changes in user code. Each is covered in more detail
in the sections above.

- C++20 is now the minimum required standard. Compiling |hpx| or applications
  using it with C++17 is no longer supported.
- The minimum supported compiler versions have changed. Please see our
  :ref:`prerequisites` page for the current support matrix.
- Applications must include only the public |hpx| headers (for example
  ``hpx/init.hpp`` or ``hpx/future.hpp``) and the generated module headers
  ``hpx/modules/<module>.hpp``. The headers inside an |hpx| module
  (``hpx/<module>/...``) are internal. They still work when
  ``HPX_WITH_CXX_MODULES`` is ``OFF``, but with C++20 modules enabled
  including them leads to redefinition errors, since the same entities are
  imported from the C++ module as well.
- The use of ``tag_invoke`` for API customization has been removed where
  possible. Code that relied on overloading |hpx| functionality through
  ``tag_invoke`` will have to be adapted.
- Our own, by now outdated, implementation of ``std::execution`` (senders and
  receivers) has been removed. It is replaced by the reference implementation
  (|stdexec|_).
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
- ``thread_schedule_hint::mode`` is no longer a public data member: use
  ``mode()`` and ``mode(x)``, as for the other parts of the hint
  (:hpx-pr:`7848`).

Closed Issues and Pull Requests
==================================

The lists below were reconciled against release-2.0.X at
``ff34ecf0ec7118366fa6d297a5cd41a7aed440e5``, using the v1.11.0 tag
(``c9b81b401fbe6ba4a0306feff71d18ef01c71b34``) as the baseline.

There are 834 merged PRs whose merge commits are reachable from the release
and absent from that baseline, including PRs outside milestone 2.0.0. The
226 closed issues are the milestone inventory; closure does not necessarily
mean an implementation was delivered. Closed, unmerged PRs are excluded.
Eight further PRs with verified transplanted changes are recorded separately
below the generated list.

Closed issues
=============

* :hpx-issue:`7908` - Out-of-bounds array access and SIGSEGV in shared_priority_queue_scheduler::destroy_thread
* :hpx-issue:`7893` - Inverted Error Check in reschedule_on_thread Causes Permanent Task Starvation
* :hpx-issue:`7887` - Hierarchical bulk execution uses a launch policy after its scope ends (hangs with GCC)
* :hpx-issue:`7884` - Compilation issues on 2.0 hpx with 11.4 g++ compiler in ubuntu 22.04
* :hpx-issue:`7875` - Atomic Publication Memory Ordering Violation in future_data_base::set_on_completed
* :hpx-issue:`7873` - Dual-Ownership Race Condition in shared_mutex::try_lock
* :hpx-issue:`7859` - chunk_size_iterator is inconsistent when the chunk size exceeds the element count: reduce, transform_reduce, inclusive_scan and exclusive_scan crash
* :hpx-issue:`7857` - Use the GitHub Actions cache for vcpkg instead of the prebuilt vcpkg export in the Windows CIs
* :hpx-issue:`7846` - local_cache::update with a replacement entry does not account for its size
* :hpx-issue:`7842` - Data Race and False Self-Cancellation UAF in stop_state::remove_callback
* :hpx-issue:`7840` - Premature Shared-State Destruction and Use-After-Free in make_future
* :hpx-issue:`7830` - A run_as_child thread run directly by its parent is not counted in /threads/count/cumulative
* :hpx-issue:`7827` - Unit test failures on Linux
* :hpx-issue:`7826` - Configuring fails with Unix Makefiles generator
* :hpx-issue:`7825` - `supervision::finalize()` leaves `await_terminal` calls pending; locality aborts in `hpx::stop`
* :hpx-issue:`7823` - local_cache::free_space skips the next entry to evict and can dereference past the end
* :hpx-issue:`7820` - HWLoc fetch CI broken since ubuntu-latest moved to 26.04
* :hpx-issue:`7792` - local_cache::reserve evicts based on the old capacity instead of the current size
* :hpx-issue:`7786` - Critical queue corruption, stack-use-after-return, and data races in condition_variable
* :hpx-issue:`7785` - Critical Use-After-Free (UAF) in split execution algorithm shared state
* :hpx-issue:`7783` - Lost wakeup and spurious wakeup deadlock in scheduler_base suspend/resume
* :hpx-issue:`7768` - Dangling pointer and double-free vulnerability in hpx::local::stop
* :hpx-issue:`7766` - Parallel hpx::find_end compares against incremented copies of the first element
* :hpx-issue:`7763` - Cross-thread data race on non-atomic requested\_ counter in local_workrequesting_scheduler
* :hpx-issue:`7761` - Document that `local_queue_scheduler` ignores thread priorities, including `bound`
* :hpx-issue:`7758` - C++20 Modules CI fails since #7723: background thread helpers are not exported
* :hpx-issue:`7748` - Small defects in the handling of hpx.thread_queue parameters
* :hpx-issue:`7747` - hpx.max_background_threads raised for one thread pool carries over to all later pools
* :hpx-issue:`7746` - local_workrequesting_scheduler loses fast_idle_mode, so idle workers send steal requests only after max_idle_loop_count / 2 iterations
* :hpx-issue:`7745` - add/remove/update_scheduler_mode bypass the scheduler specific set_scheduler_mode overrides
* :hpx-issue:`7744` - --hpx:numa-sensitive: bare option means "not sensitive", values 1 and 2 behave the same, is_scheduler_numa_sensitive() is broken
* :hpx-issue:`7743` - Two bugs in the command line handling of --hpx:queuing and --hpx:high-priority-threads
* :hpx-issue:`7742` - background_scheduler::set_scheduler_mode sets almost all scheduler mode bits
* :hpx-issue:`7734` - Intermittent AGAS bootstrap timeout in distributed MPI tests (partitioned_vector_merge_regressions)
* :hpx-issue:`7724` - Intermittent hang / crash with `HPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD=ON` when children are both stolen and run inline by their waiting parent
* :hpx-issue:`7719` - hpx-2.0.0 packaging: HPX needs support for C++11 std::atomic
* :hpx-issue:`7713` - `invoke_directly` returns `unknown` for a finished stackful child, so `execute_thread` never reports success
* :hpx-issue:`7701` - Possible data race on `wait_count` in `scheduler_base.cpp`
* :hpx-issue:`7697` - macOS CI (Debug, Local Runtime): fork_from_other_context fails after #7687
* :hpx-issue:`7696` - Linux CI (Release, C++20 Modules): partitioned_vector_merge tests fail to compile after #7545
* :hpx-issue:`7691` - tests.examples.future_reduce.rnd_future_reduce aborts with 'double free or corruption (out)' in Linux CI (Debug, _GLIBCXX_DEBUG)
* :hpx-issue:`7680` - shared_future::get() documentation incorrectly states that it invalidates the future
* :hpx-issue:`7676` - CUDA CI: thrust_policy_test.cu fails to build with nvcc 12.9 / gcc-12 due to stdexec async_scope.hpp
* :hpx-issue:`7662` - Add auto-wrapping for main for MSVC
* :hpx-issue:`7659` - Conan requires CMake aliases for Asio and Hwloc
* :hpx-issue:`7656` - `hpx-top` and `hpx_stat_viewer` are not installed
* :hpx-issue:`7650` - any_sender.hpp: constexpr empty_vtable fails to compile in MinGW shared (DLL) builds
* :hpx-issue:`7647` - Error using HPX built with `HPX_WITH_TRACY=TRUE`
* :hpx-issue:`7646` - Compiler warnings, master branch, gcc-14.2.0
* :hpx-issue:`7622` - for_loop_strided: a negative stride crashes under par and does not match P0075r2
* :hpx-issue:`7613` - `HPX_REGISTER_CHANNEL(type, name)` 2-arg form still fails for written-out template types
* :hpx-issue:`7608` - debugging: refresh DbgHelp module list after SymInitialize
* :hpx-issue:`7592` - Re-enable auto-linking on Windows for static builds
* :hpx-issue:`7591` - Windows CI: Unsynchronized `SymInitialize` calls between HPX and Tracy cause intermittent `DbgHelp` failures and heap corruption
* :hpx-issue:`7589` - Replace raw double timeouts with std::chrono in finalize.hpp
* :hpx-issue:`7580` - godbolt-minimal: document and CI-test the Windows raw-MSVC hpx_main entry path (parity with Linux wrap-main smoke)
* :hpx-issue:`7569` - godbolt-minimal does not install hpx/experimental/sandbox.hpp
* :hpx-issue:`7568` - Add HPX to Compiler Explorer
* :hpx-issue:`7563` - Data race in performance_counters::registry: countertypes\_ has no synchronization
* :hpx-issue:`7557` - examples/quickstart/sort_by_key_demo fails to compile with Apple clang 21 (libc++ __sift_down vs compare_projected)
* :hpx-issue:`7539` - libhpx_wrap has an unpropagated oneTBB dependency
* :hpx-issue:`7538` - Factor the duplicated AGAS instance-name formatting into a shared helper
* :hpx-issue:`7520` - Data race in hostname_print_helper::get_hostname() on worker thread startup
* :hpx-issue:`7517` - HPX_FORWARD does not forward under nvcc: cudafe strips && from static_cast<decltype(x)&&>
* :hpx-issue:`7513` - dijkstra_termination_disconnected_*_7474 abort on any HPX_WITH_SUPERVISION=OFF build
* :hpx-issue:`7503` - Distributed shutdown hangs over MPI since #7471 (bisected to d5af2db319)
* :hpx-issue:`7485` - Replacement for size_approx() in thread queue
* :hpx-issue:`7483` - force_disconnect: race between disconnect completion and AGAS resolve visibility
* :hpx-issue:`7480` - Apply AGAS RPC timeout during hosted locality namespace bootstrap polling
* :hpx-issue:`7474` - Add multi-locality integration test for Dijkstra termination convergence under locality failure
* :hpx-issue:`7470` - late_component_launcher: demonstrate crash detection -> force_disconnect purge -> relaunch
* :hpx-issue:`7469` - Refactor duplicated disconnected-locality dispatch guard across async/post/sync implementations
* :hpx-issue:`7468` - force_disconnect: check-then-act race allows duplicate concurrent disconnects to broadcast redundant shutdown/cache-eviction traffic
* :hpx-issue:`7467` - Use a dedicated AGAS-specific timeout instead of the process-wide future timeout for hosted namespace RPCs
* :hpx-issue:`7461` - check-circular-deps CI failing on master (pre-existing, unrelated to PR #base 4f5b0d6b)
* :hpx-issue:`7441` - Feature request: root-initiated locality-departure cleanup (forced AGAS deregistration for crashed localities)
* :hpx-issue:`7428` - v1.11.0 fails to build with asio-1.38.2
* :hpx-issue:`7411` - Regression: flat all_to_all ~66x slower since 6df14adf09 (PU offset suppressed for all localities)
* :hpx-issue:`7406` - TCP parcelport advertises its configured port, so a locality cannot bind an OS-assigned port (hpx.parcel.port=0)
* :hpx-issue:`7390` - Expose completion and loss events for dynamically connected localities
* :hpx-issue:`7389` - `waittime` vs `wait_time` name mismatch when `HPX_HAVE_THREAD_QUEUE_WAITTIME` is ON
* :hpx-issue:`7384` - addressing_service::resolve_locality calls unlock() on a non-owning unique_lock when resolving a departed locality
* :hpx-issue:`7383` - HPX affinity binds introduce randomised offsets for multiple localities in single node
* :hpx-issue:`7358` - Memory usage increases during runtime
* :hpx-issue:`7333` - Insecure Deserialization - Type Confusion in Shared Pointers - HPX v1.11.0
* :hpx-issue:`7287` - vector_pack_load::unaligned broadcasts scalar instead of loading pack
* :hpx-issue:`7265` - --hpx:bind syntax for multiple localities per node (required for Aurora experiments)
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
* :hpx-issue:`7111` - Duplicate queues in scheduler(s)
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
* :hpx-issue:`7019` - Hpx Install problems / examples won't compile
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
* :hpx-issue:`6881` - Asynchronous channel communicator construction
* :hpx-issue:`6878` - Parallel uninitialized_relocate does not preserve sequenced semantics for overlapping ranges
* :hpx-issue:`6874` - tests.examples.1d_stencil.1d_stencil_5 is failing in some CI runs
* :hpx-issue:`6867` - v1.11.0 fails to build on F44
* :hpx-issue:`6854` - hpx::shared_mutex deadlock
* :hpx-issue:`6842` - Errors when building tests relying on stdexec
* :hpx-issue:`6798` - Provide a CMakePresets.json file
* :hpx-issue:`6793` - Performance in recursive fork-join is very bad
* :hpx-issue:`6786` - Problem : Building hpx [ have boost installed ]
* :hpx-issue:`6776` - Bug with hpx::collectives::set on macOS (arm64)
* :hpx-issue:`6772` - Broken bundle HPX + vcpkg + macOS since v1.10
* :hpx-issue:`6754` - Incorrect is_known_contiguous_iterator support for string
* :hpx-issue:`6752` - zip_iterator should have conditional special member functions
* :hpx-issue:`6745` - Forward `stdexec::bulk` in HPX to support full S/R NVIDIA's integration implementation
* :hpx-issue:`6735` - Failure when calling a parallel algorithm from a Python extension
* :hpx-issue:`6733` - Annotated subroutines don't show up in OTF2 traces when executed by IO_pool_executor
* :hpx-issue:`6729` - Consider unwrapping contiguous iterators in parallel algorithms
* :hpx-issue:`6728` - Building on ubuntu-24.04-arm
* :hpx-issue:`6722` - Unreachable code warning in advance_and_get_distance
* :hpx-issue:`6721` - Unreachable code warning in merge algorithm
* :hpx-issue:`6719` - Proxy support for find algorithms
* :hpx-issue:`6718` - Bad user experience with algorithms interface
* :hpx-issue:`6717` - Extensible execution policy design
* :hpx-issue:`6710` - Support Ninja Multi-Config generator
* :hpx-issue:`6653` - Passing `HPX_WITH_HIDDEN_VISIBILITY=TRUE` on MacOS results in library which can't be used
* :hpx-issue:`6627` - Extrema algorithms with repeated values
* :hpx-issue:`6625` - Update relocation semantics to match P2786R13
* :hpx-issue:`6624` - Remove type alias sender completions
* :hpx-issue:`6601` - Implement P2079 for HPX (System execution context)
* :hpx-issue:`6574` - Using the parcelport TCP causes the memory usage to continuously increase, and I'm not sure if it is a memory leak
* :hpx-issue:`6541` - Add HPX to Godbolt Compiler Explorer
* :hpx-issue:`6540` - Race conditions related to thread_local variables
* :hpx-issue:`6536` - Non-Standard Conforming Parallel Algorithms
* :hpx-issue:`6534` - Addressing remaining Stdexec issues
* :hpx-issue:`6528` - Wrong version recommendation of shpinx when building documentation
* :hpx-issue:`6506` - Create API similar to pthread_setaffinity_np for HPX threads
* :hpx-issue:`6504` - `FindTBB.cmake` cannot find correct TBB library.
* :hpx-issue:`6502` - The test executable numa_allocator_test fails with an assertion
* :hpx-issue:`6500` - The test partitioned_vector\_ doesn't finish in a very long time
* :hpx-issue:`6399` - Warning: type attributes ignored after type is already defined
* :hpx-issue:`6347` - Failed Linking CXX executable due to undefined references
* :hpx-issue:`6345` - Initialization hangs when only setting --hpx:cores
* :hpx-issue:`6333` - Current thread_id changing after suspending in direct execution
* :hpx-issue:`6240` - HPX does not compile with G++ version prior to 9.3.
* :hpx-issue:`6232` - Par performance of hpx::reverse
* :hpx-issue:`6224` - Add C++23 fold algorithms
* :hpx-issue:`6190` - --hpx:queuing=shared fails for distributed runs
* :hpx-issue:`6163` - Expose global termination detection through a new API
* :hpx-issue:`6097` - Parcelport fails to initialize when multiple jobs run on the same cluster
* :hpx-issue:`6028` - `hpx` namespaces without corresponding `std`
* :hpx-issue:`6014` - Support C++20 modules
* :hpx-issue:`6000` - Change namespaces of Public API
* :hpx-issue:`5978` - Registering Nested Component Actions
* :hpx-issue:`5918` - HpxChannels Registration fails for written out template types
* :hpx-issue:`5907` - HPX_DEBUG/CMAKE_BUILD_TYPE not correctly decoupled/consistently used.
* :hpx-issue:`5903` - Support for Windows HPC Pack
* :hpx-issue:`5544` - Use _GLIBCXX_DEBUG in CI
* :hpx-issue:`5498` - Add concepts alongside type traits
* :hpx-issue:`5497` - Start using C++17 features unconditionally
* :hpx-issue:`5411` - Add support for conan
* :hpx-issue:`5189` - Shared priority scheduler does not always create a new thread
* :hpx-issue:`5045` - Implement P0443/P1897/P2300
* :hpx-issue:`4702` - Prefer enum class over unscoped enums
* :hpx-issue:`4697` - get_num_localities and get_locality_id should return size_t
* :hpx-issue:`4672` - Consistently use hpx::function_* instead of std::function
* :hpx-issue:`4657` - HEP 1 implementation tracker
* :hpx-issue:`4627` - Allow HPX counters to be requested before they are registered
* :hpx-issue:`4568` - Make performance counters work locally
* :hpx-issue:`4567` - Make APEX work with local runtime
* :hpx-issue:`4565` - Make iostreams component work locally
* :hpx-issue:`4371` - Using `std::get` on type derived from `std::pair` fails compilation
* :hpx-issue:`4367` - Rewrite history to remove unneeded binary files
* :hpx-issue:`4329` - HPX 2
* :hpx-issue:`4274` - Implement Task-Bench (potentially as a GSoC project)
* :hpx-issue:`4074` - HPX module "configurations"
* :hpx-issue:`2589` - Missing documentation for LCOs
* :hpx-issue:`2235` - Concurrent data structures support

Merged pull requests
====================

* :hpx-pr:`7949` - Complete index-queue bulk operations when their last chunk is done
* :hpx-pr:`7948` - Add task_size_chunk_size executor parameters
* :hpx-pr:`7946` - Size the builtin operations from the types of their arguments
* :hpx-pr:`7944` - Run one timed step of the scheduler latency report as a test
* :hpx-pr:`7942` - Partition merge over all output elements
* :hpx-pr:`7926` - Give task sizes the run-time lookup function annotations have
* :hpx-pr:`7925` - Take task sizes from functions only
* :hpx-pr:`7920` - fix the all_reduce return docs and drop a broadcast_to overload that doesn't exist
* :hpx-pr:`7919` - let late_component survive its crashing and departing workers
* :hpx-pr:`7918` - don't leave a wake_timer thread pending after the workers stopped
* :hpx-pr:`7917` - drop credit decrements for localities that are gone
* :hpx-pr:`7912` - Don't clear a connection cache entry while sends are in flight
* :hpx-pr:`7911` - Fix a race in the thread_queue_get_next_threads test
* :hpx-pr:`7909` - Fix out-of-bounds array access in shared_priority_queue_scheduler::destroy_thread
* :hpx-pr:`7904` - Fix late condition_variable wakeups and wake waiters without reference counting
* :hpx-pr:`7903` - Add scripts to backfill the performance history by hand
* :hpx-pr:`7901` - Allow hpx::force_disconnect to be called from any locality
* :hpx-pr:`7900` - Fix hierarchical dangling policy
* :hpx-pr:`7899` - Show the commits and the changes of the medians on the performance history graphs
* :hpx-pr:`7897` - Don't fail a target when an observer's locality went away
* :hpx-pr:`7896` - Mention the Discord server in the README support section
* :hpx-pr:`7895` - test shutdown with a remote activity observer
* :hpx-pr:`7894` - Fix inverted error check in reschedule_on_thread causing task starvation
* :hpx-pr:`7892` - Discard entries in the documented order with size_entry and lfu_entry
* :hpx-pr:`7889` - Use size_type for the amount local_cache::free_space has to free
* :hpx-pr:`7888` - Add scheduler throughput and latency performance reports
* :hpx-pr:`7886` - Require GCC 12 in the configuration
* :hpx-pr:`7885` - Use std::atomic in stdexec for builds without CUDA
* :hpx-pr:`7883` - Bump github/codeql-action from 4.39.0 to 4.39.1
* :hpx-pr:`7879` - Fix condition_variable abort_all/notify_one waking threads that already left the wait
* :hpx-pr:`7874` - Fix dual-ownership race in shared_mutex::try_lock
* :hpx-pr:`7872` - Add threads::get_destructive_interference_size() and regroup the scheduler's per-worker data
* :hpx-pr:`7868` - Report unexpected force_disconnect exceptions
* :hpx-pr:`7867` - leave discover_and_join time to join the peers it found
* :hpx-pr:`7866` - don't wait on remote activity agents while shutting down
* :hpx-pr:`7865` - report a send to a locality that went away as disconnected
* :hpx-pr:`7864` - Fix shutdown hang: work scheduled on the queue of a worker that has already left
* :hpx-pr:`7863` - Avoid modulo by zero in test_simd_helpers
* :hpx-pr:`7860` - Fix chunk_size_iterator for chunk sizes larger than the element count
* :hpx-pr:`7858` - Install the Windows CI dependencies with vcpkg and the GitHub Actions cache
* :hpx-pr:`7856` - Replace the iteration windows of the early steal minima
* :hpx-pr:`7855` - Let the shared priority scheduler report the length of all its queues
* :hpx-pr:`7848` - Add hpx::threads::task_size and with_task_size (step 1 of #7837)
* :hpx-pr:`7847` - Fix size accounting in local_cache::update for replaced entries
* :hpx-pr:`7845` - Align the per-worker scheduler data to the cache line size
* :hpx-pr:`7839` - Link the Jenkins builds in the yearly index of the performance history
* :hpx-pr:`7838` - Index counter_data\_ by the pool-local thread number in thread_func's exit log
* :hpx-pr:`7836` - futures: count run_as_child children run directly by their parent
* :hpx-pr:`7835` - Cancel pending await_terminal waiters before runtime shutdown
* :hpx-pr:`7833` - Fix pkg-config generation with MPI link flags wrapped in HOST_LINK
* :hpx-pr:`7832` - Document the thread scheduler settings
* :hpx-pr:`7831` - Make the links of the timeline graphs relative to their directory
* :hpx-pr:`7829` - Allow any site to be the root of a hierarchical communicator
* :hpx-pr:`7828` - Forcing unknown parcelset settings
* :hpx-pr:`7824` - Fix local_cache::free_space skipping the next entry to evict
* :hpx-pr:`7822` - synchronization: make counting_semaphore wait_until timeout test independent of scheduling
* :hpx-pr:`7821` - cmake: keep hwloc's archive timestamps and check its build result
* :hpx-pr:`7819` - futures: keep the result of a stackful run_as_child child run directly
* :hpx-pr:`7817` - threading: stop and join the running thread in jthread move assignment
* :hpx-pr:`7816` - Fix host name resolution with Asio 1.34+
* :hpx-pr:`7809` - Fix null pointer dereference in split_future on invalid futures (#7806)
* :hpx-pr:`7805` - Fill gaps in the supervision_dispatch API reference
* :hpx-pr:`7804` - Fix supervision_dispatch manual drift from the API
* :hpx-pr:`7802` - synchronization: don't lose a release() when counting_semaphore::wait_until times out
* :hpx-pr:`7800` - Set up nanobench independently of networking, and build it with MSVC
* :hpx-pr:`7797` - Create the background thread without a run_as_child hint
* :hpx-pr:`7794` - build(deps): bump github/codeql-action from 4.38.3 to 4.39.0
* :hpx-pr:`7793` - Fix local_cache::reserve evicting based on the old capacity
* :hpx-pr:`7791` - give each scatter test phase its own basename
* :hpx-pr:`7790` - Fix documentation warnings
* :hpx-pr:`7787` - Fix data race and queue desynchronization in condition_variable
* :hpx-pr:`7784` - Fix lost wakeup and spurious wakeup race condition in scheduler_base
* :hpx-pr:`7782` - synchronization: don't wait for an unregistered stop_callback in its destructor
* :hpx-pr:`7781` - give each all_to_all test phase its own basename
* :hpx-pr:`7776` - Fix the rejoin race in failure_detection_join
* :hpx-pr:`7771` - Honor min_tasks_to_steal_pending in the work-requesting schedulers
* :hpx-pr:`7770` - Document that local_queue_scheduler ignores thread priorities
* :hpx-pr:`7769` - Fix dangling pointer and double-free in hpx::local::stop (#7768)
* :hpx-pr:`7767` - Fix parallel find_end comparing the wrong elements
* :hpx-pr:`7765` - Fix cross-thread data race in local_workrequesting_scheduler
* :hpx-pr:`7764` - Fix queuing regression under configured schedulers
* :hpx-pr:`7762` - Fix uninitialized execute\_ function pointer in run_loop_opstate
* :hpx-pr:`7759` - Export the background thread helpers from the HPX.Core module
* :hpx-pr:`7757` - Fix command line scheduler option handling
* :hpx-pr:`7756` - Restore NUMA sensitivity option behavior
* :hpx-pr:`7755` - Keep work-requesting schedulers in fast idle mode
* :hpx-pr:`7754` - Fix thread queue parameter handling
* :hpx-pr:`7753` - Fix scheduler mode overrides being skipped
* :hpx-pr:`7752` - Fix hpx.max_background_threads state leakage across pools
* :hpx-pr:`7751` - Take runs_child\_ safely and keep the schedule hint in post()
* :hpx-pr:`7750` - Fix background_scheduler::set_scheduler_mode setting all mode bits
* :hpx-pr:`7749` - Cover background thread creation under every scheduler
* :hpx-pr:`7741` - Make refused TCP connection attempts fail fast on Windows
* :hpx-pr:`7740` - Keep bound threads on their worker thread when they yield with pending_boost
* :hpx-pr:`7739` - parcelport_mpi: Pre-post pool of wildcard header receives to prevent AGAS bootstrap timeout
* :hpx-pr:`7738` - Avoid names that collide with the Windows SDK and check for them in inspect
* :hpx-pr:`7736` - Don't collect bool chunk results in a std::vector<bool>
* :hpx-pr:`7735` - Fix chunk_size_iterator decrement for bidirectional iterators on MSVC
* :hpx-pr:`7732` - Pass /INCLUDE for custom allocators to the linker on MSVC
* :hpx-pr:`7731` - Update performance baseline
* :hpx-pr:`7730` - Don't apply min_tasks_to_steal_staged to a queue's own staged tasks
* :hpx-pr:`7729` - Update HPX_AddConfigTest.cmake
* :hpx-pr:`7728` - Pass build configuration to Jenkins CTest driver
* :hpx-pr:`7727` - Use yield for HPX_SMT_PAUSE on AArch64
* :hpx-pr:`7726` - Make the work-requesting schedulers actually steal
* :hpx-pr:`7725` - Reduce cross-core contention in the scheduling loop and local_priority_queue_scheduler
* :hpx-pr:`7723` - don't drop a yielded background thread at shutdown
* :hpx-pr:`7722` - build(deps): bump github/codeql-action from 4.38.2 to 4.38.3
* :hpx-pr:`7720` - don't crash the tcp sender when shutdown races its cleanup
* :hpx-pr:`7718` - futures: run stackless run_as_child children directly
* :hpx-pr:`7717` - Fix networking requirements for segmented merge tests
* :hpx-pr:`7714` - synchronization: keep waiting in timed_mutex::try_lock_until until timeout or lock
* :hpx-pr:`7711` - Fix bundled stdexec compatibility for NVCC host senders and SYCL
* :hpx-pr:`7709` - Adding explicit timeouts to test
* :hpx-pr:`7707` - Enable C++20 modules for segmented merge
* :hpx-pr:`7706` - Convert a range length to std::size_t explicitly
* :hpx-pr:`7705` - Check the checkpoint length read off a stream before using it
* :hpx-pr:`7704` - Convert counter path indices to pool indices explicitly
* :hpx-pr:`7703` - Fix the stride semantics of the strided for_loop algorithms
* :hpx-pr:`7702` - Make idle_backoff_data::wait_count atomic
* :hpx-pr:`7700` - Let HPX_CONTRACT_ASSERT fall back onto HPX_ASSERT if contracts are disabled
* :hpx-pr:`7699` - fix fork scheduling hint
* :hpx-pr:`7698` - Fix segmented merge tests with C++20 modules
* :hpx-pr:`7695` - Patch stdexec async_scope::on_empty() for nvcc
* :hpx-pr:`7693` - Fix blocking client calls on unresolved ids and MPI finalize ownership
* :hpx-pr:`7692` - Make rnd_future_reduce's random generator thread-local
* :hpx-pr:`7689` - build(deps): bump lukka/get-cmake from 4.4.2 to 4.4.4
* :hpx-pr:`7688` - Make replicate vote tests deterministic
* :hpx-pr:`7687` - Fix `launch::fork` dropping tasks from OS threads
* :hpx-pr:`7686` - Enable Apex for local runtime
* :hpx-pr:`7685` - synchronization: don't access barrier phase after releasing the lock in arrive_and_wait
* :hpx-pr:`7684` - docs: use correct HPX::iostreams_component target name in quickstart
* :hpx-pr:`7683` - godbolt-minimal: pass /SUBSYSTEM:CONSOLE in the raw MSVC path
* :hpx-pr:`7682` - Ensure shared priority scheduler creates thread when requested
* :hpx-pr:`7681` - futures: fix shared_future::get() postcondition in docs
* :hpx-pr:`7679` - Fix parallel scheduler tests on Clang 16
* :hpx-pr:`7677` - fix(futures): upgrade memory_order_relaxed to acquire after cond_var wakeup in wait()
* :hpx-pr:`7675` - tracing: task lifecycle and work stealing for the ITT backend
* :hpx-pr:`7674` - Enable specifying the number of vector lanes to use with simd execution policies
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
* :hpx-pr:`7652` - execution_base: use the static empty vtable fallback on all Windows b...
* :hpx-pr:`7651` - Preserve range shapes in scheduler executors
* :hpx-pr:`7649` - Report process setup failures without compiler warnings
* :hpx-pr:`7644` - godbolt-minimal: CI-test and document the raw MSVC hpx_main path
* :hpx-pr:`7643` - lcos_local: cancel pending get/set requests when closing a one_element_channel
* :hpx-pr:`7642` - Bump dawidd6/action-download-artifact from 24 to 25
* :hpx-pr:`7641` - Bump github/codeql-action from 4.38.1 to 4.38.2
* :hpx-pr:`7640` - Fix/coroutine asan stack recycling
* :hpx-pr:`7637` - Update Collective Benchmark
* :hpx-pr:`7636` - tracing: fill in the ITT backend - shared domain, regions, markers
* :hpx-pr:`7635` - components_base: fix wrapper heap reclamation
* :hpx-pr:`7634` - Mark zero-copy save chunks as const and harden vector chunk tests
* :hpx-pr:`7633` - Add companion concepts for futures, execution policies, and iterators
* :hpx-pr:`7632` - Re-enable auto-linking on Windows (dynamic linking, C++20 modules)
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
* :hpx-pr:`7603` - Fix build warnings of documentation
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
* :hpx-pr:`7585` - Add construction-side customization points to pair with rebind_policy_executor_t/rebind_policy_parameters_t
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
* :hpx-pr:`7228` - CI: Fix cancellation workflow API flags, permissions, and filters
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
* :hpx-pr:`6514` - Integration of perftest CI with standardised benchmarks
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
