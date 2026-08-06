//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/supervision_dispatch/dispatch_api.hpp>
#include <hpx/supervision_dispatch/server/registry.hpp>

#include <atomic>
#include <cstdint>
#include <exception>
#include <mutex>
#include <ostream>
#include <tuple>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

HPX_REGISTER_ACTION(hpx::supervision::server::registry::join_action,
    supervision_dispatch_registry_join_action)
HPX_REGISTER_ACTION(hpx::supervision::server::registry::snapshot_peers_action,
    supervision_dispatch_registry_snapshot_peers_action)
HPX_REGISTER_ACTION(hpx::supervision::server::registry::leave_action,
    supervision_dispatch_registry_leave_action)

namespace {

    // Testing infrastructure support: records the most recentlyt joined
    // locality , so tests can retrieve it via detail::last_join_locality() even
    // when join() goes on to throw (in which case it is otherwise never
    // handed back to the caller).
    hpx::spinlock last_join_locality_mtx;
    hpx::id_type last_join_locality_target;

    void unregister_observers(hpx::id_type const& peer_locality,
        hpx::id_type const& lifecycle_observer,
        hpx::id_type const& activity_observer)
    {
        if (lifecycle_observer)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::supervision::unregister_observer(
                hpx::launch::sync, peer_locality, lifecycle_observer, ec);
        }
        if (activity_observer)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::supervision::unregister_activity_observer(
                hpx::launch::sync, peer_locality, activity_observer, ec);
        }
    }

}    // namespace

namespace hpx::supervision::testing {

    hpx::id_type last_join_locality()
    {
        std::scoped_lock<hpx::spinlock> l(last_join_locality_mtx);
        return last_join_locality_target;
    }
}    // namespace hpx::supervision::testing

namespace hpx::supervision {

    std::ostream& operator<<(std::ostream& strm, joined_peer const& peer)
    {
        return strm << peer.target << " (epoch " << peer.join_epoch << ")";
    }
}    // namespace hpx::supervision

namespace hpx::supervision::server {

    registry::registry() = default;

