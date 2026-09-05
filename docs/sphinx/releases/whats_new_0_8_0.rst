..
    Copyright (C) 2007-2018 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_0_8_0:

===========================
|hpx| V0.8.0 (Mar 23, 2012)
===========================

We have had roughly 1000 commits since the last release and we have closed
approximately 70 tickets (bugs, feature requests, etc.).

General changes
===============

* Improved PBS support, allowing for arbitrary naming schemes of node-hostnames.
* Finished verification of the reference counting framework.
* Implemented decrement merging logic to optimize the distributed reference
  counting system.
* Restructured the LCO framework. Renamed ``hpx::lcos::eager_future<>`` and
  ``hpx::lcos::lazy_future<>`` into :hpx:class:`hpx::lcos::packaged_task` and
  :hpx:class:`hpx::lcos::deferred_packaged_task`. Split
  :hpx:class:`hpx::lcos::promise` into :hpx:class:`hpx::lcos::packaged_task` and
  :hpx:class:`hpx::lcos::future`. Added 'local' futures (in namespace
  ``hpx::lcos::local``).
* Improved the general performance of local and remote action invocations. This
  (under certain circumstances) drastically reduces the number of copies created
  for each of the parameters and return values.
* Reworked the performance counter framework. Performance counters are now
  created only when needed, which reduces the overall resource requirements. The
  new framework allows for much more flexible creation and management of
  performance counters. The new sine example application demonstrates some of
  the capabilities of the new infrastructure.
* Added a buildbot-based continuous build system which gives instant, automated
  feedback on each commit to SVN.
* Added more automated tests to verify proper functioning of |hpx|.
* Started to create documentation for |hpx| and its API.
* Added documentation toolchain to the build system.
* Added dataflow LCO.
* Changed default |hpx| command line options to have ``hpx:`` prefix. For
  instance, the former option ``--threads`` is now :option:`--hpx:threads`. This
  has been done to make ambiguities with possible application specific command
  line options as unlikely as possible. See the section :ref:`commandline` for a
  full list of available options.
* Added the possibility to define command line aliases. The former short
  (one-letter) command line options have been predefined as aliases for
  backwards compatibility. See the section :ref:`commandline` for a detailed
  description of command line option aliasing.
* Network connections are now cached based on the connected host. The number of
  simultaneous connections to a particular host is now limited. Parcels are
  buffered and bundled if all connections are in use.
* Added more refined thread affinity control. This is based on the external
  library |hwloc|.
* Improved support for Windows builds with CMake.
* Added support for components to register their own command line options.
* Added the possibility to register custom startup/shutdown functions for any
  component. These functions are guaranteed to be executed by an |hpx| thread.
* Added two new experimental thread schedulers: hierarchy_scheduler and
  periodic_priority_scheduler. These can be activated by using the command line
  options :option:`--hpx:queuing`\ ``=hierarchy`` or :option:`--hpx:queuing`\
  ``=periodic``.

Example applications
====================

* `Graph500 performance benchmark <http://www.graph500.org/>`_ (thanks to
  Matthew Anderson for contributing this application).
* `GTC (Gyrokinetic Toroidal Code)
  <http://www.nersc.gov/research-and-development/benchmarking-and-workload-characterization/nersc-6-benchmarks/gtc/>`_:
  a skeleton for particle in cell type codes.
* Random Memory Access: an example demonstrating random memory accesses in a
  large array
* `ShenEOS example <http://stellarcollapse.org/equationofstate>`_, demonstrating
  partitioning of large read-only data structures and exposing an interpolation
  API.
* Sine performance counter demo.
* Accumulator examples demonstrating how to write and use |hpx| components.
* Quickstart examples (like hello_world, fibonacci, quicksort, factorial, etc.)
  demonstrating simple |hpx| concepts which introduce some of the concepts in
  |hpx|.
* Load balancing and work stealing demos.

API changes
===========

* Moved all local LCOs into a separate namespace ``hpx::lcos::local`` (for
  instance, ``hpx::lcos::local_mutex`` is now
  :hpx:class:`hpx::lcos::local::mutex`).
* Replaced ``hpx::actions::function`` with :hpx:class:`hpx::util::function`.
  Cleaned up related code.
* Removed ``hpx::traits::handle_gid`` and moved handling of global reference
  counts into the corresponding serialization code.
* Changed terminology: ``prefix`` is now called ``locality_id``, renamed the
  corresponding API functions (such as ``hpx::get_prefix``, which is now called
  ``hpx::get_locality_id``).
