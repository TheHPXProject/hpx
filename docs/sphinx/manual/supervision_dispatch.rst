..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _supervision_dispatch:

====================
Supervision dispatch
====================

The supervision dispatch component (``hpx::supervision``, built on top
of the :ref:`modules_supervision` module) lets a locality discover its peers,
detect when one of them has gone silent, and safely stop dispatching work to a
peer that has latched a terminal lifecycle event - all without every caller
needing to make an authoritative, synchronous round-trip for every dispatch.

This page describes the initialization and shutdown ordering applications must
follow, the one-shot nature of peer discovery, the distinction between the
locally cached peer state and the registry's authoritative state, and how that
distinction shows up in the two-tier admission check used by fenced dispatch.
For the detailed semantics of individual lifecycle events, ``check_admission``,
and ``dispatch_outcome`` values, see :ref:`modules_supervision`; this page does
not repeat that reference material. For the API reference of the
dispatch-specific types and functions this component introduces -
``registry``, ``sentinel``, ``discovery::discover_and_join``, and
``dispatch_work`` - see :ref:`modules_supervision_dispatch`.

Initialization and shutdown ordering
=====================================

Supervision dispatch is brought up with a single call to
:hpx:func:`hpx::supervision::init` and torn down with a single call to
:hpx:func:`hpx::supervision::finalize`:

.. code-block:: c++

    #include <hpx/supervision_dispatch.hpp>

    // Blocking, one-shot startup for this locality.
    hpx::supervision::init(hpx::launch::sync);

    // ... application runs, dispatching fenced work to peers ...

    // Cooperative, one-shot shutdown for this locality.
    hpx::supervision::finalize();

``init()`` performs, in order:

#. Creation of a local ``sentinel`` and ``registry`` component for this
   locality.
#. Publication of ``event::started`` on the sentinel - *before* either
   component's symbol name is registered with AGAS, so that no peer can
   discover and join a not-yet-started sentinel.
#. Registration of both symbol names.
#. A single :hpx:func:`hpx::supervision::discover_and_join` pass that joins
   every peer registry reachable within ``discovery_timeout``. Its result is
   a list of ``joined_discovery_result`` (peer paired with the shadow id its
   ``join()`` call was assigned); peers that time out mid-pass are omitted
   from that list entirely rather than leaving a gap in it.

``init()`` is idempotent: calling it while already active is a no-op, and
concurrent callers attach to a single in-flight initialization rather than
racing to create duplicate sentinels/registries. ``finalize()`` mirrors this
ordering in reverse - it publishes ``event::completed`` for the local
sentinel, unregisters both symbol names, and releases the components - and is
always safe to call speculatively (e.g. during shutdown) even if ``init()``
was never called, since it is a no-op unless the runtime is currently active.

Applications should treat ``init()``/``finalize()`` as a matched pair around
the portion of the program that performs supervised, fenced dispatch, and
should not assume any lifecycle event is visible to peers before ``init()``'s
future (or synchronous call) has completed.

Calling ``init()`` again after it has already succeeded is a documented
no-op: it returns an already-ready future without repeating any of the four
steps above, and in particular does **not** re-run ``discover_and_join()``.
Peers that started, or otherwise became discoverable, after the first
``init()`` call completed are *not* retroactively added to the registry by a
second call -- the one-shot discovery behavior described below applies
equally to repeated ``init()`` calls, not just the first one. To pick up such
a peer, join it explicitly via ``registry::join()``. Calling ``init()`` while
a previous call is still in progress attaches to that same in-flight
initialization rather than starting a second one; calling it while
``finalize()`` is concurrently in progress fails immediately with
``hpx::error::invalid_status`` rather than queuing behind it.

What is a target, and how is it introduced
=============================================

A "target" is nothing more than an ``hpx::id_type`` - the id of any locality or
component whose lifecycle should be tracked. It is not a distinct kind of
object, and there is no separate handle or wrapper type: the same id you'd
use to invoke an action on a component is what you pass as ``target`` to
``publish_event()``, ``check_admission()``, ``dispatch_work()``, and the rest
of the supervision API.