    // Register for lifecycle-event notifications published for the peer's
    // sentinel. Once the peer reaches a terminal event (completed or failed),
    // mirror it onto peer_locality's local supervision state at the same epoch
    // (reusing the epoch reported by the notification rather than inventing
    // one), so that any local consumer consulting that state
    // (check_admission(), await_terminal(), ...) observes the peer's terminal
    // state without having to talk to the peer's locality itself.
    // `peer_locality` and `join_epoch` are captured by value rather than looked
    // up from `peers_` on every callback invocation: join() is idempotent and
    // never reassigns a peer's peer_locality once created (see the re-join
    // handling above), so that captured value can never go stale, and this
    // otherwise sidesteps the need to take `mtx_` from within a callback that
    // may run asynchronously with respect to the rest of registry's
    // mtx_-guarded operations.
    //
    // `join_epoch` is nearly as stable as `peer_locality`, but not quite:
    // join()'s floor against the peer's real epoch (see
    // hpx::supervision::current_epoch()) only covers what it can observe
    // synchronously at join time, so this callback's very first notification
    // can still arrive with an epoch strictly greater than the captured
    // `join_epoch` (e.g. the peer's real epoch advanced again between join()'s
    // floor check and this observer's registration). When that happens, the
    // callback below is the one narrow exception to the "no `mtx_` here" rule
    // above: it briefly takes `mtx_` to correct both its own captured copy and
    // the corresponding entry in `peers_` forward to match, before continuing.
    std::pair<hpx::id_type, hpx::id_type> registry::register_observers(
        hpx::id_type const& peer_sentinel, hpx::id_type const& peer_locality,
        std::uint64_t join_epoch)
    {
        hpx::id_type activity_observer;
        hpx::id_type lifecycle_observer;
        try
        {
            // Register for lifecycle-event notifications published for the
            // peer's sentinel. Terminal notifications are re-published onto
            // peer_locality's local supervision state (see below).
            auto observer =
                [peer_sentinel, peer_locality, join_epoch, this,
                    keep_alive = get_id()](
                    hpx::supervision::lifecycle_event_notification const&
                        notification) mutable {
                    // Read peer_locality's current local epoch/state before
                    // mirroring. If it hasn't been seeded yet (epoch mismatch
                    // or still "unknown"), open its epoch with `started` at the
                    // *peer's* epoch instead of assuming epoch 0. This keeps
                    // apply_new_epoch_locked()'s "new epoch must begin with
                    // started" invariant satisfied even when the peer joins
                    // mid-epoch.
                    hpx::error_code ec(hpx::throwmode::lightweight);
                    auto const local_state =
                        hpx::supervision::query_state(peer_locality, ec);
                    if (ec ||
                        (local_state.epoch != notification.epoch ||
                            local_state.last_event ==
                                hpx::supervision::event::unknown))
                    {
                        hpx::error_code ec1(hpx::throwmode::lightweight);
                        hpx::supervision::publish_event(peer_locality,
                            hpx::supervision::event::started,
                            notification.epoch, ec1);
                    }

                    // join()'s floor against the peer's real epoch (see the
                    // comment on register_observers() above) only covers what
                    // could be observed synchronously at join time; if this
                    // notification's epoch has since moved strictly ahead of
                    // the captured `join_epoch`, correct both the local capture
                    // and the stored peers_ entry forward to match, so
                    // snapshot_peers()/leave()/evict_peer() do not keep keying
                    // off a stale seed. Guarded to only ever move forward
                    // (never on a lower or equal notification epoch), and to
                    // only take effect if `peers_` still holds the very entry
                    // this callback was registered for (i.e. it has not since
                    // been re-joined at a fresh epoch of its own).
                    if (notification.epoch > join_epoch)
                    {
                        std::scoped_lock<hpx::spinlock> l(mtx_);
                        if (auto const it = peers_.find(peer_sentinel);
                            it != peers_.end() &&
                            it->second.join_epoch == join_epoch)
                        {
                            it->second.join_epoch = notification.epoch;
                        }
                        join_epoch = notification.epoch;
                    }

                    // Mirror the peer's event onto peer_locality's local
                    // state at the same epoch it actually occurred in.
                    hpx::error_code ec2(hpx::throwmode::lightweight);
                    hpx::supervision::publish_event(peer_locality,
                        notification.event, notification.epoch, ec2);

                    // Evict the peer if it has reached a terminal event, so
                    // peers_ does not grow without bound over the lifetime of a
                    // long-running registry. Deferred via hpx::post() rather
                    // than done inline here: this callback intentionally avoids
                    // taking mtx_ (see the comment on register_observers()
                    // above explaining why `peer_locality` is captured by
                    // value), so the eviction - which does need mtx_ to
                    // safely mutate peers_ - runs as a separate task once
                    // this terminal publish has completed.
                    if (hpx::supervision::is_terminal(notification.event))
                    {
                        // Preserve the mirrored terminal state just published
                        // above as a tombstone (see cleanup_peer()): an
                        // admission check racing against this eviction must
                        // keep observing the fence rather than have it
                        // vanish once eviction runs.
                        hpx::post(&registry::evict_peer, this, peer_sentinel,
                            peer_locality, join_epoch, keep_alive, true);
                    }
                    return true;
                };

            lifecycle_observer = hpx::supervision::register_observer(
                launch::sync, peer_locality, peer_sentinel, HPX_MOVE(observer));

            // Register for activity-state transitions of any target tracked on
            // the peer's locality (this includes the peer sentinel itself).
            // activity_notification carries an active/inactive transition, not
            // a lifecycle event, so it has no natural mapping onto
            // peer_locality's publish_event()-driven terminal-latch state; it
            // intentionally remains a no-op for this task and is left for a
            // later task to decide whether/how activity transitions should
            // feed into dispatch decisions.
            activity_observer = hpx::supervision::register_activity_observer(
                launch::sync, peer_locality,
                [](activity_notification const&) { return true; });
        }
        catch (...)
        {
            std::exception_ptr const original = std::current_exception();

            // The activity-observer registration failed after the lifecycle
            // observer was registered successfully; unregister both again so
            // neither is leaked on the peer's locality.
            unregister_observers(
                peer_locality, lifecycle_observer, activity_observer);

            std::rethrow_exception(original);
        }

        return std::make_pair(lifecycle_observer, activity_observer);
    }

    std::tuple<hpx::id_type, std::uint64_t, bool> registry::reserve_ownership(
        hpx::id_type const& peer_sentinel)
    {
        std::unique_lock l(mtx_);
        for (;;)
        {
            // Reserve ownership of peer_sentinel before performing any of the
            // remote registration calls below, so that concurrent join() calls
            // for the same peer cannot register duplicate observers. If an
            // entry already exists, this either means it is fully joined (ready
            // == true, in which case its shadow is returned immediately) or
            // that another call is still in the process of joining it.
            auto const [it, inserted] =
                peers_.try_emplace(peer_sentinel, peer_entry{});
            if (inserted)
            {
                break;
            }

            if (it->second.ready)
            {
                return {it->second.peer_locality, it->second.join_epoch, true};
            }

            // Another call already reserved this peer; wait for it to
            // either publish the joined entry or release the reservation
            // (on failure), then retry.
            cond_.wait(l);
        }

        return {};
    }

