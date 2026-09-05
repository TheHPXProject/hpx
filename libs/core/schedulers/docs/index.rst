..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_schedulers:

==========
schedulers
==========

This module provides schedulers used by thread pools in the
:ref:`modules_thread_pools` module. There are currently three main schedulers:

* :hpx:class:`hpx::threads::policies::local_priority_queue_scheduler`
* :hpx:class:`hpx::threads::policies::static_priority_queue_scheduler`
* :hpx:class:`hpx::threads::policies::shared_priority_queue_scheduler`

Other schedulers are specializations or variations of the above schedulers. See
the examples of the :ref:`modules_resource_partitioner` module for examples of
specifying a custom scheduler for a thread pool.

See the :ref:`API reference <modules_schedulers_api>` of this module for more
details.