Because ``dispatch_work()`` resolves ``target`` via ``hpx::colocated()`` to
find the locality to dispatch to, a target must also be independently
resolvable by AGAS to some locality - this is a separate requirement from
having recorded supervision state, and it is not something supervision
dispatch itself grants. Any id obtained the ordinary way (creating a
component, ``hpx::find_here()``, a symbolic-name lookup, a peer's joined
sentinel/registry id) already satisfies this. If a target id can no longer be
resolved to a locality - for instance, because the underlying component was
already destroyed and unregistered - ``dispatch_work()`` fails when
``hpx::colocated()`` cannot resolve it, *before* either admission check
runs; this is a distinct failure mode from ``hpx::error::target_fenced`` and
is not something publishing ``event::started`` can paper over.

In practice, two kinds of ids show up as targets in this component:

- The sentinel/registry ids of peer localities, once joined via
  :hpx:func:`hpx::supervision::registry::join` - these are the targets whose
  shadow state feeds ``registry::snapshot_peers()`` and local admission
  filtering.
- The ids of individual localities/components that fenced work is dispatched
  against via :hpx:func:`hpx::supervision::dispatch_work`
  (``dispatch_work(target, epoch, ...)``) - these need not be peers'
  localities at all; any component id that has had lifecycle events published
  for it can be fenced.

Unlike joining a peer registry, there is no explicit "register this target"
call for the underlying supervision manager. A target is introduced
implicitly, purely as a side effect of publishing its first event: the
supervision manager tracks targets in a map keyed by id, and a lookup miss is
treated as "this target has never been seen," with an implicit current epoch
of ``0`` and a default event of ``unknown``. The lifecycle-transition
validation then requires that the *first* event ever published for a target
open with ``started`` - publishing anything else for a target with no prior
recorded state is rejected as an invalid transition. Put simply:

.. code-block:: c++

    // Introduces `some_actor` as a tracked target under epoch 0. This is the
    // only step required - no prior registration call exists.
    hpx::supervision::publish_event(some_actor, hpx::supervision::event::started, 0);

    // From this point on, `some_actor` has recorded state: query_state(),
    // check_admission(), and dispatch_work() against it now reflect that
    // state rather than "unknown."

Before a target's first ``started`` publication, it has no recorded state at
all: ``query_state()`` reports it as ``unknown`` rather than as any specific
lifecycle stage, and ``check_admission()`` for an epoch-0, never-published
target behaves as admitting (there is nothing latched to fence against yet).
Once introduced, a target's tracked state persists on the supervision manager
that owns it until either it reaches a terminal event that is superseded by a
higher epoch (see "Epochs and when to change them" below), or it is
explicitly forgotten via ``remove_target()`` - typically done by callers
that know a given id will never be queried or observed again locally, e.g.
after a failed registration attempt or once a peer has been evicted from the
registry, to avoid accumulating unbounded per-target state.

Removing a target
-----------------

:hpx:func:`hpx::supervision::remove_target` is the counterpart to
introducing a target: it unconditionally erases whatever supervision state
is locally recorded for a target id, on a single locality.

.. code-block:: c++

    // Synchronous version, target on a possibly remote locality:
    hpx::supervision::remove_target(hpx::launch::sync, locality, target);

See :ref:`modules_supervision` for the full semantics of ``remove_target``,
and :ref:`modules_supervision_dispatch` for this component's own API
reference.

A few points worth calling out:

- **It is a forget operation, not a lifecycle event.** It erases the
  target's tracked state (epoch, current event, sequence number) and any
  observers still registered for it, without publishing an event of any
  kind. This is different from a target reaching a terminal event --
  removal discards history rather than recording an outcome.
- **It is local only.** Removal only clears state held on the given
  ``locality`` (or the calling locality, for the local-only overload). It
  does not touch ``target``'s state on any other locality, and it does not
  affect AGAS registration or symbolic names for that id.
- **It is safe to call on a target with no recorded state.** This is a
  documented no-op rather than an error, so callers do not need to first
  check whether a target was ever introduced before removing it.
- **Republishing afterward reintroduces the target from scratch.** After
  removal, publishing ``event::started`` for the same id behaves exactly as
  if the id had never been seen before - epoch resets to whatever the new
  ``started`` publication specifies.
- **Safe to call from within the target's own observer callback.** Calling
  ``remove_target()`` on ``target`` from inside one of ``target``'s own
  observer callbacks is supported and does not deadlock.

Removing a target is the natural cleanup step once it is known to be
permanently done with - for example, once a peer has been evicted from the
registry, or once a fenced target has reached a terminal event and will not
be reincarnated under a new epoch - so that per-target state does not
accumulate indefinitely over the lifetime of a long-running application.

