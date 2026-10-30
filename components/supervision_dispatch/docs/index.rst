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
for the semantics of :hpx:func:`hpx::supervision::check_admission` and
:hpx:type:`hpx::supervision::dispatch_outcome`; this page documents only the
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
   :hpx:func:`hpx::supervision::current_epoch`      :ref:`Dispatch-cycle epoch of a locality's runtime. <supervision_dispatch_state_query>`
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
   :hpx:struct:`hpx::supervision::joined_peer`              :ref:`Dispatch target and join epoch returned by join(). <supervision_dispatch_registry_client>`
   :hpx:struct:`hpx::supervision::discovered_peer`          :ref:`A discovered peer with locality and join epoch. <supervision_dispatch_peer_discovery>`
   :hpx:struct:`hpx::supervision::fenced_action`            :ref:`Action wrapper that runs the authoritative re-check. <supervision_dispatch_fenced_dispatch>`
   :hpx:struct:`hpx::supervision::make_fenced_action`       :ref:`Derives the fenced_action type for an action. <supervision_dispatch_fenced_dispatch>`
   =======================================================  ===============================================================================================

.. _supervision_dispatch_init:

Lifecycle initialization and shutdown
-------------------------------------

.. cpp:function:: hpx::shared_future<hpx::supervision::registry> hpx::supervision::init(hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)
.. cpp:function:: hpx::supervision::registry hpx::supervision::init(hpx::launch::sync_policy, hpx::chrono::steady_duration const& discovery_timeout = hpx::supervision::default_discovery_timeout)

    Performs one-shot, idempotent initialization of the supervision-dispatch
    runtime for this locality: creates a local registry, publishes
    ``event::started`` before its symbol name is registered, registers that
    name, and performs a single :hpx:func:`discover_and_join() <hpx::supervision::discover_and_join>` pass. Calling
    :hpx:func:`init() <hpx::supervision::init>` while already active is a no-op; concurrent callers during
    ``initializing`` attach to the in-flight operation instead of racing it.

.. cpp:function:: void hpx::supervision::finalize()

    Performs one-shot, idempotent teardown of the runtime previously started
    by :hpx:func:`init() <hpx::supervision::init>`: publishes ``event::completed`` at the current/unchanged
    epoch, unregisters the registry's symbol name, and releases the
    registry. A no-op unless the runtime is currently ``active``. It returns
    promptly even while a failure detection sweep is still waiting on
    unresponsive peers, and ``hpx::disconnect()`` or ``hpx::stop()`` may
    follow right away: the sweep's outstanding waits are resolved at the
    latest during runtime shutdown and do no further work once the
    supervision runtime has been finalized.

.. cpp:function:: bool hpx::supervision::is_initialized() noexcept

    Returns whether the runtime is currently ``active``. Never blocks; safe
    to call from any thread, including while :hpx:func:`init() <hpx::supervision::init>`/:hpx:func:`finalize() <hpx::supervision::finalize>` is in
    flight.

.. _supervision_dispatch_peer_discovery:

Peer discovery
--------------

