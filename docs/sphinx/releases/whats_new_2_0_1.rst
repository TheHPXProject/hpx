..
    Copyright (C) 2007-2026 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_2_0_1:

==================
|hpx| V2.0.1 (TBD)
==================

General changes
===============

This point release fixes problems found in |hpx| V2.0.0. It consists of fixes
that were merged to the master branch after V2.0.0 and that apply to the 2.0
code base. It doesn't contain any of the new features of the master branch.

Synchronization, futures and LCOs
---------------------------------

- ``hpx::stop_callback`` now waits for a callback running on a thread that is
  not an |hpx| thread before it is destroyed, and assigning to an
  ``hpx::stop_source`` releases the old stop state correctly.
- ``hpx::condition_variable_any::wait`` with a stop token returns the result of
  the predicate when it observes a stop request.
- Calling an ``hpx::packaged_task`` a second time no longer invokes its callable
  again.
- ``hpx::unwrap`` can be used with containers of containers that hold no
  futures, which fixes ``hpx::reduce`` over ``std::string``.
- Coroutines returning an ``hpx::future`` or ``hpx::shared_future`` no longer
  use their frame after it was freed or cause a double free, and the
  asynchronous pack traversal (used by ``hpx::dataflow`` and
  ``hpx::when_all``) destroys its iterators before its frame completes.
- A timer no longer wakes a thread after the suspension it was set for has
  ended.

Senders and receivers
---------------------

- Fixed races and lifetime problems in ``run_loop``, ``when_all_vector`` and
  ``async_rw_mutex`` that could lead to deadlocks, dropped continuations or
  calls to ``std::terminate``.
- Errors that are not exceptions are now propagated through the bridge from
  senders to futures.

Parallel algorithms
-------------------

- Fixed wrong results of the parallel set operations (``set_union``,
  ``set_symmetric_difference``, ``set_intersection``, ``includes``), of
  ``partial_sort``, ``nth_element``, ``make_heap``, ``any_of`` and
  ``reduce_by_key``, and of the reducing algorithms used with
  ``auto_chunk_size``.
- Fixed the iterators returned by ``reverse_copy``, ``rotate_copy`` and the
  sequential ``ranges::transform`` and ``ranges::set_intersection``.
- More ``hpx::ranges`` algorithms accept a sentinel of a different type than
  the iterator, and ``hpx::reduce`` no longer requires the dereferenced
  iterator to be convertible to the type of the initial value.
- ``rotate`` no longer reads past the end of the affinity masks when it runs
  with a sender policy.
- The segmented versions of ``equal``, ``mismatch``, ``adjacent_difference``
  and the binary ``transform`` handle partition boundaries correctly.
- ``for_loop_strided`` and ``for_loop_n_strided`` pass the correct induction
  values to every partition when they run with a parallel policy.

Schedulers, threads and containers
----------------------------------

- Fixed the mapping of PU masks in the scheduler mode setters, a race of the
  steal masks in the local schedulers and the counting of threads in the
  thread queues.
- The index-queue bulk executors handle failures to spawn a thread.
- The SIGSEGV and termination handlers no longer deadlock on POSIX systems.
- A thread created from outside a pool while its last worker leaves the
  scheduling loop is no longer stranded, which could make the runtime hang at
  shutdown.
- Move assignment of ``concurrent_unordered_map`` and
  ``concurrent_unordered_set`` is synchronized.

Distributed runtime, parcels and collectives
--------------------------------------------

- Fixed several problems during shutdown: parcels received after the worker
  threads stopped no longer abort the application, the parcelports are stopped
  before the distributed runtime is cleared, and the global barrier is
  detached only after all shutdown functions have run. The TCP parcelport no
  longer deadlocks while it shuts down.
- The runtime no longer hangs when a startup function throws an exception, or
  when registering the message handlers or performance counters fails during
  startup.
- The collective operations no longer deadlock on a communicator that can't
  be resolved, or when a request for a later generation arrives before the
  earlier generations are done. Such a request now returns right away instead
  of waiting for the earlier generations.
- A pinned migratable component is destroyed only when it is unpinned for the
  last time.
- Output written to ``hpx::cout`` after the runtime has stopped is no longer
  lost and no longer aborts the application. ``hpx::endl`` and ``hpx::flush``
  are aliases of ``std::endl`` and ``std::flush`` now, as they already were in
  builds without the distributed runtime.
- Failures of sends that the parcel handler reschedules are reported through
  the write handler instead of being lost.

Utilities and serialization
---------------------------

- Loading an empty ``std::list`` or ``std::deque`` from an archive clears the
  target container.
- Fixed ``hpx::util::format``, ``regex_from_pattern`` and
  ``parse_sed_expression`` for malformed input, and the size accounting and
  ``update_if`` of the LRU cache.
