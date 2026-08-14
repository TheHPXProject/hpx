..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_supervision:

==================
Supervision module
==================

The supervision module provides lifecycle event publication, state querying,
and observer registration for actors/components running on local or remote
localities.

Overview
========

Several core operations are exposed, each with a local (synchronous) call and
a remote (locality-qualified, future-returning or ``launch::sync_policy``)
call:

Functions
---------

.. table:: `hpx::supervision` functions

   ==========================================================  ===================================================================================================================
   Function                                                    Description
   ==========================================================  ===================================================================================================================
   :hpx:func:`hpx::supervision::publish_event`                 :ref:`Publish a lifecycle event for a target. <supervision_publish_event>`
   :hpx:func:`hpx::supervision::query_state`                   :ref:`Query the most recently observed lifecycle state. <supervision_query_state>`
   :hpx:func:`hpx::supervision::register_observer`             :ref:`Register an observer for a target's lifecycle events. <supervision_register_observer>`
   :hpx:func:`hpx::supervision::unregister_observer`           :ref:`Unregister a previously registered lifecycle observer. <supervision_unregister_observer>`
   :hpx:func:`hpx::supervision::remove_target`                 :ref:`Clear all locally tracked state for a target. <supervision_remove_target>`
   :hpx:func:`hpx::supervision::await_terminal`                :ref:`Wait for a target to reach a terminal lifecycle event. <supervision_await_terminal>`
   :hpx:func:`hpx::supervision::check_admission`               :ref:`Check whether a target currently admits new dispatch. <supervision_check_admission>`
   :hpx:func:`hpx::supervision::register_activity_observer`    :ref:`Register an observer for activity-state transitions of all targets. <supervision_register_activity_observer>`
   :hpx:func:`hpx::supervision::unregister_activity_observer`  :ref:`Unregister a previously registered activity observer. <supervision_unregister_activity_observer>`
   ==========================================================  ===================================================================================================================

.. _supervision_publish_event:

Publishing events
-----------------

.. cpp:function:: hpx::future<hpx::supervision::publish_result> hpx::supervision::publish_event(hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::event ev, std::uint64_t epoch = 0)
.. cpp:function:: hpx::supervision::publish_result hpx::supervision::publish_event(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::event ev, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::supervision::publish_result hpx::supervision::publish_event(hpx::id_type const& target, hpx::supervision::event ev, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws)

    Publish a lifecycle event for ``target``. Events are visible immediately
    to local observers; remote observers are notified within roughly one to
    two parcel round-trips. Publishing non-terminal events is not idempotent
    - each call creates a distinct, timestamped record. ``event::completed``
    and ``event::failed`` are latched: the first terminal publication for a
    target returns ``publish_result::applied``; every later terminal
    publication for that target is a no-op that returns
    ``publish_result::already_terminal``.

.. _supervision_query_state:

Querying lifecycle state
------------------------

.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::id_type const& locality, hpx::id_type const& target)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::id_type const& target, hpx::error_code& ec = hpx::throws)

    Query the most recently observed lifecycle state for ``target``. Includes
    a sequence number for gap detection and a staleness error code for remote
    queries whose result may lag the latest event.

.. _supervision_register_observer:

Registering observers
---------------------

.. cpp:function:: hpx::future<hpx::id_type> hpx::supervision::register_observer(hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt)
.. cpp:function:: hpx::id_type hpx::supervision::register_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::id_type hpx::supervision::register_observer(hpx::id_type const& target, hpx::supervision::lifecycle_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt, hpx::error_code& ec = hpx::throws)

    Register ``callback`` to be invoked on lifecycle events of ``target``,
    returning an observer handle usable with ``unregister_observer``. Local
    callbacks fire synchronously within the publish call; remote callbacks
    fire via a retried parcel. If ``epoch_filter`` is set, the observer only
    receives notifications (including the initial state snapshot delivered
    at registration time) whose epoch matches; by default the observer
    receives notifications for every epoch.

.. _supervision_unregister_observer:

Unregistering observers
-----------------------

.. cpp:function:: hpx::future<void> hpx::supervision::unregister_observer(hpx::id_type const& locality, hpx::id_type const& observer_handle)
.. cpp:function:: void hpx::supervision::unregister_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)
.. cpp:function:: void hpx::supervision::unregister_observer(hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)

    Unregister a previously registered observer. ``observer_handle`` must
    have been obtained from ``register_observer``; a handle obtained from
    ``register_activity_observer``, or any handle never returned by either
    registration function, is rejected.

.. _supervision_remove_target:

Removing target state
---------------------

.. cpp:function:: hpx::future<void> hpx::supervision::remove_target(hpx::id_type const& locality, hpx::id_type const& target)
.. cpp:function:: void hpx::supervision::remove_target(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, hpx::error_code& ec = hpx::throws)
.. cpp:function:: void hpx::supervision::remove_target(hpx::id_type const& target, hpx::error_code& ec = hpx::throws)

    Clear all locally tracked state for ``target``. Unlike
    ``unregister_observer``, which removes a single previously registered
    observer handle, ``remove_target`` unconditionally forgets every piece of
    local state held for ``target`` - its recorded lifecycle state and
    current epoch, and any observers still registered for it - regardless of
    any specific observer handle. Intended for callers that know ``target``
    will never be queried or observed again locally (e.g. after a failed
    registration that seeded some state for it, or once a peer has been
    evicted), so that local state does not accumulate indefinitely.

