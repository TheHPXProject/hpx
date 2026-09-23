..
    Copyright (C) 2020-2021 ETH Zurich
    Copyright (C) 2007-2020 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_1_6_0:

===========================
|hpx| V1.6.0 (Feb 17, 2021)
===========================

General changes
===============

This release continues the focus on C++20 conformance with multiple new
algorithms adapted to be C++20 conformant and becoming customization point
objects (CPOs). We have also added experimental support for HIP, allowing
previous CUDA features to now be compiled with hipcc and run on AMD GPUs.

* The following algorithms have been adapted to be C++20 conformant:
  ``adjacent_find``, ``includes``, ``inplace_merge``, ``is_heap``,
  ``is_heap_until``, ``is_partitioned``, ``is_sorted``, ``is_sorted_until``,
  ``merge``, ``set_difference``, ``set_intersection``,
  ``set_symmetric_difference``, ``set_union``.
* Experimental HIP support can be enabled by compiling |hpx| with ``hipcc``. All
  CUDA functionality in |hpx| can now be used with HIP. The HIP functionality is
  for the time being exposed through the same API as the CUDA functionality,
  i.e. no changes are required in user code. The CUDA, and now HIP,
  functionality is in the ``hpx::cuda`` namespace.
* We have added ``partial_sort`` based on Francisco Tapia's implementation.
* ``hpx::init`` and ``hpx::start`` gained new overloads taking an
  ``hpx::init_params`` struct in 1.5.0. All overloads not taking an
  ``hpx::init_params`` are now deprecated.
* We have added an experimental ``fork_join_executor``. This executor can be
  used for OpenMP-style fork-join parallelism, where the latency of a parallel
  region is important for performance.
* The ``parallel_executor`` now uses a hierarchical spawning scheme for bulk
  execution, which improves data locality and performance.
* ``hpx::dataflow`` can now be used with executors that inject additional
  parameters into the call of the user-provided function.
* We have added experimental support for properties as proposed in |p2220|_.
  Currently the only supported property is the scheduling hint on
  ``parallel_executor``.
* :hpx:func:`hpx::util::annotated_function` can now be passed a dynamically
  generated ``std::string``.
* In moving functionality to new namespaces, old names have been deprecated.  A
  deprecation warning will be issued if you are using deprecated functionality,
  with instructions on how to correct or ignore the warning.
* We have removed all support for C and Fortran from our build system.
* We have further reduced the use of Boost types within |hpx|
  (``boost::system::error_code`` and ``boost::detail::spinlock``).
* We have enabled more warnings in our CI builds (unused variables and unused
  typedefs).

Breaking changes
================

* hpxMP support has been completely removed.
* The ``verbs`` parcelport has been removed.
* The following compatibility options have been disabled by default:
  ``HPX_WITH_ACTION_BASE_COMPATIBILITY``,
  ``HPX_WITH_REGISTER_THREAD_COMPATIBILITY``,
  ``HPX_WITH_PROMISE_ALIAS_COMPATIBILITY``,
  ``HPX_WITH_UNSCOPED_ENUM_COMPATIBILITY``,
  ``HPX_PROGRAM_OPTIONS_WITH_BOOST_PROGRAM_OPTIONS_COMPATIBILITY``,
  ``HPX_WITH_EMBEDDED_THREAD_POOLS_COMPATIBILITY``,
  ``HPX_WITH_THREAD_POOL_OS_EXECUTOR_COMPATIBILITY``,
  ``HPX_WITH_THREAD_EXECUTORS_COMPATIBILITY``,
  ``HPX_THREAD_AWARE_TIMER_COMPATIBILITY``,
  ``HPX_WITH_POOL_EXECUTOR_COMPATIBILITY``. Unless noted here, the above
  functionalities do not come with replacements. Unscoped enumerations have been
  replaced by scoped enumerations. Previously deprecated unscoped enumerations
  are disabled by ``HPX_WITH_UNSCOPED_ENUM_COMPATIBILITY``. Newly deprecated
  unscoped enumerations have been given deprecation warnings and replaced by
  scoped enumerations. ``hpx::promise`` has been replaced with
  ``hpx::distributed::promise``. ``hpx::program_options`` is a drop-in
  replacement for ``boost::program_options``.
  ``hpx::execution::parallel_executor`` now has constructors which take a thread
  pool, covering the use case of ``hpx::threads::executors::pool_executor``. A
  pool can be supplied with ``hpx::resource::get_thread_pool``.

Closed issues
=============

