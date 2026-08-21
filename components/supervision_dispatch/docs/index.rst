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

Functions
---------

.. table:: `hpx::supervision` dispatch functions

   ===============================================  ===========================================================================================
   Function                                         Description
   ===============================================  ===========================================================================================
   :hpx:func:`hpx::supervision::init`               :ref:`One-shot, idempotent runtime initialization. <supervision_dispatch_init>`
   :hpx:func:`hpx::supervision::finalize`           :ref:`One-shot, idempotent runtime teardown. <supervision_dispatch_init>`
   :hpx:func:`hpx::supervision::is_initialized`     :ref:`Whether the runtime is currently active. <supervision_dispatch_init>`
   :hpx:func:`hpx::supervision::discover_peers`     :ref:`One-time discovery pull across localities. <supervision_dispatch_peer_discovery>`
   :hpx:func:`hpx::supervision::fan_out_join`       :ref:`Fan out join() calls to discovered peers. <supervision_dispatch_peer_discovery>`
   :hpx:func:`hpx::supervision::discover_and_join`  :ref:`Composed discovery-and-join pass. <supervision_dispatch_peer_discovery>`
   :hpx:func:`hpx::supervision::dispatch_work`      :ref:`Dispatch an action under supervision fencing. <supervision_dispatch_fenced_dispatch>`
   ===============================================  ===========================================================================================

Types
-----

.. table:: `hpx::supervision` dispatch types

   =======================================================  ===============================================================================================
   Type                                                     Description
   =======================================================  ===============================================================================================
   :hpx:class:`hpx::supervision::registry`                  :ref:`Client handle mirroring a peer's lifecycle state. <supervision_dispatch_registry_client>`
   :hpx:struct:`hpx::supervision::server::peer_snapshot`    :ref:`Point-in-time view of a joined peer. <supervision_dispatch_registry_client>`
   :hpx:struct:`hpx::supervision::discovered_peer`          :ref:`A discovered peer with locality and join epoch. <supervision_dispatch_peer_discovery>`
   =======================================================  ===============================================================================================

.. _supervision_dispatch_init:

Lifecycle initialization and shutdown
-------------------------------------

.. cpp:function:: hpx::shared_future<hpx::supervision::registry> hpx::supervision::init(hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)
.. cpp:function:: hpx::supervision::registry hpx::supervision::init(hpx::launch::sync_policy, hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)

    Performs one-shot, idempotent initialization of the supervision-dispatch
    runtime for this locality: creates a local registry, publishes
    ``event::started`` before its symbol name is registered, registers that
    name, and performs a single ``discover_and_join()`` pass. Calling
    ``init()`` while already active is a no-op; concurrent callers during
    ``initializing`` attach to the in-flight operation instead of racing it.

.. cpp:function:: void hpx::supervision::finalize()

    Performs one-shot, idempotent teardown of the runtime previously started
    by ``init()``: publishes ``event::completed`` at the current/unchanged
    epoch, unregisters the registry's symbol name, and releases the
    registry. A no-op unless the runtime is currently ``active``.

.. cpp:function:: bool hpx::supervision::is_initialized() noexcept

    Returns whether the runtime is currently ``active``. Never blocks; safe
    to call from any thread, including while ``init()``/``finalize()`` is in
    flight.

.. _supervision_dispatch_peer_discovery:

Peer discovery
--------------

.. cpp:struct:: hpx::supervision::discovered_peer

    A peer whose registry symbol name resolved during a
    ``discover_peers()`` call. Holds ``locality``, ``registry_client``, and
    ``join_epoch``. ``discover_peers()`` leaves ``join_epoch`` at
    ``hpx::supervision::unjoined_epoch`` (never ``0``, which is a legitimate
    join epoch); ``fan_out_join()`` fills it in with the epoch recorded by
    ``registry::join()``. ``locality`` is cached for convenience but is
    never independently resolved or validated: it is always derived from
    ``registry_client`` itself (see ``registry::get_locality()``), since
    exactly one registry exists per locality.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::discover_peers(hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Performs a one-time discovery pull: concurrently resolves the pinned
    registry name of every remote locality, bounded by a single wait for
    ``timeout``. Localities that have not yet called ``init()`` are
    silently excluded rather than causing a hang or failure.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::fan_out_join(...)

    Fans out ``join()`` calls from ``local_registry`` to every entry in
    ``peers``. Reuses ``registry::join()``'s reservation/idempotency
    machinery, so repeated or overlapping calls never create more than one
    shadow per peer locality.

    Returns a ``discovered_peer`` for each peer whose ``join()``
    call settled successfully within ``timeout``, in the same relative
    order as ``peers``. Peers whose ``join()`` call did not settle in time
    are omitted from the result (rather than left as gaps or defaulted
    entries), so the returned vector is a same-order *subset* of ``peers``,
    not index-aligned with it.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::discover_and_join(hpx::supervision::registry const& local_registry, hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Composes a single reactive discovery-and-join pass: one
    ``discover_peers()`` pull followed by one ``fan_out_join()`` call.
    Introduces no polling, timer, or repeated broadcast of its own.

    Returns the same ``discovered_peer`` vector produced by
    ``fan_out_join()`` for the peers it discovered.

State query and event publication
---------------------------------

.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::supervision::registry const& handle)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::supervision::registry const& handle, hpx::supervision::discovered_peer const& peer)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::supervision::discovered_peer const& peer, hpx::error_code& ec = hpx::throws)

    The self variants resolve both locality and target to
    ``hpx::find_here()`` and query it on the local locality; the peer
    variants resolve both locality and target from ``peer.locality`` instead
    - ``handle`` is accepted only for overload resolution and symmetry with
    the self variants, and is otherwise unused in that case. Each overload
    is a thin forwarder over the corresponding raw-id ``query_state()``
    overload in :ref:`modules_supervision`; see that module's reference for
    the fields of the returned ``lifecycle_state``, including the ``epoch``
    value useful for recovering the epoch a handle's locality was started
    at.