Application-visible state machines
====================================

Applications observe two distinct, overlapping state machines.

Per-locality dispatcher lifecycle
---------------------------------

Tracked once per locality (``dispatch_api.cpp``), driven by ``init()``/
``finalize()``:

.. code-block:: text

    uninitialized -> initializing -> active -> finalizing -> uninitialized

- ``uninitialized``: no local registry exist.
- ``initializing``: registry creation, ``event::started``
  publication, symbol registration, and the one-shot ``discover_and_join()``
  pass are in progress. Concurrent callers attach to the same in-flight
  initialization rather than racing.
- ``active``: heartbeat and failure-detection background tasks are running.
- ``finalizing``: background tasks are being stopped, ``event::completed`` is
  published, and both symbol names are being unregistered before component
  release.

``init()``/``finalize()`` are idempotent no-ops when already in the target
state. A failure during ``initializing`` unregisters any names already
registered and returns to ``uninitialized`` rather than leaving the locality
stuck mid-transition.

Per-target lifecycle event
--------------------------

Reported by an actor about itself (``hpx::supervision::event``), tracked per
epoch and mirrored into peer shadows:

.. code-block:: text

    unknown -> started -> running -> suspending -> completed
                        \-> failed
                        \-> losing_locality

``completed`` and ``failed`` are terminal: once published for a target/epoch,
later terminal publications for that same target/epoch are rejected as
already-terminal, and publications carrying a lower epoch are rejected as
stale. ``losing_locality`` is a non-terminal warning that the hosting locality
may become unavailable.

How they interact
-----------------

The dispatcher-lifecycle state machine governs whether *this locality* can
participate at all. The per-target event state machine governs whether *a
specific peer* remains eligible for fenced dispatch. Failure detection moves
a peer's locally cached shadow toward ``failed`` only when a timeout on
``await_terminal`` coincides with an unchanged event sequence number - a
timeout alone is not sufficient, since the peer may simply still be
``running``. This local ``failed`` marking never touches the peer's own
authoritative state; it only affects what this locality's client-side
filtering sees, per the shadow/authoritative distinction above.

Heartbeats: distinguishing idle from dead
=========================================

While a locality is ``active`` (see "Application-visible state machines"
above), a background heartbeat task runs alongside the failure-detection
loop. Its job is narrow but essential: periodically republish
``event::running`` against the locality, purely to keep
that localty's event sequence number and timestamp advancing even when the
application itself has nothing new to report.

Why this is needed
---------------------

Failure detection works by calling ``await_terminal()`` with a timeout
against each joined peer's shadow. On its own, a bare timeout on that call is
ambiguous: it fires identically whether the peer has crashed, or the peer is
simply alive and quietly idle -- ``running`` is a stable state that can
legitimately persist for a long time with no events published at all. Without
some independent signal of liveness, failure detection would have no way to
tell these two situations apart, and would either fence peers that are
perfectly healthy but quiet, or fail to fence peers that have genuinely died.

The heartbeat resolves this by giving every alive peer a steady, low-cost
source of forward progress -- its event sequence number -- independent of
whatever the application is actually doing. This lets
``peer_is_alive_but_silent()`` compare a peer's sequence number immediately
before and after an ``await_terminal()`` timeout:

- If the sequence number **advanced** during the timeout window, the peer's
  heartbeat kept running, so the peer is alive but simply not reporting a
  terminal event - no action is taken.
