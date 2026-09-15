..
    Copyright (c) 2019-2022 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_synchronization:

===============
synchronization
===============

This module provides synchronization primitives that should be used rather than
the C++ standard ones in |hpx| threads:

* :hpx:class:`hpx::barrier`
* :hpx:class:`hpx::binary_semaphore`
* :hpx:func:`hpx::call_once`
* :hpx:class:`hpx::condition_variable`
* :hpx:class:`hpx::condition_variable_any`
* :hpx:class:`hpx::counting_semaphore`
* :hpx:class:`hpx::lcos::local::event`
* :hpx:class:`hpx::latch`
* :hpx:class:`hpx::mutex`
* :hpx:class:`hpx::no_mutex`
* :hpx:class:`hpx::once_flag`
* :hpx:class:`hpx::recursive_mutex`
* :hpx:class:`hpx::shared_mutex`
* :hpx:class:`hpx::sliding_semaphore`
* :hpx:class:`hpx::spinlock` (`std::mutex` compatible spinlock)
* :hpx:class:`hpx::spinlock_no_backoff` (`boost::mutex` compatible spinlock)
* :hpx:class:`hpx::spinlock_pool`
* :hpx:class:`hpx::stop_callback`
* :hpx:class:`hpx::stop_source`
* :hpx:class:`hpx::stop_token`
* :hpx:class:`hpx::in_place_stop_token`
* :hpx:class:`hpx::timed_mutex`
* :hpx:class:`hpx::upgrade_to_unique_lock`
* :hpx:class:`hpx::upgrade_lock`

See :ref:`modules_lcos_local`, :ref:`modules_async_combinators`, and
:ref:`modules_async_distributed` for higher level synchronization facilities.

See the :ref:`API reference <modules_synchronization_api>` of this module for more
details.