.. _supervision_await_terminal:

Waiting for terminal events
---------------------------

.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::await_terminal(hpx::id_type const& locality, hpx::id_type const& target, std::uint64_t epoch = 0, std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::max())
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::await_terminal(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& target, std::uint64_t epoch = 0, std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::max(), hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::await_terminal(hpx::id_type const& target, std::uint64_t epoch = 0, std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::max())

    Asynchronously wait, one-shot and without blocking a worker thread, and the
    ``launch::sync_policy`` overload waits synchronously until ``target`` reaches
    a terminal event (``event::completed`` or ``event::failed``) within ``epoch``.
    If a terminal event has already been recorded for
    ``target`` in ``epoch``, the returned future is immediately satisfied
    with that state. Waits are scoped to a single epoch: if ``target``'s
    epoch advances past ``epoch`` before a terminal event occurs, any
    outstanding waiter for the stale epoch is invalidated. A waiter that
    reaches neither of these outcomes is bounded by ``timeout`` (or a
    built-in default if ``timeout`` is left at its sentinel value
    ``std::chrono::steady_clock::duration::max()``), after which it is
    swept and invalidated. Dispatch is routed locally when ``target``'s
    supervision manager lives on the calling locality, and remotely (via a
    registered distributed action) otherwise.

.. _supervision_check_admission:

Checking dispatch admission
---------------------------

.. cpp:function:: hpx::supervision::dispatch_outcome hpx::supervision::check_admission(hpx::id_type const& target, std::uint64_t epoch = 0)

    Check whether ``target`` currently admits new dispatch under ``epoch``,
    i.e. whether it is safe to schedule or route work to it right now. This
    is a pure, ``noexcept`` local read of the same terminal latch state
    consulted by ``await_terminal``: it returns
    ``dispatch_outcome::rejected_fenced`` if ``target`` has already latched a
    terminal event (``event::completed`` or ``event::failed``) under
    ``epoch``, and ``dispatch_outcome::admitted`` otherwise. Unlike
    ``publish_result``, which reports how a publication was resolved,
    ``dispatch_outcome`` answers the separate, consumer-side question of
    whether dispatch should proceed; it must only be called on the locality
    ``target`` lives on.

    This admission fence is scoped to a single locality's latch state: it
    reflects only terminal events published to the supervision manager
    hosting ``target``, and it fails open (returns ``admitted``) when
    ``target`` is unknown locally, including when its terminal event was
    only ever published elsewhere. There is no built-in mechanism that
    propagates terminal latch state across localities - observer
    registration delivers notifications, not admission state. Any
    additional locality that needs to fence its own dispatch decisions on
    ``target`` must independently mirror or route to the owning locality's
    state; this module does not provide that for you.

.. _supervision_register_activity_observer:

Registering activity observers
------------------------------

.. cpp:function:: hpx::future<hpx::id_type> hpx::supervision::register_activity_observer(hpx::id_type const& locality, hpx::supervision::activity_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt)
.. cpp:function:: hpx::id_type hpx::supervision::register_activity_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::supervision::activity_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::id_type hpx::supervision::register_activity_observer(hpx::supervision::activity_callback const& callback, std::optional<std::uint64_t> epoch_filter = std::nullopt, hpx::error_code& ec = hpx::throws)

    Register ``callback`` to observe activity-state transitions (between
    ``hpx::supervision::activity_state::inactive`` and ``active``) of *all*
    targets tracked by a locality's supervision manager. Unlike
    ``register_observer``, none of these overloads take a ``target``
    parameter: the feature reports on every target the addressed manager
    tracks rather than on a single one.

    Registering an activity observer for an already-active target triggers a
    replay of its current state. The snapshot of tracked state and the
    insertion of the observer are performed atomically under the manager's
    internal lock, which guarantees exactly-once delivery: the observer
    receives either the replay or a live transition that raced with
    registration, but never both and never neither.

    Note that the notification itself (replay or live) is delivered after the
    lock is released. Its delivery order relative to any other concurrent live
    notification for the same target is *not* guaranteed, only the
    exactly-once property is guaranteed, not relative ordering.

    Activity notifications are a pure discovery/notification signal: unlike
    ``publish_event``, registering or unregistering an activity observer
    never feeds the terminal latch consulted by ``check_admission``.

    If ``epoch_filter`` is set, ``callback`` only receives notifications -
    including the registration-time replay - whose epoch matches; by
    default it receives notifications regardless of epoch.

.. _supervision_unregister_activity_observer:

Unregistering activity observers
--------------------------------

.. cpp:function:: hpx::future<void> hpx::supervision::unregister_activity_observer(hpx::id_type const& locality, hpx::id_type const& observer_handle)
.. cpp:function:: void hpx::supervision::unregister_activity_observer(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)
.. cpp:function:: void hpx::supervision::unregister_activity_observer(hpx::id_type const& observer_handle, hpx::error_code& ec = hpx::throws)

    Unregister a previously registered activity observer. ``observer_handle``
    must have been obtained from ``register_activity_observer``; a
    handle obtained from ``register_observer``, or any handle never returned
    by either registration function, is rejected.

See the :ref:`API reference <modules_supervision_api>` of this module for
more details.
