..
    Copyright (C) 2007-2019 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_1_4_0:

===============================
|hpx| V1.4.0 (January 15, 2020)
===============================

General changes
===============

* We have added the collectives ``all_to_all`` and ``all_reduce``.
* We have added APIs for resiliency, which allows replication and replay for
  failed tasks. See the :ref:`documentation <modules_resiliency>` for more details.
* Components can now be checkpointed.
* Performance improvements to schedulers and coroutines. A significant change is
  the addition of stackless coroutines. These are to be used for tasks that do
  not need to be suspended and can reduce overheads noticeably in applications
  with short tasks. A stackless coroutine can be created with the new stack size
  ``thread_stacksize_nostack``.
* We have added an implementation of ``unique_any``, which is a non-copyable
  version of ``any``.
* The ``shared_priority_queue_scheduler`` has been improved. It now has lower
  overheads than the default scheduler in many situations. Unlike the default
  scheduler it fully supports NUMA scheduling hints. Enable it with the command
  line option :option:`--hpx:queuing`\ ``=shared-priority``. This scheduler
  should still be considered experimental, but its use is encouraged in real
  applications to help us make it production ready.
* We have added the performance counters ``background-receive-duration`` and
  ``background-receive-overhead`` for inspecting the time and overhead spent on
  receiving parcels in the background.
* Compilation time has been further improved when ``HPX_WITH_NETWORKING=OFF``.
* We no longer require compiled Boost dependencies in certain configurations.
  This requires at least Boost 1.70, compiling on x86 with GCC 9, clang (libc++)
  9, or VS2019 in C++17 mode. The dependency on Boost.Filesystem can explicitly
  be turned on with ``HPX_FILESYSTEM_WITH_BOOST_FILESYSTEM_COMPATIBILITY=ON``
  (it is off by default if the standard library supports ``std::filesystem``).
  Boost.ProgramOptions has been copied into the HPX repository. We have a
  compatibility layer for users who must explicitly use Boost.ProgramOptions
  instead of the ProgramOptions provided by HPX. To remove the dependency
  ``HPX_PROGRAM_OPTIONS_WITH_BOOST_PROGRAM_OPTIONS_COMPATIBILITY`` must be
  explicitly set to ``OFF``. This option will be removed in a future release. We
  have also removed several other header-only dependencies on Boost.
* It is now possible to use the process affinity mask set by tools like
  ``numactl`` and various batch environments with the command line option
  :option:`--hpx:use-process-mask`. Enabling this option implies
  :option:`--hpx:ignore-batch-env`.
* It is now possible to create standalone thread pools without starting the
  runtime. See the ``standalone_thread_pool_executor.cpp`` test in the
  ``execution`` module for an example.
* Tasks annotated with :hpx:func:`hpx::util::annotated_function` now have their
  correct name when using APEX to generate OTF2 files.
* Cloning of APEX was defective in previous releases (it required manual
  intervention to check out the correct tag or branch). This has been fixed.
* The option ``HPX_WITH_MORE_THAN_64_THREADS`` is now ignored and will be
  removed in a future release. The value is instead derived directly from
  ``HPX_WITH_MAX_CPU_COUNT`` option.
* We have deprecated compiling in C++11 mode. The next release will require a
  C++14 capable compiler.
* We have deprecated support for the Vc library. This option will be replaced
  with SIMD support from the standard library in a future release.
* We have significantly refactored our CMake setup. This is intended to be a
  non-breaking change and will allow for using HPX through CMake targets in the
  future.
* We have continued modularizing the HPX library. In the process we have
  rearranged many header files into module-specific directories. All moved
  headers have compatibility headers which forward from the old location to the
  new location, together with a deprecation warning. The compatibility headers
  will eventually be removed.
* We now enforce formatting with ``clang-format`` on the majority of our source
  files.
* We have added SPDX license tags to all files.
* Many bugfixes.

Breaking changes
================

* The ``HPX_WITH_THREAD_COMPATIBILITY`` option and the associated compatibility
  layer has been removed.
* The ``HPX_WITH_INCLUSIVE_SCAN_COMPATIBILITY`` option and the associated
  compatibility layer has been removed.
* The ``HPX_WITH_UNWRAPPED_COMPATIBLITY`` option and the associated
  compatibility layer has been removed.