.. cpp:var:: constexpr std::chrono::milliseconds hpx::supervision::default_discovery_timeout

    Default timeout (60 seconds) used by :hpx:func:`init() <hpx::supervision::init>`, :hpx:func:`discover_peers() <hpx::supervision::discover_peers>`,
    :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>`, and :hpx:func:`discover_and_join() <hpx::supervision::discover_and_join>`.

.. cpp:var:: constexpr std::uint64_t hpx::supervision::unjoined_epoch

    Sentinel (``std::numeric_limits<std::uint64_t>::max()``) stored in
    ``discovered_peer::join_epoch`` for a peer that has not been joined yet.
    ``0`` cannot serve this purpose because it is a legitimate join epoch.

.. cpp:struct:: hpx::supervision::discovered_peer

    A peer whose registry symbol name resolved during a
    :hpx:func:`discover_peers() <hpx::supervision::discover_peers>` call. Holds ``locality``, ``registry_client``, and
    ``join_epoch``. :hpx:func:`discover_peers() <hpx::supervision::discover_peers>` leaves ``join_epoch`` at
    :hpx:var:`hpx::supervision::unjoined_epoch <hpx::supervision::unjoined_epoch>` (never ``0``, which is a legitimate
    join epoch); :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>` fills it in with the epoch recorded by
    :hpx:func:`registry::join() <hpx::supervision::registry::join>`. ``locality`` is cached for convenience but is
    never independently resolved or validated: it is always derived from
    ``registry_client`` itself (see :hpx:func:`registry::get_locality() <hpx::supervision::registry::get_locality>`), since
    exactly one registry exists per locality.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::discover_peers(hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Performs a one-time discovery pull: concurrently resolves the pinned
    registry name of every remote locality, bounded by a single wait for
    ``timeout``. Localities that have not yet called :hpx:func:`init() <hpx::supervision::init>` are
    silently excluded rather than causing a hang or failure.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::fan_out_join(hpx::supervision::registry const& local_registry, std::vector<hpx::supervision::discovered_peer> const& peers, hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Fans out :hpx:func:`join() <hpx::supervision::registry::join>` calls from ``local_registry`` to every entry in
    ``peers``. Reuses :hpx:func:`registry::join() <hpx::supervision::registry::join>`'s reservation/idempotency
    machinery, so repeated or overlapping calls never create more than one
    shadow per peer locality.

    Returns a :hpx:struct:`discovered_peer <hpx::supervision::discovered_peer>` for each peer whose :hpx:func:`join() <hpx::supervision::registry::join>`
    call settled successfully within ``timeout``, in the same relative
    order as ``peers``, with ``join_epoch`` filled in. Peers whose :hpx:func:`join() <hpx::supervision::registry::join>`
    call did not settle in time, or settled with an exception, are omitted
    from the result (rather than left as gaps or defaulted entries), so the
    returned vector is a same-order *subset* of ``peers``, not index-aligned
    with it. A join that timed out is retracted via :hpx:func:`registry::leave() <hpx::supervision::registry::leave>` if
    it later succeeds; a join that failed has nothing to retract. Either way,
    :hpx:func:`snapshot_peers() <hpx::supervision::registry::snapshot_peers>` never reports a peer this call dropped.

.. cpp:function:: std::vector<hpx::supervision::discovered_peer> hpx::supervision::discover_and_join(hpx::supervision::registry const& local_registry, hpx::chrono::steady_duration const& timeout = hpx::supervision::default_discovery_timeout)

    Composes a single discovery-and-join pass: one :hpx:func:`discover_peers() <hpx::supervision::discover_peers>`
    pull followed by one :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>` call, then a short poll of
    ``local_registry.snapshot_peers()`` until every joined peer is visible
    there. ``timeout`` is a single budget shared by all three steps.
    Introduces no background thread, timer, or repeated broadcast.

    Returns the :hpx:struct:`discovered_peer <hpx::supervision::discovered_peer>` vector produced by :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>`.
    If the visibility poll runs out of time, the peers that did join are
    still returned, even though :hpx:func:`snapshot_peers() <hpx::supervision::registry::snapshot_peers>` may not list them yet.

.. _supervision_dispatch_state_query:

State query and event publication
---------------------------------

.. cpp:function:: hpx::future<std::uint64_t> hpx::supervision::current_epoch(hpx::id_type const& locality)
.. cpp:function:: std::uint64_t hpx::supervision::current_epoch(hpx::launch::sync_policy, hpx::id_type const& locality, hpx::error_code& ec = hpx::throws)

    Returns the dispatch-cycle epoch of the supervision-dispatch runtime on
    ``locality``: the counter :hpx:func:`init() <hpx::supervision::init>` increments once per initialization
    sequence (before registration and discovery, so a sequence that later
    fails still advances it) and publishes ``event::started`` under. ``0``
    if no initialization sequence has run on ``locality``. This is unrelated
    to any target's ``lifecycle_state::epoch``; :hpx:func:`registry::join() <hpx::supervision::registry::join>` uses it
    to seed a new peer's shadow from the peer's real epoch.

.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::supervision::registry const& handle)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::error_code& ec = hpx::throws)
.. cpp:function:: hpx::future<hpx::supervision::lifecycle_state> hpx::supervision::query_state(hpx::supervision::registry const& handle, hpx::supervision::discovered_peer const& peer)
.. cpp:function:: hpx::supervision::lifecycle_state hpx::supervision::query_state(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::supervision::discovered_peer const& peer, hpx::error_code& ec = hpx::throws)

    The self variants resolve both locality and target to
    :hpx:func:`hpx::find_here() <hpx::find_here>` and query it on the local locality; the peer
    variants resolve both locality and target from ``peer.locality`` instead
    - ``handle`` is accepted only for overload resolution and symmetry with
    the self variants, and is otherwise unused in that case. Each overload
    is a thin forwarder over the corresponding raw-id :hpx:func:`query_state() <hpx::supervision::query_state>`
    overload in :ref:`modules_supervision`; see that module's reference for
    the fields of the returned :hpx:struct:`lifecycle_state <hpx::supervision::lifecycle_state>`, including the ``epoch``
    value useful for recovering the epoch a handle's locality was started
    at.

