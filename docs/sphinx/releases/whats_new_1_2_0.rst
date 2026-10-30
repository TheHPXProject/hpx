..
    Copyright (C) 2007-2018 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_1_2_0:

===========================
|hpx| V1.2.0 (Nov 12, 2018)
===========================

General changes
===============

Here are some of the main highlights and changes for this release:

* Thanks to the work of our Google Summer of Code student, Nikunj Gupta, we now
  have a new implementation of ``hpx_main.hpp`` on supported platforms (Linux,
  BSD and MacOS). This is intended to be a less fragile drop-in replacement for
  the old implementation relying on preprocessor macros. The new implementation
  does not require changes if you are using the |cmake|_ or pkg-config. The old
  behaviour can be restored by setting ``HPX_WITH_DYNAMIC_HPX_MAIN=OFF`` during
  |cmake|_ configuration. The implementation on Windows is unchanged.
* We have added functionality to allow passing scheduling hints to our
  schedulers. These will allow us to create executors that for example target a
  specific NUMA domain or allow for |hpx| threads to be pinned to a particular
  worker thread.
* We have significantly improved the performance of our futures implementation
  by making the shared state atomic.
* We have replaced Boostbook by Sphinx for our documentation. This means the
  documentation is easier to navigate with built-in search and table of
  contents. We have also added a quick start section and restructured the
  documentation to be easier to follow for new users.
* We have added a new option to the :option:`--hpx:threads` command line option.
  It is now possible to use ``cores`` to tell |hpx| to only use one worker
  thread per core, unlike the existing option ``all`` which uses one worker
  thread per processing unit (processing unit can be a hyperthread if
  hyperthreads are available). The default value of :option:`--hpx:threads` has
  also been changed to ``cores`` as this leads to better performance in most
  cases.
* All command line options can now be passed alongside configuration options
  when initializing |hpx|. This means that some options that were previously
  only available on the command line can now be set as configuration options.
* HPXMP is a portable, scalable, and flexible application programming interface
  using the OpenMP specification that supports multi-platform shared memory
  multiprocessing programming in C and C++. HPXMP can be enabled within |hpx| by
  setting ``DHPX_WITH_HPXMP=ON`` during |cmake|_ configuration.
* Two new performance counters were added for measuring the time spent doing
  background work. ``/threads/time/background-work-duration`` returns the time
  spent doing background on a given thread or locality, while
  ``/threads/time/background-overhead`` returns the fraction of time spent doing
  background work with respect to the overall time spent running the scheduler.
  The new performance counters are disabled by default and can be turned on by
  setting ``HPX_WITH_BACKGROUND_THREAD_COUNTERS=ON`` during |cmake|_
  configuration.
* The idling behaviour of |hpx| has been tweaked to allow for faster idling.
  This is useful in interactive applications where the |hpx| worker threads may
  not have work all the time. This behaviour can be tweaked and turned off as
  before with ``HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF=OFF`` during |cmake|_
  configuration.
* It is now possible to register callback functions for |hpx| worker thread
  events. Callbacks can be registered for starting and stopping worker threads,
  and for when errors occur.

Breaking changes
================

* The implementation of ``hpx_main.hpp`` has changed. If you are using custom
  Makefiles you will need to make changes. Please see the documentation on
  :ref:`using Makefiles <makefile>` for more details.
* The default value of :option:`--hpx:threads` has changed from ``all`` to
  ``cores``. The new option ``cores`` only starts one worker thread per core.
* We have dropped support for Boost 1.56 and 1.57. The minimal version of Boost
  we now test is 1.58.
* Our ``boost::format``\ -based formatting implementation has been revised and
  replaced with a custom implementation. This changes the formatting syntax and
  requires changes if you are relying on :hpx:func:`hpx::util::format` or
  :hpx:func:`hpx::util::format_to`. The pull request for this change contains
  more information: :hpx-pr:`3266`.
* The following deprecated options have now been completely removed:
  ``HPX_WITH_ASYNC_FUNCTION_COMPATIBILITY``, ``HPX_WITH_LOCAL_DATAFLOW``,
  ``HPX_WITH_GENERIC_EXECUTION_POLICY``,
  ``HPX_WITH_BOOST_CHRONO_COMPATIBILITY``, ``HPX_WITH_EXECUTOR_COMPATIBILITY``,
  ``HPX_WITH_EXECUTION_POLICY_COMPATIBILITY``, and
  ``HPX_WITH_TRANSFORM_REDUCE_COMPATIBILITY``.