* Adding :hpx:func:`hpx::find_remote_localities`, and
  :hpx:func:`hpx::get_num_localities`.
* Changed performance counter naming scheme to make it more bash friendly.
  The new performance counter naming scheme is now

  .. code-block:: text

     /object{parentname#parentindex/instance#index}/counter#parameters

* Added ``hpx::get_worker_thread_num`` replacing
  ``hpx::threadmanager_base::get_thread_num``.
* Renamed ``hpx::get_num_os_threads`` to ``hpx::get_os_threads_count``.
* Added ``hpx::threads::get_thread_count``.
* Restructured the Futures sub-system, renaming types in accordance with the
  terminology used by the C++11 ISO standard.

Bug fixes (closed tickets)
==========================

Here is a list of the important tickets we closed for this release:

* :hpx-issue:`31` - Specialize handle_gid<> for examples and tests
* :hpx-issue:`72` - Fix AGAS reference counting
* :hpx-issue:`104` - heartbeat throws an exception when decrefing the
  performance counter it's watching
* :hpx-issue:`111` - throttle causes an exception on the target application
* :hpx-issue:`142` - One failed component loading causes an unrelated component
  to fail
* :hpx-issue:`165` - Remote exception propagation bug in AGAS reference counting
  test
* :hpx-issue:`186` - Test credit exhaustion/splitting (e.g. prepare_gid and
  symbol NS)
* :hpx-issue:`188` - Implement remaining AGAS reference counting test cases
* :hpx-issue:`258` - No type checking of GIDs in stubs classes
* :hpx-issue:`271` - Seg fault/shared pointer assertion in distributed code
* :hpx-issue:`281` - CMake options need descriptive text
* :hpx-issue:`283` - AGAS caching broken (gva_cache needs to be rewritten
  with ICL)
* :hpx-issue:`285` - HPX_INSTALL root directory not the same as
  CMAKE_INSTALL_PREFIX
* :hpx-issue:`286` - New segfault in dataflow applications
* :hpx-issue:`289` - Exceptions should only be logged if not handled
* :hpx-issue:`290` - c++11 tests failure
* :hpx-issue:`293` - Build target for component libraries
* :hpx-issue:`296` - Compilation error with Boost V1.49rc1
* :hpx-issue:`298` - Illegal instructions on termination
* :hpx-issue:`299` - gravity aborts with multiple threads
* :hpx-issue:`301` - Build error with Boost trunk
* :hpx-issue:`303` - Logging assertion failure in distributed runs
* :hpx-issue:`304` - Exception 'what' strings are lost when exceptions from
  decode_parcel are reported
* :hpx-issue:`306` - Performance counter user interface issues
* :hpx-issue:`307` - Logging exception in distributed runs
* :hpx-issue:`308` - Logging deadlocks in distributed
* :hpx-issue:`309` - Reference counting test failures and exceptions
* :hpx-issue:`311` - Merge AGAS remote_interface with the runtime_support object
* :hpx-issue:`314` - Object tracking for id_types
* :hpx-issue:`315` - Remove handle_gid and handle credit splitting in id_type
  serialization
* :hpx-issue:`320` - applier::get_locality_id() should return an error value (or
  throw an exception)
* :hpx-issue:`321` - Optimization for id_types which are never split should be
  restored
* :hpx-issue:`322` - Command line processing ignored with Boost 1.47.0
* :hpx-issue:`323` - Credit exhaustion causes object to stay alive
* :hpx-issue:`324` - Duplicate exception messages
* :hpx-issue:`326` - Integrate Quickbook with CMake
* :hpx-issue:`329` - --help and --version should still work
* :hpx-issue:`330` - Create pkg-config files
* :hpx-issue:`337` - Improve usability of performance counter timestamps
* :hpx-issue:`338` - Non-std exceptions deriving from std::exceptions in tfunc
  may be sliced
* :hpx-issue:`339` - Decrease the number of send_pending_parcels threads
* :hpx-issue:`343` - Dynamically setting the stack size doesn't work
* :hpx-issue:`351` - 'make install' does not update documents
* :hpx-issue:`353` - Disable FIXMEs in the docs by default; add a doc developer
  CMake option to enable FIXMEs
* :hpx-issue:`355` - 'make' doesn't do anything after correct configuration
* :hpx-issue:`356` - Don't use ``hpx::util::static_`` in topology code
* :hpx-issue:`359` - Infinite recursion in hpx::tuple serialization
* :hpx-issue:`361` - Add compile time option to disable logging completely
* :hpx-issue:`364` - Installation seriously broken in r7443

.. Proofread by:
   Adrian Serio 3-13-12