Closed issues
=============

* :hpx-issue:`4282` - Build Issues with Release on Windows
* :hpx-issue:`4278` - Build Issues with CMake 3.14.4
* :hpx-issue:`4273` - Clients of HPX 1.4.0-rc2 with APEX ar not linked to
  libhpx-apex
* :hpx-issue:`4269` - Building HPX 1.4.0-rc2 with support for APEX fails
* :hpx-issue:`4263` - Compilation fail on latest master
* :hpx-issue:`4232` - Configure of HPX project using CMake FetchContent fails
* :hpx-issue:`4223` - "Re-using the main() function as the main HPX entry point"
  doesn't work
* :hpx-issue:`4220` - HPX won't compile - error building
  ``resource_partitioner``
* :hpx-issue:`4215` - HPX 1.4.0rc1 does not link on s390x
* :hpx-issue:`4204` - Trouble compiling HPX with Intel compiler
* :hpx-issue:`4199` - Refactor APEX to eliminate circular dependency
* :hpx-issue:`4187` - HPX can't build on OSX
* :hpx-issue:`4185` - Simple debug output for development
* :hpx-issue:`4182` - ``@HPX_CONF_PREFIX@`` is the empty string
* :hpx-issue:`4169` - HPX won't build with APEX
* :hpx-issue:`4163` - Add back ``HPX_LIBRARIES`` and ``HPX_INCLUDE_DIRS``
* :hpx-issue:`4161` - It should be possible to call ``find_package(HPX)``
  multiple times
* :hpx-issue:`4155` - ``get_self_id()`` for stackless threads returns
  ``invalid_thread_id``
* :hpx-issue:`4151` - build error with MPI code
* :hpx-issue:`4150` - hpx won't build on POWER9 with clang 8
* :hpx-issue:`4148` - ``cacheline_data`` delivers poor performance with C++17
  compared to C++14
* :hpx-issue:`4144` - target general in ``HPX_LIBRARIES`` does not exist
* :hpx-issue:`4134` - CMake Error when ``-DHPX_WITH_HPXMP=ON``
* :hpx-issue:`4132` - parallel fill leaves elements unfilled
* :hpx-issue:`4123` - PAPI performance counters are inaccessible
* :hpx-issue:`4118` - ``static_chunk_size`` is not obeyed in scan algorithms
* :hpx-issue:`4115` - dependency chaining error with APEX
* :hpx-issue:`4107` - Initializing runtime without entry point function and
  command line arguments
* :hpx-issue:`4105` - Bug in ``hpx:bind=numa-balanced``
* :hpx-issue:`4101` - Bound tasks
* :hpx-issue:`4100` - Add SPDX identifier to all files
* :hpx-issue:`4085` - ``hpx_topology`` library should depend on hwloc
* :hpx-issue:`4067` - HPX fails to build on macOS
* :hpx-issue:`4056` - Building without thread manager idle backoff fails
* :hpx-issue:`4052` - Enforce ``clang-format`` style for modules
* :hpx-issue:`4032` - Simple hello world fails to launch correctly
* :hpx-issue:`4030` - Allow threads to skip context switching
* :hpx-issue:`4029` - Add support for mimalloc
* :hpx-issue:`4005` - Can't link HPX when APEX enabled
* :hpx-issue:`4002` - Missing header for algorithm module
* :hpx-issue:`3989` - conversion from ``long`` to ``unsigned int`` requires a
  narrowing conversion on MSVC
* :hpx-issue:`3958` - ``/statistics/average@`` perf counter can't be created
* :hpx-issue:`3953` - CMake errors from ``HPX_AddPseudoDependencies``
* :hpx-issue:`3941` - CMake error for APEX install target
* :hpx-issue:`3940` - Convert pseudo-doxygen function documentation into actual
  doxygen documentation
* :hpx-issue:`3935` - HPX compiler match too strict?
* :hpx-issue:`3929` - Buildbot failures on latest HPX stable
* :hpx-issue:`3912` - I recommend publishing a version that does not depend on
  the boost library