    joined_peer registry::join(
        hpx::id_type const& peer_sentinel, hpx::id_type const& peer_locality)
    {
        // Reserve ownership of peer_sentinel before performing any of
        // the remote registration calls.
        auto [original_peer_locality, join_epoch, reserved] =
            reserve_ownership(peer_sentinel);

        {
            std::scoped_lock<hpx::spinlock> l(last_join_locality_mtx);
            last_join_locality_target =
                reserved ? original_peer_locality : peer_locality;
        }

        if (reserved)
        {
            // Reflect the peer_locality originally recorded for this entry by
            // the join() call that created it, not necessarily the one passed
            // to this call.
            return {.target = original_peer_locality, .join_epoch = join_epoch};
        }

        hpx::id_type lifecycle_observer;
        hpx::id_type activity_observer;
        std::uint64_t seed_epoch;
        std::uint64_t reported_join_epoch;
        try
        {
            // join() itself must not rely solely on a future notification to
            // establish peer_locality's initial started state (see the
            // registered observer callback in register_observers() above).
            //
            // Query peer_locality's own current shadow state here - not
            // peer_sentinel's unrelated per-target state - since that is
            // what publish_event() below is about to mutate. A freshly
            // joined peer_sentinel always reports "unknown"/epoch 0, which
            // would otherwise make this always seed at epoch 0 regardless
            // of what state peer_locality's shared shadow is actually in
            // (e.g. still "failed" from a previous, not yet evicted, peer
            // occupying the same shadow).
            {
                hpx::error_code ec(hpx::throwmode::lightweight);
                auto const local_state =
                    hpx::supervision::query_state(peer_locality, ec);

                seed_epoch = !ec ? local_state.epoch : 0;

                bool need_publish_started = true;
                if (!ec)
                {
                    if (hpx::supervision::is_terminal(local_state.last_event))
                    {
                        // Prior occupant of this shadow reached a terminal
                        // state; start a fresh epoch for the newly joining
                        // peer.
                        seed_epoch = local_state.epoch + 1;
                    }
                    else if (local_state.last_event !=
                        hpx::supervision::event::unknown)
                    {
                        // The shadow has already progressed past "unknown" in
                        // the current epoch (e.g. "started", "running",
                        // "suspending"); re-publishing "started" here would be
                        // an illegal same-epoch transition. Treat this join as
                        // idempotent and reuse the existing epoch without
                        // republishing.
                        need_publish_started = false;
                    }
                }

                reported_join_epoch = seed_epoch;

                // reported_join_epoch (returned to callers as
                // joined_peer::join_epoch / peer_snapshot::join_epoch, and used
                // by failure_detection_loop() to seed query_failures) tracks
                // peer_locality's real dispatch-cycle epoch
                // (hpx::supervision::current_epoch(), bumped once per
                // successful init() on peer_locality itself) whenever that is
                // ahead of the shadow's own epoch space - e.g. peer_locality
                // completed init() well before this join() call ran, with no
                // query_state()/publish_event() against it in between.
                //
                // seed_epoch itself must NOT be escalated here: it feeds
                // publish_event(..., event::started, seed_epoch, ...) below and
                // register_observers()'s join_epoch baseline, both of which
                // must stay in the shadow's own epoch numbering so that
                // peer_sentinel's own lifecycle events (which legitimately
                // start at epoch 0 for a fresh sentinel) are not rejected as
                // stale against an epoch borrowed from peer_locality's
                // unrelated dispatch-cycle counter.
                if (peer_locality)
                {
                    hpx::error_code peer_ec(hpx::throwmode::lightweight);
                    std::uint64_t const peer_epoch =
                        hpx::supervision::current_epoch(
                            hpx::launch::sync, peer_locality, peer_ec);
                    if (!peer_ec && peer_epoch > reported_join_epoch)
                    {
                        reported_join_epoch = peer_epoch;
                    }
                }

                if (need_publish_started)
                {
                    hpx::error_code ec1(hpx::throwmode::lightweight);
                    hpx::supervision::publish_event(peer_locality,
                        hpx::supervision::event::started, seed_epoch, ec1);
                }
            }

            {
                std::scoped_lock<hpx::spinlock> l(mtx_);
                peers_.at(peer_sentinel).join_epoch = reported_join_epoch;
            }

            if (!hpx::naming::is_locality(peer_locality))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::supervision::server::registry::join",
                    "The id passed as peer_locality is not representing a "
                    "locality");
            }

