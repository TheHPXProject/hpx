..
    Copyright (C) 2007-2018 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_0_9_0:

==========================
|hpx| V0.9.0 (Jul 5, 2012)
==========================

We have had roughly 800 commits since the last release and we have closed
approximately 80 tickets (bugs, feature requests, etc.).

General changes
===============

* Significant improvements made to the usability of |hpx| in large-scale,
  distributed environments.
* Renamed :hpx:class:`hpx::lcos::packaged_task` to
  :hpx:class:`hpx::lcos::packaged_action` to reflect the semantic differences to
  a packaged_task as defined by the |cpp11|_.
* |hpx| now exposes :hpx:class:`hpx::thread` which is compliant to the C++11
  std::thread type except that it (purely locally) represents an |hpx| thread.
  This new type does not expose any of the remote capabilities of the underlying
  |hpx|-thread implementation.
* The type :hpx:class:`hpx::lcos::future` is now compliant to the C++11
  std::future<> type. This type can be used to synchronize both, local and
  remote operations. In both cases the control flow will 'return' to the future
  in order to trigger any continuation.
* The types :hpx:class:`hpx::lcos::local::promise` and
  :hpx:class:`hpx::lcos::local::packaged_task` are now compliant to the C++11
  ``std::promise<>`` and ``std::packaged_task<>`` types. These can be used to
  create a future representing local work only. Use the types
  :hpx:class:`hpx::lcos::promise` and :hpx:class:`hpx::lcos::packaged_action`
  to wrap any (possibly remote) action into a future.
* :hpx:class:`hpx::thread` and :hpx:class:`hpx::lcos::future` are now
  cancelable.
* Added support for sequential and logic composition of
  :hpx:class:`hpx::lcos::future`\ s. The member function
  :hpx:member:`hpx::lcos::future::when` permits futures to be sequentially
  composed. The helper functions :hpx:func:`hpx::wait_all`,
  :hpx:func:`hpx::wait_any`, and :hpx:func:`hpx::wait_n` can be used to wait for
  more than one future at a time.
* |hpx| now exposes :hpx:func:`hpx::apply` and :hpx:func:`hpx::async` as the
  preferred way of creating (or invoking) any deferred work. These functions are
  usable with various types of functions, function objects, and actions and
  provide a uniform way to spawn deferred tasks.
* |hpx| now utilizes :hpx:func:`hpx::util::bind` to (partially) bind local
  functions and function objects, and also actions. Remote bound actions can
  have placeholders as well.
* |hpx| continuations are now fully polymorphic. The class
  :hpx:class:`hpx::actions::forwarding_continuation` is an example of how the
  user can write is own types of continuations. It can be used to execute any
  function as an continuation of a particular action.
* Reworked the action invocation API to be fully conformant to normal functions.
  Actions can now be invoked using :hpx:func:`hpx::apply`,
  :hpx:func:`hpx::async`, or using the ``operator()`` implemented on actions.
  Actions themselves can now be cheaply instantiated as they do not have any
  members anymore.
* Reworked the lazy action invocation API. Actions can now be directly bound
  using :hpx:func:`hpx::util::bind` by passing an action instance as the first
  argument.
* A minimal |hpx| program now looks like this::

      #include <hpx/hpx_init.hpp>

      int hpx_main()
      {
          return hpx::finalize();
      }

      int main()
      {
          return hpx::init();
      }

  This removes the immediate dependency on the |boost_program_options|_ library.

  .. note::

     This minimal version of an |hpx| program does not support any of the
     default command line arguments (such as --help, or command line options
     related to PBS). It is suggested to always pass ``argc`` and ``argv`` to
     |hpx| as shown in the example below.

* In order to support those, but still not to depend on |boost_program_options|_,
  the minimal program can be written as::

      #include <hpx/hpx_init.hpp>

      // The arguments for hpx_main can be left off, which very similar to the
      // behavior of ``main()`` as defined by C++.
      int hpx_main(int argc, char* argv[])
      {
          return hpx::finalize();
      }

      int main(int argc, char* argv[])
      {
          return hpx::init(argc, argv);
      }

* Added performance counters exposing the number of component instances which
  are alive on a given locality.
* Added performance counters exposing then number of messages sent and received,
  the number of parcels sent and received, the number of bytes sent and
  received, the overall time required to send and receive data, and the overall
  time required to serialize and deserialize the data.
* Added a new component: :hpx:class:`hpx::components::binpacking_factory` which
  is equivalent to the existing
  :hpx:class:`hpx::components::distributing_factory` component, except that it
  equalizes the overall population of the components to create. It exposes two
  factory methods, one based on the number of existing instances of the
  component type to create, and one based on an arbitrary performance counter
  which will be queried for all relevant localities.
* Added API functions allowing to access elements of the diagnostic information
  embedded in the given exception: :hpx:func:`hpx::get_locality_id`,
  :hpx:func:`hpx::get_host_name`, :hpx:func:`hpx::get_process_id`,
  :hpx:func:`hpx::get_function_name`, :hpx:func:`hpx::get_file_name`,
  :hpx:func:`hpx::get_line_number`, :hpx:func:`hpx::get_os_thread`,
  :hpx:func:`hpx::get_thread_id`, and :hpx:func:`hpx::get_thread_description`.

Bug fixes (closed tickets)
==========================

Here is a list of the important tickets we closed for this release:

* :hpx-issue:`71` - GIDs that are not serialized via ``handle_gid<>`` should
  raise an exception
* :hpx-issue:`105` - Allow for :hpx:class:`hpx::util::function`\ s to be registered
  in the AGAS symbolic namespace
* :hpx-issue:`107` - Nasty threadmanger race condition (reproducible in
  sheneos_test)
* :hpx-issue:`108` - Add millisecond resolution to |hpx| logs on Linux
* :hpx-issue:`110` - Shutdown hang in distributed with release build
* :hpx-issue:`116` - Don't use TSS for the applier and runtime pointers
* :hpx-issue:`162` - Move local synchronous execution shortcut from
  hpx::function to the applier
* :hpx-issue:`172` - Cache sources in CMake and check if they change manually
* :hpx-issue:`178` - Add an INI option to turn off ranged-based AGAS caching
* :hpx-issue:`187` - Support for disabling performance counter deployment
* :hpx-issue:`202` - Support for sending performance counter data to a specific
  file
* :hpx-issue:`218` - boost.coroutines allows different stack sizes, but stack
  pool is unaware of this
* :hpx-issue:`231` - Implement movable ``boost::bind``
* :hpx-issue:`232` - Implement movable ``boost::function``
* :hpx-issue:`236` - Allow binding :hpx:class:`hpx::util::function` to actions
* :hpx-issue:`239` - Replace ``hpx::function`` with
  :hpx:class:`hpx::util::function`
* :hpx-issue:`240` - Can't specify RemoteResult with lcos::async
* :hpx-issue:`242` - REGISTER_TEMPLATE support for plain actions
* :hpx-issue:`243` - ``handle_gid<>`` support for
  :hpx:class:`hpx::util::function`
* :hpx-issue:`245` - ``*_c_cache code`` throws an exception if the queried GID
  is not in the local cache
* :hpx-issue:`246` - Undefined references in dataflow/adaptive1d example
* :hpx-issue:`252` - Problems configuring sheneos with CMake
* :hpx-issue:`254` - Lifetime of components doesn't end when client goes out of
  scope
* :hpx-issue:`259` - CMake does not detect that MSVC10 has lambdas
* :hpx-issue:`260` - io_service_pool segfault
* :hpx-issue:`261` - Late parcel executed outside of pxthread
* :hpx-issue:`263` - Cannot select allocator with CMake
* :hpx-issue:`264` - Fix allocator select
* :hpx-issue:`267` - Runtime error for hello_world
* :hpx-issue:`269` - pthread_affinity_np test fails to compile
* :hpx-issue:`270` - Compiler noise due to -Wcast-qual
* :hpx-issue:`275` - Problem with configuration tests/include paths on Gentoo
* :hpx-issue:`325` - Sheneos is 200-400 times slower than the fortran equivalent
* :hpx-issue:`331` - :hpx:func:`hpx::init` and ``hpx_main()`` should not depend
  on program_options
* :hpx-issue:`333` - Add doxygen support to CMake for doc toolchain
* :hpx-issue:`340` - Performance counters for parcels
* :hpx-issue:`346` - Component loading error when running hello_world in
  distributed on MSVC2010
* :hpx-issue:`362` - Missing initializer error
* :hpx-issue:`363` - Parcel port serialization error
* :hpx-issue:`366` - Parcel buffering leads to types incompatible exception
* :hpx-issue:`368` - Scalable alternative to rand() needed for |hpx|
* :hpx-issue:`369` - IB over IP is substantially slower than just using standard
  TCP/IP
* :hpx-issue:`374` - :hpx:func:`hpx::lcos::wait` should work with dataflows and
  arbitrary classes meeting the future interface
* :hpx-issue:`375` - Conflicting/ambiguous overloads of
  :hpx:func:`hpx::lcos::wait`
* :hpx-issue:`376` - Find_HPX.cmake should set CMake variable HPX_FOUND for out
  of tree builds
* :hpx-issue:`377` - ShenEOS interpolate bulk and interpolate_one_bulk are
  broken
* :hpx-issue:`379` - Add support for distributed runs under SLURM
* :hpx-issue:`382` - _Unwind_Word not declared in boost.backtrace
* :hpx-issue:`387` - Doxygen should look only at list of specified files
* :hpx-issue:`388` - Running ``make install`` on an out-of-tree application is
  broken
* :hpx-issue:`391` - Out-of-tree application segfaults when running in qsub
* :hpx-issue:`392` - Remove HPX_NO_INSTALL option from cmake build system
* :hpx-issue:`396` - Pragma related warnings when compiling with older gcc
  versions
* :hpx-issue:`399` - Out of tree component build problems
* :hpx-issue:`400` - Out of source builds on Windows: linker should not receive
  compiler flags
* :hpx-issue:`401` - Out of source builds on Windows: components need to be
  linked with hpx_serialization
* :hpx-issue:`404` - gfortran fails to link automatically when fortran files are
  present
* :hpx-issue:`405` - Inability to specify linking order for external libraries
* :hpx-issue:`406` - Adapt action limits such that dataflow applications work
  without additional defines
* :hpx-issue:`415` - ``locality_results`` is not a member of
  ``hpx::components::server``
* :hpx-issue:`425` - Breaking changes to ``traits::*result`` wrt
  ``std::vector<id_type>``
* :hpx-issue:`426` - AUTOGLOB needs to be updated to support fortran
