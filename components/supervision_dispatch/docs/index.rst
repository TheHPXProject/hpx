..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_supervision_dispatch:

===========================
Supervision dispatch module
===========================

The supervision dispatch module provides peer discovery, cooperative
lifecycle management, failure detection, and fenced action dispatch across
localities, built on top of the :ref:`supervision <modules_supervision>`
module's lifecycle events and admission checks. See that module's reference
for the semantics of :cpp:func:`hpx::supervision::check_admission` and
:cpp:type:`hpx::supervision::dispatch_outcome`; this page documents only the
dispatch-specific surface layered on top of them. For the narrative on
initialization ordering, one-shot discovery, shadow-state semantics, and
client-side filtering versus fenced-dispatch admission, see
:doc:`/manual/supervision_dispatch`.

Overview
========

Lifecycle initialization and shutdown
-------------------------------------

.. cpp:function:: hpx::shared_future<void> hpx::supervision::init(hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)
.. cpp:function:: void hpx::supervision::init(hpx::launch::sync_policy, hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)

    Performs one-shot, idempotent initialization of the supervision-dispatch
    runtime for this locality: creates a local sentinel and registry,
    publishes ``event::started`` before either symbol name is registered,
    registers both names, and performs a single ``discover_and_join()`` pass.
    Calling ``init()`` while already active is a no-op; concurrent callers
    during ``initializing`` attach to the in-flight operation instead of
    racing it.

.. cpp:function:: void hpx::supervision::finalize()

    Performs one-shot, idempotent teardown of the runtime previously started
    by ``init()``: publishes ``event::completed`` at a new epoch, unregisters
    both symbol names, and releases both components. A no-op unless the
    runtime is currently ``active``.

.. cpp:function:: bool hpx::supervision::is_initialized() noexcept

    Returns whether the runtime is currently ``active``. Never blocks; safe
    to call from any thread, including while ``init()``/``finalize()`` is in
    flight.

Peer discovery
--------------

.. cpp:struct:: hpx::supervision::discovered_peer

    A peer whose sentinel and registry symbol names both resolved during a
    ``discover_peers()`` call. Holds ``locality``, ``sentinel_client``, and
    ``registry_client``.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::discover_peers(hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Performs a one-time discovery pull: concurrently resolves the pinned
    sentinel and registry names of every remote locality, bounded by a
    single wait for ``timeout``. Localities that have not yet called
    ``init()`` are silently excluded rather than causing a hang or failure.

.. cpp:function:: std::vector<hpx::id_type> hpx::supervision::fan_out_join(hpx::supervision::registry const& local_registry, std::vector<hpx::supervision::discovered_peer> const& peers)

    Fans out ``join()`` calls from ``local_registry`` to every entry in
    ``peers``. Reuses ``registry::join()``'s reservation/idempotency
    machinery, so repeated or overlapping calls never create more than one
    shadow per peer sentinel.

.. cpp:function:: std::vector<hpx::id_type> hpx::supervision::discover_and_join(hpx::supervision::registry const& local_registry, hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Composes a single reactive discovery-and-join pass: one
    ``discover_peers()`` pull followed by one ``fan_out_join()`` call.
    Introduces no polling, timer, or repeated broadcast of its own.

Sentinel client
---------------

.. cpp:class:: hpx::supervision::sentinel

    A lightweight, self-supervising client handle. Constructing it with a
    target locality creates the underlying server-side component; calling
    ``start()`` publishes the ``started`` lifecycle event for it.

    .. cpp:function:: hpx::future<hpx::supervision::publish_result> start(std::uint64_t epoch = 0) const
    .. cpp:function:: hpx::supervision::publish_result start(hpx::launch::sync_policy, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws) const
    .. cpp:function:: hpx::future<bool> register_name()
    .. cpp:function:: bool register_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws)
    .. cpp:function:: hpx::future<hpx::id_type> unregister_name() const
    .. cpp:function:: hpx::id_type unregister_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

Registry client
---------------

.. cpp:class:: hpx::supervision::registry

    A lightweight, self-supervising client handle for pairing with peer
    sentinels; mirrors a peer's lifecycle state locally via ``join()`` and
    can itself be discovered by name in AGAS.

    .. cpp:function:: hpx::future<hpx::id_type> join(hpx::supervision::sentinel const& peer_sentinel, hpx::id_type const& peer_locality) const
    .. cpp:function:: hpx::id_type join(hpx::launch::sync_policy, hpx::supervision::sentinel const& peer_sentinel, hpx::id_type const& peer_locality, hpx::error_code& ec = hpx::throws) const

        Joins a peer sentinel: creates (or reuses) a local shadow target
        mirroring its lifecycle state, and registers this registry as an
        observer of the peer's lifecycle/activity events.

    .. cpp:function:: hpx::future<std::vector<hpx::supervision::server::peer_snapshot>> snapshot_peers() const
    .. cpp:function:: std::vector<hpx::supervision::server::peer_snapshot> snapshot_peers(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

        Returns a point-in-time snapshot of all fully joined, non-evicting
        peers.

    .. cpp:function:: hpx::future<bool> register_name()
    .. cpp:function:: bool register_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws)
    .. cpp:function:: hpx::future<hpx::id_type> unregister_name() const
    .. cpp:function:: hpx::id_type unregister_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

.. cpp:struct:: hpx::supervision::server::peer_snapshot

    Plain-data view of a single joined peer: ``peer_sentinel``,
    ``peer_locality``, ``shadow``, and ``join_epoch``.

Fenced dispatch
---------------

.. cpp:function:: template <typename Action, typename... Ts> decltype(auto) hpx::supervision::dispatch_work(hpx::id_type const& target, std::uint64_t epoch, Ts&&... ts)

    Dispatches ``Action`` to ``target`` under supervision fencing. Performs a
    cheap, non-authoritative admission check on the caller's locality to
    short-circuit an already-known-fenced target, then dispatches to the
    target's own locality (via ``hpx::colocated``) where an authoritative
    re-check against the same latch consulted by
    :cpp:func:`hpx::supervision::check_admission` runs immediately before the
    wrapped action executes, on the same thread, closing the
    admission/invocation race. See
    :doc:`/manual/supervision_dispatch` for the full client-side-filtering
    versus fenced-admission narrative and worked examples.

    :throws: ``hpx::exception`` with ``hpx::error::target_fenced`` if the
             target has latched a terminal event for ``epoch`` since the
             client-side check; the wrapped action is not invoked in that
             case.

Testing support
---------------

The following are declared in ``hpx/supervision_dispatch/testing.hpp`` and are
**not** part of the stable public dispatch API; they exist solely to make
unit tests for this module deterministic:

.. cpp:function:: std::vector<hpx::supervision::server::peer_snapshot> hpx::supervision::testing::local_snapshot_peers()
.. cpp:function:: void hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(hpx::chrono::steady_duration const& timeout)
.. cpp:function:: hpx::id_type hpx::supervision::testing::last_join_shadow()
.. cpp:function:: void hpx::supervision::testing::suspend_heartbeat_for_testing()
.. cpp:function:: bool hpx::supervision::testing::failure_detection_sweep_in_flight_for_testing()

See the :ref:`API reference <modules_supervision_dispatch_api>` of this
module for more details, and :ref:`modules_supervision` for the underlying
lifecycle event, admission, and dispatch-outcome semantics this module
builds on.
