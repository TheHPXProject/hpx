..
    Copyright (C) 2007-2025 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_2_0_0:

============================
|hpx| V2.0.0 (TBD)
============================

This release requires compilation using C++20 or above. V1.11.0 was the last
version of |hpx| that supported C++17.

General changes
===============

- |hpx| now requires C++20. Relying on the newer standard has allowed for a
  widespread modernization of the code base.
- |hpx| can now be consumed as C++20 modules. All modules of the library have
  been adapted, and no changes are required in user code to benefit from this.
- |hpx| now supports C++26.

  - C++26 reflection is used for the serialization of arbitrary C++ types.
    Users no longer have to write serialization support functions for their own
    types.
  - C++26 reflection is used to simplify the code generation for actions and
    components.
  - C++26 contracts are now supported. |hpx| API functions have started to be
    augmented with contract specifications.

Breaking changes
================

- C++20 is now the minimum required standard. Compiling |hpx| or applications
  using it with C++17 is no longer supported.

Closed issues
=============

* :hpx-issue:`7520` - Data race in hostname_print_helper::get_hostname() on worker thread startup
* :hpx-issue:`7517` - HPX_FORWARD does not forward under nvcc: cudafe strips && from static_cast<decltype(x)&&>
* :hpx-issue:`7513` - dijkstra_termination_disconnected_*_7474 abort on any HPX_WITH_SUPERVISION=OFF build
* :hpx-issue:`7483` - force_disconnect: race between disconnect completion and AGAS resolve visibility
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
* :hpx-issue:`7212` - Make inspect report links use the scanned tree’s actual Git commit
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
* :hpx-issue:`7000` - tests: 7 sender test files declare std::mt19937 gen but hpx_main calls  std::srand(seed) instead of gen.seed(seed), making --seed silently ineffective
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
* :hpx-issue:`6931` - Two correctness bugs in  chunk_size.hpp : swapped next_or_subrange args and mismatched  add_ready_future_idx  parameter order
* :hpx-issue:`6922` - fork_join_executor silently truncates ranges > 2^32
* :hpx-issue:`6908` - Implement missing C++23 range algorithms: fold, chunk_by, slide, stride, cartesian_product
* :hpx-issue:`6906` - Add missing subtraction unit tests for gid_type
* :hpx-issue:`6905` - Split fast path ignores receiver’s scheduler leading to mismatched completion thread
* :hpx-issue:`6904` - reduce lock contention in local_priority_queue_scheduler via randomized victim selection
* :hpx-issue:`6896` - Fix incomplete sentence in README.rst
* :hpx-issue:`6892` - Add find_last, find_last_if, and find_last_if_not algorithms
* :hpx-issue:`6888` - Enhancement: Add timeout and cancellation support to hpx::local::termination_detection()
* :hpx-issue:`6878` - Parallel uninitialized_relocate does not preserve sequenced semantics for overlapping ranges
* :hpx-issue:`6874` - tests.examples.1d_stencil.1d_stencil_5 is failing in some CI runs
* :hpx-issue:`6854` - hpx::shared_mutex deadlock
* :hpx-issue:`6842` - Errors when building tests relying on stdexec
* :hpx-issue:`6798` - Provide a CMakePresets.json file
* :hpx-issue:`6786` - Problem : Building hpx  [ have boost installed ]
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
* :hpx-issue:`6500` - The test partitioned_vector\_ doesn't finish in a very long time
* :hpx-issue:`6347` - Failed Linking CXX executable due to undefined references
* :hpx-issue:`6345` - Initialization hangs when only setting --hpx:cores
* :hpx-issue:`6240` - HPX does not compile with G++ version prior to 9.3.
* :hpx-issue:`6232` - Par performance of hpx::reverse
* :hpx-issue:`6163` - Expose global termination detection through a new API
* :hpx-issue:`6014` - Support C++20 modules
* :hpx-issue:`5907` - HPX_DEBUG/CMAKE_BUILD_TYPE not correctly decoupled/consistently used.
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

Closed pull requests
====================

