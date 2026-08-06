..
    Copyright (C) 2023 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _public_distributed_api:

======================
Public distributed API
======================

Our Public Distributed API offers a rich set of tools and functions that enable developers
to harness the full potential of distributed computing. Here, you'll find a comprehensive
list of header files, classes and functions for various distributed computing features
provided by |hpx|.

.. _public_distr_api_header_barrier:

``hpx/barrier.hpp``
===================

The header :hpx-header:`libs/full/include/include,hpx/barrier.hpp` includes
a distributed barrier implementation. For information regarding the C++ standard
library header :cppreference-header:`barrier`, see :ref:`public_api`.

Classes
-------

.. table:: Distributed implementation of classes of header ``hpx/barrier.hpp``

   +----------------------------------------+
   | Class                                  |
   +========================================+
   | :cpp:class:`hpx::distributed::barrier` |
   +----------------------------------------+

Functions
---------

.. table:: `hpx` functions of header ``hpx/barrier.hpp``

   +-------------------------------------------+
   | Function                                  |
   +===========================================+
   | :hpx-api:`hpx::distributed::wait`         |
   +-------------------------------------------+
   | :hpx-api:`hpx::distributed::synchronize`  |
   +-------------------------------------------+

.. _public_distr_api_header_collectives:

``hpx/collectives.hpp``
=======================

The header :hpx-header:`libs/full/include/include,hpx/collectives.hpp`
contains definitions and implementations related to the collectives operations.

Classes
-------

.. table:: `hpx` classes of header ``hpx/collectives.hpp``

   +-----------------------------------------------------+
   | Class                                               |
   +=====================================================+
   | :cpp:struct:`hpx::collectives::num_sites_arg`       |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::this_site_arg`       |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::that_site_arg`       |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::generation_arg`      |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::root_site_arg`       |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::tag_arg`             |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::arity_arg`           |
   +-----------------------------------------------------+
   | :cpp:struct:`hpx::collectives::communicator`        |
   +-----------------------------------------------------+
   | :cpp:class:`hpx::collectives::channel_communicator` |
   +-----------------------------------------------------+

Functions
---------

.. table:: `hpx` functions of header ``hpx/collectives.hpp``

   +----------------------------------------------------------------+
   | Function                                                       |
   +================================================================+
   | :hpx-api:`hpx::collectives::all_gather`                        |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::all_reduce`                        |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::all_to_all`                        |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::broadcast_to`                      |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::broadcast_from`                    |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::create_channel_communicator`       |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::set`                               |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::get`                               |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::create_communicator`               |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::create_hierarchical_communicator`  |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::create_local_communicator`         |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::communicator::set_info`            |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::communicator::get_info`            |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::communicator::is_root`             |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::exclusive_scan`                    |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::gather_here`                       |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::gather_there`                      |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::inclusive_scan`                    |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::reduce_here`                       |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::reduce_there`                      |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::scatter_from`                      |
   +----------------------------------------------------------------+
   | :hpx-api:`hpx::collectives::scatter_to`                        |
   +----------------------------------------------------------------+

.. _public_distr_api_header_latch:

``hpx/latch.hpp``
=================

The header :hpx-header:`libs/full/include/include,hpx/latch.hpp` includes
a distributed latch implementation. For information regarding the C++ standard
library header :cppreference-header:`latch`, see :ref:`public_api`.

Classes
-------

.. table:: Distributed implementation of classes of header ``hpx/latch.hpp``

   +--------------------------------------+
   | Class                                |
   +======================================+
   | :cpp:class:`hpx::distributed::latch` |
   +--------------------------------------+

Member functions
^^^^^^^^^^^^^^^^

.. table:: `hpx` functions of class :cpp:class:`hpx::distributed::latch` from header ``hpx/latch.hpp``

   +----------------------------------------------------------+
   | Function                                                 |
   +==========================================================+
   | :hpx-api:`hpx::distributed::latch::count_down_and_wait`  |
   +----------------------------------------------------------+
   | :hpx-api:`hpx::distributed::latch::arrive_and_wait`      |
   +----------------------------------------------------------+
   | :hpx-api:`hpx::distributed::latch::count_down`           |
   +----------------------------------------------------------+
   | :hpx-api:`hpx::distributed::latch::is_ready`             |
   +----------------------------------------------------------+
   | :hpx-api:`hpx::distributed::latch::try_wait`             |
   +----------------------------------------------------------+
   | :hpx-api:`hpx::distributed::latch::wait`                 |
   +----------------------------------------------------------+

.. _public_distr_api_header_async:

``hpx/async.hpp``
=================

The header :hpx-header:`libs/full/async_distributed/include,hpx/async.hpp`
includes distributed implementations of :hpx-api:`hpx::async`,
:hpx-api:`hpx::post`, :hpx-api:`hpx::sync`, and :hpx-api:`hpx::dataflow`.
For information regarding the C++ standard library header, see :ref:`public_api`.

Functions
---------

.. table:: Distributed implementation of functions of header ``hpx/async.hpp``

   +-------------------------------------------------------+
   | Functions                                             |
   +=======================================================+
   | :ref:`modules_hpx/async_distributed/async.hpp_api`    |
   +-------------------------------------------------------+
   | :ref:`modules_hpx/async_distributed/sync.hpp_api`     |
   +-------------------------------------------------------+
   | :ref:`modules_hpx/async_distributed/post.hpp_api`     |
   +-------------------------------------------------------+
   | :ref:`modules_hpx/async_distributed/dataflow.hpp_api` |
   +-------------------------------------------------------+

.. _public_distr_api_header_components:

``hpx/components.hpp``
======================

The header :hpx-header:`libs/full/include/include,hpx/include/components.hpp`
includes the components implementation. A component in `hpx` is a C++ class
which can be created remotely and for which its member functions can be invoked
remotely as well. More information about how components can be defined,
created, and used can be found in :ref:`components`. :ref:`examples_accumulator`
includes examples on the accumulator, template accumulator and template function
accumulator.

Macros
------

.. table:: `hpx` macros of header ``hpx/components.hpp``

   +----------------------------------------------+
   | Macro                                        |
   +==============================================+
   | :c:macro:`HPX_DEFINE_COMPONENT_ACTION`       |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_ACTION_DECLARATION`   |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_ACTION`               |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_COMMANDLINE_MODULE`   |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_COMPONENT`            |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_COMPONENT_MODULE`     |
   +----------------------------------------------+
   | :c:macro:`HPX_REGISTER_STARTUP_MODULE`       |
   +----------------------------------------------+

Classes
-------

.. table:: `hpx` classes of header ``hpx/components.hpp``

   +----------------------------------------------------------+
   | Class                                                    |
   +==========================================================+
   | :cpp:class:`hpx::components::client`                     |
   +----------------------------------------------------------+
   | :cpp:class:`hpx::components::client_base`                |
   +----------------------------------------------------------+
   | :cpp:class:`hpx::components::component`                  |
   +----------------------------------------------------------+
   | :cpp:class:`hpx::components::component_base`             |
   +----------------------------------------------------------+
   | :cpp:class:`hpx::components::component_commandline_base` |
   +----------------------------------------------------------+

Functions
---------

.. table:: `hpx` functions of header ``hpx/components.hpp``

   +----------------------------------------------------------+
   | Function                                                 |
   +==========================================================+
   | :hpx-api:`hpx::new_`                                     |
   +----------------------------------------------------------+