* :hpx-issue:`5148` - ``runtime_support.hpp`` does not work with newer cling
* :hpx-issue:`5147` - Wrong results with parallel reduce
* :hpx-issue:`5129` - Missing specialization for ``std::hash<hpx::thread::id>``
* :hpx-issue:`5126` - Use ``std::string`` for task annotations
* :hpx-issue:`5115` - Don't expect hwloc to always report Cores
* :hpx-issue:`5113` - Handle threadmanager exceptions during startup
* :hpx-issue:`5112` - libatomic problems causing unexpected fails
* :hpx-issue:`5089` - Remove non-BSL files
* :hpx-issue:`5088` - Unwrapping problem
* :hpx-issue:`5087` - Remove hpxMP support
* :hpx-issue:`5077` - PAPI counters are not accessible when HPX is installed
* :hpx-issue:`5075` - Make the structs in all ``iter_sent.hpp`` lower case
* :hpx-issue:`5067` - Bug ``string_util/split.hpp``
* :hpx-issue:`5049` - Change back the hipcc jenkins config to the fury partition
  on rostam
* :hpx-issue:`5038` - Not all examples link in the latest HPX master
* :hpx-issue:`5035` - Build with ``HPX_WITH_EXAMPLES`` fails
* :hpx-issue:`5019` - Broken help string for hpx
* :hpx-issue:`5016` - ``hpx::parallel::fill`` fails compiling
* :hpx-issue:`5014` - Rename all ``.cc`` to ``.cpp`` and ``.hh`` to ``.hpp``
* :hpx-issue:`4988` - MPI is not finalized if running with only one locality
* :hpx-issue:`4978` - Change feature test macros to expand to zero/one
* :hpx-issue:`4949` - Crash when not enabling TCP parcelport
* :hpx-issue:`4933` - Improve test coverage for unused variable warnings etc.
* :hpx-issue:`4878` - HPX mpi async might call ``MPI_FINALIZE`` before app calls it
* :hpx-issue:`4127` - Local runtime entry-points

Closed pull requests
====================

* :hpx-pr:`5178` - Fix parallel ``remove``\ /\ ``remove_copy``\ /\ ``transform``
  namespace references in docs
* :hpx-pr:`5169` - Attempt to get Piz Daint jenkins setup running after
  maintenance
* :hpx-pr:`5168` - Remove include of itself
* :hpx-pr:`5167` - Fixing deprecation warnings that slipped through the net
* :hpx-pr:`5159` - Update APEX tag to 2.3.1
* :hpx-pr:`5154` - Splitting unit tests on circleci to avoid timeouts
* :hpx-pr:`5151` - Use C++20 on ``clang-newest`` Jenkins CI configuration
* :hpx-pr:`5149` - Rename ``'module'`` symbols to avoid keyword conflict
* :hpx-pr:`5145` - Adjust handling of CUDA/HIP options in CMake
* :hpx-pr:`5142` - Store annotated_function annotations as ``std::strings``
* :hpx-pr:`5140` - Scheduler mode
* :hpx-pr:`5139` - Fix path problem in pre-commit hook, add summary commit line
* :hpx-pr:`5138` - Add program options variable map to resource partitioner init
* :hpx-pr:`5137` - Remove the use of ``boost::throw_exception``
* :hpx-pr:`5136` - Make sure codespell checks run on CircleCI
* :hpx-pr:`5132` - Fixing spelling errors
* :hpx-pr:`5131` - Mark ``counting_iterator`` member functions as
  ``HPX_HOST_DEVICE``
* :hpx-pr:`5130` - Adding specialization for ``std::hash<hpx::thread::id>``
* :hpx-pr:`5128` - Fixing environment handling for FreeBSD
* :hpx-pr:`5127` - Fix typo in fibonacci documentation
* :hpx-pr:`5123` - Reduce vector sizes in partial sort benchmarks when running
  in debug mode
* :hpx-pr:`5122` - Making sure exceptions during runtime initialization are
  correctly reported
* :hpx-pr:`5121` - Working around hwloc limitation on certain platforms
* :hpx-pr:`5120` - Fixing compatibility warnings in ``hpx::transform``
  implementation
* :hpx-pr:`5119` - Use ``sequential_find`` and friends from separate detail
  header
* :hpx-pr:`5116` - Fix compilation with timer pool off
* :hpx-pr:`5114` - Fix 5112 - make sure libatomic is used when needed
* :hpx-pr:`5109` - Remove default runtime mode argument from init overload,
  again
* :hpx-pr:`5108` - Refactor ``iter_sent.hpp`` to make structs lowercase
* :hpx-pr:`5107` - Relax ``dataflow`` internals
* :hpx-pr:`5106` - Change initialization of property CPOs to satisfy older nvcc
  versions
* :hpx-pr:`5104` - Fix regeneration of two files that trigger unnecessary
  rebuilds
* :hpx-pr:`5103` - Remove default runtime mode argument from start/init
  overloads
