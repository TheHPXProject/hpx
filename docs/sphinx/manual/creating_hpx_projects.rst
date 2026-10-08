..
    Copyright (C) 2018 Mikael Simberg
    Copyright (C) 2014 Thomas Heller
    Copyright (C) 2007-2013 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _creating_hpx_projects:

=======================
Creating |hpx| projects
=======================

.. _pkgconfig:

Using HPX with pkg-config
=========================

.. _apps:

How to build |hpx| applications with pkg-config
-----------------------------------------------

After you are done installing |hpx|, you should be able to build the following
program. It prints ``Hello World!`` on the :term:`locality` you run it on.

.. literalinclude:: ../../examples/quickstart/hello_world_1.cpp
   :language: c++
   :start-after: //[hello_world_1_getting_started
   :end-before: //]

Copy the text of this program into a file called hello_world.cpp.

Now, in the directory where you put hello_world.cpp, issue the following
commands (where ``$HPX_LOCATION`` is the build directory or
``CMAKE_INSTALL_PREFIX`` you used while building |hpx|):

.. code-block:: shell-session

   $ export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HPX_LOCATION/lib/pkgconfig
   $ c++ -o hello_world hello_world.cpp \
      `pkg-config --cflags --libs hpx_application`\
       -lhpx_iostreams -DHPX_APPLICATION_NAME=hello_world

.. important::

   When using pkg-config with |hpx|, the pkg-config flags must go after the
   ``-o`` flag.

.. note::

   |hpx| libraries have different names in debug and release mode. If you want
   to link against a debug |hpx| library, you need to use the ``_debug`` suffix
   for the pkg-config name. That means instead of ``hpx_application`` or
   ``hpx_component``, you will have to use ``hpx_application_debug`` or
   ``hpx_component_debug`` Moreover, all referenced |hpx| components need to
   have an appended ``d`` suffix. For example, instead of ``-lhpx_iostreams`` you will
   need to specify ``-lhpx_iostreamsd``.

.. important::

    If the |hpx| libraries are in a path that is not found by the dynamic
    linker, you will need to add the path ``$HPX_LOCATION/lib`` to your linker search
    path (for example ``LD_LIBRARY_PATH`` on Linux).

To test the program, type:

.. code-block:: shell-session

   $ ./hello_world

which should print ``Hello World!`` and exit.

.. _comps:

How to build |hpx| components with pkg-config
---------------------------------------------

Let's try a more complex example involving an |hpx| component. An |hpx|
component is a class that exposes |hpx| actions. |hpx| components are compiled
into dynamically loaded modules called component libraries. Here's the source
code:

**hello_world_component.cpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_component.cpp
   :language: c++
   :start-after: //[hello_world_cpp_getting_started
   :end-before: //]

**hello_world_component.hpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_component.hpp
   :language: c++
   :start-after: //[hello_world_hpp_getting_started
   :end-before: //]

**hello_world_client.cpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_client.cpp
   :language: c++
   :start-after: //[hello_world_client_getting_started
   :end-before: //]

Copy the three source files above into three files (called
``hello_world_component.cpp``, ``hello_world_component.hpp`` and
``hello_world_client.cpp``, respectively).

Now, in the directory where you put the files, run the following command to
build the component library. (where ``$HPX_LOCATION`` is the build directory or
``CMAKE_INSTALL_PREFIX`` you used while building |hpx|):

.. code-block:: shell-session

   $ export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HPX_LOCATION/lib/pkgconfig
   $ c++ -o libhpx_hello_world.so hello_world_component.cpp \
      `pkg-config --cflags --libs hpx_component` \
       -lhpx_iostreams -DHPX_COMPONENT_NAME=hpx_hello_world

Now pick a directory in which to install your |hpx| component libraries. For
this example, we'll choose a directory named ``my_hpx_libs``:

.. code-block:: shell-session

   $ mkdir ~/my_hpx_libs
   $ mv libhpx_hello_world.so ~/my_hpx_libs

