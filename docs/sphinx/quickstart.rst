..
    Copyright (C) 2018 Mikael Simberg

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _quickstart:

===========
Quick start
===========

The following steps will help you get started with |hpx|. Before getting started, make sure you have
all the necessary prerequisites, which are listed in :ref:`_prerequisites`. After :ref:`installing_hpx`,
you can check how to run a simple example :ref:`hello_world`. :ref:`writing_task_based_applications`
explains how you can get started with |hpx|. You can refer to our :ref:`migration_guide` if you use
other APIs for parallelism (like |openmp|, |mpi| or |tbb|) and you would like to convert your code to
|hpx| code.

.. _installing_hpx:

Installing |hpx|
================

The easiest way to install |hpx| on your system is by choosing one of the steps
below:

#. **vcpkg**

   You can download and install |hpx| using the `vcpkg
   <https://github.com/Microsoft/vcpkg>`_ dependency manager:

   .. code-block:: shell-session

      $ vcpkg install hpx

#. **Spack**

   Another way to install |hpx| is using
   `Spack <https://spack.readthedocs.io/en/latest/>`_:

   .. code-block:: shell-session

      $ spack install hpx

#. **Conan**

   You can also use the `Conan <https://conan.io/>`_ package manager to build
   and use |hpx| in your projects. First, export the |hpx| recipe from the |hpx|
   repository to your local Conan cache:

   .. code-block:: shell-session

      $ git clone https://github.com/TheHPXProject/hpx.git
      $ cd hpx
      $ conan create . --build=missing

   This builds |hpx| and makes it available in your local Conan cache. You can
   then use it in your Conan-based projects by adding it to your ``conanfile.txt``:

   .. code-block:: ini

      [requires]
      hpx/2.0.0

      [generators]
      CMakeDeps
      CMakeToolchain

   Then use ``find_package(HPX REQUIRED)`` in your ``CMakeLists.txt`` as usual.
   Conan will automatically handle building |hpx| with your chosen configuration
   options and provide the necessary CMake integration files.

#. **Fedora**

   Installation can be done with
   `Fedora <https://fedoraproject.org/wiki/DNF>`_ as well:

   .. code-block:: shell-session

      $ dnf install hpx*

#. **Arch Linux**

   |hpx| is available in the
   `Arch User Repository (AUR) <https://wiki.archlinux.org/title/Arch_User_Repository>`_
   as ``hpx`` too.

More information or alternatives regarding the installation can be found in the
:ref:`building_hpx`, a detailed guide with thorough explanation of ways to build
and use |hpx|.

.. _hello_world:

Hello, World!
=============

To get started with this minimal example you need to create a new project
directory and a file ``CMakeLists.txt`` with the contents below in order to
build an executable using |cmake|_ and |hpx|:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.19)
   project(my_hpx_project CXX)
   find_package(HPX REQUIRED)
   add_executable(my_hpx_program main.cpp)
   target_link_libraries(my_hpx_program HPX::hpx HPX::wrap_main hpx::iostreams_component)

The next step is to create a ``main.cpp`` with the contents below:

.. literalinclude:: ../examples/quickstart/hello_world_1.cpp
   :language: c++
   :start-after: //[hello_world_1_getting_started
   :end-before: //]

Then, in your project directory run the following:

.. code-block:: shell-session

   $ mkdir build && cd build
   $ cmake -DHPX_DIR=</path/to/hpx/installation> ..
   $ make all
   $ ./my_hpx_program

.. code-block:: shell-session

    $ ./my_hpx_program
    Hello World!

The program looks almost like a regular C++ hello world with the exception of
the two includes and ``hpx::cout``.

* When you include ``hpx_main.hpp`` |hpx| makes sure that ``main`` actually gets
  launched on the |hpx| runtime. So while it looks almost the same you can now use
  futures, ``async``, parallel algorithms and more which make use of the |hpx|
  runtime with lightweight threads.

* ``hpx::cout`` is a replacement for ``std::cout`` to make sure printing never blocks
  a lightweight thread. You can read more about ``hpx::cout`` in :ref:`iostreams`.