* :hpx-issue:`3890` - ``hpx.ini`` not working
* :hpx-issue:`3883` - cuda compilation fails because of ``-faligned-new``
* :hpx-issue:`3879` - HPX fails to configure with ``-DHPX_WITH_TESTS=OFF``
* :hpx-issue:`3871` - ``dataflow`` does not support void allocators
* :hpx-issue:`3867` - Latest HTML docs placed in wrong directory on GitHub pages
* :hpx-issue:`3866` - Make sure all tests use ``HPX_TEST*`` macros and not
  ``HPX_ASSERT``
* :hpx-issue:`3857` - CMake all-keyword or all-plain for
  ``target_link_libraries``
* :hpx-issue:`3856` - ``hpx_setup_target`` adds rogue flags
* :hpx-issue:`3850` - HPX fails to build on POWER8 with Clang7
* :hpx-issue:`3848` - Remove ``lva`` member from ``thread_init_data``
* :hpx-issue:`3838` - ``hpx::parallel::count/count_if`` failing tests
* :hpx-issue:`3651` - ``hpx::parallel::transform_reduce`` with non const
  reference as lambda parameter
* :hpx-issue:`3560` - Apex integration with HPX not working properly
* :hpx-issue:`3322` - No warning when mixing debug/release builds

Closed pull requests
====================

* :hpx-pr:`4300` - Checks for ``MPI_Init`` being called twice
* :hpx-pr:`4299` - Small CMake fixes
* :hpx-pr:`4298` - Remove extra call to annotate function that messes up traces
* :hpx-pr:`4296` - Fixing collectives locking problem
* :hpx-pr:`4295` - Do not check ``LICENSE_1_0.txt`` for inspect violations
* :hpx-pr:`4293` - Applying two small changes fixing carious MSVC/Windows
  problems
* :hpx-pr:`4285` - Delete ``apex.hpp``
* :hpx-pr:`4276` - Disable doxygen generation for ``hpx/debugging/print.hpp``
  file
* :hpx-pr:`4275` - Make sure APEX is linked to even when not explicitly
  referenced
* :hpx-pr:`4272` - Fix pushing of documentation
* :hpx-pr:`4271` - Updating APEX tag, don't create new task_wrapper on
  ``operator=`` of hpx_thread object
* :hpx-pr:`4268` - Testing for noexcept function specializations in C++11/14
  mode
* :hpx-pr:`4267` - Fixing MSVC warning
* :hpx-pr:`4266` - Make sure macOS Travis CI fails if build step fails
* :hpx-pr:`4264` - Clean up compatibility header options
* :hpx-pr:`4262` - Cleanup modules ``CMakeLists.txt``
* :hpx-pr:`4261` - Fixing HPX/APEX linking and dependencies for external
  projects like Phylanx
* :hpx-pr:`4260` - Fix docs compilation problems
* :hpx-pr:`4258` - Couple of minor changes
* :hpx-pr:`4257` - Fix apex annotation for async dispatch
* :hpx-pr:`4256` - Remove lambdas from assert expressions
* :hpx-pr:`4255` - Ignoring lock in ``all_to_all`` and ``all_reduce``
* :hpx-pr:`4254` - Adding action specializations for noexcept functions
* :hpx-pr:`4253` - Move ``partlit.hpp`` to affinity module
* :hpx-pr:`4252` - Make mismatching build types a hard error in CMake
* :hpx-pr:`4249` - Scheduler improvement
* :hpx-pr:`4248` - update hpxmp tag to v0.3.0
* :hpx-pr:`4245` - Adding high performance channels
* :hpx-pr:`4244` - Ignore lock in ignore_while_locked_1485 test
* :hpx-pr:`4243` - Fix PAPI command line option documentation
* :hpx-pr:`4242` - Ignore lock in target_distribution_policy
* :hpx-pr:`4241` - Fix ``start_stop_callbacks`` test
* :hpx-pr:`4240` - Mostly fix clang CUDA compilation
* :hpx-pr:`4238` - Google Season of Docs updates to documentation; grammar
  edits.
