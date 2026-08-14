..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _module_structure:

================
Module structure
================

This section explains the structure of an |hpx| module.

The tool `create_library_skeleton.py
<https://github.com/TheHPXProject/hpx/blob/master/libs/create_module_skeleton.py>`_
can be used to generate a basic skeleton. To create a library skeleton, run the
tool in the ``libs`` subdirectory with the module name as an argument:

.. code-block:: shell-session

    $ ./create_library_skeleton <lib_name>

This creates a skeleton with the necessary files for an |hpx| module. It will not create any actual source files. The structure of this skeleton is as follows:

* ``<lib_name>/``

  * ``README.rst``
  * ``CMakeLists.txt``
  * ``cmake``
  * ``docs/``

    * ``index.rst``

  * ``examples/``

    * ``CMakeLists.txt``

  * ``include/``

    * ``hpx/``

      * ``<lib_name>``

  * ``src/``

    * ``CMakeLists.txt``

  * ``tests/``

    * ``CMakeLists.txt``
    * ``unit/``

      * ``CMakeLists.txt``

    * ``regressions/``

      * ``CMakeLists.txt``

    * ``performance/``

      * ``CMakeLists.txt``

A ``README.rst`` should be always included which explains the basic purpose of
the library and a link to the generated documentation.

A main ``CMakeLists.txt`` is created in the root directory of the module. By
default it contains a call to ``add_hpx_module`` which takes care of most of the
boilerplate required for a module. You only need to fill in the source and
header files in most cases.

``add_hpx_module`` requires a module name. Optional flags are:

Optional single-value arguments are:

* ``INSTALL_BINARIES``: Install the resulting library.

Optional multi-value arguments-are:

* ``SOURCES``: List of source files.
* ``HEADERS``: List of header files.
* ``COMPAT_HEADERS``: List of compatibility header files.
* ``DEPENDENCIES``: Libraries that this module depends on, such as other modules.
* ``CMAKE_SUBDIRS``: List of subdirectories to add to the module.

The ``include`` directory should contain only headers that other libraries need.
For each of those headers, an automatic header test to check for self
containment will be generated. Private headers should be placed under the
``src`` directory. This allows for clear separation. The ``cmake`` subdirectory
may include additional |cmake|_ scripts needed to generate the respective build
configurations.

Compatibility headers (forwarding headers for headers whose location is changed
when creating a module, if moving them from the main library) should be placed
in an ``include_compatibility`` directory. This directory is not created by
default.

Documentation is placed in the ``docs`` folder. A empty skeleton for the index
is created, which is picked up by the main build system and will be part of the
generated documentation. Each header inside the ``include`` directory will
automatically be processed by Doxygen and included into the documentation.

Tests are placed in suitable subdirectories of ``tests``.

When in doubt, consult existing modules for examples on how to structure the
module.

Finding circular dependencies
=============================

Our CI will perform a check to see if there are circular dependencies between
modules. In cases where it's not clear what is causing the circular dependency,
running the |cppdependencies|_ tool manually can be helpful. It can give you
detailed information on exactly which files are causing the circular dependency.
If you do not have the ``cpp-dependencies`` tool already installed, one way of
obtaining it is by using our docker image. This way you will have exactly the
same environment as on the CI. See :ref:`using_docker` for details on how to use
the docker image.

To produce the graph produced by CI run the following command (``HPX_SOURCE`` is
assumed to hold the path to the |hpx| source directory):

.. code-block:: shell-session

   $ cpp-dependencies --dir $HPX_SOURCE/libs --graph-cycles circular_dependencies.dot

This will produce a ``dot`` file in the current directory. You can inspect this
manually with a text editor. You can also convert this to an image if you have
``graphviz`` installed:

.. code-block:: shell-session

   $ dot circular_dependencies.dot -Tsvg -o circular_dependencies.svg

This produces an ``svg`` file in the current directory which shows the circular
dependencies. Note that if there are no cycles the image will be empty.

You can use ``cpp-dependencies`` to print the include paths between two modules.

.. code-block:: shell-session

   $ cpp-dependencies --dir $HPX_SOURCE/libs --shortest <from> <to>

prints all possible paths from the module ``<from>`` to the module ``<to>``. For
example, as most modules depend on ``config``, the following should give you a
long list of paths from ``algorithms`` to ``config``:

.. code-block:: shell-session

   $ cpp-dependencies --dir $HPX_SOURCE/libs --shortest algorithms config

The following should report that it can't find a path between the two modules:

.. code-block:: shell-session

   $ cpp-dependencies --dir $HPX_SOURCE/libs --shortest config algorithms