.. cpp:function:: hpx::future<hpx::supervision::publish_result> hpx::supervision::publish_event(hpx::supervision::registry const& handle, hpx::supervision::event ev, std::uint64_t epoch = 0)
.. cpp:function:: hpx::supervision::publish_result hpx::supervision::publish_event(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::supervision::event ev, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws)

    Handle-based convenience overloads of ``hpx::supervision::publish_event``
    that resolve both locality and target to ``hpx::find_here()`` instead of
    requiring the caller to name them explicitly. Thin forwarders over the
    raw-id ``publish_event()`` overloads in :ref:`modules_supervision`.
    There is no peer variant: a locality publishes lifecycle events only for
    itself, never on behalf of a peer.
    The default epoch value of 0 is only correct for a locality's very first
    publication after init(); once the target's real epoch has advanced,
    0 is stale. Callers publishing events after the initial one should first
    obtain the current epoch - e.g. via
    query_state(hpx::launch::sync, handle).epoch - and pass it explicitly."

.. _supervision_dispatch_registry_client:

Registry client
---------------

.. cpp:class:: hpx::supervision::registry

    A lightweight, self-supervising client handle for pairing with peer
    localities; mirrors a peer's lifecycle state locally via ``join()`` and
    can itself be discovered by name in AGAS.

    .. cpp:function:: hpx::future<hpx::supervision::joined_peer> join(hpx::id_type const& peer_locality) const
    .. cpp:function:: hpx::supervision::joined_peer join(hpx::launch::sync_policy, hpx::id_type const& peer_locality, hpx::error_code& ec = hpx::throws) const

        Joins a peer locality: creates (or reuses) local supervision state
        mirroring the peer's lifecycle. The returned ``joined_peer::target``
        is ``peer_locality`` itself (not a generated local id) and is what
        ``dispatch_work()`` colocates against.

    .. cpp:function:: hpx::future<std::vector<hpx::supervision::server::peer_snapshot>> snapshot_peers() const
    .. cpp:function:: std::vector<hpx::supervision::server::peer_snapshot> snapshot_peers(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

        Returns a point-in-time snapshot of all fully joined, non-evicting
        peers.

    .. cpp:function:: hpx::future<bool> register_name()
    .. cpp:function:: bool register_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws)
    .. cpp:function:: hpx::future<hpx::id_type> unregister_name() const
    .. cpp:function:: hpx::id_type unregister_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

.. cpp:struct:: hpx::supervision::server::peer_snapshot

    Plain-data view of a single joined peer: ``peer_locality`` and
    ``join_epoch``.

.. _supervision_dispatch_fenced_dispatch:

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

.. _supervision_dispatch_testing_support:

Testing support
---------------

The following are declared in ``hpx/supervision_dispatch/testing.hpp`` and are
**not** part of the stable public dispatch API; they exist solely to make
unit tests for this module deterministic:

.. cpp:function:: std::vector<hpx::supervision::server::peer_snapshot> hpx::supervision::testing::local_snapshot_peers()
.. cpp:function:: void hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(hpx::chrono::steady_duration const& timeout)
.. cpp:function:: hpx::id_type hpx::supervision::testing::last_join_locality()
.. cpp:function:: void hpx::supervision::testing::suspend_heartbeat_for_testing()
.. cpp:function:: bool hpx::supervision::testing::failure_detection_sweep_in_flight_for_testing()

See the :ref:`API reference <modules_supervision_dispatch_api>` of this
module for more details, and :ref:`modules_supervision` for the underlying
lifecycle event, admission, and dispatch-outcome semantics this module
builds on.