* :hpx-pr:`4237` - fixing annotated task to use the name, not the desc
* :hpx-pr:`4236` - Move module print summary to modules
* :hpx-pr:`4235` - Don't use alignas in ``cache_{aligned,line}_data``
* :hpx-pr:`4234` - Add basic overview sentence to all modules
* :hpx-pr:`4230` - Add OS X builds to Travis CI
* :hpx-pr:`4229` - Remove leftover queue compatibility checks
* :hpx-pr:`4226` - Fixing APEX shutdown by explicitly shutting down throttling
* :hpx-pr:`4225` - Allow ``CMAKE_INSTALL_PREFIX`` to be a relative path
* :hpx-pr:`4224` - Deprecate verbs parcelport
* :hpx-pr:`4222` - Update ``register_{thread,work}`` namespaces
* :hpx-pr:`4221` - Changing ``HPX_GCC_VERSION`` check from ``70000`` to
  ``70300``
* :hpx-pr:`4218` - Google Season of Docs updates to documentation; grammar
  edits.
* :hpx-pr:`4217` - Google Season of Docs updates to documentation; grammar
  edits.
* :hpx-pr:`4216` - Fixing gcc warning on 32bit platforms (integer truncation)
* :hpx-pr:`4214` - Apex callback refactoring
* :hpx-pr:`4213` - Clean up allocator checks for dependent projects
* :hpx-pr:`4212` - Google Season of Docs updates to documentation; grammar
  edits.
* :hpx-pr:`4211` - Google Season of Docs updates to documentation; contributing
  to hpx
* :hpx-pr:`4210` - Attempting to fix Intel compilation
* :hpx-pr:`4209` - Fix CUDA 10 build
* :hpx-pr:`4205` - Making sure that differences in ``CMAKE_BUILD_TYPE`` are not
  reported on multi-configuration cmake generators
* :hpx-pr:`4203` - Deprecate Vc
* :hpx-pr:`4202` - Fix CUDA configuration
* :hpx-pr:`4200` - Making sure ``hpx_wrap`` is not passed on to linker on
  non-Linux systems
* :hpx-pr:`4198` - Fix ``execution_agent.cpp`` compilation with GCC 5
* :hpx-pr:`4197` - Remove deprecated options for 1.4.0 release
* :hpx-pr:`4196` - minor fixes for building on OSX Darwin
* :hpx-pr:`4195` - Use full clone on CircleCI for pushing stable tag
* :hpx-pr:`4193` - Add scheduling hints to hello_world_distributed
* :hpx-pr:`4192` - Set up CUDA in HPXConfig.cmake
* :hpx-pr:`4191` - Export allocators root variables
* :hpx-pr:`4190` - Don't use ``constexpr`` in ``thread_data`` with GCC <= 6
* :hpx-pr:`4189` - Only use ``quick_exit`` if available
* :hpx-pr:`4188` - Google Season of Docs updates to documentation; writing
  single node hpx applications
* :hpx-pr:`4186` - correct vc to cuda in cuda cmake
* :hpx-pr:`4184` - Resetting some cached variables to make sure those are
  re-filled
* :hpx-pr:`4183` - Fix ``hpxcxx`` configuration
* :hpx-pr:`4181` - Rename base libraries var
* :hpx-pr:`4180` - Move header left behind earlier to plugin module
* :hpx-pr:`4179` - Moving ``zip_iterator`` and ``transform_iterator`` to
  iterator_support module
* :hpx-pr:`4178` - Move checkpointing support to its own module
* :hpx-pr:`4177` - Small const fix to ``basic_execution`` module
* :hpx-pr:`4176` - Add back ``HPX_LIBRARIES`` and friends to ``HPXConfig.cmake``
* :hpx-pr:`4175` - Make Vc public and add it to ``HPXConfig.cmake``
* :hpx-pr:`4173` - Wait for runtime to be running before returning from
  hpx::start
* :hpx-pr:`4172` - More protection against shutdown problems in error handling
  scenarios.
* :hpx-pr:`4171` - Ignore lock in ``condition_variable::wait``
* :hpx-pr:`4170` - Adding APEX dependency to MPI parcelport
* :hpx-pr:`4168` - Adding utility include
* :hpx-pr:`4167` - Add a condition to setup the external libraries
* :hpx-pr:`4166` - Add an ``INTERNAL_FLAGS`` option to link to
  ``hpx_internal_flags``