* :hpx-pr:`5102` - Untie deprecated thread enums from the CMake option
* :hpx-pr:`5101` - Update APEX tag for 1.6.0
* :hpx-pr:`5100` - Bump minimum required Boost version to 1.66 and update CI
  configurations
* :hpx-pr:`5098` - Minor fixes to public API listing
* :hpx-pr:`5097` - Remove hpxMP support
* :hpx-pr:`5096` - Remove fractals examples
* :hpx-pr:`5095` - Use all AMD nodes again on rostam
* :hpx-pr:`5094` - Attempt to remove macOS workaround for GH actions environment
* :hpx-pr:`5093` - Remove verbs parcelport
* :hpx-pr:`5091` - Avoid moving from lvalues
* :hpx-pr:`5090` - Adopt C++20 ``std::endian``
* :hpx-pr:`5085` - Update daint CI to use Boost 1.75.0
* :hpx-pr:`5084` - Disable compatibility options for 1.6.0 release
* :hpx-pr:`5083` - Remove duplicated call to the ``limiting_executor`` in
  ``future_overhead`` test
* :hpx-pr:`5079` - Add checks to make sure that MPI/CUDA polling is enabled/not
  disabled too early
* :hpx-pr:`5078` - Add install lib directory to list of component search paths
* :hpx-pr:`5076` - Fix a typo in the jenkins ``clang-newest`` cmake config
* :hpx-pr:`5074` - Fixing warnings generated by MSVC
* :hpx-pr:`5073` - Allow using noncopyable types with unwrapping
* :hpx-pr:`5072` - Fix ``is_convertible`` args in ``result_types``
* :hpx-pr:`5071` - Fix unused parameters
* :hpx-pr:`5070` - Fix unused variables warnings in hipcc
* :hpx-pr:`5069` - Add support for sentinels to ``adjacent_find``
* :hpx-pr:`5068` - Fix string split function
* :hpx-pr:`5066` - Adapt ``search`` to C++20 and Range TS
* :hpx-pr:`5065` - Fix ``hpx::range::adjacent_find`` doxygen function signatures
* :hpx-pr:`5064` - Refactor runtime configuration, command line handling, and
  resource partitioner
* :hpx-pr:`5063` - Limit the device code guards to the distributed parts of the
  ``future_overhead`` bench
* :hpx-pr:`5061` - Remove hipcc guards in examples and tests
* :hpx-pr:`5060` - Fix deprecation warnings generated by msvc
* :hpx-pr:`5059` - Add warning about suspending/resuming the runtime in
  multi-locality scenarios
* :hpx-pr:`5057` - Fix unused variable warnings
* :hpx-pr:`5056` - Fix ``hpx::util::get``
* :hpx-pr:`5055` - Remove hipcc guards
* :hpx-pr:`5054` - Fix typo
* :hpx-pr:`5051` - Adapt transform to C++20
* :hpx-pr:`5050` - Replace old init overloads in tests and examples
* :hpx-pr:`5048` - Limit jenkins hipcc to the reno node
* :hpx-pr:`5047` - Limit cuda jenkins run to nodes with exclusively Nvidia GPUs
* :hpx-pr:`5046` - Convert thread and future enums to class enums
* :hpx-pr:`5043` - Improve ``hpxrun.py`` for Phylanx
* :hpx-pr:`5042` - Add missing header to partial sort test
* :hpx-pr:`5041` - Adding Francisco Tapia's implementation of ``partial_sort``
* :hpx-pr:`5040` - Remove generated headers left behind from a previous
  configuration
* :hpx-pr:`5039` - Fix GCC 10 release builds
* :hpx-pr:`5037` - Add ``is_invocable`` typedefs to top-level ``hpx`` namespace
  and public API list
* :hpx-pr:`5036` - Deprecate ``hpx::util::decay`` in favor of ``std::decay``
* :hpx-pr:`5034` - Use versioned container image on CircleCI
* :hpx-pr:`5033` - Implement P2220 properties module
* :hpx-pr:`5032` - Do codespell comparison only on files changed from common
  ancestor
* :hpx-pr:`5031` - Moving traits files to ``actions_base``
* :hpx-pr:`5030` - Add codespell version print in circleci
* :hpx-pr:`5029` - Work around problems in GitHub actions macOS builder
* :hpx-pr:`5028` - Moving move files to naming and naming_base
* :hpx-pr:`5027` - Lessen constraints on certain algorithm arguments
* :hpx-pr:`5025` - Adapt ``is_sorted`` and ``is_sorted_until`` to C++20
* :hpx-pr:`5024` - Moving ``naming_base`` to full modules
* :hpx-pr:`5022` - Remove C language from ``CMakeLists.txt``
* :hpx-pr:`5021` - Warn about unused arguments given to ``add_hpx_module``
* :hpx-pr:`5020` - Fixing help string
* :hpx-pr:`5018` - Update CSCS jenkins configuration to clang 11
* :hpx-pr:`5017` - Fixing broken backwards compatibility for
  ``hpx::parallel::fill``