.. note::

   |hpx| libraries have different names in debug and release mode. If you want
   to link against a debug |hpx| library, you need to use the ``_debug`` suffix
   for the pkg-config name. That means instead of ``hpx_application`` or
   ``hpx_component`` you will have to use ``hpx_application_debug`` or
   ``hpx_component_debug``. Moreover, all referenced |hpx| components need to
   have a appended ``d`` suffix, e.g. instead of ``-lhpx_iostreams`` you will
   need to specify ``-lhpx_iostreamsd``.

.. important::

   If the |hpx| libraries are in a path that is not found by the dynamic linker.
   You need to add the path ``$HPX_LOCATION/lib`` to your linker search path
   (for example ``LD_LIBRARY_PATH`` on Linux).

Now, to build the application that uses this component (``hello_world_client.cpp``),
we do:

.. code-block:: shell-session

   $ export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HPX_LOCATION/lib/pkgconfig
   $ c++ -o hello_world_client hello_world_client.cpp \
      ``pkg-config --cflags --libs hpx_application``\
       -L${HOME}/my_hpx_libs -lhpx_hello_world -lhpx_iostreams

.. important::

   When using pkg-config with |hpx|, the pkg-config flags must go after the
   ``-o`` flag.

Finally, you'll need to set your LD_LIBRARY_PATH before you can run the program.
To run the program, type:

.. code-block:: shell-session

   $ export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$HOME/my_hpx_libs"
   $ ./hello_world_client

which should print ``Hello HPX World!`` and exit.

.. _using_hpx_cmake:

Using |hpx| with CMake-based projects
=====================================

In addition to the pkg-config support discussed on the previous pages, |hpx|
comes with full CMake support. In order to integrate |hpx| into existing or
new CMakeLists.txt, you can leverage the `find_package
<https://www.cmake.org/cmake/help/latest/command/find_package.html>`_ command
integrated into CMake. Following, is a Hello World component example using CMake.

Let's revisit what we have. We have three files that compose our example
application:

* ``hello_world_component.hpp``
* ``hello_world_component.cpp``
* ``hello_world_client.hpp``

The basic structure to include |hpx| into your CMakeLists.txt is shown here:

.. code-block:: cmake

   # Require a recent version of cmake
   cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

   # This project is C++ based.
   project(your_app CXX)

   # Instruct cmake to find the HPX settings
   find_package(HPX)

In order to have CMake find |hpx|, it needs to be told where to look for the
``HPXConfig.cmake`` file that is generated when |hpx| is built or installed. It is
used by ``find_package(HPX)`` to set up all the necessary macros needed to use
|hpx| in your project. The ways to achieve this are:

* Set the ``HPX_DIR`` CMake variable to point to the directory containing the
  ``HPXConfig.cmake`` script on the command line when you invoke CMake:

  .. code-block:: shell-session

     $ cmake -DHPX_DIR=$HPX_LOCATION/lib/cmake/HPX ...

  where ``$HPX_LOCATION`` is the build directory or ``CMAKE_INSTALL_PREFIX`` you
  used when building/configuring |hpx|.

* Set the ``CMAKE_PREFIX_PATH`` variable to the root directory of your |hpx|
  build or install location on the command line when you invoke CMake:

  .. code-block:: shell-session

     $ cmake -DCMAKE_PREFIX_PATH=$HPX_LOCATION ...

  The difference between ``CMAKE_PREFIX_PATH`` and ``HPX_DIR`` is that CMake
  will add common postfixes, such as ``lib/cmake/<project``, to the
  ``CMAKE_PREFIX_PATH`` and search in these locations too. Note that if your
  project uses |hpx| as well as other CMake-managed projects, the paths to the
  locations of these multiple projects may be concatenated in the
  ``CMAKE_PREFIX_PATH``.

* The variables above may be set in the CMake GUI or curses ccmake interface
  instead of the command line.

