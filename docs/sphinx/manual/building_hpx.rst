..
    Copyright (c) 2021 Dimitra Karatza
    Copyright (c) 2015 Adrian Serio
    Copyright (c) 2015 Harris Brakmic
    Copyright (C) 2014 Thomas Heller
    Copyright (C) 2007-2013 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _building_hpx:

==============
Building |hpx|
==============

.. _info:

Basic information
=================

The build system for |hpx| is based on |cmake|_, a cross-platform
build-generator tool which is not responsible for building the project
but rather generates the files needed by your build tool (GNU make, Visual
Studio, etc.) for building |hpx|. If CMake is not already installed in your
system, you can download it and install it here: |cmake_download|_.

Once |cmake|_ has been run, the build process can be started. The build process consists of the following parts:

* The |hpx| core libraries (target ``core``): This forms the basic set of |hpx|
  libraries.
* |hpx| Examples (target ``examples``): This target is enabled by default and
  builds all |hpx| examples (disable by setting
  :option:`HPX_WITH_EXAMPLES:BOOL`\ ``=Off``). |hpx| examples are part of the
  ``all`` target and are included in the installation if enabled.
* |hpx| Tests (target ``tests``): This target builds the |hpx| test suite and is
  enabled by default (disable by setting :option:`HPX_WITH_TESTS:BOOL`
  ``=Off``). They are not built by the ``all`` target and have to be built
  separately.
