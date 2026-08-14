..
    Copyright (C) 2007-2018 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_0_9_11:

============================
|hpx| V0.9.11 (Nov 11, 2015)
============================

Our main focus for this release was the design and development of a coherent set
of higher-level APIs exposing various types of parallelism to the application
programmer. We introduced the concepts of an ``executor``, which can be used to
customize the ``where`` and ``when`` of execution of tasks in the context of
parallelizing codes. We extended all APIs related to managing parallel tasks to
support executors which gives the user the choce of either using one of the
predefined executor types or to provide its own, possibly application specific,
executor. We paid very close attention to align all of these changes with the
existing C++ Standards documents or with the ongoing proposals for
standardization.

This release is the first after our change to a new development policy. We
switched all development to be strictly performed on branches only, all direct
commits to our main branch (``master``) are prohibited. Any change has to go
through a peer review before it will be merged to ``master``. As a result the
overall stability of our code base has significantly increased, the development
process itself has been simplified. This change manifests itself in a large
number of pull-requests which have been merged (please see below for a full list
of closed issues and pull-requests). All in all for this release, we closed
almost 100 issues and merged over 290 pull-requests. There have been over 1600
commits to the master branch since the last release.

General changes
===============

* We are moving into the direction of unifying managed and simple components. As
  such, the classes :cpp:class:`hpx::components::component` and
  :cpp:class:`hpx::components::component_base` have been added which currently
  just forward to the currently existing simple component facilities. The
  examples have been converted to only use those two classes.
* Added integration with the `CircleCI
  <https://circleci.com/gh/TheHPXProject/hpx>`__ hosted continuous integration
  service. This gives us constant and immediate feedback on the health of our
  master branch.
* The compiler configuration subsystem in the build system has been
  reimplemented. Instead of using Boost.Config we now use our own lightweight
  set of cmake scripts to determine the available language and library features
  supported by the used compiler.
* The API for creating instances of components has been consolidated. All
  component instances should be created using the :cpp:func:`hpx::new_` only. It
  allows one to instantiate both, single component instances and multiple
  component instances. The placement of the created components can be controlled
  by special distribution policies. Please see the corresponding documentation
  outlining the use of :cpp:func:`hpx::new_`.
* Introduced four new distribution policies which can be used with many API
  functions which traditionally expected to be used with a locality id. The new
  distribution policies are:

  * :cpp:class:`hpx::components::default_distribution_policy` which tries to
    place multiple component instances as evenly as possible.
  * :cpp:class:`hpx::components::colocating_distribution_policy` which will
    refer to the locality where a given component instance is currently placed.
  * :cpp:class:`hpx::components::binpacking_distribution_policy` which will
    place multiple component instances as evenly as possible based on any
    performance counter.
  * :cpp:class:`hpx::components::target_distribution_policy` which allows one to
    represent a given locality in the context of a distrwibution policy.
* The new distribution policies can now be also used with ``hpx::async``. This
  change also deprecates ``hpx::async_colocated(id, ...)`` which now is replaced
  by a distribution policy: ``hpx::async(hpx::colocated(id), ...)``.
* The ``hpx::vector`` and ``hpx::unordered_map`` data structures can now be used
  with the new distribution policies as well.
* The parallel facility ``hpx::parallel::task_region`` has been renamed to
  :cpp:class:`hpx::parallel::task_block` based on the changes in the
  corresponding standardization proposal |cpp11_n4088|_.
* Added extensions to the parallel facility
  :cpp:class:`hpx::parallel::task_block` allowing to combine a task_block with
  an execution policy. This implies a minor breaking change as the
  ``hpx::parallel::task_block`` is now a template.
* Added new LCOs: ``hpx::lcos::latch`` and ``hpx::lcos::local::latch`` which
  semantically conform to the proposed ``std::latch`` (see |cpp17_n4399|_).
* Added performance counters exposing data related to data transferred by
  input/output (filesystem) operations (thanks to Maciej Brodowicz).
* Added performance counters allowing to track the number of action invocations
  (local and remote invocations).
* Added new command line options `--hpx:print-counter-at <commandline>`_ and
  `--hpx:reset-counters <commandline>`_.
* The ``hpx::vector`` component has been renamed to ``hpx::partitioned_vector``
  to make it explicit that the underlying memory is not contiguous.
* Introduced a completely new and uniform higher-level parallelism API which is
  based on executors. All existing parallelism APIs have been adapted to this.
  We have added a large number of different executor types, such as a numa-aware
  executor, a this-thread executor, etc.
* Added support for the MingW toolchain on Windows (thanks to Eric Lemanissier).
* HPX now includes support for APEX, (Autonomic Performance Environment for
  eXascale). APEX is an instrumentation and software adaptation library that
  provides an interface to TAU profiling / tracing as well as runtime adaptation
  of HPX applications through policy definitions. For more information and
  documentation, please see `<https://github.com/UO-OACISS/xpress-apex>`_. To
  enable APEX at configuration time, specify ``-DHPX_WITH_APEX=On``. To also
  include support for TAU profiling, specify ``-DHPX_WITH_TAU=On`` and specify
  the ``-DTAU_ROOT``, ``-DTAU_ARCH`` and ``-DTAU_OPTIONS`` cmake parameters.