- If the sequence number **did not change**, the peer's own heartbeat task
  has itself gone silent, which is treated as genuine evidence of a stall,
  and the failure-detection loop fences the peer locally by publishing
  ``event::failed`` against its shadow (never against the peer's own,
  remote, authoritative state - see "Shadow state versus the authoritative
  registry" above).

Operational properties
----------------------

- **Best-effort.** A failed heartbeat publication attempt does not stop or
  crash the loop; it is simply retried on the next interval.
- **Fixed interval, tied to detection timing.** The heartbeat republishes at
  roughly a third of the failure-detection poll timeout, so that under
  normal conditions several heartbeats land within any single detection
  window -- making a missed heartbeat a meaningful signal rather than noise
  from timing jitter.
- **Started and stopped in lockstep with failure detection.** Both
  background tasks are started together, immediately after the one-shot
  ``discover_and_join()`` pass inside ``init()``, and both are signaled to
  stop and joined together during ``finalize()``, before ``event::completed``
  is published and either symbol name is unregistered.
- **Entirely internal.** Applications do not start, stop, or configure the
  heartbeat directly, and do not need to publish any events themselves to
  keep it running -- it is a side effect of calling ``init()``, and requires
  no ongoing application action beyond staying inside the
  ``init()``/``finalize()`` bracket.

In short, the heartbeat is what makes failure detection's timeout-based
stall check meaningful at all: without it, a timeout by itself would not be
able to distinguish "no news" from "no pulse."

This is also why fenced dispatch depends on the heartbeat working correctly,
not just on failure detection running. ``check_admission()`` - whether
called as the caller's local, non-authoritative early-out or as the target's
own authoritative re-check inside ``invoke_fenced_action()`` - ultimately
answers "is this target still available" by consulting whatever lifecycle
state has been latched for it, which for peers is only ever updated to
``failed`` once failure detection has genuine evidence of a stall. Without
the heartbeat, that evidence would not exist: a silent-but-alive peer and a
genuinely dead one would look identical to ``await_terminal()``, and
admission would have no reliable basis for deciding whether it is safe to
keep dispatching work to that locality. In short, the heartbeat is not just
an internal detail of failure detection - it is what makes the fencing
decisions applications rely on throughout this page trustworthy in the first
place.

One-shot discovery and staleness
=================================

The ``discover_and_join()`` pass performed by ``init()`` runs exactly once. It
finds whichever peer registries are reachable *at that moment* and
joins them; it does not re-run periodically, and peers that join the system
later are not automatically discovered by localities that have already
initialized. This has two practical implications:

- A locality's set of known peers is fixed at ``init()`` time (plus any peers
  joined explicitly afterwards via ``registry::join()``). It does not grow on
  its own as new peers appear.
- Because failure detection (see below) only tracks peers that have actually
  been joined, a peer that starts too late to be discovered is invisible to
  both admission filtering and fencing until it is joined explicitly.

Applications that need a stable, complete peer set at some later point should
join peers explicitly (``registry::join()``) rather than relying on rediscovery
after ``init()``.

Shadow state versus the authoritative registry
==============================================

Once a peer is joined, its lifecycle state is mirrored locally into a
*shadow* target owned by the joining registry - a lightweight, locally
readable copy of that peer's most recently observed state, updated as
lifecycle and heartbeat events arrive. This shadow state is what
``registry::snapshot_peers()`` and the local admission check consult.

It is important to keep two things distinct:

- **Shadow state** (local): a locally cached view of a peer's lifecycle,
  updated asynchronously as events are received. It can lag reality by however
  long it takes an event to propagate, and can go stale entirely if the peer's
  heartbeat has stopped but failure detection has not yet timed out.
- **Registry state** (remote, authoritative): the actual lifecycle/fencing
  state tracked by the peer's own supervision manager, on the peer's own
  locality. This is the only state that is authoritative for admission
  decisions.

Failure detection narrows - but does not eliminate - the gap between the
two: a background loop periodically checks each joined peer's shadow for
silence and marks it accordingly, but there is always a window between a
peer actually going silent and that silence being locally observed.

How registries stay synchronized
=================================================

The shadow-mirroring described above is not maintained by polling. It is
driven entirely by event-based observer callbacks, registered once at
``registry::join()`` time and then firing automatically for the lifetime of
the joined peer.

Registration, at join time
--------------------------

When ``registry::join(peer_locality)`` is called, the local registry does
two things before returning a shadow id to the caller. A peer's identity
*is* its locality since exactly one registry exists per locality.

#. **Seeds the shadow with the peer's current state.** It performs a single,
   synchronous ``query_state()`` call against ``peer_locality`` to read
   whatever epoch the peer is currently in, then publishes ``event::started``
   for that epoch onto a freshly minted shadow id. This seed step exists so
   the shadow reflects the peer's state as of joining, rather than starting
   from ``unknown`` and waiting for the first subsequent event to arrive.
#. **Registers a lifecycle-event observer** on ``peer_locality`` (via
   ``hpx::supervision::register_observer()``), along with an
   activity-transition observer (via ``register_activity_observer()``,
   currently a no-op placeholder reserved for future use). These
   registrations live on the peer's own locality and are what make the
   subsequent synchronization automatic: no further polling or explicit
   refresh call from the local registry is needed.