* :hpx-pr:`4165` - Forward ``HPX_*`` cmake cache variables to external projects
* :hpx-pr:`4164` - Affinity and batch environment modules
* :hpx-pr:`4162` - Handle ``quick exit``
* :hpx-pr:`4160` - Using ``target_link_libraries`` for cmake versions >= 3.12
* :hpx-pr:`4159` - Make sure ``HPX_WITH_NATIVE_TLS`` is forwarded to dependent
  projects
* :hpx-pr:`4158` - Adding allocator imported target as a dependency of allocator
  module
* :hpx-pr:`4157` - Add ``hpx_memory`` as a dependency of parcelport plugins
* :hpx-pr:`4156` - Stackless coroutines now can refer to themselves (through
  get_self() and friends)
* :hpx-pr:`4154` - Added CMake policy CMP0060 for HPX applications.
* :hpx-pr:`4153` - add header ``iomanip`` to tests and tool
* :hpx-pr:`4152` - Casting MPI tag value
* :hpx-pr:`4149` - Add back private ``m_desc`` member variable in
  program_options module
* :hpx-pr:`4147` - Resource partitioner and threadmanager modules
* :hpx-pr:`4146` - Google Season of Docs updates to documentation; creating hpx
  projects
* :hpx-pr:`4145` - Adding basic support for stackless threads
* :hpx-pr:`4143` - Exclude ``test_client_1950`` from all target
* :hpx-pr:`4142` - Add a new ``thread_pool_executor``
* :hpx-pr:`4140` - Google Season of Docs updates to documentation; why hpx
* :hpx-pr:`4139` - Remove runtime includes from coroutines module
* :hpx-pr:`4138` - Forking ``boost::intrusive_ptr`` and adding it as
  ``hpx::intrusive_ptr``
* :hpx-pr:`4137` - Fixing TSS destruction
* :hpx-pr:`4136` - HPX.Compute modules
* :hpx-pr:`4133` - Fix ``block_executor``
* :hpx-pr:`4131` - Applying fixes based on reports from PVS Studio
* :hpx-pr:`4130` - Adding missing header to build system
* :hpx-pr:`4129` - Fixing compilation if ``HPX_WITH_DATAPAR_VC`` is enabled
* :hpx-pr:`4128` - Renaming ``moveonly_any`` to ``unique_any``
* :hpx-pr:`4126` - Attempt to fix ``basic_any`` constructor for gcc 7
* :hpx-pr:`4125` - Changing ``extra_archive_data`` implementation
* :hpx-pr:`4124` - Don't link to Boost.System unless required
* :hpx-pr:`4122` - Add kernel launch helper utility (+saxpy demo) and merge in
  octotiger changes
* :hpx-pr:`4121` - Fixing migration test if networking is disabled.
* :hpx-pr:`4120` - Google Season of Docs updates to documentation; hpx build
  system v1
* :hpx-pr:`4119` - Making sure ``chunk_size`` and ``max_chunk`` are actually
  applied to parallel algorithms if specified
* :hpx-pr:`4117` - Make CircleCI formatting check store diff
* :hpx-pr:`4116` - Fix automatically setting C++ standard
* :hpx-pr:`4114` - Module serialization
* :hpx-pr:`4113` - Module datastructures
* :hpx-pr:`4111` - Fixing performance regression introduced earlier
* :hpx-pr:`4110` - Adding missing SPDX tags
* :hpx-pr:`4109` - Overload for start without entry point/argv.
* :hpx-pr:`4108` - Making sure C++ standard is properly detected and propagated
* :hpx-pr:`4106` - use ``std::round`` for guaranteed rounding without errors
* :hpx-pr:`4104` - Extend ``scheduler_mode`` with new ``work_stealing`` and task
  assignment modes
* :hpx-pr:`4103` - Add this to lambda capture list
* :hpx-pr:`4102` - Add spdx license and check
* :hpx-pr:`4099` - Module coroutines
* :hpx-pr:`4098` - Fix append module path in module CMakeLists template
* :hpx-pr:`4097` - Function tests
* :hpx-pr:`4096` - Removing return of ``thread_result_type`` from functions not
  needing them
* :hpx-pr:`4095` - Stop-gap measure until cmake overhaul is in place
* :hpx-pr:`4094` - Deprecate ``HPX_WITH_MORE_THAN_64_THREADS``
* :hpx-pr:`4093` - Fix initialization of ``global_num_tasks`` in
  ``parallel_executor``