Closed issues
=============

* :hpx-issue:`3538` - numa handling incorrect for hwloc 2
* :hpx-issue:`3533` - Cmake version 3.5.1does not work (git ff26b35 2018-11-06)
* :hpx-issue:`3526` - Failed building hpx-1.2.0-rc1 on Ubuntu16.04 x86-64 Virtualbox VM
* :hpx-issue:`3512` - Build on aarch64 fails
* :hpx-issue:`3475` - HPX fails to link if the MPI parcelport is enabled
* :hpx-issue:`3462` - CMake configuration shows a minor and inconsequential failure to create a symlink
* :hpx-issue:`3461` - Compilation Problems with the most recent Clang
* :hpx-issue:`3460` - Deadlock when create_partitioner fails (assertion fails) in debug mode
* :hpx-issue:`3455` - HPX build failing with HWLOC errors on POWER8 with hwloc 1.8
* :hpx-issue:`3438` - HPX no longer builds on IBM POWER8
* :hpx-issue:`3426` - hpx build failed on MacOS
* :hpx-issue:`3424` - CircleCI builds broken for forked repositories
* :hpx-issue:`3422` - Benchmarks in tests.performance.local are not run nightly
* :hpx-issue:`3408` - CMake Targets for HPX
* :hpx-issue:`3399` - processing unit out of bounds
* :hpx-issue:`3395` - Floating point bug in hpx/runtime/threads/policies/scheduler_base.hpp
* :hpx-issue:`3378` - compile error with lcos::communicator
* :hpx-issue:`3376` - Failed to build HPX with APEX using clang
* :hpx-issue:`3366` - Adapted Safe_Object example fails for --hpx:threads > 1
* :hpx-issue:`3360` - Segmentation fault when passing component id as parameter
* :hpx-issue:`3358` - HPX runtime hangs after multiple (~thousands) start-stop sequences
* :hpx-issue:`3352` - Support TCP provider in libfabric ParcelPort
* :hpx-issue:`3342` - undefined reference to __atomic_load_16
* :hpx-issue:`3339` - setting command line options/flags from init cfg is not obvious
* :hpx-issue:`3325` - AGAS migrates components prematurely
* :hpx-issue:`3321` - hpx bad_parameter handling is awful
* :hpx-issue:`3318` - Benchmarks fail to build with C++11
* :hpx-issue:`3304` - hpx::threads::run_as_hpx_thread does not properly handle exceptions
* :hpx-issue:`3300` - Setting pu step or offset results in no threads in default pool
* :hpx-issue:`3297` - Crash with APEX when running Phylanx lra_csv with > 1 thread
* :hpx-issue:`3296` - Building HPX with APEX configuration gives compiler warnings
* :hpx-issue:`3290` - make tests failing at hello_world_component
* :hpx-issue:`3285` - possible compilation error when "using namespace std;" is defined before including "hpx" headers files
* :hpx-issue:`3280` - HPX fails on OSX
* :hpx-issue:`3272` - CircleCI does not upload generated docker image any more
* :hpx-issue:`3270` - Error when compiling CUDA examples
* :hpx-issue:`3267` - ``tests.unit.host_.block_allocator`` fails occasionally
* :hpx-issue:`3264` - Possible move to Sphinx for documentation
* :hpx-issue:`3263` - Documentation improvements
* :hpx-issue:`3259` - ``set_parcel_write_handler`` test fails occasionally
* :hpx-issue:`3258` - Links to source code in documentation are broken
* :hpx-issue:`3247` - Rare ``tests.unit.host_.block_allocator`` test failure on 1.1.0-rc1
* :hpx-issue:`3244` - Slowing down and speeding up an interval_timer
* :hpx-issue:`3215` - Cannot build both tests and examples on MSVC with pseudo-dependencies enabled
* :hpx-issue:`3195` - Unnecessary customization point route causing performance penalty
* :hpx-issue:`3088` - A strange thing in parallel::sort.
* :hpx-issue:`2650` - libfabric support for passive endpoints
* :hpx-issue:`1205` - TSS is broken

Closed pull requests
====================