.. note::

   * You will most likely have more than one ``main.cpp`` file in your project.
     See the section on :ref:`using_hpx_cmake` for more details on how to use
     ``add_hpx_executable``.

   * ``HPX::wrap_main`` is required if you are implicitly using ``main()`` as the
     runtime entry point. See :ref:`minimal` for more information.

   * ``hpx::iostreams_component`` is optional for a minimal project but lets us
     use the |hpx| equivalent of ``std::cout``, i.e., the |hpx| :ref:`iostreams`
     functionality in our application.

   * You do not have to let |hpx| take over your main function like in the
     example. See :ref:`starting_hpx` for more details on how to initialize and run
     the |hpx| runtime.

.. caution::

   Ensure that |hpx| is installed with ``HPX_WITH_DISTRIBUTED_RUNTIME=ON`` to
   prevent encountering an error indicating that the ``HPX::iostreams_component``
   target is not found.

   When including ``hpx_main.hpp`` the user-defined ``main`` gets renamed and
   the real ``main`` function is defined by |hpx|. This means that the
   user-defined ``main`` must include a return statement, unlike the real
   ``main``. If you do not include the return statement, you may end up with
   confusing compile time errors mentioning ``user_main`` or even runtime
   errors.

.. _writing_task_based_applications:

Writing task-based applications
===============================

So far we haven't done anything that can't be done using the C++ standard
library. In this section we will give a short overview of what you can do with
|hpx| on a single node. The essence is to avoid global synchronization and break
up your application into small, composable tasks whose dependencies control the
flow of your application. Remember, however, that |hpx| allows you to write
distributed applications similarly to how you would write applications for a
single node (see :ref:`why_hpx` and
:ref:`writing_distributed_hpx_applications`).

If you are already familiar with ``async`` and ``future`` from the C++ standard
library, the same functionality is available in |hpx|.

The following terminology is essential when talking about task-based C++
programs:

* lightweight thread: Essential for good performance with task-based programs.
  Lightweight refers to smaller stacks and faster context switching compared to
  OS threads. Smaller overheads allow the program to be broken up into smaller
  tasks, which in turns helps the runtime fully utilize all processing units.

* ``async``: The most basic way of launching tasks asynchronously. Returns a
  ``future<T>``.

* ``future<T>``: Represents a value of type ``T`` that will be ready in the future.
  The value can be retrieved with ``get`` (blocking) and one can check if the
  value is ready with ``is_ready`` (non-blocking).

* ``shared_future<T>``: Same as ``future<T>`` but can be copied (similar to
  ``std::unique_ptr`` vs ``std::shared_ptr``).

* continuation: A function that is to be run after a previous task has run
  (represented by a future). ``then`` is a method of ``future<T>`` that takes a
  function to run next. Used to build up dataflow DAGs (directed acyclic
  graphs). ``shared_future``\ s help you split up nodes in the DAG and functions
  like ``when_all`` help you join nodes in the DAG.

The following example is a collection of the most commonly used functionality in
|hpx|:

.. literalinclude:: ../examples/quickstart/potpourri.cpp
   :language: c++
   :lines: 6-

Try copying the contents to your ``main.cpp`` file and look at the output. It can
be a good idea to go through the program step by step with a debugger. You can
also try changing the types or adding new arguments to functions to make sure
you can get the types to match. The type of the ``then`` method can be especially
tricky to get right (the continuation needs to take the future as an argument).

.. note::

   |hpx| programs accept command line arguments. The most important one is
   :option:`--hpx:threads`\ ``=N`` to set the number of OS threads used by
   |hpx|. |hpx| uses one thread per core by default. Play around with the
   example above and see what difference the number of threads makes on the
   ``sort`` function. See :ref:`launching_and_configuring` for more details on
   how and what options you can pass to |hpx|.

.. tip::

   The example above used the construction ``hpx::when_all(...).then(...)``. For
   convenience and performance it is a good idea to replace uses of
   ``hpx::when_all(...).then(...)`` with ``dataflow``. See
   :ref:`examples_interest_calculator` for more details on ``dataflow``.

.. tip::

   If possible, try to use the provided parallel algorithms instead of
   writing your own implementation. This can save you time and the resulting
   program is often faster.

Next steps
==========

If you haven't done so already, reading the :ref:`terminology` section will help
you get familiar with the terms used in |hpx|.

The :ref:`examples` section contains small, self-contained walkthroughs of
example |hpx| programs. The :ref:`examples_1d_stencil` example is a thorough,
realistic example starting from a single node implementation and going stepwise
to a distributed implementation.

The :ref:`manual` contains detailed information on writing, building and running
|hpx| applications.