* We have implemented many more of the :ref:`parallel_algorithms`. Please see
  :hpx-issue:`1141` for the list of all available parallel algorithms (thanks to
  Daniel Bourgeois and John Biddiscombe for contributing their work).

Breaking changes
================

* We are moving into the direction of unifying managed and simple components. In
  order to stop exposing the old facilities, all examples have been converted to
  use the new classes. The breaking change in this release is that performance
  counters are now a :cpp:class:`hpx::components::component_base` instead of
  :cpp:class:`hpx::components::managed_component_base`.
* We removed the support for stackless threads. It turned out that there was no
  performance benefit when using stackless threads. As such, we decided to clean
  up our codebase. This feature was not documented.
* The CMake project name has changed from 'hpx' to 'HPX' for consistency and
  compatibility with naming conventions and other CMake projects. Generated
  config files go into <prefix>/lib/cmake/HPX and not <prefix>/lib/cmake/hpx.
* The macro ``HPX_REGISTER_MINIMAL_COMPONENT_FACTORY`` has been deprecated.
  Please use :c:macro:`HPX_REGISTER_COMPONENT`.
  instead. The old macro will be removed in the next release.
* The obsolete distributing_factory and binpacking_factory components have been
  removed. The corresponding functionality is now provided by the
  :cpp:func:`hpx::new_()` API function in conjunction with the
  ``hpx::default_layout`` and ``hpx::binpacking`` distribution policies
  (:cpp:class:`hpx::components::default_distribution_policy` and
  :cpp:class:`hpx::components::binpacking_distribution_policy`)
* The API function ``hpx::new_colocated`` has been deprecated. Please use the
  consolidated API :cpp:func:`hpx::new_` in conjunction with the new
  ``hpx::colocated`` distribution policy
  (:cpp:class:`hpx::components::colocating_distribution_policy`) instead. The
  old API function will still be available for at least one release of |hpx| if
  the configuration variable ``HPX_WITH_COLOCATED_BACKWARDS_COMPATIBILITY`` is
  enabled.
* The API function ``hpx::async_colocated`` has been deprecated. Please use the
  consolidated API ``hpx::async`` in conjunction with the new ``hpx::colocated``
  distribution policy
  (:cpp:class:`hpx::components::colocating_distribution_policy`) instead. The
  old API function will still be available for at least one release of |hpx| if
  the configuration variable ``HPX_WITH_COLOCATED_BACKWARDS_COMPATIBILITY`` is
  enabled.
* The obsolete remote_object component has been removed.
* Replaced the use of Boost.Serialization with our own solution. While the new
  version is mostly compatible with Boost.Serialization, this change requires
  some minor code modifications in user code. For more information, please see
  the corresponding `announcement
  <http://thread.gmane.org/gmane.comp.lib.hpx.devel/196>`_ on the
  |stellar_list|_ mailing list.
* The names used by cmake to influence various configuration options have been
  unified. The new naming scheme relies on all configuration constants to start
  with ``HPX_WITH_...``, while the preprocessor constant which is used at build
  time starts with ``HPX_HAVE_...``. For instance, the former cmake command line
  ``-DHPX_MALLOC=...`` now has to be specified a ``-DHPX_WITH_MALLOC=...`` and
  will cause the preprocessor constant ``HPX_HAVE_MALLOC`` to be defined. The
  actual name of the constant (i.e. ``MALLOC``) has not changed. Please see the
  corresponding documentation for more details (:ref:`cmake_variables`).
* The ``get_gid()`` functions exposed by the component base classes
  ``hpx::components::server::simple_component_base``,
  ``hpx::components::server::managed_component_base``, and
  ``hpx::components::server::fixed_component_base`` have been replaced by two
  new functions: ``get_unmanaged_id()`` and ``get_id()``. To enable the old
  function name for backwards compatibility, use the cmake configuration option
  ``HPX_WITH_COMPONENT_GET_GID_COMPATIBILITY=On``.
* All functions which were named ``get_gid()`` but were returning
  ``hpx::id_type`` have been renamed to ``get_id()``. To enable the old function
  names for backwards compatibility, use the cmake configuration option
  ``HPX_WITH_COMPONENT_GET_GID_COMPATIBILITY=On``.

Bug fixes (closed tickets)
==========================

Here is a list of the important tickets we closed for this release.

* :hpx-pr:`1855` - Completely removing external/endian
* :hpx-pr:`1854` - Don't pollute CMAKE_CXX_FLAGS through find_package()
* :hpx-pr:`1853` - Updating CMake configuration to get correct version of TAU
  library
* :hpx-pr:`1852` - Fixing Performance Problems with MPI Parcelport
* :hpx-pr:`1851` - Fixing hpx_add_link_flag() and hpx_remove_link_flag()
* :hpx-pr:`1850` - Fixing 1836, adding parallel::sort
* :hpx-pr:`1849` - Fixing configuration for use of more than 64 cores
* :hpx-pr:`1848` - Change default APEX version for release
* :hpx-pr:`1847` - Fix client_base::then on release
* :hpx-pr:`1846` - Removing broken lcos::local::channel from release
* :hpx-pr:`1845` - Adding example demonstrating a possible safe-object
  implementation to release