* :hpx-pr:`3542` - Fix numa lookup from pu when using hwloc 2.x
* :hpx-pr:`3541` - Fixing the build system of the MPI parcelport
* :hpx-pr:`3540` - Updating HPX people section
* :hpx-pr:`3539` - Splitting test to avoid OOM on CircleCI
* :hpx-pr:`3537` - Fix guided exec
* :hpx-pr:`3536` - Updating grants which support the LSU team
* :hpx-pr:`3535` - Fix hiding of docker credentials
* :hpx-pr:`3534` - Fixing #3533
* :hpx-pr:`3532` - fixing minor doc typo --hpx:print-counter-at arg
* :hpx-pr:`3530` - Changing APEX default tag to v2.1.0
* :hpx-pr:`3529` - Remove leftover security options and documentation
* :hpx-pr:`3528` - Fix hwloc version check
* :hpx-pr:`3524` - Do not build guided pool examples with older GCC compilers
* :hpx-pr:`3523` - Fix logging regression
* :hpx-pr:`3522` - Fix more warnings
* :hpx-pr:`3521` - Fixing argument handling in induction and reduction clauses for parallel::for_loop
* :hpx-pr:`3520` - Remove docs symlink and versioned docs folders
* :hpx-pr:`3519` - hpxMP release
* :hpx-pr:`3518` - Change all steps to use new docker image on CircleCI
* :hpx-pr:`3516` - Drop usage of deprecated facilities removed in C++17
* :hpx-pr:`3515` - Remove remaining uses of Boost.TypeTraits
* :hpx-pr:`3513` - Fixing a CMake problem when trying to use libfabric
* :hpx-pr:`3508` - Remove memory_block component
* :hpx-pr:`3507` - Propagating the MPI compile definitions to all relevant targets
* :hpx-pr:`3503` - Update documentation colors and logo
* :hpx-pr:`3502` - Fix bogus \`throws\` bindings in scheduled_thread_pool_impl
* :hpx-pr:`3501` - Split parallel::remove_if tests to avoid OOM on CircleCI
* :hpx-pr:`3500` - Support NONAMEPREFIX in add_hpx_library()
* :hpx-pr:`3497` - Note that cuda support requires cmake 3.9
* :hpx-pr:`3495` - Fixing dataflow
* :hpx-pr:`3493` - Remove deprecated options for 1.2.0 part 2
* :hpx-pr:`3492` - Add CUDA_LINK_LIBRARIES_KEYWORD to allow PRIVATE keyword in linkage t…
* :hpx-pr:`3491` - Changing Base docker image
* :hpx-pr:`3490` - Don't create tasks immediately with hpx::apply
* :hpx-pr:`3489` - Remove deprecated options for 1.2.0
* :hpx-pr:`3488` - Revert "Use BUILD_INTERFACE generator expression to fix cmake flag exports"
* :hpx-pr:`3487` - Revert "Fixing type attribute warning for transfer_action"
* :hpx-pr:`3485` - Use BUILD_INTERFACE generator expression to fix cmake flag exports
* :hpx-pr:`3483` - Fixing type attribute warning for transfer_action
* :hpx-pr:`3481` - Remove unused variables
* :hpx-pr:`3480` - Towards a more lightweight transfer action
* :hpx-pr:`3479` - Fix FLAGS - Use correct version of target_compile_options
* :hpx-pr:`3478` - Making sure the application's exit code is properly propagated back to the OS
* :hpx-pr:`3476` - Don't print docker credentials as part of the environment.
* :hpx-pr:`3473` - Fixing invalid cmake code if no jemalloc prefix was given
* :hpx-pr:`3472` - Attempting to work around recent clang test compilation failures
* :hpx-pr:`3471` - Enable jemalloc on windows
* :hpx-pr:`3470` - Updates readme
* :hpx-pr:`3468` - Avoid hang if there is an exception thrown during startup
* :hpx-pr:`3467` - Add compiler specific fallthrough attributes if C++17 attribute is not available
* :hpx-pr:`3466` - - bugfix : fix compilation with llvm-7.0
* :hpx-pr:`3465` - This patch adds various optimizations extracted from the thread_local_allocator work
* :hpx-pr:`3464` - Check for forked repos in CircleCI docker push step
* :hpx-pr:`3463` - - cmake : create the parent directory before symlinking
* :hpx-pr:`3459` - Remove unused/incomplete functionality from util/logging
* :hpx-pr:`3458` - Fix a problem with scope of CMAKE_CXX_FLAGS and hpx_add_compile_flag
* :hpx-pr:`3457` - Fixing more size_t -> int16_t (and similar) warnings
* :hpx-pr:`3456` - Add #ifdefs to topology.cpp to support old hwloc versions again
* :hpx-pr:`3454` - Fixing warnings related to silent conversion of size_t --> int16_t
* :hpx-pr:`3451` - Add examples as unit tests
* :hpx-pr:`3450` - Constexpr-fying bind and other functional facilities
* :hpx-pr:`3446` - Fix some thread suspension timeouts
* :hpx-pr:`3445` - Fix various warnings
* :hpx-pr:`3443` - Only enable service pool config options if pools are enabled
* :hpx-pr:`3441` - Fix missing closing brackets in documentation
* :hpx-pr:`3439` - Use correct MPI CXX libraries for MPI parcelport
* :hpx-pr:`3436` - Add projection function to find_* (and fix very bad bug)
* :hpx-pr:`3435` - Fixing 1205
* :hpx-pr:`3434` - Fix threads cores
* :hpx-pr:`3433` - Add Heise Online to release announcement list
* :hpx-pr:`3432` - Don't track task dependencies for distributed runs
* :hpx-pr:`3431` - Circle CI setting changes for hpxMP
* :hpx-pr:`3430` - Fix unused params warning
* :hpx-pr:`3429` - One thread per core
* :hpx-pr:`3428` - This suppresses a deprecation warning that is being issued by MSVC 19.15.26726
* :hpx-pr:`3427` - Fixes #3426
* :hpx-pr:`3425` - Use source cache and workspace between job steps on CircleCI
* :hpx-pr:`3421` - Add CDash timing output to future overhead test (for graphs)
* :hpx-pr:`3420` - Add guided_pool_executor
* :hpx-pr:`3419` - Fix typo in CircleCI config
* :hpx-pr:`3418` - Add sphinx documentation
* :hpx-pr:`3415` - Scheduler NUMA hint and shared priority scheduler
* :hpx-pr:`3414` - Adding step to synchronize the APEX release
* :hpx-pr:`3413` - Fixing multiple defines of APEX_HAVE_HPX
* :hpx-pr:`3412` - Fixes linking with libhpx_wrap error with BSD and Windows based systems
* :hpx-pr:`3410` - Fix typo in CMakeLists.txt
* :hpx-pr:`3409` - Fix brackets and indentation in existing_performance_counters.qbk
* :hpx-pr:`3407` - Fix unused param and extra ; warnings emitted by gcc 8.x
* :hpx-pr:`3406` - Adding thread local allocator and use it for future shared states
* :hpx-pr:`3405` - Adding DHPX_HAVE_THREAD_LOCAL_STORAGE=ON to builds
* :hpx-pr:`3404` - fixing multiple definition of main() in linux
* :hpx-pr:`3402` - Allow debug option to be enabled only for Linux systems with dynamic main on
* :hpx-pr:`3401` - Fix cuda_future_helper.h when compiling with C++11
* :hpx-pr:`3400` - Fix floating point exception scheduler_base idle backoff
* :hpx-pr:`3398` - Atomic future state
* :hpx-pr:`3397` - Fixing code for older gcc versions
* :hpx-pr:`3396` - Allowing to register thread event functions (start/stop/error)
* :hpx-pr:`3394` - Fix small mistake in primary_namespace_server.cpp
* :hpx-pr:`3393` - Explicitly instantiate configured schedulers
* :hpx-pr:`3392` - Add performance counters background overhead and background work duration
* :hpx-pr:`3391` - Adapt integration of HPXMP to latest build system changes
* :hpx-pr:`3390` - Make AGAS measurements optional
* :hpx-pr:`3389` - Fix deadlock during shutdown
* :hpx-pr:`3388` - Add several functionalities allowing to optimize synchronous action invocation
* :hpx-pr:`3387` - Add cmake option to opt out of fail-compile tests
* :hpx-pr:`3386` - Adding support for boost::container::small_vector to dataflow
* :hpx-pr:`3385` - Adds Debug option for hpx initializing from main
* :hpx-pr:`3384` - This hopefully fixes two tests that occasionally fail
* :hpx-pr:`3383` - Making sure thread local storage is enable for hpxMP
* :hpx-pr:`3382` - Fix usage of HPX_CAPTURE together with default value capture [=]
* :hpx-pr:`3381` - Replace undefined instantiations of uniform_int_distribution
* :hpx-pr:`3380` - Add missing semicolons to uses of HPX_COMPILER_FENCE
* :hpx-pr:`3379` - Fixing #3378
* :hpx-pr:`3377` - Adding build system support to integrate hpxmp into hpx at the user's machine
* :hpx-pr:`3375` - Replacing wrapper for __libc_start_main with main
* :hpx-pr:`3374` - Adds hpx_wrap to HPX_LINK_LIBRARIES which links only when specified.
* :hpx-pr:`3373` - Forcing cache settings in HPXConfig.cmake to guarantee updated values
* :hpx-pr:`3372` - Fix some more c++11 build problems
* :hpx-pr:`3371` - Adds HPX_LINKER_FLAGS to HPX applications without editing their source codes
* :hpx-pr:`3370` - util::format: add type_specifier<> specializations for %!s(MISSING) and %!l(MISSING)s
* :hpx-pr:`3369` - Adding configuration option to allow explicit disable of the new hpx_main feature on Linux
* :hpx-pr:`3368` - Updates doc with recent hpx_wrap implementation
* :hpx-pr:`3367` - Adds Mac OS implementation to hpx_main.hpp
* :hpx-pr:`3365` - Fix order of hpx libs in HPX_CONF_LIBRARIES.
* :hpx-pr:`3363` - Apex fixing null wrapper
* :hpx-pr:`3361` - Making sure all parcels get destroyed on an HPX thread (TCP pp)
* :hpx-pr:`3359` - Feature/improveerrorforcompiler
* :hpx-pr:`3357` - Static/dynamic executable implementation
* :hpx-pr:`3355` - Reverting changes introduced by #3283 as those make applications hang
* :hpx-pr:`3354` - Add external dependencies to HPX_LIBRARY_DIR
* :hpx-pr:`3353` - Fix libfabric tcp
* :hpx-pr:`3351` - Move obsolete header to tests directory.
* :hpx-pr:`3350` - Renaming two functions to avoid problem described in #3285
* :hpx-pr:`3349` - Make idle backoff exponential with maximum sleep time
* :hpx-pr:`3347` - Replace `simple_component*` with `component*` in the Documentation
* :hpx-pr:`3346` - Fix CMakeLists.txt example in quick start
* :hpx-pr:`3345` - Fix automatic setting of HPX_MORE_THAN_64_THREADS
* :hpx-pr:`3344` - Reduce amount of information printed for unknown command line options
* :hpx-pr:`3343` - Safeguard HPX against destruction in global contexts
* :hpx-pr:`3341` - Allowing for all command line options to be used as configuration settings
* :hpx-pr:`3340` - Always convert inspect results to JUnit XML
* :hpx-pr:`3336` - Only run docker push on master on CircleCI
* :hpx-pr:`3335` - Update description of hpx.os_threads config parameter.
* :hpx-pr:`3334` - Making sure early logging settings don't get mixed with others
* :hpx-pr:`3333` - Update CMake links and versions in documentation
* :hpx-pr:`3332` - Add notes on target suffixes to CMake documentation
* :hpx-pr:`3331` - Add quickstart section to documentation
* :hpx-pr:`3330` - Rename resource_partitioner test to avoid conflicts with pseudodependencies
* :hpx-pr:`3328` - Making sure object is pinned while executing actions, even if action returns a future
* :hpx-pr:`3327` - Add missing std::forward to tuple.hpp
* :hpx-pr:`3326` - Make sure logging is up and running while modules are being discovered.
* :hpx-pr:`3324` - Replace C++14 overload of std::equal with C++11 code.
* :hpx-pr:`3323` - Fix a missing apex thread data (wrapper) initialization
* :hpx-pr:`3320` - Adding support for -std=c++2a (define `HPX_WITH_CXX2A=On`)
* :hpx-pr:`3319` - Replacing C++14 feature with equivalent C++11 code
* :hpx-pr:`3317` - Fix compilation with VS 15.7.1 and /std:c++latest
* :hpx-pr:`3316` - Fix includes for 1d_stencil_*_omp examples
* :hpx-pr:`3314` - Remove some unused parameter warnings
* :hpx-pr:`3313` - Fix pu-step and pu-offset command line options
* :hpx-pr:`3312` - Add conversion of inspect reports to JUnit XML
* :hpx-pr:`3311` - Fix escaping of closing braces in format specification syntax
* :hpx-pr:`3310` - Don't overwrite user settings with defaults in registration database
* :hpx-pr:`3309` - Fixing potential stack overflow for dataflow
* :hpx-pr:`3308` - This updates the .clang-format configuration file to utilize newer features
* :hpx-pr:`3306` - Marking migratable objects in their gid to allow not handling migration in AGAS
* :hpx-pr:`3305` - Add proper exception handling to run_as_hpx_thread
* :hpx-pr:`3303` - Changed std::rand to a better inbuilt PRNG Generator
* :hpx-pr:`3302` - All non-migratable (simple) components now encode their lva and component type in their gid
* :hpx-pr:`3301` - Add nullptr_t overloads to resource partitioner
* :hpx-pr:`3298` - Apex task wrapper memory bug
* :hpx-pr:`3295` - Fix mistakes after merge of CircleCI config
* :hpx-pr:`3294` - Fix partitioned vector include in partitioned_vector_find tests
* :hpx-pr:`3293` - Adding emplace support to promise and make_ready_future
* :hpx-pr:`3292` - Add new cuda kernel synchronization with hpx::future demo
* :hpx-pr:`3291` - Fixes #3290
* :hpx-pr:`3289` - Fixing Docker image creation
* :hpx-pr:`3288` - Avoid allocating shared state for wait_all
* :hpx-pr:`3287` - Fixing /scheduler/utilization/instantaneous performance counter
* :hpx-pr:`3286` - dataflow() and future::then() use sync policy where possible
* :hpx-pr:`3284` - Background thread can use relaxed atomics to manipulate thread state
* :hpx-pr:`3283` - Do not unwrap ready future
* :hpx-pr:`3282` - Fix virtual method override warnings in static schedulers
* :hpx-pr:`3281` - Disable set_area_membind_nodeset for OSX
* :hpx-pr:`3279` - Add two variations to the future_overhead benchmark
* :hpx-pr:`3278` - Fix circleci workspace
* :hpx-pr:`3277` - Support external plugins
* :hpx-pr:`3276` - Fix missing parenthesis in hello_compute.cu.
* :hpx-pr:`3274` - Reinit counters synchronously in reinit_counters test
* :hpx-pr:`3273` - Splitting tests to avoid compiler OOM
* :hpx-pr:`3271` - Remove leftover code from context_generic_context.hpp
* :hpx-pr:`3269` - Fix bulk_construct with count = 0
* :hpx-pr:`3268` - Replace constexpr with HPX_CXX14_CONSTEXPR and HPX_CONSTEXPR
* :hpx-pr:`3266` - Replace boost::format with custom sprintf-based implementation
* :hpx-pr:`3265` - Split parallel tests on CircleCI
* :hpx-pr:`3262` - Making sure documentation correctly links to source files
* :hpx-pr:`3261` - Apex refactoring fix rebind
* :hpx-pr:`3260` - Isolate performance counter parser into a separate TU
* :hpx-pr:`3256` - Post 1.1.0 version bumps
* :hpx-pr:`3254` - Adding trait for actions allowing to make runtime decision on whether to execute it directly
* :hpx-pr:`3253` - Bump minimal supported Boost to 1.58.0
* :hpx-pr:`3251` - Adds new feature: changing interval used in interval_timer (issue 3244)
* :hpx-pr:`3239` - Changing std::rand() to a better inbuilt PRNG generator.
* :hpx-pr:`3234` - Disable background thread when networking is off
* :hpx-pr:`3232` - Clean up suspension tests
* :hpx-pr:`3230` - Add optional scheduler mode parameter to create_thread_pool function
* :hpx-pr:`3228` - Allow suspension also on static schedulers
* :hpx-pr:`3163` - libfabric parcelport w/o HPX_PARCELPORT_LIBFABRIC_ENDPOINT_RDM
* :hpx-pr:`3036` - Switching to CircleCI 2.0
