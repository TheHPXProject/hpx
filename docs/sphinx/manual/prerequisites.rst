..
    Copyright (c) 2021 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _prerequisites:

=============
Prerequisites
=============

Supported platforms
===================

At this time, |hpx| supports the following platforms. Other platforms may
work, but we do not test |hpx| with other platforms, so please be warned.

.. table:: Supported Platforms for |hpx|

   ========= ================== ====================
   Name      Minimum Version    Architectures
   ========= ================== ====================
   Linux     2.6                x86-32, x86-64, k1om
   BlueGeneQ V1R2M0             PowerPC A2
   Windows   Any Windows system x86-32, x86-64
   Mac OSX   Any OSX system     x86-64
   ARM       Any ARM system     Any architecture
   RISC-V    Any RISC-V system  Any architecture
   ========= ================== ====================

Supported compilers
===================

The table below shows the supported compilers for |hpx|.

.. table:: Supported Compilers for |hpx|

   =================== ================== ==================
   Name                Minimum Version    Latest tested
   =================== ================== ==================
   |gcc|_              12.0               15.0
   |clang|_            16.0               20.0
   |visual_cxx|_ (x64) 2022               2022
   =================== ================== ==================

Software and libraries
======================

The table below presents all the necessary prerequisites for building |hpx|.

.. table:: Software prerequisites for |hpx|

   ====================== =================== ================== ==================
   \                      Name                Minimum Version    Latest tested
   ====================== =================== ================== ==================
   **Build System**       |cmake|_            3.18               4.1
   **Required Libraries** |boost|_            1.71.0             1.88.0
   \                      |hwloc|_            1.5                2.4
   ====================== =================== ================== ==================

C++20 module builds (``HPX_WITH_CXX_MODULES=ON``, default ``OFF``) require
CMake 3.28 or newer, a Ninja or Visual Studio generator, and GCC 14+, LLVM
Clang 16+, or Visual Studio 17.4+. AppleClang is not supported for module
builds.

The most important dependencies are |boost|_ and |hwloc|_. The installation of Boost
is described in detail in Boost's `Getting Started <https://www.boost.org/more/getting_started/index.html>`_
document. A recent version of hwloc is required in order to support thread
pinning and NUMA awareness and can be found in |hwloc_downloads|_.

|hpx| is written in 99.99% Standard C++ (the remaining 0.01% is platform
specific assembly code). As such, |hpx| is compilable with almost any standards
compliant C++ compiler. The code base takes advantage of C++ language and
standard library features when available.

.. note::

   When building Boost using gcc, please note that it is required to specify a
   ``cxxflags=-std=c++20`` command line argument to ``b2`` (``bjam``).

.. note::

   In most configurations, |hpx| depends only on header-only Boost.
   Boost.Filesystem is required if the standard library does not support
   filesystem. The following are not needed by default, but are required in
   certain configurations: Boost.Chrono, Boost.DateTime, Boost.Log,
   Boost.LogSetup, Boost.Regex, and Boost.Thread.

Depending on the options you chose while building and installing |hpx|,
you will find that |hpx| may depend on several other libraries such as those
listed below.

.. note::

   In order to use a high speed parcelport, we currently recommend configuring
   |hpx| to use MPI so that MPI can be used for communication between different
   localities. Please set the CMake variable ``MPI_CXX_COMPILER`` to your MPI
   C++ compiler wrapper if not detected automatically.

.. list-table:: Optional software prerequisites for |hpx|

   * * Name
     * Minimum version
   * * |google_perftools|_
     * 1.7.1
   * * |jemalloc|_
     * 2.1.0
   * * |mimalloc|_
     * 1.0.0
   * * |papi|
     *