* :hpx-pr:`1844` - Removing stubs from accumulator examples
* :hpx-pr:`1843` - Don't pollute CMAKE_CXX_FLAGS through find_package()
* :hpx-pr:`1841` - Fixing client_base<>::then
* :hpx-pr:`1840` - Adding example demonstrating a possible safe-object
  implementation
* :hpx-pr:`1838` - Update version rc1
* :hpx-pr:`1837` - Removing broken lcos::local::channel
* :hpx-pr:`1835` - Adding explicit move constructor and assignment operator to
  hpx::lcos::promise
* :hpx-pr:`1834` - Making hpx::lcos::promise move-only
* :hpx-pr:`1833` - Adding fedora docs
* :hpx-issue:`1832` - hpx::lcos::promise<> must be move-only
* :hpx-pr:`1831` - Fixing resource manager gcc5.2
* :hpx-pr:`1830` - Fix intel13
* :hpx-pr:`1829` - Unbreaking thread test
* :hpx-pr:`1828` - Fixing #1620
* :hpx-pr:`1827` - Fixing a memory management issue for the Parquet application
* :hpx-issue:`1826` - Memory management issue in hpx::lcos::promise
* :hpx-pr:`1825` - Adding hpx::components::component and
  hpx::components::component_base
* :hpx-pr:`1823` - Adding git commit id to circleci build
* :hpx-pr:`1822` - applying fixes suggested by clang 3.7
* :hpx-pr:`1821` - Hyperlink fixes
* :hpx-pr:`1820` - added parallel multi-locality sanity test
* :hpx-pr:`1819` - Fixing #1667
* :hpx-issue:`1817` - Hyperlinks generated by inspect tool are wrong
* :hpx-pr:`1816` - Support hpxrx
* :hpx-pr:`1814` - Fix async to dispatch to the correct locality in all cases
* :hpx-issue:`1813` - async(launch::..., action(), ...) always invokes locally
* :hpx-pr:`1812` - fixed syntax error in CMakeLists.txt
* :hpx-pr:`1811` - Agas optimizations
* :hpx-pr:`1810` - drop superfluous typedefs
* :hpx-pr:`1809` - Allow HPX to be used as an optional package in 3rd party code
* :hpx-pr:`1808` - Fixing #1723
* :hpx-pr:`1807` - Making sure resolve_localities does not hang during normal
  operation
* :hpx-issue:`1806` - Spinlock no longer movable and deletes operator '=',
  breaks MiniGhost
* :hpx-issue:`1804` - register_with_basename causes hangs
* :hpx-pr:`1801` - Enhanced the inspect tool to take user directly to the
  problem with hyperlinks
* :hpx-issue:`1800` - Problems compiling application on smic
* :hpx-pr:`1799` - Fixing cv exceptions
* :hpx-pr:`1798` - Documentation refactoring & updating
* :hpx-pr:`1797` - Updating the activeharmony CMake module
* :hpx-pr:`1795` - Fixing cv
* :hpx-pr:`1794` - Fix connect with hpx::runtime_mode_connect
* :hpx-pr:`1793` - fix a wrong use of HPX_MAX_CPU_COUNT instead of
  HPX_HAVE_MAX_CPU_COUNT
* :hpx-pr:`1792` - Allow for default constructed parcel instances to be moved
* :hpx-pr:`1791` - Fix connect with hpx::runtime_mode_connect
* :hpx-issue:`1790` - assertion ``action_.get()`` failed: HPX(assertion_failure)
  when running Octotiger with pull request 1786
* :hpx-pr:`1789` - Fixing discover_counter_types API function
* :hpx-issue:`1788` - connect with hpx::runtime_mode_connect
* :hpx-issue:`1787` - discover_counter_types not working
* :hpx-pr:`1786` - Changing addressing_service to use std::unordered_map instead
  of std::map
* :hpx-pr:`1785` - Fix is_iterator for container algorithms
* :hpx-pr:`1784` - Adding new command line options:
* :hpx-pr:`1783` - Minor changes for APEX support
* :hpx-pr:`1782` - Drop legacy forwarding action traits
* :hpx-pr:`1781` - Attempt to resolve the race between cv::wait_xxx and
  cv::notify_all
* :hpx-pr:`1780` - Removing serialize_sequence
* :hpx-pr:`1779` - Fixed #1501: hwloc configuration options are wrong for MIC
* :hpx-pr:`1778` - Removing ability to enable/disable parcel handling
* :hpx-pr:`1777` - Completely removing stackless threads
* :hpx-pr:`1776` - Cleaning up util/plugin
* :hpx-pr:`1775` - Agas fixes
* :hpx-pr:`1774` - Action invocation count
* :hpx-pr:`1773` - replaced MSVC variable with WIN32
* :hpx-pr:`1772` - Fixing Problems in MPI parcelport and future serialization.
* :hpx-pr:`1771` - Fixing intel 13 compiler errors related to variadic template
  template parameters for ``lcos::when_`` tests