* :hpx-pr:`4092` - Add support for mi-malloc
* :hpx-pr:`4090` - Execution context
* :hpx-pr:`4089` - Make counters in coroutines optional
* :hpx-pr:`4087` - Making ``hpx::util::any`` compatible with C++17
* :hpx-pr:`4084` - Making sure destination array for ``std::transform`` is
  properly resized
* :hpx-pr:`4083` - Adapting ``thread_queue_mc`` to behave even if no 128bit
  atomics are available
* :hpx-pr:`4082` - Fix compilation on GCC 5
* :hpx-pr:`4081` - Adding option allowing to force using Boost.FileSystem
* :hpx-pr:`4080` - Updating module dependencies
* :hpx-pr:`4079` - Add missing tests for iterator_support module
* :hpx-pr:`4078` - Disable parcel-layer if networking is disabled
* :hpx-pr:`4077` - Add missing include that causes build fails
* :hpx-pr:`4076` - Enable compatibility headers for functional module
* :hpx-pr:`4075` - Coroutines module
* :hpx-pr:`4073` - Use ``configure_file`` for generated files in modules
* :hpx-pr:`4071` - Fixing MPI detection for PMIx
* :hpx-pr:`4070` - Fix macOS builds
* :hpx-pr:`4069` - Moving more facilities to the collectives module
* :hpx-pr:`4068` - Adding main HPX ``#include`` directory to modules
* :hpx-pr:`4066` - Switching the use of ``message(STATUS "...")`` to hpx_info
* :hpx-pr:`4065` - Move Boost.Filesystem handling to filesystem module
* :hpx-pr:`4064` - Fix program_options test with older boost versions
* :hpx-pr:`4062` - The ``cpu_features`` tool fails to compile on anything but
  x86 architectures
* :hpx-pr:`4061` - Add ``clang-format`` checking step for modules
* :hpx-pr:`4060` - Making sure ``HPX_IDLE_BACKOFF_TIME_MAX`` is always defined
  (even if its unused)
* :hpx-pr:`4059` - Renaming module ``hpx_parallel_executors`` into
  ``hpx_execution``
* :hpx-pr:`4058` - Do not build networking tests when networking disabled
* :hpx-pr:`4057` - Printing configuration summary for modules as well
* :hpx-pr:`4055` - Google Season of Docs updates to documentation; hpx build
  systems
* :hpx-pr:`4054` - Add troubleshooting section to manual
* :hpx-pr:`4051` - Add more variations to ``future_overhead`` test
* :hpx-pr:`4050` - Creating plugin module
* :hpx-pr:`4049` - Move missing modules tests
* :hpx-pr:`4047` - Add boost/filesystem headers to inspect deprecated headers
* :hpx-pr:`4045` - Module functional
* :hpx-pr:`4043` - Fix preconditions and error messages for suspension functions
* :hpx-pr:`4041` - Pass HPX_STANDARD on to dependent projects via
  HPXConfig.cmake
* :hpx-pr:`4040` - Program options module
* :hpx-pr:`4039` - Moving non-serializable ``any`` (``any_nonser``) to
  datastructures module
* :hpx-pr:`4038` - Adding MPark's variant (V1.4.0) to HPX
* :hpx-pr:`4037` - Adding resiliency module
* :hpx-pr:`4036` - Add C++17 filesystem compatibility header
* :hpx-pr:`4035` - Fixing support for mpirun
* :hpx-pr:`4028` - CMake to target based directives
* :hpx-pr:`4027` - Remove GitLab CI configuration
* :hpx-pr:`4026` - Threading refactoring
* :hpx-pr:`4025` - Refactoring thread queue configuration options
* :hpx-pr:`4024` - Fix padding calculation in ``cache_aligned_data.hpp``
* :hpx-pr:`4023` - Fixing Codacy issues
* :hpx-pr:`4022` - Make sure process mask option is passed to ``affinity_data``
* :hpx-pr:`4021` - Warn about compiling in C++11 mode
* :hpx-pr:`4020` - Module concurrency
* :hpx-pr:`4019` - Module topology
* :hpx-pr:`4018` - Update deprecated header in ``thread_queue_mc.hpp``
* :hpx-pr:`4015` - Avoid overwriting artifacts
* :hpx-pr:`4014` - Future overheads
* :hpx-pr:`4013` - Update URL to test output conversion script
* :hpx-pr:`4012` - Fix CUDA compilation
* :hpx-pr:`4011` - Fixing cyclic dependencies between modules
* :hpx-pr:`4010` - Ignore stable tag on CircleCI
* :hpx-pr:`4009` - Check circular dependencies in a circle ci step
* :hpx-pr:`4008` - Extend cache aligned data to handle tuple-like data
* :hpx-pr:`4007` - Fixing migration for components that have actions returning a
  client
