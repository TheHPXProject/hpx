..
    Copyright (C) 2007-2018 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _hpx_0_9_10:

============================
|hpx| V0.9.10 (Mar 24, 2015)
============================

General changes
===============

This is the 12th official release of |hpx|. It coincides with the 7th
anniversary of the first commit to our source code repository. Since then, we
have seen over 12300 commits amounting to more than 220000 lines of C++ code.

The major focus of this release was to improve the reliability of large scale
runs. We believe to have achieved this goal as we now can reliably run |hpx|
applications on up to ~24k cores. We have also shown that HPX can be used with
success for symmetric runs (applications using both, host cores and Intel
Xeon/Phi coprocessors). This is a huge step forward in terms of the usability of
|hpx|. The main focus of this work involved isolating the causes of the
segmentation faults at start up and shut down. Many of these issues were
discovered to be the result of the suspension of threads which hold locks.

A very important improvement introduced with this release is the refactoring of
the code representing our parcel-port implementation. Parcel- ports can now be
implemented by 3rd parties as independent plugins which are dynamically loaded
at runtime (static linking of parcel-ports is also supported). This refactoring
also includes a massive improvement of the performance of our existing
parcel-ports. We were able to significantly reduce the networking latencies and
to improve the available networking bandwidth. Please note that in this release
we disabled the ibverbs and ipc parcel ports as those have not been ported to
the new plugin system yet (see :hpx-issue:`839`).

Another corner stone of this release is our work towards a complete
implementation of __cpp11_n4104__ (Working Draft, Technical Specification for
C++ Extensions for Parallelism). This document defines a set of parallel
algorithms to be added to the C++ standard library. We now have implemented
about 75% of all specified parallel algorithms (see [link
hpx.manual.parallel.parallel_algorithms Parallel Algorithms] for more details).
We also implemented some extensions to __cpp11_n4104__ allowing to invoke all of
the algorithms asynchronously.

This release adds a first implementation of ``hpx::vector`` which is a
distributed data structure closely aligned to the functionality of
``std::vector``. The difference is that ``hpx::vector`` stores the data in
partitions where the partitions can be distributed over different localities. We
started to work on allowing to use the parallel algorithms with ``hpx::vector``.
At this point we have implemented only a few of the parallel algorithms to
support distributed data structures (like ``hpx::vector``) for testing purposes
(see :hpx-issue:`1338` for a documentation of our progress).

Breaking changes
================

With this release we put a lot of effort into changing the code base to be more
compatible to C++11. These changes have caused the following issues for backward
compatibility:

* Move to Variadics- All of the API now uses variadic templates. However, this
  change required to modify the argument sequence for some of the exiting API
  functions (:hpx:func:`hpx::async_continue`, :hpx:func:`hpx::apply_continue`,
  :hpx:func:`hpx::when_each`, :hpx:func:`hpx::wait_each`, synchronous invocation
  of actions).
* Changes to Macros- We also removed the macros ``HPX_STD_FUNCTION`` and
  ``HPX_STD_TUPLE``. This shouldn't affect any user code as we replaced
  ``HPX_STD_FUNCTION`` with ``hpx::util::function_nonser`` which was the default
  expansion used for this macro. All |hpx| API functions which expect a
  ``hpx::util::function_nonser`` (or a ``hpx::util::unique_function_nonser``)
  can now be transparently called with a compatible ``std::function`` instead.
  Similarly, ``HPX_STD_TUPLE`` was replaced by its default expansion as well:
  ``hpx::util::tuple``.
* Changes to ``hpx::unique_future``- ``hpx::unique_future``, which was
  deprecated in the previous release for :hpx:func:`hpx::future` is now
  completely removed from |hpx|. This completes the transition to a completely
  standards conforming implementation of ``hpx::future``.
* Changes to Supported Compilers. Finally, in order to utilize more C++11
  semantics, we have officially dropped support for GCC 4.4 and MSVC 2012.
  Please see our :ref:`prerequisites` page for more details.

Bug fixes (closed tickets)
==========================

Here is a list of the important tickets we closed for this release.

* :hpx-issue:`1402` - Internal shared_future serialization copies
* :hpx-issue:`1399` - Build takes unusually long time...
* :hpx-issue:`1398` - Tests using the scan partitioner are broken on at least
  gcc 4.7 and intel compiler