* :hpx-pr:`1770` - Forwarding decay to ``std::``
* :hpx-pr:`1769` - Add more characters with special regex meaning to the
  existing patch
* :hpx-pr:`1768` - Adding test for receive_buffer
* :hpx-pr:`1767` - Making sure that uptime counter throws exception on any
  attempt to be reset
* :hpx-pr:`1766` - Cleaning up code related to throttling scheduler
* :hpx-pr:`1765` - Restricting thread_data to creating only with
  intrusive_pointers
* :hpx-pr:`1764` - Fixing 1763
* :hpx-issue:`1763` - UB in thread_data::operator delete
* :hpx-pr:`1762` - Making sure all serialization registries/factories are unique
* :hpx-pr:`1761` - Fixed #1751: hpx::future::wait_for fails a simple test
* :hpx-pr:`1758` - Fixing #1757
* :hpx-issue:`1757` - pinning not correct using --hpx:bind
* :hpx-issue:`1756` - compilation error with MinGW
* :hpx-pr:`1755` - Making output serialization const-correct
* :hpx-issue:`1753` - HPX performance degrades with time since execution begins
* :hpx-issue:`1752` - Error in AGAS
* :hpx-issue:`1751` - hpx::future::wait_for fails a simple test
* :hpx-pr:`1750` - Removing hpx_fwd.hpp includes
* :hpx-pr:`1749` - Simplify result_of and friends
* :hpx-pr:`1747` - Removed superfluous code from message_buffer.hpp
* :hpx-pr:`1746` - Tuple dependencies
* :hpx-issue:`1745` - Broken when_some which takes iterators
* :hpx-pr:`1744` - Refining archive interface
* :hpx-pr:`1743` - Fixing when_all when only a single future is passed
* :hpx-pr:`1742` - Config includes
* :hpx-pr:`1741` - Os executors
* :hpx-issue:`1740` - hpx::promise has some problems
* :hpx-pr:`1739` - Parallel composition with generic containers
* :hpx-issue:`1738` - After building program and successfully linking to a
  version of hpx DHPX_DIR seems to be ignored
* :hpx-issue:`1737` - Uptime problems
* :hpx-pr:`1736` - added convenience c-tor and begin()/end() to serialize_buffer
* :hpx-pr:`1735` - Config includes
* :hpx-pr:`1734` - Fixed #1688: Add timer counters for tfunc_total and
  exec_total
* :hpx-issue:`1733` - Add unit test for hpx/lcos/local/receive_buffer.hpp
* :hpx-pr:`1732` - Renaming get_os_thread_count
* :hpx-pr:`1731` - Basename registration
* :hpx-issue:`1730` - Use after move of thread_init_data
* :hpx-pr:`1729` - Rewriting channel based on new gate component
* :hpx-pr:`1728` - Fixing #1722
* :hpx-pr:`1727` - Fixing compile problems with apply_colocated
* :hpx-pr:`1726` - Apex integration
* :hpx-pr:`1725` - fixed test timeouts
* :hpx-pr:`1724` - Renaming vector
* :hpx-issue:`1723` - Drop support for intel compilers and gcc 4.4. based
  standard libs
* :hpx-issue:`1722` - Add support for detecting non-ready futures before
  serialization
* :hpx-pr:`1721` - Unifying parallel executors, initializing from launch policy
* :hpx-pr:`1720` - dropped superfluous typedef
* :hpx-issue:`1718` - Windows 10 x64, VS 2015 - Unknown CMake command
  "add_hpx_pseudo_target".
* :hpx-pr:`1717` - Timed executor traits for thread-executors
* :hpx-pr:`1716` - serialization of arrays didn't work with non-pod types. fixed
* :hpx-pr:`1715` - List serialization
* :hpx-pr:`1714` - changing misspellings
* :hpx-pr:`1713` - Fixed distribution policy executors
* :hpx-pr:`1712` - Moving library detection to be executed after feature tests
* :hpx-pr:`1711` - Simplify parcel
* :hpx-pr:`1710` - Compile only tests
* :hpx-pr:`1709` - Implemented timed executors
* :hpx-pr:`1708` - Implement parallel::executor_traits for thread-executors
* :hpx-pr:`1707` - Various fixes to threads::executors to make custom schedulers
  work
* :hpx-pr:`1706` - Command line option --hpx:cores does not work as expected
* :hpx-issue:`1705` - command line option --hpx:cores does not work as expected
* :hpx-pr:`1704` - vector deserialization is speeded up a little
* :hpx-pr:`1703` - Fixing shared_mutes
* :hpx-issue:`1702` - Shared_mutex does not compile with no_mutex cond_var
* :hpx-pr:`1701` - Add distribution_policy_executor
* :hpx-pr:`1700` - Executor parameters
* :hpx-pr:`1699` - Readers writer lock
* :hpx-pr:`1698` - Remove leftovers
* :hpx-pr:`1697` - Fixing held locks
* :hpx-pr:`1696` - Modified Scan Partitioner for Algorithms
* :hpx-pr:`1695` - This thread executors
* :hpx-pr:`1694` - Fixed #1688: Add timer counters for tfunc_total and
  exec_total