Both registrations are made atomically with respect to concurrent
``join()`` calls for the same peer (a reservation is taken in ``peers_``
before either remote call is issued), and either observer failing to
register cleanly rolls back the other, so a peer is never left half-joined.

Ongoing synchronization
-----------------------

From then on, every event the peer publishes for itself - including the
heartbeat's periodic ``event::running`` republication (see "Heartbeats:
distinguishing idle from dead" above) -- invokes the registered lifecycle
observer callback on the *peer's* locality, which in turn calls back into
the joining registry to mirror that event onto the local shadow, at the same
epoch it actually occurred in. No round trip is initiated by the joining
side to "check in" on a peer; synchronization is entirely push-driven by
whichever locality actually observes the peer's own state changing.

If the mirrored event is terminal (``completed`` or ``failed``), the
callback additionally schedules - via ``hpx::post()``, deferred rather than
inline, since the callback itself does not hold the registry's lock - the
peer's eviction from ``peers_``. Once evicted, the peer no longer appears in
``registry::snapshot_peers()``, and its observer registrations are
unregistered on the peer's locality so nothing is left listening for further
events from a peer that has already reached a terminal state.

What this does and does not cover
---------------------------------

This automatic synchronization keeps the shadow up to date with whatever the
peer *chooses to publish* - it is not a substitute for failure detection.
A peer that stops publishing anything at all (e.g. because it crashed)
generates no event, so no observer callback fires, and the shadow simply
stops advancing. Detecting that silence, and deciding whether it means the
peer is dead rather than merely idle, is exactly the job of the
heartbeat/failure-detection pairing described earlier in this page -- the
observer-based synchronization described here and the polling-with-timeout
failure detection described earlier are complementary, not overlapping,
mechanisms.

Because ``observers_`` is keyed per target and holds a list rather than a
single callback, this fan-out is genuinely one-to-many: if several
localities have each independently joined the same peer, a single event
published by that peer notifies every one of their registries in the same
pass, each mirroring the event onto its own local shadow. This reach is
scoped to the publishing target, however -- only localities that have
actually called ``join()`` against that specific peer are registered as
observers and receive its events. A locality that has not joined a given
peer is not notified when that peer publishes, regardless of whether it
participates in supervision dispatch generally.

Client-side filtering versus fenced-dispatch admission
=========================================================

This shadow/authoritative distinction is exactly why fenced dispatch performs
*two* admission checks rather than one:

- **Client-side filtering** (``check_admission()`` called directly by the
  caller, or peer filtering based on ``registry::snapshot_peers()``) consults
  only the locally cached shadow state. It is fast and involves no remote
  round-trip, but its answer can be stale: it may say a peer is admitted when
  the peer has, in fact, already latched a terminal event that just hasn't
  propagated back yet. This check is an optimization - a cheap way to avoid
  dispatching work that is *already known locally* to be pointless - and
  must never be treated as authoritative.