* :hpx-pr:`7537` - Refactor disconnected locality dispatch guard
* :hpx-pr:`7535` - Fix data race in hostname_print_helper::get_hostname()"
* :hpx-pr:`7534` - Keep LSU matrix artifacts separate
* :hpx-pr:`7533` - Stop failed LSU builds from publishing installs
* :hpx-pr:`7532` - Fix LSU GitHub status reporting
* :hpx-pr:`7531` - Bound Slurm waits in LSU CI
* :hpx-pr:`7530` - Add feature build check for std::filesystem::display_string
* :hpx-pr:`7529` - Tracing: add per-subsystem gates for event classes
* :hpx-pr:`7528` - Tracy: bump to v0.14.1
* :hpx-pr:`7526` - Avoid communicator reuse between collectives test phases
* :hpx-pr:`7525` - Fix data race in debug hostname printing
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
* :hpx-pr:`7506` - Fix the two shutdown regressions from #7471 that hang the distributed CIs
* :hpx-pr:`7505` - Waiting for threads in pools to start running before continuing
* :hpx-pr:`7504` - build(deps): bump github/codeql-action from 4.37.8 to 4.37.9
* :hpx-pr:`7502` - Fix the -Werror=comment build break in reflect_action_overhead
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
* :hpx-pr:`7452` - Remove stale hpx_tag_invoke module dependencies
* :hpx-pr:`7451` - Remove stale hpx_tag_invoke module dependencies
* :hpx-pr:`7450` - tracing: Add work-stealing instrumentation for Tracy profiler
* :hpx-pr:`7449` - Fix pairwise all_to_all build with Clang 17 and OpenMP
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
* :hpx-pr:`7430` - Add size-based pairwise dispatch to all_to_all
* :hpx-pr:`7429` - Bump github/codeql-action from 4.37.3 to 4.37.4
* :hpx-pr:`7427` - Supervision dispatch and target resolution
* :hpx-pr:`7426` - Fix the build with HPX_WITH_THREAD_QUEUE_WAITTIME=ON
* :hpx-pr:`7425` - Adding support for fenced dispatch to supervision component
* :hpx-pr:`7423` - tracing: add causal tracing API for Tracy
* :hpx-pr:`7422` - Bump github/codeql-action from 4 to 4.37.3
* :hpx-pr:`7420` - 🔧 CodeRabbit CI Fix: Fix ARM SVE CI build failures