.. cpp:function:: hpx::future<hpx::supervision::publish_result> hpx::supervision::publish_event(hpx::supervision::registry const& handle, hpx::supervision::event ev, std::uint64_t epoch = 0)
.. cpp:function:: hpx::supervision::publish_result hpx::supervision::publish_event(hpx::launch::sync_policy, hpx::supervision::registry const& handle, hpx::supervision::event ev, std::uint64_t epoch = 0, hpx::error_code& ec = hpx::throws)

    Handle-based convenience overloads of :hpx:func:`hpx::supervision::publish_event <hpx::supervision::publish_event>`
    that resolve both locality and target to :hpx:func:`hpx::find_here() <hpx::find_here>` instead of
    requiring the caller to name them explicitly. Thin forwarders over the
    raw-id :hpx:func:`publish_event() <hpx::supervision::publish_event>` overloads in :ref:`modules_supervision`.
    There is no peer variant: a locality publishes lifecycle events only for
    itself, never on behalf of a peer.
    The default epoch value of 0 is only correct for a locality's very first
    publication after :hpx:func:`init() <hpx::supervision::init>`; once the target's real epoch has advanced,
    0 is stale. Callers publishing events after the initial one should first
    obtain the current epoch - e.g. via
    ``query_state(hpx::launch::sync, handle).epoch`` - and pass it explicitly.

.. _supervision_dispatch_registry_client:

Registry client
---------------