* :hpx-issue:`1397` - Completely remove hpx::unique_future
* :hpx-issue:`1396` - Parallel scan algorithms with different initial values
* :hpx-issue:`1395` - Race Condition - 1d_stencil_8 - SuperMIC
* :hpx-issue:`1394` - "suspending thread while at least one lock is being
  held" - 1d_stencil_8 - SuperMIC
* :hpx-issue:`1393` - SEGFAULT in 1d_stencil_8 on SuperMIC
* :hpx-issue:`1392` - Fixing #1168
* :hpx-issue:`1391` - Parallel Algorithms for scan partitioner for small number
  of elements
* :hpx-issue:`1387` - Failure with more than 4 localities
* :hpx-issue:`1386` - Dispatching unhandled exceptions to outer user code
* :hpx-issue:`1385` - Adding Copy algorithms, fixing ``parallel::copy_if``
* :hpx-issue:`1384` - Fixing 1325
* :hpx-issue:`1383` - Fixed #504: Refactor Dataflow LCO to work with futures,
  this removes the dataflow component as it is obsolete
* :hpx-issue:`1382` - ``is_sorted``, ``is_sorted_until`` and ``is_partitioned``
  algorithms
* :hpx-issue:`1381` - fix for CMake versions prior to 3.1
* :hpx-issue:`1380` - resolved warning in CMake 3.1 and newer
* :hpx-issue:`1379` - Compilation error with papi
* :hpx-issue:`1378` - Towards safer migration
* :hpx-issue:`1377` - HPXConfig.cmake should include ``TCMALLOC_LIBRARY`` and
  ``TCMALLOC_INCLUDE_DIR``
* :hpx-issue:`1376` - Warning on uninitialized member
* :hpx-issue:`1375` - Fixing 1163
* :hpx-issue:`1374` - Fixing the MSVC 12 release builder
* :hpx-issue:`1373` - Modifying parallel search algorithm for zero length
  searches
* :hpx-issue:`1372` - Modifying parallel search algorithm for zero length
  searches
* :hpx-issue:`1371` - Avoid holding a lock during agas::incref while doing a
  credit split
* :hpx-issue:`1370` - ``--hpx:bind`` throws unexpected error
* :hpx-issue:`1369` - Getting rid of (void) in loops
* :hpx-issue:`1368` - Variadic templates support for tuple
* :hpx-issue:`1367` - One last batch of variadic templates support
* :hpx-issue:`1366` - Fixing symbolic namespace hang
* :hpx-issue:`1365` - More held locks
* :hpx-issue:`1364` - Add counters 1363
* :hpx-issue:`1363` - Add thread overhead counters
* :hpx-issue:`1362` - Std config removal
* :hpx-issue:`1361` - Parcelport plugins
* :hpx-issue:`1360` - Detuplify transfer_action
* :hpx-issue:`1359` - Removed obsolete checks
* :hpx-issue:`1358` - Fixing 1352
* :hpx-issue:`1357` - Variadic templates support for runtime_support and
  components
* :hpx-issue:`1356` - fixed coordinate test for intel13
* :hpx-issue:`1355` - fixed coordinate.hpp
* :hpx-issue:`1354` - Lexicographical Compare completed
* :hpx-issue:`1353` - HPX should set ``Boost_ADDITIONAL_VERSIONS`` flags
* :hpx-issue:`1352` - Error: Cannot find action '' in type registry:
  HPX(bad_action_code)
* :hpx-issue:`1351` - Variadic templates support for appliers
* :hpx-issue:`1350` - Actions simplification
* :hpx-issue:`1349` - Variadic when and wait functions
* :hpx-issue:`1348` - Added hpx_init header to test files
* :hpx-issue:`1347` - Another batch of variadic templates support
* :hpx-issue:`1346` - Segmented copy
* :hpx-issue:`1345` - Attempting to fix hangs during shutdown
* :hpx-issue:`1344` - Std config removal
* :hpx-issue:`1343` - Removing various distribution policies for hpx::vector
* :hpx-issue:`1342` - Inclusive scan
* :hpx-issue:`1341` - Exclusive scan
* :hpx-issue:`1340` - Adding ``parallel::count`` for distributed data
  structures, adding tests