- The names of Tracy plots stay valid for the whole run.

Build system and platform support
---------------------------------

- |hpx| builds and runs with CUDA on Windows again.
- Fixed building with nvcc, and including the |hpx| startup headers in builds
  with C++20 modules.
- C++20 module builds register components under their own names. With MSVC
  19.44 and newer, components, examples and tests are now also built in module
  mode.
- Fetched dependencies no longer install their source paths and empty
  directories.
- |hpx| is now compliant with the `REUSE <https://reuse.software/>`_
  specification.

Closed issues
=============

* :hpx-issue:`8105` - hpx::heap_until (par) 6% slower since #7963
* :hpx-issue:`8070` - Runtime hangs, busy, when a startup function throws on one of two localities
* :hpx-issue:`8064` - Parallel set operations lose results when a chunk size is set
* :hpx-issue:`8041` - hpx::util::format never returns for a replacement field with an explicit index and no closing '}'
* :hpx-issue:`8039` - parse_sed_expression throws std::out_of_range when the search part has no closing '/'
* :hpx-issue:`8017` - Thread count of a scheduler can miss a thread that moves between queues
* :hpx-issue:`8013` - packaged_task::operator() invokes the callable again when called a second time
* :hpx-issue:`8011` - regex_from_pattern reads past the end of the string when the pattern ends in '['
* :hpx-issue:`8003` - Several hpx::ranges algorithms don't compile when the end of a range is a sentinel of a different type
* :hpx-issue:`7999` - Binary segmented traversal mishandles partition boundaries and crashes with different localities
* :hpx-issue:`7995` - hpx::stop_source copy/move assignment doesn't release the old stop state's source count
* :hpx-issue:`7990` - Make HPX compatible with https://reuse.software/
* :hpx-issue:`7971` - std::list and std::deque deserialization doesn't clear the target when the archive holds an empty container
* :hpx-issue:`7969` - reduce_by_key ignores the user-supplied comparator for the last key
* :hpx-issue:`7960` - hpx::ranges::is_partitioned fails to compile with distinct C++20 sentinels (e.g., std::views::take_while)
* :hpx-issue:`7950` - Credit decrement to a disconnected locality can still throw when put_parcel() reschedules the send
* :hpx-issue:`7945` - hpx::unwrap fails to compile for a container of containers without futures, which breaks hpx::reduce of std::string
* :hpx-issue:`7940` - Sequential ranges::transform (two ranges) and ranges::set_intersection return the wrong in1/in2 iterators
* :hpx-issue:`7935` - Parallel set_union and set_symmetric_difference silently drop elements of the second range
* :hpx-issue:`7929` - partial_sort and nth_element return wrong results when the recursion depth limit is reached: the heap fallback selects the largest elements
* :hpx-issue:`7913` - hpx::ranges::reverse_copy and rotate_copy with par return the wrong input iterator
* :hpx-issue:`7906` - Uncatchable std::terminate() and dropped continuations in async_rw_mutex
* :hpx-issue:`7905` - make_heap with par can produce an invalid heap for sizes 2^k and 2^k + 1
* :hpx-issue:`7890` - State Machine Race and Permanent Deadlock in run_loop::push_back
* :hpx-issue:`7881` - hpx::adjacent_difference doesn't handle values at segmented partition boundaries
* :hpx-issue:`7870` - includes returns true with a parallel policy if the second range contains a run of equal elements that is not in the first range
* :hpx-issue:`7861` - rotate_sender test aborts with std::bad_array_new_length
* :hpx-issue:`7851` - Unsynchronized Move-Assignment in concurrent_unordered_map and concurrent_unordered_set
* :hpx-issue:`7814` - Segmented equal and mismatch mishandle unaligned ranges and partition placement
* :hpx-issue:`7811` - Scheduler mode setters apply PU masks to the wrong workers, and write past modes_ with several pools
* :hpx-issue:`7806` - Critical concurrency and safety defects in P2300 execution algorithms and async combinators
* :hpx-issue:`7788` - stop_callback destructor doesn't wait for a callback running on another non-HPX thread
* :hpx-issue:`7712` - Preserve assertion behavior when HPX_WITH_CONTRACTS=OFF
* :hpx-issue:`6647` - Incorrect reduce implementation
* :hpx-issue:`6430` - HPX distributed barrier runs into assertion error

Closed pull requests
====================