* :hpx-pr:`5015` - Detect if generated global header conflicts with explicitly
  listed module headers
* :hpx-pr:`5012` - Properly reset pointer tracking data in ``output_archive``
* :hpx-pr:`5011` - Inspect command line tweaks
* :hpx-pr:`5010` - Creating AGAS module
* :hpx-pr:`5009` - Replace ``boost::system::error_code`` with
  ``std::error_code``
* :hpx-pr:`5008` - Replace uses of ``boost::detail::spinlock``
* :hpx-pr:`5007` - Bump minimal Boost version to 1.65.0
* :hpx-pr:`5006` - Adapt is_partitioned to C++20
* :hpx-pr:`5005` - Making sure ``reduce_by_key`` compiles again
* :hpx-pr:`5004` - Fixing template specializations that make extra archive data
  types unique across module boundaries
* :hpx-pr:`5003` - Relax ``dataflow`` argument constraints
* :hpx-pr:`5001` - Add ``<random>`` inspect check
* :hpx-pr:`4999` - Attempt to fix MacOS Github action error
* :hpx-pr:`4997` - Fix unused variable and typedef warnings
* :hpx-pr:`4996` - Adapt ``adjacent_find`` to C++20
* :hpx-pr:`4995` - Test all schedulers in ``cross_pool_injection`` test except
  ``shared_priority_queue_scheduler``
* :hpx-pr:`4993` - Fix deprecation warnings
* :hpx-pr:`4991` - Avoid unnecessarily including entire modules
* :hpx-pr:`4990` - Fixing some warnings from HPX complaining about use of
  obsolete types
* :hpx-pr:`4989` - add a \*destroy\* trait for ParcelPort plugins
* :hpx-pr:`4986` - Remove serialization to functional module dependency
* :hpx-pr:`4985` - Compatibility header generation
* :hpx-pr:`4980` - Add ranges overloads to ``for_loop`` (and variants)
* :hpx-pr:`4979` - Actually enable unity builds on Jenkins
* :hpx-pr:`4977` - Cleaning up ``debug::print`` functionalities
* :hpx-pr:`4976` - Remove indirection layer in ``at_index_impl``
* :hpx-pr:`4975` - Remove indirection layer in ``at_index_impl``
* :hpx-pr:`4973` - Avoid warnings/errors for older gcc complaining about
  multi-line comments
* :hpx-pr:`4970` - Making set algorithms conform to C++20
* :hpx-pr:`4969` - Moving ``is_execution_policy`` and friends into namespace
  ``hpx``
* :hpx-pr:`4968` - Enable deprecation warnings for 1.6.0 and move ``any``
  functionality to hpx namespace
* :hpx-pr:`4967` - Define deprecation macros conditionally
* :hpx-pr:`4966` - Add ``clang-format`` and ``cmake-format`` version prints
* :hpx-pr:`4965` - Making ``is_heap`` and ``is_heap_until`` conforming to C++20
* :hpx-pr:`4964` - Adding parallel ``make_heap``
* :hpx-pr:`4962` - Fix external timer function pointer exports
* :hpx-pr:`4960` - Fixing folder names for module tests and examples
* :hpx-pr:`4959` - Adding communications set
* :hpx-pr:`4958` - Deprecate tuple and timing functionality ``in hpx::util``
* :hpx-pr:`4957` - Fixing unity build option for parcelports
* :hpx-pr:`4953` - Fixing MSVC problems after recent restructurings
* :hpx-pr:`4952` - Make ``parallel_executor`` use ``thread_pool_executor``
  spawning mechanism
* :hpx-pr:`4948` - Clean up old artifacts better and more aggressively on
  Jenkins
* :hpx-pr:`4947` - Add HIP support for AMD GPUs
* :hpx-pr:`4945` - Enable ``HPX_WITH_UNITY_BUILD`` option on one of the Jenkins
  configurations
* :hpx-pr:`4943` - Move public ``hpx::parallel::execution`` functionality to
  hpx::execution
* :hpx-pr:`4938` - Post release cleanup
* :hpx-pr:`4858` - Extending resilience APIs to support distributed invocations
* :hpx-pr:`4744` - Fork-join executor
* :hpx-pr:`4665` - Implementing sender, receiver, and ``operation_state``
  concepts in terms of P0443r13
* :hpx-pr:`4649` - Split libhpx into multiple libraries
* :hpx-pr:`4642` - Implementing ``operation_state`` concept in terms of P0443r13
* :hpx-pr:`4640` - Implementing receiver concept in terms of P0443r13
* :hpx-pr:`4622` - Sanitizer fixes