- **Fenced-dispatch admission** (the re-check performed inside
  ``invoke_fenced_action()``, once the dispatched action has actually reached
  the target's own locality via ``hpx::colocated``) runs on the same
  locality/thread that owns the target's authoritative supervision state, with
  no suspension point between the check and the action invocation that
  follows it. This is the check that actually decides whether the wrapped
  action runs, and it is what closes the race between a stale client-side
  "admitted" result and a peer that has since gone terminal.

In short: use client-side filtering to avoid *obviously* wasted dispatches;
rely only on the fenced-dispatch outcome to know whether work actually ran.

Epochs and when to change them
=================================

Every lifecycle event and every fenced dispatch is scoped to an epoch
(``std::uint64_t``, defaulting to ``0``). The epoch is what lets a target be
"un-fenced" and reused after it has latched a terminal event, instead of
being permanently unusable once it completes or fails once.

A target's epoch only ever increases, and publications are compared against
whatever epoch is currently in effect for that target:

- A publication for the **same** epoch as the target's current one is normal
  progression (``started`` -> ``running`` -> ... -> a terminal event), subject
  to the terminal/already-terminal rule described above.
- A publication for a **lower** epoch than the target's current one is
  rejected as stale (``publish_result::stale_epoch``) and has no effect -
  this guards against a delayed or reordered event from an earlier
  generation of the target clobbering newer state.
- A publication for a **higher** epoch than the target's current one resets
  the target: it becomes the new current epoch, the event sequence number
  resets, and any previously latched terminal event (``completed``/
  ``failed``) under the old epoch no longer fences new dispatches. This is
  the only way to make a target eligible for fenced dispatch again after it
  has gone terminal.

The same epoch value is consulted end-to-end: ``check_admission(target,
epoch)`` compares the *epoch you pass in* against the target's current epoch,
not just whether the target is terminal in some absolute sense. This is why
both ``dispatch_work()``'s local check and ``invoke_fenced_action()``'s
remote re-check take an explicit ``epoch`` argument rather than operating on
"the" state of the target.

When to keep the epoch the same
----------------------------------

Use the same epoch value across all fenced dispatches that belong to the same
logical run/incarnation of the target - e.g. repeated or retried calls
against a target that is still expected to be ``started``/``running`` under
the epoch it was initialized with. Fencing is only meaningful when related
dispatches share an epoch: if a target has already latched ``failed`` for
epoch ``N``, every further dispatch under epoch ``N`` is correctly rejected,
which is the intended behavior - it stops work from continuing to target
something that has already terminated in this generation.

When to bump the epoch
----------------------

Bump to a new, higher epoch when you are intentionally starting a new
generation of the target and want prior terminal state to stop fencing new
work - typically when a target is restarted/reincarnated after failing or
completing, and you want dispatch to resume against the restarted instance
rather than remain permanently fenced by the old generation's terminal event.
Concretely: publish the target's own ``event::started`` under the new epoch
as part of bringing the restarted instance up, then use that same, higher
epoch value for subsequent dispatches and admission checks against it.

Do not reuse an old epoch value after bumping, and do not bump the epoch
merely to "clear" a fence for the *same* running instance - if the target
has not actually restarted, admission is correctly rejecting dispatch because
the target is genuinely terminal, and forcing an epoch bump only defeats that
protection rather than fixing the underlying condition.

How fenced work is invoked
==========================

``hpx::supervision::dispatch_work<action_type>(target, epoch, action, args...)``
is the entry point callers use. It performs two distinct steps, on two different localities:

#. **Local, non-authoritative early-out (caller's locality).**
   ``dispatch_work()`` first calls ``check_admission(target, epoch)`` against
   the caller's own locally cached shadow state. If this reports
   ``dispatch_outcome::rejected_fenced``, ``dispatch_work()`` returns an
   already-exceptional future carrying ``hpx::error::target_fenced``
   immediately - no parcel is ever sent, and the wrapped action is never
   invoked. This is purely an optimization to avoid a wasted round trip; it
   is not authoritative, since the shadow state it consults can be stale (see
   the sections above).

#. **Colocated dispatch and authoritative re-check (target's locality).**
   If the local check does not reject, ``dispatch_work()`` wraps the caller's
   action in a generated ``fenced_action`` and dispatches it via
   ``hpx::async`` to ``hpx::colocated(target)`` - i.e. it runs on the same
   locality that hosts ``target`` and owns its authoritative supervision
   state. The action body that actually runs there is
   ``invoke_fenced_action()``, which performs a *second*
   ``check_admission(target, epoch)`` call. Because this call executes on
   the same locality/thread that owns the target's supervision state, with
   no suspension point before the wrapped action runs immediately afterward
   (via ``hpx::sync``, invoked directly/locally with no further parcel hop),
   this second check is authoritative. If it also reports
   ``rejected_fenced`` - meaning the target latched a terminal event for
   this epoch sometime between the caller's stale check and this point -
   ``invoke_fenced_action()`` throws ``hpx::error::target_fenced`` instead of
   invoking the action.

   Note that "dispatched to the target's locality" does not imply a network
   round-trip: ``hpx::colocated(target)`` resolves to whichever locality
   actually hosts ``target``, including the invoking locality itself when
   ``target`` happens to be local. In that case no parcel is sent, but
   ``invoke_fenced_action()`` - and, if admitted, the wrapped action inside
   it - still each run on a newly spawned HPX thread rather than inline on the
   calling thread, exactly as an unfenced ``hpx::async<Action>(target, args...)``
   call behaves when ``target`` is local: it schedules a new thread rather than
   invoking the action synchronously in place. The one exception, on both sides,
   is an action explicitly declared as a ``direct_action``, which HPX is
   permitted to invoke inline when the target is local; ``fenced_action`` itself
   is an ordinary action, and the wrapped user ``Action`` follows whatever
   declaration the caller gave it. Either way, fenced dispatch adds only the
   two-tier admission check described above - it does not change whether or
   where the action is scheduled to run relative to unfenced dispatch.

Both the local early-out and the remote re-check surface failure as the same
exception type/message shape (``hpx::error::target_fenced``), so callers
handle a single error regardless of which side detected the fence:

.. code-block:: text

    caller locality                          target locality
    ----------------                         ---------------
    dispatch_work(action, target, epoch, args...)
      check_admission(target, epoch)   <-- reads local shadow (non-authoritative)
        rejected -> exceptional future, done (no dispatch sent)
        admitted -> hpx::async(fenced_action, hpx::colocated(target), ...)
                                          |
                                          v
                                    invoke_fenced_action(action, target, epoch)
                                      check_admission(target, epoch)  <-- authoritative,
                                                                          same locality/thread
                                        rejected -> throw target_fenced
                                        admitted -> hpx::sync(action, target, ...)

Only ``target`` (not ``epoch``) is forwarded into the wrapped action's own
argument list; the epoch is fencing metadata consumed entirely by the
dispatch machinery. A convenience overload of ``dispatch_work()`` accepts an
action instance directly rather than only an action type, but follows the
identical two-check flow described above.

Relationship to unfenced dispatch
---------------------------------

From the caller's point of view, converting an unfenced dispatch into a
fenced one requires exactly two changes to the call site:

.. code-block:: c++

    // Unfenced:
    hpx::future<result_type> f = hpx::async<Action>(target, args...);

    // Fenced:
    hpx::future<result_type> f =
        hpx::supervision::dispatch_work<Action>(target, epoch, args...);

#. Replace the call to ``hpx::async<Action>`` with
   ``hpx::supervision::dispatch_work<Action>``.
#. Insert the fencing ``epoch`` as the second argument, ahead of the
   action's own arguments (``args...`` is otherwise unchanged and is
   forwarded to ``Action`` verbatim).