* :hpx-pr:`7419` - Advertise the parcelport port that was actually bound
* :hpx-pr:`7418` - actions_base: annotation-driven action registration via C++26 reflection
* :hpx-pr:`7417` - components: add hpx::launch::sync overload to reflect_client
* :hpx-pr:`7416` - Bump lukka/get-cmake from 4.3.4 to 4.4.1
* :hpx-pr:`7415` - Bump actions/stale from 10 to 11
* :hpx-pr:`7414` - Don't disabled first core placement if --hpx:bind=none
* :hpx-pr:`7413` - Adding supervision dispatch component
* :hpx-pr:`7412` - Give the launched locality the endpoint the probe actually bound
* :hpx-pr:`7409` - Implement check_admission in supervision module
* :hpx-pr:`7408` - Adding a set of tests for pool_timer
* :hpx-pr:`7405` - Let the OS pick the parcelport port in the departed locality test
* :hpx-pr:`7404` -  Make MPI parcelport teardown orderly: quiesce before MPI_Finalize
* :hpx-pr:`7403` - Tracing inline future continuations
* :hpx-pr:`7402` - Feat/distributed adaptors
* :hpx-pr:`7401` - Cache the first collective finalizer failure and rethrow it for every site
* :hpx-pr:`7399` - Add supervision infrastructure for lifecycle events and observers
* :hpx-pr:`7398` - Move the all_reduce reduction seed instead of copying it
* :hpx-pr:`7396` - Move distributed sender&receiver facilities to separate module
* :hpx-pr:`7395` - remove tag_invoke from container algorithms
* :hpx-pr:`7394` - execution: fix sync_wait notify lifetime race
* :hpx-pr:`7392` - Tracing data parallel workloads
* :hpx-pr:`7391` - Fix doxygen warnings when building docs
* :hpx-pr:`7388` - Fix resolve_locality error handling for departed localities
* :hpx-pr:`7387` - Don't apply PU offset for localities that explicitly use core bindings
* :hpx-pr:`7386` - Bump lukka/get-cmake from 4.3.4 to 4.4.0
* :hpx-pr:`7385` - components: add reflect_client<^^Server> and HPX_REFLECT_CLIENT macro
* :hpx-pr:`7381` - Implement projection support for unique and unique_copy algorithms
* :hpx-pr:`7380` - remove tag invoke from parallel algorithms
* :hpx-pr:`7379` - Thread data size reductions
* :hpx-pr:`7378` - Move communicator basenames into owned storage
* :hpx-pr:`7377` - Flatten hierarchical all-to-all exchange payloads
* :hpx-pr:`7376` - async: add hpx::async<^^func>(target, ...) reflection overload
* :hpx-pr:`7375` - Flatten hierarchical gather and scatter payloads
* :hpx-pr:`7374` - async_cuda: preserve transform_stream environment
* :hpx-pr:`7373` - async_cuda: preserve transform_stream environment
* :hpx-pr:`7372` - Tracing suspend resume hooks
* :hpx-pr:`7371` - Working around Clang ICE happening when compiling the algorithm sender tests
* :hpx-pr:`7370` - Flatten hierarchical collective payloads
* :hpx-pr:`7369` - Reject explicit generations after auto generation on the same communicator
* :hpx-pr:`7368` - remove tag_invoke from core execution
* :hpx-pr:`7367` - Fixing future::wait_until (and wait_for) to return once future was made ready
* :hpx-pr:`7366` - Adding unit tests for and_gate
* :hpx-pr:`7365` - add sender/receiver support to set_intersection
* :hpx-pr:`7364` - Avoid communicator reuse between scan test phases
* :hpx-pr:`7363` - Implement minmax_element to use less comparisons
* :hpx-pr:`7361` - add pre commit hooks
* :hpx-pr:`7359` - Harden collectives validation paths
* :hpx-pr:`7357` - Tracing runtime instrumentation
* :hpx-pr:`7356` - Allowing for a test to fail only for a given configuration
* :hpx-pr:`7355` - keep future bridge redesign
* :hpx-pr:`7354` - Feat/distributed transfer
* :hpx-pr:`7353` -  remove tag_invoke usage
* :hpx-pr:`7352` - ci: add actions_base reflection tests to CI workflows
* :hpx-pr:`7351` - CodeRabbit Generated Unit Tests: Add unit tests for PR changes
* :hpx-pr:`7350` - Introduces better lifetime management for background/pool threads
* :hpx-pr:`7349` - actions_base: add reflect_direct_action and direct action macro reflection support
* :hpx-pr:`7348` - docs: add C++26 reflection-based action migration guide
* :hpx-pr:`7347` - Add tracing support for task lifecycle events
* :hpx-pr:`7346` - feat(algorithms): implement P2300 allocator support for when_all_vector
* :hpx-pr:`7345` - Refactoring serializing parcels
* :hpx-pr:`7344` - add sender/receiver support to set_difference
* :hpx-pr:`7343` - Add hierarchical scan collectives
* :hpx-pr:`7342` - actions_base: fix reflect_action for noexcept functions, add arity
* :hpx-pr:`7341` - async_mpi: modernize transform_mpi sender participation
* :hpx-pr:`7339` - Fixing various test failures.
* :hpx-pr:`7338` - examples: migrate spell_check examples to HPX_ACTION (C++26 reflection)
* :hpx-pr:`7336` - removed unnecessary thread id  info in merge
* :hpx-pr:`7335` - feat(executors): implement domain-optimized continues_on for thread_p…
* :hpx-pr:`7334` - Fixing shared_ptr serialization vulnerabilities
* :hpx-pr:`7332` - actions_base: add compile-time benchmark for macro vs reflect_action
* :hpx-pr:`7331` - async_mpi: use member async_execute on mpi executor
* :hpx-pr:`7330` - async_cuda: use member dispatch for executor entry points
* :hpx-pr:`7329` - build(deps): bump lukka/get-cmake from 4.3.3 to 4.3.4
* :hpx-pr:`7328` - fix(datapar): guard mismatch SIMD path on zip_iterator compatibility
* :hpx-pr:`7327` - build(deps): bump actions/checkout from 6 to 7
* :hpx-pr:`7326` - Hierarchical collectives: unify the per-call generation step for cross-collective sharing
* :hpx-pr:`7325` - Docs: Add troubleshooting note for local runs
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
* :hpx-pr:`7310` - execution_base: trim leftover trait glue
* :hpx-pr:`7308` - executors: Add bulk_chunked and bulk_unchunked member functions to executor_scheduler
* :hpx-pr:`7307` - Add hierarchical all_to_all collective
* :hpx-pr:`7306` - Fix warnings and errors when building docs
* :hpx-pr:`7305` - algorithms: unify projection overloads in min/max/minmax_element and is_heap
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
* :hpx-pr:`7272` - execution: Add scheduler-aware overload to as_sender() for P2300 pipe…
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
* :hpx-pr:`7254` - Re-enable Clang executor tests
* :hpx-pr:`7253` - Add parallel distributed algorithms for segmented copy
* :hpx-pr:`7252` - Execution: Migrate `transform_completion_signatures` to avoid deprecation warnings (#7251)
* :hpx-pr:`7250` - Unified tracing API: merge ITT/Tracy instrumentation
* :hpx-pr:`7249` - ci: pin VS 2022 workflows to windows-2022 runner
* :hpx-pr:`7248` - make_future: static_assert against silent-hang run_loop_scheduler sender
* :hpx-pr:`7247` - make_future: isolate __loop\_ private-member access in single detail helper
* :hpx-pr:`7246` - fix #7245: switch Stdexec FetchContent_Populate to FetchContent_MakeA vailable
* :hpx-pr:`7244` - drop dead trait, dead test guards, and migrate async_mpi receiver to native P2300
* :hpx-pr:`7243` - Implement P2300 stopped_as_optional sender adapter
* :hpx-pr:`7242` - Update HPX URL for perftest commenting
* :hpx-pr:`7241` - Implement P2300 ensure_started algorithm
* :hpx-pr:`7240` - Implement P2300 bulk adapter for HPX executors
* :hpx-pr:`7239` - Implement P2300 get_scheduler bridge for parallel_executor
* :hpx-pr:`7238` - Implement P2300 get_scheduler bridge for executors
* :hpx-pr:`7237` - Adapting HPX actions module to C++20 modules
* :hpx-pr:`7236` - fix(execution): correct sends_stopped in bulk and schedule_from completion signatures
* :hpx-pr:`7235` - Silence Codacy warnings
* :hpx-pr:`7234` - Bump lukka/get-cmake from 4.3.0 to 4.3.2
* :hpx-pr:`7233` - Suppress MSVC linker warnings in C++ 20 module mode
* :hpx-pr:`7232` - ci: switch to lukka/get-cmake to resolve GitHub API rate limits
* :hpx-pr:`7231` - Bump dawidd6/action-download-artifact from 20 to 21
* :hpx-pr:`7230` - algorithms: expose Proj parameter in hpx::min/max/minmax_element CPOs
* :hpx-pr:`7229` - Use secret token for downloading cmake in github actions
* :hpx-pr:`7227` - Adding const version of some of the looping constructs to support datapar
* :hpx-pr:`7224` - Making sure chunking iterators are detected as random access
* :hpx-pr:`7223` - Fix missing invoke header in for_each_index algorithm
* :hpx-pr:`7222` - Add hierarchical all-to-all design documentation
* :hpx-pr:`7221` - Fixed CPO forwarding issue
* :hpx-pr:`7220` - [parallel] Add projection support to hpx::is_sorted, hpx::is_sorted_until, and hpx::is_partitioned CPOs
* :hpx-pr:`7219` - Fix iteration mismatch in collective tests local timing
* :hpx-pr:`7218` - Multi line warning fix
* :hpx-pr:`7217` - Allow using counting_iterator with datapar policies
* :hpx-pr:`7216` - tracing: add hpx::tracing::set_thread_name abstraction
* :hpx-pr:`7215` - Implement hpx::experimental::for_each_index (P4150R0)
* :hpx-pr:`7213` - Use scanned tree commit in inspect links
* :hpx-pr:`7211` - [parallel] Fix incorrect return types in max_element and minmax_element parallel reduction
* :hpx-pr:`7210` -  Reimplement distributed::barrier on top of collectives infrastructure
* :hpx-pr:`7209` - executors: fix performance regressions
* :hpx-pr:`7208` - [parallel] Fix element_type deduction in minmax algorithms
* :hpx-pr:`7207` - ci: Introduce HPX PR Sentinel (Automated Rule-Based Review & Labeling Bot)
* :hpx-pr:`7206` - Fix C++ modules BMI installation and re-enable external build tests
* :hpx-pr:`7205` - CI: Add manual "Cancel All Workflows" automation via label
* :hpx-pr:`7204` - serialization: add reflection tests for static member function name e…
* :hpx-pr:`7202` - Re-enable external build tests with C++ modules
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
* :hpx-pr:`7165` - execution: fix split scheduler preservation for late subscribers (P2300)
* :hpx-pr:`7164` - pass executor to hpx::dataflow in sort and partial_sort
* :hpx-pr:`7163` - Fix missing early termination in parallel `find_first_of` inner loop
* :hpx-pr:`7162` - partial ordering tests for merge
* :hpx-pr:`7161` - fix(find_first_of): add missing early return and fix datapar iterator dependency
* :hpx-pr:`7160` - Add hierarchical all_reduce and all_gather via reduce+broadcast composition
* :hpx-pr:`7159` - concurrency: harden anchor consistency checks in lockfree::deque
* :hpx-pr:`7157` - execution: add native upon_error and upon_stopped sender adaptors
* :hpx-pr:`7156` - Add segmented is_partitioned
* :hpx-pr:`7155` - docs: add 'Using HPX on Compiler Explorer' manual page
* :hpx-pr:`7154` - ci: add CI workflow validating godbolt-minimal preset
* :hpx-pr:`7153` - Optimize exception safety in zip_iterator by adding missing noexcept
* :hpx-pr:`7152` - ci: restore dropped dynamic workflow exclusions
* :hpx-pr:`7149` - Allow for fork_join_executor to work with stackless HPX threads
* :hpx-pr:`7148` - ci: add concurrency cancel-in-progress to all workflow files
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
* :hpx-pr:`7119` - Improve/spsc queue performance 7112
* :hpx-pr:`7118` - Ci profile
* :hpx-pr:`7116` - Refactor build-and-test workflow into matrix + composite action
* :hpx-pr:`7115` - Cache read/write indices in channel_spsc
* :hpx-pr:`7114` - Fix heap corruption in partial_sort filter() function
* :hpx-pr:`7113` - Fix heap corruption in partial_sort filter() function
* :hpx-pr:`7110` - Implement segmented version of equal algorithm
* :hpx-pr:`7109` - feat: add contract assertions to hpx::optional::operator* and operator->
* :hpx-pr:`7108` - Update reflection_qualified_name_of tests with server/client request …
* :hpx-pr:`7107` - fixed the stable_sort_range test
* :hpx-pr:`7106` - feat(futures): implement C++23 monadic operations
* :hpx-pr:`7105` - refactor: decouple hpx_main and core init from distributed runtime
* :hpx-pr:`7104` - Removing concurrency group from documentation build and push
* :hpx-pr:`7103` - feat: enable transparent hpx::init dispatch for local-only builds
* :hpx-pr:`7102` - Disable pushing generated docs from CircleCI
* :hpx-pr:`7101` - schedulers: add NUMA-distance-aware victim list and NUMA hint routing
* :hpx-pr:`7100` - Removed unused lambda captures
* :hpx-pr:`7098` - refactor: narrow umbrella includes in acquire_future.hpp
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
* :hpx-pr:`7083` - Fix critical collectives correctness issues (hierarchical partitioning, scatter/all_to_all validation, barrier release re-entry)
* :hpx-pr:`7082` - Add Conan package support
* :hpx-pr:`7081` - test: add right-overlap tests for forward uninitialized_relocate CPOs
* :hpx-pr:`7080` - complete all missing P2300R10 algorithms and P3425 receiver inlining
* :hpx-pr:`7079` - Feat/sandbox laboratory
* :hpx-pr:`7078` - examples: add sender_diamond quickstart example
* :hpx-pr:`7076` - add slash command
* :hpx-pr:`7073` - fix: correct overlap detection in parallel uninitialized_relocate CPOs
* :hpx-pr:`7072` - Add segmented is_sorted_until , is_sorted
* :hpx-pr:`7070` - Feat/local convenience header
* :hpx-pr:`7069` - Improve hpx_wrap error message with explicit linker flags for non-CMa…
* :hpx-pr:`7068` - cmake: remove HPX_WITH_CXX20_STD_EXECUTION_POLICES feature test
* :hpx-pr:`7067` - cmake: remove HPX_WITH_CXX20_LAMBDA_CAPTURE feature test
* :hpx-pr:`7066` - cmake: remove HPX_WITH_CXX20_STD_CONSTRUCT_AT feature test
* :hpx-pr:`7065` - cmake: Improve error messages for missing optional dependencies Improve dependency errors
* :hpx-pr:`7064` - tests: add sender/receiver for hpx::copy_if
* :hpx-pr:`7063` - Add godbolt-minimal CMake preset for browser-based compiler environments
* :hpx-pr:`7061` - build(deps): bump jwlawson/actions-setup-cmake from 2.1 to 2.2
* :hpx-pr:`7060` - cmake: remove HPX_WITH_CXX20_STD_RANGES_ITER_SWAP feature test
* :hpx-pr:`7058` - Tracing unified interface
* :hpx-pr:`7057` - Add inspect checks to pre-commit and DCO commit-msg hook
* :hpx-pr:`7056` - Feat: Replaced the old lockfree queue with the moodycamel ConcurrentQ…
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
* :hpx-pr:`7013` - Fix incorrect Doxygen for hpx::search_n: "last" -> "first" subsequence
* :hpx-pr:`7012` - Added Dark mode to the website
* :hpx-pr:`7011` - Cleaning up minor issues that slipped into master
* :hpx-pr:`7010` - use optimised image for reflection ci
* :hpx-pr:`7009` - Add segmented replace & replace_if
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
* :hpx-pr:`6987` - feat(components_base): Enable native C++20 module generation
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
* :hpx-pr:`6972` - Fix data race in concurrent container move operations
* :hpx-pr:`6971` - Fix get_num_items logic in default_distribution_policy 
* :hpx-pr:`6968` - schedulers: fix NUMA hint not validating suspended PUs
* :hpx-pr:`6967` - async_cuda, async_mpi: replace tuple-based keep_alive capture with C+…
* :hpx-pr:`6966` - cmake: remove HPX_WITH_CXX20_STD_ENDIAN feature test
* :hpx-pr:`6965` - executors: replace tuple-based lambda capture with C++20 pack capture
* :hpx-pr:`6964` - cmake: Remove obsolete feature test for CXX20_PERFECT_PACK_CAPTURE
* :hpx-pr:`6962` - Replace BLOCK_SIZE with different name to avoid clashes with Linux-defined macro
* :hpx-pr:`6960` - Attempting to fix sorting tests
* :hpx-pr:`6959` - cmake: Remove obsolete feature test for CXX20_TRIVIAL_VIRTUAL_DESTRUCTOR
* :hpx-pr:`6958` - Re-add iterator traits
* :hpx-pr:`6957` - Implement datapar for move with tests
* :hpx-pr:`6956` - docs: add lifetime warning for create_channel_communicator 
* :hpx-pr:`6955` - fix: correct argument order for `next_or_subrange` and `add_ready_future_idx` functions
* :hpx-pr:`6954` - Fixed the managed to unmanaged change
* :hpx-pr:`6952` - Remove HPX_WITH_CXX20_STD_ENDIAN feature test and config guard
* :hpx-pr:`6951` - Add zero-validation for with_processing_units_count
* :hpx-pr:`6950` - Merge Path Algorithm
* :hpx-pr:`6949` - Bump actions/upload-artifact from 6 to 7
* :hpx-pr:`6948` - implement c++20 modules for full libraries
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
* :hpx-pr:`6932` - feat: expose HPX using C++ Modules (HPX.Core and HPX.Full)
* :hpx-pr:`6930` - Adjusting build system to HPX conventions
* :hpx-pr:`6929` - Do bisection to diagnose performance regression 
* :hpx-pr:`6928` - Replace scan implementation with explicit functions to avoid triggering concept checking
* :hpx-pr:`6927` - Attempt to work around Github API limitations
* :hpx-pr:`6926` - Disable async_cuda test as nvcc fails
* :hpx-pr:`6925` - implement c++20 modules for core libraries
* :hpx-pr:`6924` - shared_mutex : add ignore_while_checking around upgrade_cond notify in unlock_shared()
* :hpx-pr:`6923` - Convert enum message_buffer_append_state to enum class
* :hpx-pr:`6921` - Add workflow_dispatch for local testing and fix PR triggers
* :hpx-pr:`6920` - Fixing various compilation regressions
* :hpx-pr:`6919` -  Fix performance regression in fork_join_executor by implementing missing traits
* :hpx-pr:`6918` - Bump actions/checkout from 4 to 6
* :hpx-pr:`6917` - Bump actions/upload-artifact from 4 to 6
* :hpx-pr:`6916` - Add pull_request triggers to GHA workflows and fixed check-formatting jobs
* :hpx-pr:`6915` - Adding missing module dependencies to the HPX algorithm module
* :hpx-pr:`6914` - Cleaning up recently added debug suffix configuration
* :hpx-pr:`6913` - Add comprehensive subtraction unit tests for gid_type
* :hpx-pr:`6912` - Add comprehensive subtraction unit tests for gid_type
* :hpx-pr:`6911` - Fix split fast-path to preserve completion scheduler semantics
* :hpx-pr:`6910` - Implement C++23 Algorithms: fold_left, fold_right.
* :hpx-pr:`6909` - Refactoring local_priority_queue_scheduler for better cache behavior
* :hpx-pr:`6907` - reduce lock contention in local_priority_queue_scheduler via randomized victim selection
* :hpx-pr:`6903` - Migrate HPX CI from CircleCI to GitHub Actions
* :hpx-pr:`6902` - using standard traits present in iterator library
* :hpx-pr:`6901` - Migrate HPX CI from CircleCI to GitHub Actions
* :hpx-pr:`6900` - shared_mutex: follow-up fix and add regression test 
* :hpx-pr:`6899` - Implemented forwarding_sender_query for sender adaptors
* :hpx-pr:`6898` - Removed the incomplete line in Readme
* :hpx-pr:`6897` - Implement concurrent data structures for Issue #2235
* :hpx-pr:`6894` - Implement find_last, find_last_if, and find_last_if_not algorithms as CPOs
* :hpx-pr:`6893` - fix: decouple hpx_debug from cmake_build_type and allow custom debug postfix
* :hpx-pr:`6891` - Refine unordered_map unit tests and add operator[] size stability check
* :hpx-pr:`6890` - Feature/termination detection timeout
* :hpx-pr:`6889` - Fixing set_affinity_test
* :hpx-pr:`6887` - Fix run loop and connect awaitable
* :hpx-pr:`6886` - Expose global termination detection through new API (#6163)
* :hpx-pr:`6885` - refactor execution parameters and steady clock usage
* :hpx-pr:`6884` - implement fold algorithms acc. to c++26
* :hpx-pr:`6883` - Fix initialization hang when --hpx:threads exceeds --hpx:cores
* :hpx-pr:`6882` - Adjust filewriter for collective benchmark
* :hpx-pr:`6880` - Added SPDX License Identifier
* :hpx-pr:`6879` - Implement parallel hpx::uninitialized_relocate_* algorithms for overlapping ranges
* :hpx-pr:`6876` - Feature/task bench support
* :hpx-pr:`6875` - implement serialization for partition_data in 1d_stencil_5
* :hpx-pr:`6873` - Fix runtime shutdown ordering for collectives on macOS Debug -  Issue #6776
* :hpx-pr:`6872` - Added random repeating numbers in tests and fixed sentinel tests for extrema algorithms
* :hpx-pr:`6871` - fix assertion failure in distributed barrier
* :hpx-pr:`6870` - standardise hpx::generate function signature
* :hpx-pr:`6869` - update relocation definitions to track p2786r13 standard
* :hpx-pr:`6868` - Fixing zip_iterator::operator[]() to agree with gcc 16 std::sort
* :hpx-pr:`6866` - Fix threads hanging indefinitely by acquiring the state mutex before checking for an exclusive lock
* :hpx-pr:`6865` - Fix threads hanging indefinitely by acquiring the state mutex before checking for an exclusive lock
* :hpx-pr:`6864` - [Serialization] [Feature] Add serialization for multi containers and unordered_set
* :hpx-pr:`6863` - Fix partitioned_vector tests timeout by reducing vector dimensions #6500
* :hpx-pr:`6862` - Bump jwlawson/actions-setup-cmake from 2.0 to 2.1
* :hpx-pr:`6861` - implement thread affinity api similar to pthread_setaffinity_np
* :hpx-pr:`6860` - cmake: fix cmake-format issues for ARM architecture support
* :hpx-pr:`6859` - [Serialization] [Feature] Add experimental C++26 reflection support
* :hpx-pr:`6858` - Adding intrinsic support for the Tracy profiler
* :hpx-pr:`6857` - Bump jwlawson/actions-setup-cmake from 2.0 to 2.1
* :hpx-pr:`6856` - Godbolt Boost detection: search for static libs if shared not found
* :hpx-pr:`6855` - Add spin-then-sleep idle policy for local parallelism
* :hpx-pr:`6853` - Remove type alias sender completions support (#6624)
* :hpx-pr:`6852` - Add HPX_EXPORT to plugin factory types to fix MacOS visibility issues
* :hpx-pr:`6851` - added CMakePresets.json with multiple build configurations for HPX development
* :hpx-pr:`6850` - Fix Ninja Multi-Config generator support for pkg-config files
* :hpx-pr:`6849` - Fix ARM build failure by automatically enabling generic context coroutines Fixes #6728
* :hpx-pr:`6848` - Forking and modernizing Boost.iostreams 
* :hpx-pr:`6847` - Add CI infrastructure for cloud deployment (Phase CI)
* :hpx-pr:`6846` - fix(stdexec): added adaption to ensure successful build with stdexec
* :hpx-pr:`6845` - Starting to adapt HPX full modules to C++ modules
* :hpx-pr:`6844` - Fixing cyclic dependency between memory and serialization module
* :hpx-pr:`6843` - Attempting to improve compiler error messages related to tag_invoke
* :hpx-pr:`6841` - rewrite hpx::search_n with standard-conforming API and add searcher-based overloads to hpx::search
* :hpx-pr:`6840` - Fix worker_timed.hpp: type-safe delay using chrono::nanoseconds
* :hpx-pr:`6839` - Use c++20 concepts where possible
* :hpx-pr:`6838` - initialize parallel reduce from init value
* :hpx-pr:`6837` - Removing HPX_HAVE_CXX11_STD_SHARED_PTR_LWG3018 option
* :hpx-pr:`6836` - Removing boost.shared_array
* :hpx-pr:`6835` - Enforce clang-format east-const
* :hpx-pr:`6834` - Feature/hierarchical collectives
* :hpx-pr:`6833` - Add docs for SIMD image blur example
* :hpx-pr:`6832` - Applying minor optimizations and fixes in various places
* :hpx-pr:`6831` - Fix linker error for print_dec<unsigned long> on ARM64
* :hpx-pr:`6830` - Fix numa_allocator test failure on macOS
* :hpx-pr:`6829` - Fix deadlock in for_each_on_main_thread regression test
* :hpx-pr:`6828` - Distribute chunk calculation for hpx::merge using memoizing iterators
* :hpx-pr:`6827` - Implement and integrate sized range concepts for parallel container algorithms
* :hpx-pr:`6826` - Apply partitioner optimizations to all code paths
* :hpx-pr:`6825` - Cleaning up C++ module configuration
* :hpx-pr:`6824` - Use constexpr and hpx::wait_each in future_reduce test 
* :hpx-pr:`6823` - More work on exposing C++ modules
* :hpx-pr:`6822` - Use constexpr and hpx::wait_each in future_reduce test
* :hpx-pr:`6821` - Switch to executing a parallel algorithm to synchronous execution
* :hpx-pr:`6820` - Fix expected failure in contract tests
* :hpx-pr:`6818` - Adapt last HPX modules of the HPX core library to C++ modules
* :hpx-pr:`6817` - Adapting HPX CUDA modules to C++ modules 
* :hpx-pr:`6816` - Use ccache for Rostam CI
* :hpx-pr:`6815` - The new LCW (or MPIx) parcelport
* :hpx-pr:`6814` - Adapting HPX algorithm module to C++ modules
* :hpx-pr:`6813` - Simplify find_package CMake modules
* :hpx-pr:`6812` - Adapting HPX networking base modules to C++ modules
* :hpx-pr:`6811` - Adapting HPX modules of levels 24, 25 and 26 to C++ modules
* :hpx-pr:`6810` - Fix: Allow MPI auto-detection and correct CMakePresets.json
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
* :hpx-pr:`6788` - Contract test
* :hpx-pr:`6787` - Update the LCI parcelport to LCI v2
* :hpx-pr:`6785` - Bump github/codeql-action from 3 to 4
* :hpx-pr:`6784` - Add executors documentation
* :hpx-pr:`6783` - Adapting HPX Modules hash, datastructures, and memory to C++ modules
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
* :hpx-pr:`6764` - Bump actions/checkout from 4 to 5
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
* :hpx-pr:`6750` - Fix missing instantiation of print_dec<unsigned long> in print.cpp
* :hpx-pr:`6749` - Reducing the size of CirclCI tasks
* :hpx-pr:`6748` - Allowing for the fork_join_executor to be used on the main thread
* :hpx-pr:`6746` - Integrate NVIDIA's S/R Bulk implementation into HPX
* :hpx-pr:`6744` - Integrating thrust into HPX 
* :hpx-pr:`6741` - Algorithm tests using partitioner with cleanup
* :hpx-pr:`6739` - Fixing the chunking algorithm for parallel loops to more evenly distribute the workload
* :hpx-pr:`6738` - Add aclocal-1.16 symlink
* :hpx-pr:`6737` - Allowing to invoke parallel algorithms from main thread
* :hpx-pr:`6736` - Fix sr partitioner with cleanup
* :hpx-pr:`6734` - Resolves the redundant use of `resolver_client` in `agas/agas_fwd.hpp`
* :hpx-pr:`6732` - Removing support for C++17
* :hpx-pr:`6727` - Fixed the S/R version of partial_sort with unit test added
* :hpx-pr:`6726` -  run_on_all prueba - Issue #6651
* :hpx-pr:`6725` - Adding Boost 1.88
* :hpx-pr:`6724` - modified existing extrema algo tests
* :hpx-pr:`6723` - Optimize `hpx::merge`
* :hpx-pr:`6720` - Fixed the S/R version of nth_element with unit test added
* :hpx-pr:`6712` - fixes issue 6647
* :hpx-pr:`6711` - Use cpp20 concepts in libs 
* :hpx-pr:`6709` - Transition SR tag_invoke to member functions
* :hpx-pr:`6708` - Assume HPX_HAVE_STDEXEC
* :hpx-pr:`6704` - Simplify code by using NS aliases
* :hpx-pr:`6702` - When_all_vector update
* :hpx-pr:`6700` - Update parallel algorithms to use cpp20 concepts 
* :hpx-pr:`6694` - Updated adjacent difference to use c++20 concepts
* :hpx-pr:`6684` - Add semantic tests for extrema algorithms with repeated values
* :hpx-pr:`6655` - Implement parallel_scheduler in HPX 
* :hpx-pr:`6597` - Updated the image version of config.yml
* :hpx-pr:`6515` - Adding process example
* :hpx-pr:`6465` - Always return outermost thread id
* :hpx-pr:`6428` - Enable testing of new scheduler in CI
* :hpx-pr:`6376` - openshmem parcelport
* :hpx-pr:`6318` - Adding hierarchical operation to index_queue spawning
* :hpx-pr:`6255` - #6224, fold algorithms
* :hpx-pr:`6145` - More various tweaks and minor optimizations
* :hpx-pr:`6053` - Support for new performance tool suite : LIKWID
* :hpx-pr:`5984` - Reimplement distributed::barrier on top of existing collectives infrastructure
* :hpx-pr:`5843` - Remove staged threads, immediately create thread objects
