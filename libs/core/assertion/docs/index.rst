..
    Copyright (c) 2018 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_assertion:

=========
assertion
=========

The assertion library implements the macros :c:macro:`HPX_ASSERT` and
:c:macro:`HPX_ASSERT_MSG`. Those two macros can be used to implement assertions
which are turned of during a release build.

By default, the location and function where the assert has been called from are
displayed when the assertion fires. This behavior can be modified by using
:hpx:func:`hpx::assertion::set_assertion_handler`. When HPX initializes, it uses
this function to specify a more elaborate assertion handler. If your application
needs to customize this, it needs to do so before calling
:hpx:func:`hpx::init`, :ref:`hpx_main <starting_hpx>` or using the C-main
wrappers.


See the :ref:`API reference <modules_assertion_api>` of the module for more
details.