* |hpx| Documentation (target ``docs``): This target builds the documentation,
  and is not enabled by default (enable by setting
  :option:`HPX_WITH_DOCUMENTATION:BOOL`\ ``=On``. For more information see
  :ref:`documentation`.


The |hpx| build process is highly configurable through |cmake|_, and various |cmake|_ variables
influence the build process. A list with the most important |cmake|_ variables can be found in
the section that follows, while the complete list of available |cmake|_ variables is in
:ref:`cmake_variables`. These variables can be used to refine the recipes that can be found at
:ref:`build_recipes`, a section that shows some basic steps on how to build |hpx| for a
specific platform.

In order to use |hpx|, only the core libraries are required. In order to use the optional
libraries, you need to specify them as link dependencies in your build (See
:ref:`creating_hpx_projects`).

.. _important_cmake_options:

Most important |cmake| options
==============================

While building |hpx|, you are provided with multiple CMake options which correspond
to different configurations. Below, there is a set of the most important and frequently
used CMake options.

.. option:: HPX_WITH_MALLOC

   Use a custom allocator. Using a custom allocator tuned for multithreaded applications is very
   important for the performance of |hpx| applications. When debugging applications, it's useful to set
   this to ``system``, as custom allocators can hide some memory-related bugs. Note that setting this to
   something other than ``system`` requires an external dependency.

.. option:: HPX_WITH_CUDA

   Enable support for CUDA. Use ``CMAKE_CUDA_COMPILER`` to set the CUDA compiler. This is a standard
   |cmake|_ variable, like ``CMAKE_CXX_COMPILER``.

.. option:: HPX_WITH_PARCELPORT_MPI

   Enable the MPI parcelport. This enables the use of MPI for the networking operations in the HPX runtime.
   The default value is ``OFF`` because it's not available on all systems and/or requires another dependency. However,
   it is the recommended parcelport.

.. option:: HPX_WITH_PARCELPORT_TCP

   Enable the TCP parcelport. Enables the use of TCP for networking in the runtime. The default value is ``ON``.
   However, it's only recommended for debugging purposes, as it is slower than the MPI parcelport.

.. option:: HPX_WITH_PARCELPORT_LCI

   Enable the LCI parcelport. This enables the use of LCI for the networking operations in the HPX runtime.
   The default value is ``OFF`` because it's not available on all systems and/or requires another dependency. However,
   this experimental parcelport may provide better performance than the MPI parcelport. Please refer to
   :ref:`using_the_lci_parcelport` for more information about the LCI parcelport.

.. option:: HPX_WITH_APEX

   Enable APEX integration. `APEX <https://uo-oaciss.github.io/apex/quickstarthpx/>`_ can be used to profile |hpx|
   applications. In particular, it provides information about individual tasks in the |hpx| runtime.

.. option:: HPX_WITH_TRACY

   Enable |tracy|_ integration. Tracy shows task lifecycle, work stealing, suspension, and (with
   :option:`HPX_WITH_PARCEL_PROFILING`) distributed parcel events. See :ref:`optimizing_with_tracy` for
   build and attach instructions.

.. option:: HPX_WITH_ITTNOTIFY

   Enable |ittnotify|_ integration. Requires VTune or any other |ittnotify|
   provider; point ``Amplifier_ROOT`` at the install (the name is legacy;
   it accepts any ITTNotify installation).

.. option:: HPX_WITH_PARCEL_PROFILING

   Enable per-parcel profiling data. Adds per-parcel identifier and timestamp fields (creation time,
   start time, parcel id) that are carried on the wire and surfaced to whichever tracing backend is
   enabled. Defaults to ``ON`` when :option:`HPX_WITH_APEX` is on, ``OFF`` otherwise.

.. option:: HPX_WITH_TRACING_LIFECYCLE_EVENTS

   Emit per-task lifecycle events (staged, created, executing, yielded, suspended, resumed, completed,
   deleted). Defaults to ``ON``. Turn ``OFF`` to compile out the per-task hooks and their runtime
   connection check when profiling a workload where only causal or work-stealing events matter. The
   saving applies on Tracy builds; the other backends already treat these hooks as no-ops.

.. option:: HPX_WITH_TRACING_CAUSAL_EVENTS

   Emit causal-chain events (``future_fulfilled``, ``future_exception_set``, ``continuation_run``,
   ``continuation_finished``, ``handle_on_completed_fired``). Defaults to ``ON``. Turning this ``OFF``
   also elides the active-continuations counter update in ``continuation_run`` /
   ``continuation_finished``. The saving applies on Tracy builds; the other backends already treat
   these hooks as no-ops.

.. option:: HPX_WITH_TRACING_WORK_STEALING_EVENTS

   Emit the work-stealing signal when a worker steals a task from another worker. Defaults to ``ON``.
   Turn ``OFF`` when profiling a workload with heavy work stealing where the signal itself is not
   being consumed. The saving applies on Tracy builds; the other backends already treat this hook as
   a no-op.

.. option:: HPX_WITH_GENERIC_CONTEXT_COROUTINES

   Enable Boost. Context for task context switching. It must be enabled for non-x86 architectures such as ARM and Power.

.. option:: HPX_WITH_MAX_CPU_COUNT

   Set the maximum CPU count supported by |hpx|. The default value is 64, and should be set to a number at least as
   high as the number of cores on a system including virtual cores such as hyperthreads.

.. option:: HPX_WITH_CXX_STANDARD

   Set a specific C++ standard version e.g. ``HPX_WITH_CXX_STANDARD=23``.
   The default and minimum value is ``20``. Possible values are ``20``, ``23``, or ``26``.

.. option:: HPX_WITH_EXAMPLES

   Build examples.

.. option:: HPX_WITH_TESTS

   Build tests.

.. option:: HPX_WITH_DEBUG_POSTFIX

   Set the postfix for debug libraries. The default is ``d``. This variable is used to set ``CMAKE_DEBUG_POSTFIX`` and is only relevant for Debug builds or multi-configuration generators. The default rarely needs to be changed. It ensures that generated debug binaries have a different name than release binaries, which is important to avoid ABI problems when both debug and release binaries are installed on the same system.

For a complete list of available |cmake|_ variables that influence the build of
|hpx|, see :ref:`cmake_variables`.

.. _build_types:

Build types
===========

|cmake|_ can be configured to generate project files suitable for builds that
have enabled debugging support or for an optimized build (without debugging
support). The |cmake|_ variable used to set the build type is
``CMAKE_BUILD_TYPE`` (for more information see the `CMake Documentation
<https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html>`_).
Available build types are:

* **Debug**: Full debug symbols are available as well as additional assertions to
  help debugging. To enable the debug build type for the |hpx| API, the C++ Macro
  ``HPX_DEBUG`` is defined.
* **RelWithDebInfo**: Release build with debugging symbols. This is most useful
  for profiling applications
* **Release**: Release build. This disables assertions and enables default
  compiler optimizations.
* **RelMinSize**: Release build with optimizations for small binary sizes.

.. important::

   We currently don't guarantee ABI compatibility between Debug and Release
   builds. Please make sure that applications built against |hpx| use the same
   build type as you used to build |hpx|. For CMake builds, this means that
   the ``CMAKE_BUILD_TYPE`` variables have to match and for projects not using
   |cmake|_, the ``HPX_DEBUG`` macro has to be set in debug mode.

.. _cmake_presets:

Using CMake Presets
===================

|hpx| provides a ``CMakePresets.json`` file which includes a variety of pre-defined build configurations.
These presets allow you to easily configure the build for common scenarios without needing to manually specify multiple CMake variables.

To use a preset, you can use the ``--preset`` option with CMake:

.. code-block:: shell-session

    $ cmake --preset <preset-name>
    $ cmake --build --preset <preset-name>

Some of the available presets include:

* ``default``: Standard release build with tests and examples enabled.
* ``minimal``: Minimal build with only core features (no tests, examples, or tools).
* ``full``: Full build with all standard features enabled.
* ``debug``: Debug build with symbols and debug-optimized settings.
* ``performance``: Build optimized for performance analysis with APEX profiling.
* ``cuda``: Build with CUDA support (requires CUDA toolkit).
* ``sycl``: Build with SYCL support (requires compatible compiler).
* ``sanitizer-address``: Build with AddressSanitizer enabled.

For a full list of available presets, you can run:

.. code-block:: shell-session

    $ cmake --list-presets

.. _build_recipes:

Platform specific build recipes
===============================

.. _unix_installation:

Unix variants
-------------

Once you have the source code and the dependencies and assuming all your dependencies are in paths
known to |cmake|_, the following gets you started:

#. First, set up a separate build directory to configure the project:

   .. code-block:: shell-session

      $ mkdir build && cd build

#. To configure the project you have the following options:

   * To build the core |hpx| libraries and examples, and install them to your chosen location (recommended):

    .. code-block:: shell-session

        $ cmake -DCMAKE_INSTALL_PREFIX=/install/path ..

    .. tip::

       If you want to change |cmake|_ variables for your build, it is usually a good
       idea to start with a clean build directory to avoid configuration problems.
       It is especially important that you use a clean build directory when changing
       between ``Release`` and ``Debug`` modes.

   * To install |hpx| to the default system folders, simply leave out the ``CMAKE_INSTALL_PREFIX`` option:

    .. code-block:: shell-session

        $ cmake ..

   * If your dependencies are in custom locations, you may need to tell |cmake|_
     where to find them by passing one or more options to |cmake|_ as shown below:

    .. code-block:: shell-session

        $ cmake -DBoost_ROOT=/path/to/boost
              -DHwloc_ROOT=/path/to/hwloc
              -DTcmalloc_ROOT=/path/to/tcmalloc
              -DJemalloc_ROOT=/path/to/jemalloc
              [other CMake variable definitions]
              /path/to/source/tree

    For instance:

    .. code-block:: shell-session

        $ cmake -DBoost_ROOT=~/packages/boost -DHwloc_ROOT=/packages/hwloc -DCMAKE_INSTALL_PREFIX=~/packages/hpx ~/downloads/hpx_1.5.1

   * If you want to try |hpx| without using a custom allocator pass ``-DHPX_WITH_MALLOC=system`` to |cmake|_:

    .. code-block:: shell-session

        $ cmake -DCMAKE_INSTALL_PREFIX=/install/path -DHPX_WITH_MALLOC=system ..

    .. note::
       Please pay special attention to the section about :option:`HPX_WITH_MALLOC:STRING` as this is crucial for getting decent performance.

   .. important::

       If you are building |hpx| for a system with more than 64 processing units,
       you must change the |cmake|_ variable ``HPX_WITH_MAX_CPU_COUNT`` (to a value at least as big as the
       number of (virtual) cores on your system). Note that the default value is 64.

   .. caution::

       Compiling and linking |hpx| needs a considerable amount of memory. It is
       advisable that at least 2 GB of memory per parallel process is available.

#. Once the configuration is complete, to build the project you run:

  .. code-block:: shell-session

      $ cmake --build . --target install

.. _windows_installation:

Windows
-------

.. note::

   The following build recipes are mostly user-contributed and may be outdated.
   We always welcome updated and new build recipes.

To build |hpx| under Windows 10 x64 with Visual Studio 2015:

* Download the CMake V3.19 installer (or latest version) from `here
  <https://blog.kitware.com/cmake-3-19-0-available-for-download/>`__
* Download the hwloc V1.11.0 (or the latest version) from `here
  <https://www.open-mpi.org/software/hwloc/v2.11/>`__
  and unpack it.
* Download the latest Boost libraries from `here
  <https://www.boost.org/users/download/>`__ and unpack them.
* Build the Boost DLLs and LIBs by using these commands from Command Line (or
  PowerShell). Open CMD/PowerShell inside the Boost dir and type in:

  .. code-block:: bash

     .\bootstrap.bat

  This batch file will set up everything needed to create a successful build.
  Now execute:

  .. code-block:: bash

     .\b2.exe link=shared variant=release,debug architecture=x86 address-model=64 threading=multi --build-type=complete install

  This command will start a (very long) build of all available Boost libraries.
  Please, be patient.

* Open CMake-GUI.exe and set up your source directory (input field 'Where is the
  source code') to the *base directory* of the source code you downloaded from
  |hpx|'s GitHub pages. Here's an example of CMake path settings, which point to
  the ``Documents/GitHub/hpx`` folder:

  .. _win32_cmake_settings1:

  .. figure:: ../_static/images/cmake_settings1.png

     Example CMake path settings.

  Inside 'Where is the source-code' enter the base directory of your |hpx|
  source directory (do not enter the "src" sub-directory!). Inside 'Where to
  build the binaries' you should put in the path where all the building processes
  will happen. This is important because the building machinery will do an
  "out-of-tree" build. CMake will not touch or change the original source files
  in any way. Instead, it will generate Visual Studio Solution Files, which
  will build |hpx| packages out of the |hpx| source tree.

* Set new configuration variables (in CMake, not in Windows environment):
  ``Boost_ROOT``, ``Hwloc_ROOT``, ``Asio_ROOT``, ``CMAKE_INSTALL_PREFIX``. The meaning of
  these variables is as follows:

  * ``Boost_ROOT`` the |hpx| root directory of the unpacked Boost headers/cpp files.
  * ``Hwloc_ROOT`` the |hpx| root directory of the unpacked Portable Hardware Locality
    files.
  * ``Asio_ROOT`` the |hpx| root directory of the unpacked ASIO files. Alternatively use
    ``HPX_WITH_FETCH_ASIO`` with value ``True``.
  * ``CMAKE_INSTALL_PREFIX`` the |hpx| root directory where the future builds of |hpx|
    should be installed.

    .. note::

       |hpx| is a very large software collection, so it is not recommended to use the
       default ``C:\Program Files\hpx``. Many users may prefer to use simpler paths *without*
       whitespace, like ``C:\bin\hpx`` or ``D:\bin\hpx`` etc.

  To insert new env-vars click on "Add Entry" and then insert the name inside
  "Name", select ``PATH`` as Type and put the path-name in the "Path" text field.
  Repeat this for the first three variables.

  This is how variable insertion will look:

  .. _win32_cmake_settings2:

  .. figure:: ../_static/images/cmake_settings2.png

     Example CMake adding entry.

  Alternatively, users could provide ``Boost_LIBRARYDIR`` instead of
  ``Boost_ROOT``; the difference is that ``Boost_LIBRARYDIR`` should point to
  the subdirectory inside Boost root where all the compiled DLLs/LIBs are. For
  example, ``Boost_LIBRARYDIR`` may point to the ``bin.v2`` subdirectory under
  the Boost rootdir. It is important to keep the meanings of these two variables
  separated from each other: ``Boost_DIR`` points to the ROOT folder of the
  Boost library. ``Boost_LIBRARYDIR`` points to the subdir inside the Boost root
  folder where the compiled binaries are.

* Click the 'Configure' button of CMake-GUI. You will be immediately presented with a
  small window where you can select the C++ compiler to be used within Visual
  Studio. This has been tested using the latest v14 (a.k.a C++ 2015) but older
  versions should be sufficient too. Make sure to select the 64Bit compiler.

* After the generate process has finished successfully, click the 'Generate'
  button. Now, CMake will put new VS Solution files into the BUILD folder you
  selected at the beginning.

* Open Visual Studio and load the ``HPX.sln`` from your build folder.

* Go to ``CMakePredefinedTargets`` and build the ``INSTALL`` project:

  .. _win32_vs_targets:

  .. figure:: ../_static/images/vs_targets_install.png

     Visual Studio INSTALL target.

  It will take some time to compile everything, and in the end you should see an
  output similar to this one:

  .. _win32_vs_build_output:

  .. figure:: ../_static/images/vs_build_output.png

     Visual Studio build output.