            // Register for lifecycle-event notifications published for the
            // peer's sentinel.
            std::tie(lifecycle_observer, activity_observer) =
                register_observers(peer_sentinel, peer_locality, seed_epoch);
        }
        catch (...)
        {
            // Unconditionally release the reservation made by
            // reserve_ownership() above so a concurrent join() for the same
            // peer, waiting in cond_.wait(l), is not left hanging forever.
            {
                std::unique_lock l(mtx_);
                peers_.erase(peer_sentinel);
                cond_.notify_all(HPX_MOVE(l));
            }

            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::supervision::remove_target(peer_locality, ec);

            std::rethrow_exception(std::current_exception());
        }

        bool need_cleanup = false;
        bool preserve_terminal_state = false;

        {
            std::unique_lock l(mtx_);
            peer_entry& entry = peers_.at(peer_sentinel);
            HPX_ASSERT(!entry.ready);

            entry.peer_locality = peer_locality;
            entry.lifecycle_observer = lifecycle_observer;
            entry.activity_observer = activity_observer;
            entry.join_epoch = reported_join_epoch;
            entry.ready = true;

            if (entry.evict_pending)
            {
                preserve_terminal_state = entry.evict_preserve_terminal_state;
                peers_.erase(peer_sentinel);
                need_cleanup = true;
            }
            cond_.notify_all(HPX_MOVE(l));
        }

        if (need_cleanup)
        {
            cleanup_peer(peer_locality, lifecycle_observer, activity_observer,
                preserve_terminal_state);
        }
        return {.target = peer_locality, .join_epoch = reported_join_epoch};
    }

    // Implementation note: iterates peers_ under mtx_ and copies out the
    // peer_sentinel/peer_locality/join_epoch fields for entries that are
    // fully joined and not pending eviction, deliberately omitting the ready/
    // evict_pending bookkeeping fields from the returned view.
    std::vector<peer_snapshot> registry::snapshot_peers() const
    {
        std::scoped_lock<hpx::spinlock> l(mtx_);

        std::vector<peer_snapshot> result;
        result.reserve(peers_.size());
        for (auto const& [peer_sentinel, entry] : peers_)
        {
            if (entry.ready && !entry.evict_pending)
            {
                result.push_back(peer_snapshot{.peer_sentinel = peer_sentinel,
                    .peer_locality = entry.peer_locality,
                    .join_epoch = entry.join_epoch});
            }
        }
        return result;
    }

    void registry::leave(hpx::id_type const& peer_sentinel,
        hpx::id_type const& peer_locality, std::uint64_t const join_epoch)
    {
        // A graceful leave has nothing to fence against, so it should fully
        // forget peer_locality's local supervision state immediately rather
        // than leave a tombstone behind (contrast with the terminal-
        // notification call site in register_observers(), which passes true).
        evict_peer(
            peer_sentinel, peer_locality, join_epoch, hpx::invalid_id, false);
    }

    void registry::evict_peer(hpx::id_type const& peer_sentinel,
        hpx::id_type const& peer_locality, std::uint64_t const join_epoch,
        [[maybe_unused]] hpx::id_type keep_alive,
        bool const preserve_terminal_state)
    {
        hpx::id_type lifecycle_observer;
        hpx::id_type activity_observer;

        {
            std::unique_lock l(mtx_);

            auto const it = peers_.find(peer_sentinel);

            // The entry may already be gone (a racing eviction for the same
            // peer, e.g. a duplicate terminal notification) or may now refer to
            // a different, freshly re-joined peer (if the peer was re-joined
            // after the terminal notification fired but before this deferred
            // task ran); only evict it if it is still the very entry this
            // notification was published for.
            if (it == peers_.end() || it->second.join_epoch != join_epoch)
            {
                return;
            }

            if (!it->second.ready)
            {
                it->second.evict_pending = true;
                it->second.evict_preserve_terminal_state =
                    preserve_terminal_state;
                return;
            }

            lifecycle_observer = it->second.lifecycle_observer;
            activity_observer = it->second.activity_observer;

            peers_.erase(it);

            cond_.notify_all(HPX_MOVE(l));
        }

        cleanup_peer(peer_locality, lifecycle_observer, activity_observer,
            preserve_terminal_state);
    }

    void registry::cleanup_peer(hpx::id_type const& peer_locality,
        hpx::id_type const& lifecycle_observer,
        hpx::id_type const& activity_observer,
        bool const preserve_terminal_state)
    {
        unregister_observers(
            peer_locality, lifecycle_observer, activity_observer);

        // Drop peer_locality's local supervision state now that the peer has
        // been evicted, unless it was last mirrored to a terminal event (see
        // register_observers()): in that case leave it in place as a tombstone
        // so a check_admission()/await_terminal() call racing against this
        // eviction still observes the fence instead of it reverting to
        // "unknown". A later join() re-seeding this locality with a fresh epoch
        // overwrites the tombstone naturally.
        if (!preserve_terminal_state)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::supervision::remove_target(peer_locality, ec);
        }
    }
}    // namespace hpx::supervision::server

#include <hpx/config/warnings_suffix.hpp>
