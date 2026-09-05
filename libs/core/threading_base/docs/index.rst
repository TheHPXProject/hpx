..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_threading_base:

==============
threading_base
==============

This module contains the base class definition required for threads. The base
class :hpx:class:`hpx::threads::thread_data` is inherited by two specializations
for stackful and stackless threads:
:hpx:class:`hpx::threads::thread_data_stackful` and
:hpx:class:`hpx::threads::thread_data_stackless`. In addition, the module
defines the base classes for schedulers and thread pools:
:hpx:class:`hpx::threads::policies::scheduler_base` and
:hpx:class:`hpx::threads::thread_pool_base`.

See the :ref:`API reference <modules_thread_data_api>` of this module for more
details.