* :hpx-pr:`4006` - Move is_value_proxy.hpp to algorithms module
* :hpx-pr:`4004` - Shorten CTest timeout on CircleCI
* :hpx-pr:`4003` - Refactoring to remove (internal) dependencies
* :hpx-pr:`4001` - Exclude tests from all target
* :hpx-pr:`4000` - Module errors
* :hpx-pr:`3999` - Enable support for compatibility headers for logging module
* :hpx-pr:`3998` - Add process thread binding option
* :hpx-pr:`3997` - Export handle_assert function
* :hpx-pr:`3996` - Attempt to solve issue where ``-latomic`` does not support
  128bit atomics
* :hpx-pr:`3993` - Make sure ``__LINE__`` is an unsigned
* :hpx-pr:`3991` - Fix dependencies and flags for header tests
* :hpx-pr:`3990` - Documentation tags fixes
* :hpx-pr:`3988` - Adding missing solution folder for format module test
* :hpx-pr:`3987` - Move runtime-dependent functions out of command line handling
* :hpx-pr:`3986` - Fix CMake configuration with PAPI on
* :hpx-pr:`3985` - Module timing
* :hpx-pr:`3984` - Fix default behaviour of paths in ``add_hpx_component``
* :hpx-pr:`3982` - Parallel executors module
* :hpx-pr:`3981` - Segmented algorithms module
* :hpx-pr:`3980` - Module logging
* :hpx-pr:`3979` - Module util
* :hpx-pr:`3978` - Fix ``clang-tidy`` step on CircleCI
* :hpx-pr:`3977` - Fixing solution folders for moved components
* :hpx-pr:`3976` - Module format
* :hpx-pr:`3975` - Enable deprecation warnings on CircleCI
* :hpx-pr:`3974` - Fix typos in documentation
* :hpx-pr:`3973` - Fix compilation with GCC 9
* :hpx-pr:`3972` - Add condition to clone apex + use of new cmake var APEX_ROOT
* :hpx-pr:`3971` - Add testing module
* :hpx-pr:`3968` - Remove unneeded file in hardware module
* :hpx-pr:`3967` - Remove leftover PIC settings from main CMakeLists.txt
* :hpx-pr:`3966` - Add missing export option in ``add_hpx_module``
* :hpx-pr:`3965` - Change ``current_function_helper`` back to non-constexpr
* :hpx-pr:`3964` - Fixing merge problems
* :hpx-pr:`3962` - Add a trait for ``std::array`` for unwrapping
* :hpx-pr:`3961` - Making ``hpx::util::tuple<Ts...>`` and ``std::tuple<Ts...>``
  convertible
* :hpx-pr:`3960` - fix compilation with CUDA 10 and GCC 6
* :hpx-pr:`3959` - Fix C++11 incompatibility
* :hpx-pr:`3957` - Algorithms module
* :hpx-pr:`3956` - [``HPX_AddModule``] Fix lower name var to upper
* :hpx-pr:`3955` - Fix CMake configuration with examples off and tests on
* :hpx-pr:`3954` - Move components to separate subdirectory in root of
  repository
* :hpx-pr:`3952` - Update ``papi.cpp``
* :hpx-pr:`3951` - Exclude modules header tests from all target
* :hpx-pr:`3950` - Adding ``all_reduce`` facility to collectives module
* :hpx-pr:`3949` - This adds a configuration file that will cause for stale
  issues to be automatically closed
* :hpx-pr:`3948` - Fixing ALPS environment
* :hpx-pr:`3947` - Add major compiler version check for building hpx as a binary
  package