Both a template-argument form and an instance-argument form are available for
``dispatch_work()``, mirroring the two ways ``hpx::async`` can be called:

.. code-block:: c++

    // Action specified as an explicit template argument (as above):
    hpx::future<result_type> f =
        hpx::supervision::dispatch_work<Action>(target, epoch, args...);

    // Convenience overload: action passed as an instance, deduced instead of
    // spelled out explicitly. Note the action instance comes *first*, ahead
    // of `target` and `epoch`.
    hpx::future<result_type> f =
        hpx::supervision::dispatch_work(Action(), target, epoch, args...);

The instance-argument overload is a thin wrapper that simply forwards to the
template-argument overload above; the two are otherwise identical in
behavior (same two-check admission flow, same exception on fencing, same
argument forwarding to ``Action``). It exists purely so that call sites
already holding an action instance - rather than naming the action type
directly -- don't need an extra step to obtain one, the same convenience
``hpx::async`` itself offers by accepting either an action type or an action
instance.

Everything else - ``target``, the returned ``hpx::future<result_type>``, and
how the caller consumes it (``.get()``, continuations, etc.) - stays the
same. The only additional caller-side responsibility is handling
``hpx::error::target_fenced`` on that future (see below), since a fenced
dispatch never invokes ``Action`` at all, whereas an unfenced
``hpx::async<Action>`` call has no equivalent rejection path.

Observing whether an action was fenced
--------------------------------------

From the caller's point of view, ``dispatch_work()`` always returns an
``hpx::future``. Whether the fence was detected locally (early-out, no
dispatch sent) or remotely (authoritative re-check on the target's own
locality), the future becomes exceptional in exactly the same way, so callers
do not need to special-case which side caught it:

.. code-block:: c++

    hpx::future<result_type> f =
        hpx::supervision::dispatch_work(action, target, epoch, args...);

    try
    {
        result_type result = f.get();
        // Admitted on both checks; action ran and produced `result`.
    }
    catch (hpx::exception const& e)
    {
        if (e.get_error() == hpx::error::target_fenced)
        {
            // Fenced - either the caller's local admission check rejected
            // it before any dispatch was sent, or the target's own
            // authoritative re-check rejected it just before invocation.
            // The wrapped action was NOT invoked in either case.
        }
        else
        {
            throw;    // some other failure (e.g. transport, target action
                      // itself threw)
        }
    }