* :hpx-pr:`1693` - Fix #1691: is_executor template specification fails for
  inherited executors
* :hpx-pr:`1692` - Fixed #1662: Possible exception source in
  coalescing_message_handler
* :hpx-issue:`1691` - is_executor template specification fails for inherited
  executors
* :hpx-pr:`1690` - added macro for non-intrusive serialization of classes
  without a default c-tor
* :hpx-pr:`1689` - Replace value_or_error with custom storage, unify future_data
  state
* :hpx-issue:`1688` - Add timer counters for tfunc_total and exec_total
* :hpx-pr:`1687` - Fixed interval timer
* :hpx-pr:`1686` - Fixing cmake warnings about not existing pseudo target
  dependencies
* :hpx-pr:`1685` - Converting partitioners to use bulk async execute
* :hpx-pr:`1683` - Adds a tool for inspect that checks for character limits
* :hpx-pr:`1682` - Change project name to (uppercase) HPX
* :hpx-pr:`1681` - Counter shortnames
* :hpx-pr:`1680` - Extended Non-intrusive Serialization to Ease Usage for
  Library Developers
* :hpx-pr:`1679` - Working on 1544: More executor changes
* :hpx-pr:`1678` - Transpose fixes
* :hpx-pr:`1677` - Improve Boost compatibility check
* :hpx-pr:`1676` - 1d stencil fix
* :hpx-issue:`1675` - hpx project name is not HPX
* :hpx-pr:`1674` - Fixing the MPI parcelport
* :hpx-pr:`1673` - added move semantics to map/vector deserialization
* :hpx-pr:`1672` - Vs2015 await
* :hpx-pr:`1671` - Adapt transform for #1668
* :hpx-pr:`1670` - Started to work on #1668
* :hpx-pr:`1669` - Add this_thread_executors
* :hpx-issue:`1667` - Apple build instructions in docs are out of date
* :hpx-pr:`1666` - Apex integration
* :hpx-pr:`1665` - Fixes an error with the whitespace check that showed the
  incorrect location of the error
* :hpx-issue:`1664` - Inspect tool found incorrect endline whitespace
* :hpx-pr:`1663` - Improve use of locks
* :hpx-issue:`1662` - Possible exception source in coalescing_message_handler
* :hpx-pr:`1661` - Added support for 128bit number serialization
* :hpx-pr:`1660` - Serialization 128bits
* :hpx-pr:`1659` - Implemented inner_product and adjacent_diff algos
* :hpx-pr:`1658` - Add serialization for std::set (as there is for std::vector
  and std::map)
* :hpx-pr:`1657` - Use of shared_ptr in io_service_pool changed to unique_ptr
* :hpx-issue:`1656` - 1d_stencil codes all have wrong factor
* :hpx-pr:`1654` - When using runtime_mode_connect, find the correct localhost
  public ip address
* :hpx-pr:`1653` - Fixing 1617
* :hpx-pr:`1652` - Remove traits::action_may_require_id_splitting
* :hpx-pr:`1651` - Fixed performance counters related to AGAS cache timings
* :hpx-pr:`1650` - Remove leftovers of traits::type_size
* :hpx-pr:`1649` - Shorten target names on Windows to shorten used path names
* :hpx-pr:`1648` - Fixing problems introduced by merging #1623 for older
  compilers
* :hpx-pr:`1647` - Simplify running automatic builds on Windows
* :hpx-issue:`1646` - Cache insert and update performance counters are broken
* :hpx-issue:`1644` - Remove leftovers of traits::type_size
* :hpx-issue:`1643` - Remove traits::action_may_require_id_splitting
* :hpx-pr:`1642` - Adds spell checker to the inspect tool for qbk and doxygen
  comments
* :hpx-pr:`1640` - First step towards fixing 688
* :hpx-pr:`1639` - Re-apply remaining changes from limit_dataflow_recursion
  branch
* :hpx-pr:`1638` - This fixes possible deadlock in the test
  ignore_while_locked_1485
* :hpx-pr:`1637` - Fixing hpx::wait_all() invoked with two vector<future<T>>
* :hpx-pr:`1636` - Partially re-apply changes from limit_dataflow_recursion
  branch
* :hpx-pr:`1635` - Adding missing test for #1572
* :hpx-pr:`1634` - Revert "Limit recursion-depth in dataflow to a configurable
  constant"
* :hpx-pr:`1633` - Add command line option to ignore batch environment
* :hpx-pr:`1631` - hpx::lcos::queue exhibits strange behavior
* :hpx-pr:`1630` - Fixed endline_whitespace_check.cpp to detect lines with only
  whitespace
* :hpx-issue:`1629` - Inspect trailing whitespace checker problem
* :hpx-pr:`1628` - Removed meaningless const qualifiers. Minor icpc fix.
* :hpx-pr:`1627` - Fixing the queue LCO and add example demonstrating its use
* :hpx-pr:`1626` - Deprecating get_gid(), add get_id() and get_unmanaged_id()
* :hpx-pr:`1625` - Allowing to specify whether to send credits along with
  message