Additionally, if you wish to require |hpx| for your project, replace the
``find_package(HPX)`` line with ``find_package(HPX REQUIRED)``.

You can check if |hpx| was successfully found with the ``HPX_FOUND`` CMake variable.

.. _using_hpx_cmake_targets:

Using |cmake| targets
---------------------

The recommended way of setting up your targets to use |hpx| is to link to the
``HPX::hpx`` |cmake|_ target:

.. code-block:: cmake

   target_link_libraries(hello_world_component PUBLIC HPX::hpx)

This requires that you have already created the target like this:

.. code-block:: cmake

   add_library(hello_world_component SHARED hello_world_component.cpp)
   target_include_directories(hello_world_component PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

When you link your library to the ``HPX::hpx`` |cmake|_ target, you will be able
use |hpx| functionality in your library. To use ``main()`` as the implicit entry
point in your application you must additionally link your application to the
|cmake| target ``HPX::wrap_main``. This target is automatically linked to
executables if you are using the macros described below
(:ref:`using_hpx_cmake_macros`). See :ref:`minimal` for more information on
implicitly using ``main()`` as the entry point. If you want the same wrapping
behavior without including :hpx-header:`wrap/include,hpx/hpx_main.hpp`, link to
the ``HPX::auto_wrap_main`` target instead. This enables the runtime
initialization around ``main()`` unconditionally and is useful for codebases
where adding the header to ``main.cpp`` is impractical

.. note::

   When using the native Windows MSVC toolchain, ``HPX::auto_wrap_main``
   force-includes :hpx-header:`wrap/include,hpx/hpx_main.hpp` into all C++
   source files of the executable (using ``/FI``), as MSVC does not support
   the weak symbols the wrapping relies on elsewhere.

If you want to use the facilities exposed by ``hpx::runtime_manager`` in binaries
that were not linked as executables (e.g., in shared libraries), you will need
make your cmake target explicitly depend on the ``HPX::init`` target:

.. code-block:: cmake

   add_library(hello_world_component SHARED hello_world_component.cpp)
   target_link_libraries(hello_world_component PRIVATE HPX::init)

Otherwise you may see compilation errors complaining about the header file
``hpx/runtime_manager.hpp`` not being found.

Creating a component requires setting two additional compile definitions:

.. code-block:: cmake

   target_compile_options(hello_world_component
     HPX_COMPONENT_NAME=hello_world
     HPX_COMPONENT_EXPORTS)

Instead of setting these definitions manually you may link to the
``HPX::component`` target, which sets ``HPX_COMPONENT_NAME`` to
``hpx_<target_name>``, where ``<target_name>`` is the target name of your
library. Note that these definitions should be ``PRIVATE`` to make sure these
definitions are not propagated transitively to dependent targets.

In addition to making your library a component you can make it a plugin. To do
so link to the ``HPX::plugin`` target. Similarly to ``HPX::component`` this will
set ``HPX_PLUGIN_NAME`` to ``hpx_<target_name>``. This definition should also be
``PRIVATE``. Unlike regular shared libraries, plugins are loaded at runtime from
certain directories and will not be found without additional configuration.
Plugins should be installed into a directory containing only plugins. For
example, the plugins created by |hpx| itself are installed into the ``hpx``
subdirectory in the library install directory (typically ``lib`` or ``lib64``).
When using the ``HPX::plugin`` target you need to install your plugins into an
appropriate directory. You may also want to set the location of your plugin in
the build directory with the ``*_OUTPUT_DIRECTORY*`` CMake target properties to
be able to load the plugins in the build directory. Once you've set the install
or output directory of your plugin you need to tell your executable where to
find it at runtime. You can do this either by setting the environment variable
``HPX_COMPONENT_PATHS`` or the ini setting ``hpx.component_paths`` (see
:option:`--hpx:ini`) to the directory containing your plugin.

.. _using_hpx_cmake_macros:

Using macros to create new targets
----------------------------------

In addition to the targets described above, |hpx| provides convenience macros
to hide optional boilerplate code that may be useful for your project. The link
to the targets described above. We recommend that you use the targets directly
whenever possible as they tend to compose better with other targets.

The macro for adding an |hpx| component is ``add_hpx_component``. It can be
used in your ``CMakeLists.txt`` file like this:

.. code-block:: cmake

   # build your application using HPX
   add_hpx_component(hello_world
       SOURCES hello_world_component.cpp
       HEADERS hello_world_component.hpp
       COMPONENT_DEPENDENCIES iostreams)

.. note::

   ``add_hpx_component`` adds a ``_component`` suffix to the target name. In the
   example above, a ``hello_world_component`` target will be created.

The available options to ``add_hpx_component`` are:

* ``SOURCES``: The source files for that component
* ``HEADERS``: The header files for that component
* ``DEPENDENCIES``: Other libraries or targets this component depends on
* ``COMPONENT_DEPENDENCIES``: The components this component depends on
* ``PLUGIN``: Treats this component as a plugin-able library
* ``COMPILE_FLAGS``: Additional compiler flags
* ``LINK_FLAGS``: Additional linker flags
* ``FOLDER``: Adds the headers and source files to this Source Group folder

..
   * ``SOURCE_ROOT``
   * ``HEADER_ROOT``
   * ``SOURCE_GLOB``
   * ``HEADER_GLOB``
   * ``OUTPUT_SUFFIX``
   * ``INSTALL_SUFFIX``

* ``EXCLUDE_FROM_ALL``: Do not build this component as part of the ``all`` target

..
   * ``LANGUAGE``

After adding the component, the way you add the executable is as follows:

.. code-block:: cmake

   # build your application using HPX
   add_hpx_executable(hello_world
       SOURCES hello_world_client.cpp
       COMPONENT_DEPENDENCIES hello_world)

.. note::

   ``add_hpx_executable`` automatically adds a ``_component`` suffix to dependencies
   specified in ``COMPONENT_DEPENDENCIES``, meaning you can directly use the name given
   when adding a component using ``add_hpx_component``.

When you configure your application, all you need to do is set the ``HPX_DIR``
variable to point to the installation of |hpx|.

.. note::

   All library targets built with |hpx| are exported and readily available to be
   used as arguments to `target_link_libraries
   <https://www.cmake.org/cmake/help/latest/command/target_link_libraries.html>`_
   in your targets. The |hpx| include directories are available with the
   ``HPX_INCLUDE_DIRS`` CMake variable.

.. _hpxcxx_documentation:

Using the |hpx| compiler wrapper ``hpxcxx``
-------------------------------------------

The ``hpxcxx`` compiler wrapper helps to compile a |hpx| component, application,
or object file, based on the arguments passed to it.

.. code-block:: shell-session

   $ hpxcxx [--exe=<APPLICATION_NAME> | --comp=<COMPONENT_NAME> | -c] FLAGS FILES

The ``hpxcxx`` command **requires** that either an application or a component is
built or ``-c`` flag is specified. If the build is against a debug build, the
``-g`` is to be specified while building.

Optional ``FLAGS``
..................

* ``-l <LIBRARY> | -l<LIBRARY>``: Links ``<LIBRARY>`` to the build
* ``-g``: Specifies that the application or component build is against a debug
  build
* ``-rd``: Sets ``release-with-debug-info`` option
* ``-mr``: Sets ``minsize-release`` option

All other flags (like ``-o OUTPUT_FILE``) are directly passed to the underlying
C++ compiler.
 
.. _cmake_integrate_hpx:

Using macros to set up existing targets to use |hpx|
----------------------------------------------------

In addition to the ``add_hpx_component`` and ``add_hpx_executable``, you can use
the ``hpx_setup_target`` macro to have an already existing target to be used
with the |hpx| libraries:

.. code-block:: cmake

   hpx_setup_target(target)

Optional parameters are:

* ``EXPORT``: Adds it to the CMake export list HPXTargets
* ``INSTALL``: Generates an install rule for the target
* ``PLUGIN``: Treats this component as a plugin-able library
* ``TYPE``: The type can be: EXECUTABLE, LIBRARY or COMPONENT
* ``DEPENDENCIES``: Other libraries or targets this component depends on
* ``COMPONENT_DEPENDENCIES``: The components this component depends on
* ``COMPILE_FLAGS``: Additional compiler flags
* ``LINK_FLAGS``: Additional linker flags

..
   * ``NO_HPXINIT``
   * ``NOLIBS``
   * ``FOLDER``
   * ``NAME``
   * ``SOVERSION``
   * ``VERSION``

If you do not use CMake, you can still build against |hpx|, but you should refer
to the section on :ref:`comps`.

.. note::

   Since |hpx| relies on dynamic libraries, the dynamic linker needs to know
   where to look for them. If |hpx| isn't installed into a path that is
   configured as a linker search path, external projects need to either set
   ``RPATH`` or adapt ``LD_LIBRARY_PATH`` to point to where the |hpx| libraries
   reside. In order to set ``RPATH``\ s, you can include ``HPX_SetFullRPATH`` in
   your project after all libraries you want to link against have been added.
   Please also consult the CMake documentation `here
   <https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling>`_.

.. _makefile:

Using |hpx| with Makefile
=========================

A basic project building with |hpx| is through creating makefiles. The process
of creating one can get complex depending upon the use of cmake parameter
``HPX_WITH_HPX_MAIN`` (which defaults to ON).

How to build |hpx| applications with makefile
---------------------------------------------

If |hpx| is installed correctly, you should be able to build and run a simple
Hello World program. It prints ``Hello World!`` on the :term:`locality` you
run it on.

.. literalinclude:: ../../examples/quickstart/hello_world_1.cpp
   :language: c++
   :start-after: //[hello_world_1_getting_started
   :end-before: //]
 
Copy the content of this program into a file called hello_world.cpp.

Now, in the directory where you put hello_world.cpp, create a Makefile.
Add the following code:

.. code-block:: makefile

   CXX=(CXX)  # Add your favourite compiler here or let makefile choose default.

   CXXFLAGS=-O3 -std=c++17

   Boost_ROOT=/path/to/boost
   Hwloc_ROOT=/path/to/hwloc
   Tcmalloc_ROOT=/path/to/tcmalloc
   HPX_ROOT=/path/to/hpx

   INCLUDE_DIRECTIVES=$(HPX_ROOT)/include $(Boost_ROOT)/include $(Hwloc_ROOT)/include

   LIBRARY_DIRECTIVES=-L$(HPX_ROOT)/lib $(HPX_ROOT)/lib/libhpx_init.a $(HPX_ROOT)/lib/libhpx.so $(Boost_ROOT)/lib/libboost_atomic-mt.so $(Boost_ROOT)/lib/libboost_filesystem-mt.so $(Boost_ROOT)/lib/libboost_program_options-mt.so $(Boost_ROOT)/lib/libboost_regex-mt.so $(Boost_ROOT)/lib/libboost_system-mt.so -lpthread $(Tcmalloc_ROOT)/libtcmalloc_minimal.so $(Hwloc_ROOT)/libhwloc.so -ldl -lrt

   LINK_FLAGS=$(HPX_ROOT)/lib/libhpx_wrap.a -Wl,-wrap=main  # should be left empty for HPX_WITH_HPX_MAIN=OFF

   hello_world: hello_world.o
      $(CXX) $(CXXFLAGS) -o hello_world hello_world.o $(LIBRARY_DIRECTIVES) $(LINK_FLAGS)

   hello_world.o:
      $(CXX) $(CXXFLAGS) -c -o hello_world.o hello_world.cpp $(INCLUDE_DIRECTIVES)

.. important::

   ``LINK_FLAGS`` should be left empty if HPX_WITH_HPX_MAIN is set to OFF.
   Boost in the above example is build with ``--layout=tagged``. Actual Boost
   flags may vary on your build of Boost.

To build the program, type:

.. code-block:: shell-session

   $ make

A successful build should result in hello_world binary. To test, type:

.. code-block:: shell-session

   $ ./hello_world

How to build |hpx| components with makefile
-------------------------------------------

Let's try a more complex example involving an |hpx| component. An |hpx|
component is a class that exposes |hpx| actions. |hpx| components are compiled
into dynamically-loaded modules called component libraries. Here's the source
code:

**hello_world_component.cpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_component.cpp
   :language: c++
   :start-after: //[hello_world_cpp_getting_started
   :end-before: //]

**hello_world_component.hpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_component.hpp
   :language: c++
   :start-after: //[hello_world_hpp_getting_started
   :end-before: //]

**hello_world_client.cpp**

.. literalinclude:: ../../examples/hello_world_component/hello_world_client.cpp
   :language: c++
   :start-after: //[hello_world_client_getting_started
   :end-before: //]

Now, in the directory, create a Makefile. Add the following code:

.. code-block:: makefile

   CXX=(CXX)  # Add your favourite compiler here or let makefile choose default.

   CXXFLAGS=-O3 -std=c++17

   Boost_ROOT=/path/to/boost
   Hwloc_ROOT=/path/to/hwloc
   Tcmalloc_ROOT=/path/to/tcmalloc
   HPX_ROOT=/path/to/hpx

   INCLUDE_DIRECTIVES=$(HPX_ROOT)/include $(Boost_ROOT)/include $(Hwloc_ROOT)/include

   LIBRARY_DIRECTIVES=-L$(HPX_ROOT)/lib $(HPX_ROOT)/lib/libhpx_init.a $(HPX_ROOT)/lib/libhpx.so $(Boost_ROOT)/lib/libboost_atomic-mt.so $(Boost_ROOT)/lib/libboost_filesystem-mt.so $(Boost_ROOT)/lib/libboost_program_options-mt.so $(Boost_ROOT)/lib/libboost_regex-mt.so $(Boost_ROOT)/lib/libboost_system-mt.so -lpthread $(Tcmalloc_ROOT)/libtcmalloc_minimal.so $(Hwloc_ROOT)/libhwloc.so -ldl -lrt

   LINK_FLAGS=$(HPX_ROOT)/lib/libhpx_wrap.a -Wl,-wrap=main  # should be left empty for HPX_WITH_HPX_MAIN=OFF

   hello_world_client: libhpx_hello_world hello_world_client.o
     $(CXX) $(CXXFLAGS) -o hello_world_client $(LIBRARY_DIRECTIVES) libhpx_hello_world $(LINK_FLAGS)

   hello_world_client.o: hello_world_client.cpp
     $(CXX) $(CXXFLAGS) -o hello_world_client.o hello_world_client.cpp $(INCLUDE_DIRECTIVES)

   libhpx_hello_world: hello_world_component.o
     $(CXX) $(CXXFLAGS) -o libhpx_hello_world hello_world_component.o $(LIBRARY_DIRECTIVES)

   hello_world_component.o: hello_world_component.cpp
     $(CXX) $(CXXFLAGS) -c -o hello_world_component.o hello_world_component.cpp $(INCLUDE_DIRECTIVES)

To build the program, type:

.. code-block:: shell-session

   $ make

A successful build should result in hello_world binary. To test, type:

.. code-block:: shell-session

   $ ./hello_world

.. note::

   Due to high variations in CMake flags and library dependencies, it is
   recommended to build |hpx| applications and components with pkg-config
   or CMakeLists.txt. Writing Makefile may result in broken builds if
   due care is not taken.
   pkg-config files and CMake systems are configured with CMake build of
   |hpx|. Hence, they are stable when used together and provide better support overall.