* :hpx-issue:`1339` - Update argument order for transform_reduce
* :hpx-issue:`1337` - Fix dataflow to handle properly ranges of futures
* :hpx-issue:`1336` - dataflow needs to hold onto futures passed to it
* :hpx-issue:`1335` - Fails to compile with msvc14
* :hpx-issue:`1334` - Examples build problem
* :hpx-issue:`1333` - Distributed transform reduce
* :hpx-issue:`1332` - Variadic templates support for actions
* :hpx-issue:`1331` - Some ambiguous calls of map::erase have been prevented by
  adding additional check in locality constructor.
* :hpx-issue:`1330` - Defining Plain Actions does not work as described in the
  documentation
* :hpx-issue:`1329` - Distributed vector cleanup
* :hpx-issue:`1328` - Sync docs and comments with code in hello_world example
* :hpx-issue:`1327` - Typos in docs
* :hpx-issue:`1326` - Documentation and code diverged in Fibonacci tutorial
* :hpx-issue:`1325` - Exceptions thrown during parcel handling are not handled
  correctly
* :hpx-issue:`1324` - fixed bandwidth calculation
* :hpx-issue:`1323` - mmap() failed to allocate thread stack due to insufficient
  resources
* :hpx-issue:`1322` - HPX fails to build aa182cf
* :hpx-issue:`1321` - Limiting size of outgoing messages while coalescing
  parcels
* :hpx-issue:`1320` - passing a future with launch::deferred in remote function
  call causes hang
* :hpx-issue:`1319` - An exception when tries to specify number high priority
  threads with abp-priority
* :hpx-issue:`1318` - Unable to run program with abp-priority and
  numa-sensitivity enabled
* :hpx-issue:`1317` - N4071 Search/Search_n finished, minor changes
* :hpx-issue:`1316` - Add config option to make -Ihpx.run_hpx_main!=1 the
  default
* :hpx-issue:`1314` - Variadic support for async and apply
* :hpx-issue:`1313` - Adjust when_any/some to the latest proposed interfaces
* :hpx-issue:`1312` - Fixing #857: hpx::naming::locality leaks parcelport
  specific information into the public interface
* :hpx-issue:`1311` - Distributed get'er/set'er_values for distributed vector
* :hpx-issue:`1310` - Crashing in
  hpx::parcelset::policies::mpi::connection_handler::handle_messages() on
  SuperMIC
* :hpx-issue:`1308` - Unable to execute an application with --hpx:threads
* :hpx-issue:`1307` - merge_graph linking issue
* :hpx-issue:`1306` - First batch of variadic templates support
* :hpx-issue:`1305` - Create a compiler wrapper
* :hpx-issue:`1304` - Provide a compiler wrapper for hpx
* :hpx-issue:`1303` - Drop support for GCC44
* :hpx-issue:`1302` - Fixing #1297
* :hpx-issue:`1301` - Compilation error when tried to use boost range iterators
  with wait_all
* :hpx-issue:`1298` - Distributed vector
* :hpx-issue:`1297` - Unable to invoke component actions recursively
* :hpx-issue:`1294` - HDF5 build error
* :hpx-issue:`1275` - The parcelport implementation is non-optimal
* :hpx-issue:`1267` - Added classes and unit tests for local_file, orangefs_file
  and pxfs_file
* :hpx-issue:`1264` - Error "assertion '!m_fun' failed" randomly occurs when
  using TCP
* :hpx-issue:`1254` - thread binding seems to not work properly
* :hpx-issue:`1220` - parallel::copy_if is broken
* :hpx-issue:`1217` - Find a better way of fixing the issue patched by #1216
* :hpx-issue:`1168` - Starting HPX on Cray machines using aprun isn't working
  correctly
* :hpx-issue:`1085` - Replace startup and shutdown barriers with broadcasts
* :hpx-issue:`981` - With SLURM, --hpx:threads=8 should not be necessary
* :hpx-issue:`857` - hpx::naming::locality leaks parcelport specific information
  into the public interface
* :hpx-issue:`850` - "flush" not documented
* :hpx-issue:`763` - Create buildbot instance that uses std::bind as
  HPX_STD_BIND
* :hpx-issue:`680` - Convert parcel ports into a plugin system
* :hpx-issue:`582` - Make exception thrown from HPX threads available from
  ``hpx::init``
* :hpx-issue:`504` - Refactor Dataflow LCO to work with futures
* :hpx-issue:`196` - Don't store copies of the locality network metadata in the
  gva table