* :hpx-issue:`1624` - Lifetime issue
* :hpx-issue:`1623` - hpx::wait_all() invoked with two vector<future<T>> fails
* :hpx-pr:`1622` - Executor partitioners
* :hpx-pr:`1621` - Clean up coroutines implementation
* :hpx-issue:`1620` - Revert #1535
* :hpx-pr:`1619` - Fix result type calculation for hpx::make_continuation
* :hpx-pr:`1618` - Fixing RDTSC on Xeon/Phi
* :hpx-issue:`1617` - hpx cmake not working when run as a subproject
* :hpx-issue:`1616` - cmake problem resulting in RDTSC not working correctly for
  Xeon Phi creates very strange results for duration counters
* :hpx-issue:`1615` - hpx::make_continuation requires input and output to be the
  same
* :hpx-pr:`1614` - Fixed remove copy test
* :hpx-issue:`1613` - Dataflow causes stack overflow
* :hpx-pr:`1612` - Modified foreach partitioner to use bulk execute
* :hpx-pr:`1611` - Limit recursion-depth in dataflow to a configurable constant
* :hpx-pr:`1610` - Increase timeout for CircleCI
* :hpx-pr:`1609` - Refactoring thread manager, mainly extracting thread pool
* :hpx-pr:`1608` - Fixed running multiple localities without localities
  parameter
* :hpx-pr:`1607` - More algorithm fixes to adjacentfind
* :hpx-issue:`1606` - Running without localities parameter binds to bogus port
  range
* :hpx-issue:`1605` - Too many serializations
* :hpx-pr:`1604` - Changes the HPX image into a hyperlink
* :hpx-pr:`1601` - Fixing problems with remove_copy algorithm tests
* :hpx-pr:`1600` - Actions with ids cleanup
* :hpx-pr:`1599` - Duplicate binding of global ids should fail
* :hpx-pr:`1598` - Fixing array access
* :hpx-pr:`1597` - Improved the reliability of connecting/disconnecting
  localities
* :hpx-issue:`1596` - Duplicate id binding should fail
* :hpx-pr:`1595` - Fixing more cmake config constants
* :hpx-pr:`1594` - Fixing preprocessor constant used to enable C++11 chrono
* :hpx-pr:`1593` - Adding operator|() for hpx::launch
* :hpx-issue:`1592` - Error (typo) in the docs
* :hpx-issue:`1590` - CMake fails when CMAKE_BINARY_DIR contains '+'.
* :hpx-issue:`1589` - Disconnecting a locality results in segfault using
  heartbeat example
* :hpx-pr:`1588` - Fix doc string for config option HPX_WITH_EXAMPLES
* :hpx-pr:`1586` - Fixing 1493
* :hpx-pr:`1585` - Additional Check for Inspect Tool to detect Endline
  Whitespace
* :hpx-issue:`1584` - Clean up coroutines implementation
* :hpx-pr:`1583` - Adding a check for end line whitespace
* :hpx-pr:`1582` - Attempt to fix assert firing after scheduling loop was exited
* :hpx-pr:`1581` - Fixed adjacentfind_binary test
* :hpx-pr:`1580` - Prevent some of the internal cmake lists from growing
  indefinitely
* :hpx-pr:`1579` - Removing type_size trait, replacing it with special archive
  type
* :hpx-issue:`1578` - Remove demangle_helper
* :hpx-pr:`1577` - Get ptr problems
* :hpx-issue:`1576` - Refactor async, dataflow, and future::then
* :hpx-pr:`1575` - Fixing tests for parallel rotate
* :hpx-pr:`1574` - Cleaning up schedulers
* :hpx-pr:`1573` - Fixing thread pool executor
* :hpx-pr:`1572` - Fixing number of configured localities
* :hpx-pr:`1571` - Reimplement decay
* :hpx-pr:`1570` - Refactoring async, apply, and dataflow APIs
* :hpx-pr:`1569` - Changed range for mach-o library lookup
* :hpx-pr:`1568` - Mark decltype support as required
* :hpx-pr:`1567` - Removed const from algorithms
* :hpx-issue:`1566` - CMAKE Configuration Test Failures for clang 3.5 on debian
* :hpx-pr:`1565` - Dylib support
* :hpx-pr:`1564` - Converted partitioners and some algorithms to use executors
* :hpx-pr:`1563` - Fix several #includes for Boost.Preprocessor
* :hpx-pr:`1562` - Adding configuration option disabling/enabling all message
  handlers
* :hpx-pr:`1561` - Removed all occurrences of boost::move replacing it with
  std::move
* :hpx-issue:`1560` - Leftover HPX_REGISTER_ACTION_DECLARATION_2
* :hpx-pr:`1558` - Revisit async/apply SFINAE conditions
* :hpx-pr:`1557` - Removing type_size trait, replacing it with special archive
  type
* :hpx-pr:`1556` - Executor algorithms
* :hpx-pr:`1555` - Remove the necessity to specify archive flags on the
  receiving end