.. cpp:class:: hpx::supervision::registry

    A lightweight, self-supervising client handle for pairing with peer
    localities; mirrors a peer's lifecycle state locally via :hpx:func:`join() <hpx::supervision::registry::join>` and
    can itself be discovered by name in AGAS.

    .. cpp:function:: registry() = default

        Constructs an empty client that is not connected to any component.
        Does not create a server-side component.

    .. cpp:function:: explicit registry(hpx::id_type const& target_locality)

        Creates a new registry component on ``target_locality``.

    .. cpp:function:: explicit registry(std::string const& symbolic_name)

        Connects to an existing registry component registered under
        ``symbolic_name`` (see :hpx:func:`register_name() <hpx::supervision::registry::register_name>`).

    .. cpp:function:: registry(hpx::future<hpx::id_type>&& f)

        Implicit conversion from a future holding a component id.

    .. cpp:function:: hpx::future<hpx::supervision::joined_peer> join(hpx::id_type const& peer_locality) const
    .. cpp:function:: hpx::supervision::joined_peer join(hpx::launch::sync_policy, hpx::id_type const& peer_locality, hpx::error_code& ec = hpx::throws) const

        Joins a peer locality: creates (or reuses) local supervision state
        mirroring the peer's lifecycle. The returned ``joined_peer::target``
        is ``peer_locality`` itself (not a generated local id) and is what
        :hpx:func:`dispatch_work() <hpx::supervision::dispatch_work>` colocates against. Joining a peer that is already
        fully joined returns the existing :hpx:struct:`joined_peer <hpx::supervision::joined_peer>` without seeding or
        registering observers again.

    .. cpp:function:: void leave(hpx::id_type const& peer_locality, hpx::future<hpx::supervision::joined_peer>&& f) const

        Attaches a fire-and-forget continuation to a pending :hpx:func:`join() <hpx::supervision::registry::join>`
        future that retracts the join once it settles. Used by
        :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>` for joins that did not complete within its
        timeout. Exceptions from the underlying :hpx:func:`join() <hpx::supervision::registry::join>` are swallowed.

    .. cpp:function:: hpx::future<std::vector<hpx::supervision::server::peer_snapshot>> snapshot_peers() const
    .. cpp:function:: std::vector<hpx::supervision::server::peer_snapshot> snapshot_peers(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

        Returns a point-in-time snapshot of all fully joined, non-evicting
        peers.

    .. cpp:function:: hpx::future<bool> register_name()
    .. cpp:function:: bool register_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws)
    .. cpp:function:: hpx::future<hpx::id_type> unregister_name() const
    .. cpp:function:: hpx::id_type unregister_name(hpx::launch::sync_policy, hpx::error_code& ec = hpx::throws) const

    .. cpp:function:: hpx::id_type get_locality() const

        Returns the locality hosting this registry's component.

.. cpp:struct:: hpx::supervision::joined_peer

    Returned by :hpx:func:`registry::join() <hpx::supervision::registry::join>`. Holds ``target``, the peer's locality
    (the value passed to :hpx:func:`join() <hpx::supervision::registry::join>`), and ``join_epoch``, the epoch the peer
    was joined at. ``join_epoch`` is used by :hpx:func:`registry::leave() <hpx::supervision::registry::leave>` to tell a
    join apart from a later re-join of the same locality. Note that
    ``joined_peer::join_epoch`` defaults to ``0``, while
    ``discovered_peer::join_epoch`` defaults to :hpx:var:`unjoined_epoch <hpx::supervision::unjoined_epoch>`.

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
    target's own locality (via :hpx:func:`hpx::colocated <hpx::colocated>`) where an authoritative
    re-check against the same latch consulted by
    :hpx:func:`hpx::supervision::check_admission` runs immediately before the
    wrapped action executes, on the same thread, closing the
    admission/invocation race. See
    :doc:`/manual/supervision_dispatch` for the full client-side-filtering
    versus fenced-admission narrative and worked examples.

    :returns: A future holding the result of the dispatched action. If
              either admission check rejects the call, the future becomes
              exceptional with ``hpx::error::target_fenced`` (surfacing at
              ``get()``), and the wrapped action is not invoked.
              :hpx:func:`dispatch_work() <hpx::supervision::dispatch_work>` itself does not throw for a fenced target.

.. cpp:function:: template <typename Action, typename... Ts> decltype(auto) hpx::supervision::dispatch_work(Action, hpx::id_type const& target, std::uint64_t epoch, Ts&&... ts)

    Same as above, with the action passed as an instance instead of a
    template argument. Constrained to ``hpx::traits::is_action_v<Action>``.

.. cpp:function:: template <typename Action, typename... Ts> decltype(auto) hpx::supervision::dispatch_work(Action, hpx::supervision::discovered_peer const& peer, Ts&&... ts)

    Dispatches against a peer returned by :hpx:func:`fan_out_join() <hpx::supervision::fan_out_join>` or
    :hpx:func:`discover_and_join() <hpx::supervision::discover_and_join>`; forwards to
    ``dispatch_work<Action>(peer.locality, peer.join_epoch, ts...)``. A peer
    whose ``join_epoch`` is still :hpx:var:`unjoined_epoch <hpx::supervision::unjoined_epoch>` (e.g. one returned by
    :hpx:func:`discover_peers() <hpx::supervision::discover_peers>` alone) is rejected up front with the same
    ``target_fenced`` exceptional future.

.. cpp:function:: template <std::meta::info F, typename... Ts> decltype(auto) hpx::supervision::dispatch_work(hpx::id_type const& target, std::uint64_t epoch, Ts&&... ts)
.. cpp:function:: template <std::meta::info F, typename... Ts> decltype(auto) hpx::supervision::dispatch_work(hpx::supervision::discovered_peer const& peer, Ts&&... ts)

    Reflection-based overloads, available only when
    ``HPX_HAVE_CXX26_REFLECTION`` is defined. ``F`` is a reflection of a
    namespace-scope free function (``^^fn``); these forward to the
    corresponding overloads above with
    ``hpx::actions::reflect_action<F>`` as the action.

.. cpp:function:: template <typename Action, typename IdType, typename Epoch, typename... Ts> decltype(auto) hpx::supervision::invoke_fenced_action(Action act, IdType target, Epoch const epoch, Ts... ts)

    The body that runs on the target's locality once :hpx:func:`dispatch_work() <hpx::supervision::dispatch_work>` has
    dispatched. Performs the authoritative :hpx:func:`check_admission() <hpx::supervision::check_admission>` re-check
    and, if admitted, invokes ``act`` via :hpx:func:`hpx::sync <hpx::sync>` with ``target`` and
    ``ts...`` (``epoch`` is not forwarded).

    :throws: :hpx:class:`hpx::exception <hpx::exception>` with ``hpx::error::target_fenced`` if the
             target has latched a terminal event for ``epoch``; ``act`` is
             not invoked in that case. The caller of :hpx:func:`dispatch_work() <hpx::supervision::dispatch_work>`
             sees this as the exceptional state of the returned future.

.. cpp:struct:: template <typename Action, typename IdType, typename Epoch, typename... Ts> hpx::supervision::fenced_action

    HPX action wrapper for ``invoke_fenced_action<Action, IdType, Epoch,
    Ts...>``, dispatched by ``dispatch_work()`` via ``hpx::colocated``.

.. cpp:struct:: template <typename Action> hpx::supervision::make_fenced_action

    Type trait yielding the :hpx:struct:`fenced_action <hpx::supervision::fenced_action>` specialization for ``Action``,
    with ``IdType`` fixed to :hpx:class:`hpx::id_type <hpx::id_type>`, ``Epoch`` to
    ``std::uint64_t``, and the remaining arguments taken from
    ``Action::arguments_type``. Constrained to
    ``hpx::traits::is_action_v<Action>``. ``make_fenced_action_t<Action>``
    is shorthand for ``make_fenced_action<Action>::type``.

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
.. cpp:function:: void hpx::supervision::testing::stop_background_loops()

See the :ref:`API reference <modules_supervision_dispatch_api>` of this
module for more details, and :ref:`modules_supervision` for the underlying
lifecycle event, admission, and dispatch-outcome semantics this module
builds on.
