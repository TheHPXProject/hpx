..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_supervision:

===========
supervision
===========

The supervision module provides lifecycle event publication, state querying,
and observer registration for actors/components running on local or remote
localities.

Overview
========

Four core operations are exposed, each with a local (synchronous) call and
a remote (locality-qualified, future-returning or ``launch::sync_policy``)
call:

Publishing events
------------------

.. cpp:function:: hpx::future<void> hpx::supervision::publish_event(hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::event ev)
.. cpp:function:: void hpx::supervision::publish_event(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::event ev, hpx::error_code& ec = hpx::throws)
.. cpp:function:: void hpx::supervision::publish_event(hpx::id_type const& target, hpx::supervision::event ev, hpx::error_code& ec = hpx::throws)

    Publish a lifecycle event for ``target``. Events are visible immediately
    to local observers; remote observers are notified within roughly one to
    two parcel round-trips. Not idempotent - each call creates a distinct,
    timestamped record.

Querying lifecycle state
-------------------------

.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::id_type const& locality, hpx::id_type const& target)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::id_type const& target, hpx::error_code& ec = hpx::throws)

    Query the most recently observed lifecycle state for ``target``. Includes
    a sequence number for gap detection and a staleness error code for remote
    queries whose result may lag the latest event.

Registering observers
----------------------

.. cpp:function:: hpx::future<hpx::id_type> hpx::supervision::register_observer(hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback)
.. cpp:function:: hpx::id_type hpx::supervision::register_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::id_type hpx::supervision::register_observer(hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback, hpx::error_code& ec = hpx::throws)

    Register ``callback`` to be invoked on lifecycle events of ``target``,
    returning an observer handle usable with ``unregister_observer``. Local
    callbacks fire synchronously within the publish call; remote callbacks
    fire via a retried parcel.

Unregistering observers
------------------------

.. cpp:function:: hpx::future<void> hpx::supervision::unregister_observer(hpx::id_type const& locality, hpx::id_type const& observer_handle)
.. cpp:function:: void hpx::supervision::unregister_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)
.. cpp:function:: void hpx::supervision::unregister_observer(hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)

    Unregister a previously registered observer.

See the :ref:`API reference <modules_supervision_api>` of this module for
more details.