* :hpx-pr:`1554` - Removing obsolete Boost.Serialization macros
* :hpx-pr:`1553` - Properly fix HPX_DEFINE_*_ACTION macros
* :hpx-pr:`1552` - Fixed algorithms relying on copy_if implementation
* :hpx-pr:`1551` - Pxfs - Modifying FindOrangeFS.cmake based on OrangeFS 2.9.X
* :hpx-issue:`1550` - Passing plain identifier inside HPX_DEFINE_PLAIN_ACTION_1
* :hpx-pr:`1549` - Fixing intel14/libstdc++4.4
* :hpx-pr:`1548` - Moving raw_ptr to detail namespace
* :hpx-pr:`1547` - Adding support for executors to future.then
* :hpx-pr:`1546` - Executor traits result types
* :hpx-pr:`1545` - Integrate executors with dataflow
* :hpx-pr:`1543` - Fix potential zero-copy for
  primarynamespace::bulk_service_async et.al.
* :hpx-pr:`1542` - Merging HPX0.9.10 into pxfs branch
* :hpx-pr:`1541` - Removed stale cmake tests, unused since the great cmake
  refactoring
* :hpx-pr:`1540` - Fix idle-rate on platforms without TSC
* :hpx-pr:`1539` - Reporting situation if zero-copy-serialization was performed
  by a parcel generated from a plain apply/async
* :hpx-pr:`1538` - Changed return type of bulk executors and added test
* :hpx-issue:`1537` - Incorrect cpuid config tests
* :hpx-pr:`1536` - Changed return type of bulk executors and added test
* :hpx-pr:`1535` - Make sure promise::get_gid() can be called more than once
* :hpx-pr:`1534` - Fixed async_callback with bound callback
* :hpx-pr:`1533` - Updated the link in the documentation to a publically-
  accessible URL
* :hpx-pr:`1532` - Make sure sync primitives are not copyable nor movable
* :hpx-pr:`1531` - Fix unwrapped issue with future ranges of void type
* :hpx-pr:`1530` - Serialization complex
* :hpx-issue:`1528` - Unwrapped issue with future<void>
* :hpx-issue:`1527` - HPX does not build with Boost 1.58.0
* :hpx-pr:`1526` - Added support for boost.multi_array serialization
* :hpx-pr:`1525` - Properly handle deferred futures, fixes #1506
* :hpx-pr:`1524` - Making sure invalid action argument types generate clear
  error message
* :hpx-issue:`1522` - Need serialization support for boost multi array
* :hpx-issue:`1521` - Remote async and zero-copy serialization optimizations
  don't play well together
* :hpx-pr:`1520` - Fixing UB whil registering polymorphic classes for
  serialization
* :hpx-pr:`1519` - Making detail::condition_variable safe to use
* :hpx-pr:`1518` - Fix when_some bug missing indices in its result
* :hpx-issue:`1517` - Typo may affect CMake build system tests
* :hpx-pr:`1516` - Fixing Posix context
* :hpx-pr:`1515` - Fixing Posix context
* :hpx-pr:`1514` - Correct problems with loading dynamic components
* :hpx-pr:`1513` - Fixing intel glibc4 4
* :hpx-issue:`1508` - memory and papi counters do not work
* :hpx-issue:`1507` - Unrecognized Command Line Option Error causing exit status
  0
* :hpx-issue:`1506` - Properly handle deferred futures
* :hpx-pr:`1505` - Adding #include - would not compile without this
* :hpx-issue:`1502` - ``boost::filesystem::exists`` throws unexpected exception
* :hpx-issue:`1501` - hwloc configuration options are wrong for MIC
* :hpx-pr:`1504` - Making sure boost::filesystem::exists() does not throw
* :hpx-pr:`1500` - Exit application on ``--hpx:version``/``-v`` and
  ``--hpx:info``
* :hpx-pr:`1498` - Extended task block
* :hpx-pr:`1497` - Unique ptr serialization
* :hpx-pr:`1496` - Unique ptr serialization (closed)
* :hpx-pr:`1495` - Switching circleci build type to debug
* :hpx-issue:`1494` - ``--hpx:version``/``-v`` does not exit after printing
  version information
* :hpx-issue:`1493` - add an ``hpx_`` prefix to libraries and components to
  avoid name conflicts
* :hpx-issue:`1492` - Define and ensure limitations for arguments to async/apply
* :hpx-pr:`1489` - Enable idle rate counter on demand
* :hpx-pr:`1488` - Made sure ``detail::condition_variable`` can be safely
  destroyed
* :hpx-pr:`1487` - Introduced default (main) template implementation for
  ``ignore_while_checking``
* :hpx-pr:`1486` - Add HPX inspect tool
* :hpx-issue:`1485` - ``ignore_while_locked`` doesn't support all Lockable types
* :hpx-pr:`1484` - Docker image generation
* :hpx-pr:`1483` - Move external endian library into HPX
* :hpx-pr:`1482` - Actions with integer type ids
* :hpx-issue:`1481` - Sync primitives safe destruction
* :hpx-issue:`1480` - Move external/boost/endian into hpx/util
* :hpx-issue:`1478` - Boost inspect violations
* :hpx-pr:`1479` - Adds serialization for arrays; some further/minor fixes
* :hpx-pr:`1477` - Fixing problems with the Intel compiler using a GCC 4.4 std
  library