* :hpx-pr:`3946` - [Modules] Move the location of the generated headers
* :hpx-pr:`3945` - Simplify tests and examples cmake
* :hpx-pr:`3943` - Remove example module
* :hpx-pr:`3942` - Add ``NOEXPORT`` option to ``add_hpx_{component,library}``
* :hpx-pr:`3938` - Use https for CDash submissions
* :hpx-pr:`3937` - Add ``HPX_WITH_BUILD_BINARY_PACKAGE`` to the compiler check
  (refs #3935)
* :hpx-pr:`3936` - Fixing installation of binaries on windows
* :hpx-pr:`3934` - Add set function for ``sliding_semaphore`` ``max_difference``
* :hpx-pr:`3933` - Remove ``cudadevrt`` from compile/link flags as it breaks
  downstream projects
* :hpx-pr:`3932` - Fixing 3929
* :hpx-pr:`3931` - Adding ``all_to_all``
* :hpx-pr:`3930` - Add test demonstrating the use of broadcast with component
  actions
* :hpx-pr:`3928` - fixed number of tasks and number of threads for heterogeneous
  slurm environments
* :hpx-pr:`3927` - Moving Cache module's tests into separate solution folder
* :hpx-pr:`3926` - Move unit tests to cache module
* :hpx-pr:`3925` - Move version check to config module
* :hpx-pr:`3924` - Add schedule hint executor parameters
* :hpx-pr:`3923` - Allow aligning objects bigger than the cache line size
* :hpx-pr:`3922` - Add Windows builds with Travis CI
* :hpx-pr:`3921` - Add ccls cache directory to gitignore
* :hpx-pr:`3920` - Fix ``git_external`` fetching of tags
* :hpx-pr:`3905` - Correct rostambod url. Fix typo in doc
* :hpx-pr:`3904` - Fix bug in context_base.hpp
* :hpx-pr:`3903` - Adding new performance counters
* :hpx-pr:`3902` - Add ``add_hpx_module`` function
* :hpx-pr:`3901` - Factoring out container remapping into a separate trait
* :hpx-pr:`3900` - Making sure errors during command line processing are
  properly reported and will not cause assertions
* :hpx-pr:`3899` - Remove old compatibility bases from ``make_action``
* :hpx-pr:`3898` - Make parameter size be of type ``size_t``
* :hpx-pr:`3897` - Making sure all tests are disabled if ``HPX_WITH_TESTS=OFF``
* :hpx-pr:`3895` - Add documentation for annotated_function
* :hpx-pr:`3894` - Working around VS2019 problem with ``make_action``
* :hpx-pr:`3892` - Avoid MSVC compatibility warning in internal allocator
* :hpx-pr:`3891` - Removal of the default intel config include
* :hpx-pr:`3888` - Fix ``async_customization`` dataflow example and Clarify
  what's being tested
* :hpx-pr:`3887` - Add Doxygen documentation
* :hpx-pr:`3882` - Minor docs fixes
* :hpx-pr:`3880` - Updating APEX version tag
* :hpx-pr:`3878` - Making sure symbols are properly exported from modules
  (needed for Windows/MacOS)
* :hpx-pr:`3877` - Documentation
* :hpx-pr:`3876` - Module hardware
* :hpx-pr:`3875` - Converted typedefs in actions submodule to using directives
* :hpx-pr:`3874` - Allow one to suppress target keywords in ``hpx_setup_target``
  for backwards compatibility
* :hpx-pr:`3873` - Add scripts to create releases and generate lists of PRs and
  issues
* :hpx-pr:`3872` - Fix latest HTML docs location
* :hpx-pr:`3870` - Module cache
* :hpx-pr:`3869` - Post 1.3.0 version bumps
* :hpx-pr:`3868` - Replace the macro ``HPX_ASSERT`` by ``HPX_TEST`` in tests
* :hpx-pr:`3845` - Assertion module
* :hpx-pr:`3839` - Make tuple serialization non-intrusive
* :hpx-pr:`3832` - Config module
* :hpx-pr:`3799` - Remove compat namespace and its contents
* :hpx-pr:`3701` - MoodyCamel lockfree
* :hpx-pr:`3496` - Disabling MPI's (deprecated) C++ interface
* :hpx-pr:`3192` - Move type info into ``hpx::debug`` namespace and add print
  helper functions
* :hpx-pr:`3159` - Support Checkpointing Components
