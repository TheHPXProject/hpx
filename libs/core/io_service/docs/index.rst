..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_io_service:

==========
io_service
==========

This module provides an abstraction over Boost.ASIO, combining multiple
``asio::io_context``\ s into a single pool.
:hpx:class:`hpx::util::io_service_pool` provides a simple pool of
``asio::io_context``\ s with an API similar to ``asio::io_context``.
:hpx:class:`hpx::threads::detail::io_service_thread_pool` wraps
:hpx:class:`hpx::util::io_service_pool` into an interface derived from
:hpx:class:`hpx::threads::detail::thread_pool_base`.

See the :ref:`API reference <modules_io_service_api>` of this module for more
details.