* :hpx-pr:`1476` - Adding ``hpx::lcos::latch`` and ``hpx::lcos::local::latch``
* :hpx-issue:`1475` - Boost inspect violations
* :hpx-pr:`1473` - Fixing action move tests
* :hpx-issue:`1471` - Sync primitives should not be movable
* :hpx-pr:`1470` - Removing ``hpx::util::polymorphic_factory``
* :hpx-pr:`1468` - Fixed container creation
* :hpx-issue:`1467` - HPX application fail during finalization
* :hpx-issue:`1466` - HPX doesn't pick up Torque's nodefile on SuperMIC
* :hpx-issue:`1464` - HPX option for pre and post bootstrap performance counters
* :hpx-pr:`1463` - Replacing ``async_colocated(id, ...)`` with
  ``async(colocated(id), ...)``
* :hpx-pr:`1462` - Consolidated task_region with N4411
* :hpx-pr:`1461` - Consolidate inconsistent CMake option names
* :hpx-issue:`1460` - Which malloc is actually used? or at least which one is
  HPX built with
* :hpx-issue:`1459` - Make cmake configure step fail explicitly if compiler
  version is not supported
* :hpx-issue:`1458` - Update ``parallel::task_region`` with N4411
* :hpx-pr:`1456` - Consolidating ``new_<>()``
* :hpx-issue:`1455` - Replace ``async_colocated(id, ...)`` with
  ``async(colocated(id), ...)``
* :hpx-pr:`1454` - Removed harmful std::moves from return statements
* :hpx-pr:`1453` - Use range-based for-loop instead of Boost.Foreach
* :hpx-pr:`1452` - C++ feature tests
* :hpx-pr:`1451` - When serializing, pass archive flags to traits::get_type_size
* :hpx-issue:`1450` - traits:get_type_size needs archive flags to enable
  zero_copy optimizations
* :hpx-issue:`1449` - "couldn't create performance counter" - AGAS
* :hpx-issue:`1448` - Replace distributing factories with ``new_<T[]>(...)``
* :hpx-pr:`1447` - Removing obsolete remote_object component
* :hpx-pr:`1446` - Hpx serialization
* :hpx-pr:`1445` - Replacing travis with circleci
* :hpx-pr:`1443` - Always stripping HPX command line arguments before executing
  start function
* :hpx-pr:`1442` - Adding --hpx:bind=none to disable thread affinities
* :hpx-issue:`1439` - Libraries get linked in multiple times, RPATH is not
  properly set
* :hpx-pr:`1438` - Removed superfluous typedefs
* :hpx-issue:`1437` - ``hpx::init()`` should strip HPX-related flags from argv
* :hpx-issue:`1436` - Add strong scaling option to htts
* :hpx-pr:`1435` - Adding ``async_cb``, ``async_continue_cb``, and
  ``async_colocated_cb``
* :hpx-pr:`1434` - Added missing install rule, removed some dead CMake code
* :hpx-pr:`1433` - Add GitExternal and SubProject cmake scripts from
  eyescale/cmake repo
* :hpx-issue:`1432` - Add command line flag to disable thread pinning
* :hpx-pr:`1431` - Fix #1423
* :hpx-issue:`1430` - Inconsistent CMake option names
* :hpx-issue:`1429` - Configure setting ``HPX_HAVE_PARCELPORT_MPI`` is ignored
* :hpx-pr:`1428` - Fixes #1419 (closed)
* :hpx-pr:`1427` - Adding stencil_iterator and transform_iterator
* :hpx-pr:`1426` - Fixes #1419
* :hpx-pr:`1425` - During serialization memory allocation should honour
  allocator chunk size
* :hpx-issue:`1424` - chunk allocation during serialization does not use memory
  pool/allocator chunk size
* :hpx-issue:`1423` - Remove ``HPX_STD_UNIQUE_PTR``
* :hpx-issue:`1422` - hpx:threads=all allocates too many os threads
* :hpx-pr:`1420` - added .travis.yml
* :hpx-issue:`1419` - Unify enums: ``hpx::runtime::state`` and ``hpx::state``
* :hpx-pr:`1416` - Adding travis builder
* :hpx-issue:`1414` - Correct directory for dispatch_gcc46.hpp iteration
* :hpx-issue:`1410` - Set operation algorithms
* :hpx-issue:`1389` - Parallel algorithms relying on scan partitioner break for
  small number of elements
* :hpx-issue:`1325` - Exceptions thrown during parcel handling are not handled
  correctly
* :hpx-issue:`1315` - Errors while running performance tests
* :hpx-issue:`1309` - ``hpx::vector`` partitions are not easily extendable by
  applications
* :hpx-pr:`1300` - Added serialization/de-serialization to examples.tuplespace
* :hpx-issue:`1251` - hpx::threads::get_thread_count doesn't consider pending
  threads
* :hpx-issue:`1008` - Decrease in application performance overtime; occasional
  spikes of major slowdown
* :hpx-issue:`1001` - Zero copy serialization raises assert
* :hpx-issue:`721` - Make HPX usable for Xeon Phi
* :hpx-issue:`524` - Extend scheduler to support threads which can't be stolen