Two points worth calling out explicitly:

- ``e.get_error() == hpx::error::target_fenced`` is the single, reliable
  signal that an action was fenced. Matching on the exception's ``what()``
  text is unnecessary and discouraged, since the message wording is not part
  of the stable contract; only the error code is.
- A fenced result and a "the action itself threw" result are both surfaced
  through the same future/exception mechanism, so callers must check
  ``get_error()`` to distinguish "not invoked because fenced" from "invoked,
  but failed." There is no separate return value, flag, or callback that
  reports fencing independently of the future's exceptional state.

Because ``f.get()`` throws synchronously the moment either check rejects, a
caller waiting with a timeout (see below) sees ready-with-exception rather
than ready-with-result if either side fenced the dispatch before the timeout
elapsed.

Worked examples
================

Fencing a silent peer
---------------------

.. code-block:: c++

    #include <hpx/supervision_dispatch.hpp>

    hpx::supervision::registry local_registry(hpx::find_here());

    hpx::supervision::joined_peer const peer = local_registry.join(
        hpx::launch::sync, hpx::find_here());

    // ... time passes; the peer's heartbeat stops ...

    // Client-side filtering: cheap, but only as fresh as the last observed
    // heartbeat/lifecycle event mirrored onto `shadow`.
    for (auto const& peer : local_registry.snapshot_peers(hpx::launch::sync))
    {
        if (peer.state == hpx::supervision::lifecycle_state::failed)
        {
            // Skip obviously-dead peers before even attempting dispatch.
            continue;
        }
        // Attempt fenced dispatch anyway; the target-side re-check inside
        // invoke_fenced_action() is authoritative and will reject the action
        // with hpx::error::target_fenced if the peer has latched a terminal
        // event we haven't observed locally yet.
    }

Waiting on a dispatch outcome with a timeout
--------------------------------------------

.. code-block:: c++

    #include <hpx/async_combinators/wait_all_for.hpp>
    #include <hpx/supervision_dispatch.hpp>

    hpx::future<result_type> f =
        hpx::supervision::dispatch_work(target, epoch, action, args...);

    if (hpx::wait_all_for(std::chrono::seconds(5), f) ==
        hpx::future_status::ready)
    {
        try
        {
            result_type result = f.get();
        }
        catch (hpx::exception const& e)
        {
            if (e.get_error() == hpx::error::target_fenced)
            {
                // Authoritative rejection: the target latched a terminal
                // event for this epoch before the action could run.
            }
        }
    }
    else
    {
        // Dispatch did not complete within the timeout; the target may be
        // slow, unreachable, or the shadow state used for client-side
        // filtering may be stale.
    }

Querying state and publishing events through a handle
-----------------------------------------------------

The handle returned by ``init()`` also doubles as the argument to a small
family of convenience overloads of ``query_state()``/``publish_event()``
that resolve locality and target automatically, instead of requiring the
caller to extract ``handle.get_id()`` and ``hpx::find_here()`` manually:

.. code-block:: c++

    #include <hpx/supervision_dispatch.hpp>

    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync);

    // Recover the epoch init() started this handle's locality at, for use
    // with dispatch_work()/publish_event() later on.
    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    // ... later, e.g. once this locality's own supervised work completes ...
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

Querying a peer's state after joining it follows the same pattern, using a
``discovered_peer`` in place of an explicit locality/target pair:

.. code-block:: c++

    for (auto const& peer :
        hpx::supervision::discover_and_join(handle))
    {
        hpx::supervision::lifecycle_state const peer_state =
            hpx::supervision::query_state(hpx::launch::sync, handle, peer);
        // `handle` is only needed here for overload resolution; `peer`
        // alone carries the locality/target this call actually queries.
    }

There is no peer-publishing overload: a locality only ever publishes
lifecycle events it self, never on behalf of a peer's.

A worked example combining this with a supervised worker loop - covering
when to publish ``started``/``running``/a terminal event for a worker's own
locality, and how a supervisor queries peer worker state through the handle
above - is available in ``components/supervision_dispatch/examples/plain_worker.cpp``.

See :ref:`modules_supervision` for the full semantics of ``check_admission``,
``dispatch_outcome``, and the individual lifecycle events referenced above,
and :ref:`modules_supervision_dispatch` for the API reference of this
component's own types and functions (``registry``, ``sentinel``,
``discovery``, and ``dispatch_work``).