* :hpx-pr:`8126` - Fix use after free of coroutine frames returning hpx::future
* :hpx-pr:`8121` - Build the C++20 modules CI with clang 23
* :hpx-pr:`8120` - build(deps): bump lukka/get-cmake from 4.4.5 to 4.5.0
* :hpx-pr:`8118` - Don't strand a thread created while the last worker leaves
* :hpx-pr:`8116` - Fix distributed bootstrap hangs on registration failures
* :hpx-pr:`8113` - Fix a run_loop future deadlock and an async_rw_mutex assertion
* :hpx-pr:`8112` - Fix TCP parcelport shutdown deadlocks
* :hpx-pr:`8110` - executors: combine caller and local chunk completion
* :hpx-pr:`8109` - Repeat the workrequesting_steal round until a task is stolen
* :hpx-pr:`8108` - Don't let a timer wake a thread after its suspension has ended
* :hpx-pr:`8107` - Fix use after scope in the algorithm_as_sender test
* :hpx-pr:`8101` - Fix the DCO check, bound the backfill queue wait, and fix legend labels and contracts examples [without the contracts examples]
* :hpx-pr:`8100` - Split the segmented merge regression test into five smaller tests
* :hpx-pr:`8097` - Fix size accounting and update_if of lru_cache, don't evict for failing local_cache inserts
* :hpx-pr:`8095` - Break ties between lru_entry access times
* :hpx-pr:`8094` - Measure condition variable wake time in its waiter
* :hpx-pr:`8093` - Keep swap_ranges test values distinct from sentinels
* :hpx-pr:`8092` - Fix runtime hang when a startup or pre-startup functions throw [startup functions only] (fixes #8070)
* :hpx-pr:`8087` - Fix sender support for loop algorithms [strided induction fix only]
* :hpx-pr:`8079` - Stop the parcelports before the distributed runtime is cleared
* :hpx-pr:`8078` - Destroy a pinned migratable component at its last unpin
* :hpx-pr:`8077` - Check the sign-offs of pull requests without passing the commits as input
* :hpx-pr:`8073` - Don't abort on parcels received after the worker threads have stopped
* :hpx-pr:`8072` - Keep the external build test binaries in their own directories
* :hpx-pr:`8069` - Keep iostreams output after shutdown on all paths, restore hpx::endl and hpx::flush
* :hpx-pr:`8068` - Keep hpx::cout output written after the runtime has stopped
* :hpx-pr:`8067` - Don't use the distributed runtime in parcel callbacks once it has stopped
* :hpx-pr:`8066` - Shard the C++20 modules workflows for Windows and Linux
* :hpx-pr:`8065` - Don't drop set operation results for chunk sizes larger than one
* :hpx-pr:`8063` - Return shared_future from get_return_object of shared_future coroutines
* :hpx-pr:`8062` - Keep the child thread alive while checking it after running it directly
* :hpx-pr:`8061` - Work around MSVC problems with C++20 modules and enable module builds for MSVC 19.44 and newer
* :hpx-pr:`8059` - Register components built with C++20 modules under their own name
* :hpx-pr:`8058` - Run the Linux hwloc fetch workflow in the build_env container
* :hpx-pr:`8057` - Fix a livelock in the freelist test
* :hpx-pr:`8056` - Exclude command_line_handling from the C++20 modules CI for now
* :hpx-pr:`8055` - collectives: keep a request for a later generation instead of waiting for it
* :hpx-pr:`8054` - Build and run only the examples in the macOS fetch workflows
* :hpx-pr:`8053` - Build and test macOS Debug in shards
* :hpx-pr:`8052` - Add a reusable workflow for the sharded build and test scheme
* :hpx-pr:`8051` - Make the sharded CI build steps portable to macOS and Windows
* :hpx-pr:`8050` - collectives: detach the global barrier after all shutdown functions
* :hpx-pr:`8047` - Fix the C++20 modules CI and compile more tests with modules on Clang
* :hpx-pr:`8045` - collectives: apply the unresolved communicator fix to all collectives
* :hpx-pr:`8044` - Keep Tracy plot names alive for the whole run
* :hpx-pr:`8042` - Fix hpx::util::format not rejecting a replacement field without a closing brace
* :hpx-pr:`8040` - Fix parse_sed_expression throwing when the search part is not closed
* :hpx-pr:`8031` - Bump github/codeql-action from 4.39.2 to 4.39.3
* :hpx-pr:`8026` - Construct reinitializable_static in the host pass of nvcc
* :hpx-pr:`8024` - Check the thread count again after suspending all workers
* :hpx-pr:`8023` - execution: Type-erase non-exception errors in sender_future bridge
* :hpx-pr:`8021` - Add missing runs to the performance history from every master build
* :hpx-pr:`8016` - Read the thread queue counters in a race-free order
* :hpx-pr:`8015` - Fix intermittent failure of supervision_await_terminal on remote localities
* :hpx-pr:`8014` - Don't invoke the callable again when a packaged_task is called twice
* :hpx-pr:`8012` - Fix regex_from_pattern reading past the end on a trailing '['
* :hpx-pr:`8010` - collectives: fix barrier deadlock on an unresolved communicator
* :hpx-pr:`8009` - Preserve options after a missing queuing value
* :hpx-pr:`8008` - Fix PU lookup and steal mask race in local schedulers
* :hpx-pr:`8006` - Suppress use-after-move warning in stop_token test
* :hpx-pr:`8004` - Accept a sentinel of a different type in several ranges algorithms
* :hpx-pr:`8002` - Bump github/codeql-action from 4.39.1 to 4.39.2
* :hpx-pr:`8000` - Fix segmented binary transform across partition boundaries
* :hpx-pr:`7998` - execution: Fix critical lifecycle and race conditions in when_all_vector
* :hpx-pr:`7996` - Fix stop_source assignment leaking the old stop state's source count
* :hpx-pr:`7993` - Keep the legends of the performance history graphs within the README width
* :hpx-pr:`7992` - Restrict permissions and credentials of the Check Formatting workflow
* :hpx-pr:`7991` - Make HPX compliant with the REUSE specification
* :hpx-pr:`7989` - tests: add regression tests for partial_sort early return
* :hpx-pr:`7987` - Don't deadlock in the SIGSEGV and termination handlers on POSIX systems
* :hpx-pr:`7986` - Destroy the iterators of the async pack traversal before its frame completes
* :hpx-pr:`7985` - Let performance tests choose their warmup, use 2 steps in the scheduler latency report
* :hpx-pr:`7984` - Mark the releases on the performance history graphs
* :hpx-pr:`7982` - Support unsized sentinels in make_heap
* :hpx-pr:`7981` - Fix partial_sort's sorted-prefix fast path
* :hpx-pr:`7979` - Bump dawidd6/action-download-artifact from 27 to 28
* :hpx-pr:`7976` - Don't make processing_units_count of num_cores and task_size_chunk_size noexcept
* :hpx-pr:`7975` - Bump lukka/get-cmake from 4.4.4 to 4.4.5
* :hpx-pr:`7974` - Fix building and running HPX with CUDA on Windows
* :hpx-pr:`7972` - Clear std::list and std::deque when loading an empty archive
* :hpx-pr:`7970` - Fix reduce_by_key ignoring the comparator for the last key
* :hpx-pr:`7965` - parcelset: report rescheduled put_parcel failures through the write handler
* :hpx-pr:`7963` - Handle spawn failures in the index-queue bulk executors
* :hpx-pr:`7961` - Make hpx::ranges::is_partitioned support distinct C++20 sentinels
* :hpx-pr:`7959` - Don't install the source paths and empty directories of fetched dependencies
* :hpx-pr:`7958` - Include <ios> where std::streamsize is used
* :hpx-pr:`7951` - Make hpx::unwrap SFINAE-friendly for nested containers without futures
* :hpx-pr:`7941` - Fix the input iterators returned by sequential ranges::transform and set_intersection
* :hpx-pr:`7934` - Fix reducing algorithms with auto_chunk_size and parallel_executor
* :hpx-pr:`7930` - Fix the heap fallback of partial_sort and nth_element
* :hpx-pr:`7914` - Return last as the input iterator from reverse_copy and rotate_copy
* :hpx-pr:`7910` - Fix hpx::make_heap for power-of-two sized ranges
* :hpx-pr:`7907` - Fix uncatchable std::terminate and dropped continuations in async_rw_mutex
* :hpx-pr:`7902` - synchronization: return pred() when condition_variable_any wait observes stop
* :hpx-pr:`7891` - Fix state machine race and deadlock in run_loop
* :hpx-pr:`7882` - Fix segmented adjacent_difference across partition boundaries
* :hpx-pr:`7880` - Fix any_of returning true for an empty range with parallel policies
* :hpx-pr:`7877` - Fix startup header redefinition with C++ modules
* :hpx-pr:`7871` - Fix includes returning true for a run of equal elements not in the first range
* :hpx-pr:`7869` - algorithms: do not require \*first convertible to T in parallel reduce
* :hpx-pr:`7862` - algorithms: bind core count onto scheduler in rotate
* :hpx-pr:`7852` - Fix unsynchronized move-assignment in concurrent unordered containers
* :hpx-pr:`7844` - algorithms: do not skip leading elements of the second range in parallel set operations
* :hpx-pr:`7815` - fix: handle segmented comparison alignment
* :hpx-pr:`7812` - Fix scheduler mode setters mapping of PU masks across pools
* :hpx-pr:`7808` - Fix unconditional std::terminate in run_loop destructor (#7806)
* :hpx-pr:`7798` - Fix stop_callback destructor not waiting on another non-HPX thread
* :hpx-pr:`7795` - Stop dereferencing a null pointer in lambda_to_action
