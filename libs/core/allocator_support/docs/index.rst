..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_allocator_support:

=================
allocator_support
=================

This module provides utilities for allocators. It contains
:hpx:class:`hpx::util::internal_allocator` which directly forwards allocation
calls to ``jemalloc``. This utility is is mainly useful on Windows.

See the :ref:`API reference <modules_allocator_support_api>` of the module for more
details.
