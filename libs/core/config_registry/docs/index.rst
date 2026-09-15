..
    Copyright (c) 2020 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_config_registry:

===============
config_registry
===============

The config_registry module is a low level module providing helper functionality
for registering configuration entries to a global registry from other modules.
The :hpx:func:`hpx::config_registry::add_module_config` function is used to add
configuration options, and :hpx:func:`hpx::config_registry::get_module_configs`
can be used to retrieve configuration entries registered so far.
:hpx:class:`add_module_config_helper` can be used to register configuration
entries through static global options.

See the :ref:`API reference <modules_config_registry_api>` of this module for more
details.
