//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/supervision_dispatch/discovery.hpp>
#include <hpx/supervision_dispatch/dispatch_api.hpp>
#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/testing.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace {

    // Defaults to hpx::supervision::default_discovery_timeout; overridable only
    // for tests, via testing::set_failure_detection_poll_timeout_for_testing.
    // Relaxed atomic is sufficient: this is a coarse test knob, not something
    // failure_detection_loop() needs strict ordering against other state for.
    std::atomic<std::int64_t> failure_detection_poll_timeout_ms{
        hpx::supervision::default_discovery_timeout.count()};

    std::chrono::milliseconds current_failure_detection_poll_timeout()
    {
        return std::chrono::milliseconds(
            failure_detection_poll_timeout_ms.load(std::memory_order_relaxed));
    }

    // Interval must stay comfortably below the detector's poll_timeout so a
    // stale-sweep counter (once added) reliably observes progress before
    // fencing. A fourth of the poll_timeout gives at least one heartbeat per
    // sweep even under scheduling jitter.
    std::chrono::milliseconds current_heartbeat_interval()
    {
        return current_failure_detection_poll_timeout() / 4;
    }

    // The four states of the supervision-dispatch lifecycle. Transitions are
    // always driven by a successful compare_exchange on dispatch_state::state,
    // so exactly one caller ever "wins" each transition and is responsible for
    // the side effects that go with it.
    enum class dispatcher_lifecycle_state : std::uint8_t
    {
        uninitialized = 0,
        initializing = 1,
        active = 2,
        finalizing = 3
    };

    // File-local singleton holding all mutable state for init()/finalize()/
    // is_initialized(). A single instance is shared by every caller on this
    // locality via get_dispatch_state() below.
    struct dispatch_state
    {
        // Monotonically increasing epoch, incremented once per successful
        // init() call, and used as the epoch argument to the completed-event
        // publish_event() call.
        std::atomic<std::uint64_t> epoch_{0};

        // The locally owned registry component. Only non-null while state is
        // initializing/active; reset to nullptr (destroying the underlying
        // components via client_'s default manage_lifetime=true) once
        // finalize() unregisters their names.
        hpx::supervision::registry registry_;

        // Shared handle to the in-flight init() call's outcome, published by
        // the transition winner immediately after it wins the uninitialized ->
        // initializing compare_exchange, and cleared (reset) once that call
        // settles (whether it succeeds, throws, or rolls back). Concurrent
        // callers observing state == initializing attach to this shared_future
        // instead of starting a second, redundant init sequence.
        hpx::shared_future<hpx::supervision::registry> in_flight_init_;

        // Owned/guarded the same way registry_ already are.
        // failure_detection_task_ holds the running poll loop so finalize() can
        // join it before tearing down registry_.
        hpx::future<void> failure_detection_task_;

        // Owned/guarded the same way failure_detection_task_ is. Keeps the
        // local locality's last_event/timestamp/sequence-number moving while
        // active, so failure_detection_loop() on peer localities can
        // distinguish idle-but-alive from actually dead.
        hpx::future<void> heartbeat_task_;

        // Cooperative stop flag. Checked at loop-top and per-peer so a call to
        // finalize() can interrupt an in-flight poll cycle promptly instead of
        // waiting out the full await_terminal timeout.
        std::atomic<bool> stop_failure_detection_{false};

        // Cooperative stop flag for heartbeat_loop(), mirroring
        // stop_failure_detection_.
        std::atomic<bool> stop_heartbeat_{false};

        // Current lifecycle state. Read (via is_initialized()) without locking
        // mtx_; all writes happen only while transitioning under a winning
        // compare_exchange, matched by the corresponding side effects being
        // fully applied (init) or fully undone (finalize/rollback) before the
        // state becomes externally visible as active/uninitialized again.
        std::atomic<dispatcher_lifecycle_state> state_{
            dispatcher_lifecycle_state::uninitialized};

        // Guards registry_ and in_flight_init_ together, since both
        // are read-modify-write as a unit whenever a lifecycle transition is in
        // progress (a losing caller reading in_flight_init_ must never observe
        // a partially-constructed registry_, and vice versa).
        hpx::spinlock mtx_;

        // Testing infrastructure: track ongoing await_terminal calls.
        std::atomic<bool> sweep_in_flight_{false};
    };

    // Meyer's singleton: constructed on first use, destroyed at program exit,
    // and shared by every init()/finalize()/is_initialized() call on this
    // locality.
    dispatch_state& get_dispatch_state() noexcept
    {
        static dispatch_state state;
        return state;
    }

    // Builds a ready shared_future carrying a copy of the currently-active
    // registry_, taken under mtx_ so the read never observes a partially-
    // constructed registry racing a concurrent init()/finalize() transition.
    // Used by every "already active" return path in init().
    hpx::shared_future<hpx::supervision::registry> make_active_handle(
        dispatch_state& ds)
    {
        std::scoped_lock<hpx::spinlock> l(ds.mtx_);
        return hpx::make_ready_future(ds.registry_);
    }

    // Runs the actual initialization sequence. Only ever invoked by the caller
    // that won the uninitialized -> initializing transition.
    void run_init_sequence(
        hpx::chrono::steady_duration const& discovery_timeout)
    {
        dispatch_state& ds = get_dispatch_state();

        // Steps 1-2: create the local registry and publish event::started
        // before it is registered, so no peer can ever discover and join a
        // not-yet-started locality.
        hpx::id_type const& here = hpx::find_here();
        auto local_registry = hpx::supervision::registry(here);

        hpx::detail::try_catch_exception_ptr(
            [&]() {
                // New, strictly increasing epoch for this cycle's event::start
                // publication.
                std::uint64_t const new_epoch = ++ds.epoch_;
                hpx::supervision::publish_event(
                    here, hpx::supervision::event::started, new_epoch);

                // Step 3: register symbol name.
                local_registry.register_name(hpx::launch::sync);

                // Step 4: single reactive discover_and_join() pass.
                discover_and_join(local_registry, discovery_timeout);

                // Success: install the pair for finalize() and is_initialized()
                // to observe.
                {
                    std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                    ds.registry_ = HPX_MOVE(local_registry);
                }
            },
            [&](std::exception_ptr const& e) {
                // Step 5: unregister both names, best-effort.
                hpx::error_code ec1(hpx::throwmode::lightweight);
                local_registry.unregister_name(hpx::launch::sync, ec1);

                std::rethrow_exception(e);
            });
    }

    bool needs_stopping(std::atomic<bool> const& flag,
        std::memory_order const mo = std::memory_order_relaxed) noexcept
    {
        return flag.load(mo) || !hpx::supervision::is_initialized();
    }

    // Active-probing complement to the reactive eviction path (evict_peer/
    // cleanup_peer). While that path fences a peer when its lifecycle observer
    // receives a terminal callback, this loop catches the case where a peer
    // goes silent without ever delivering one (e.g. a hard locality crash where
    // no callback fires).
    //
    // Lifecycle:
    //   - Started as the last step of run_init_sequence()'s success path, after
    //     discover_and_join() returns, so it only ever iterates over peers
    //     already joined and never races initialization.
    //   - Stopped from finalize(): stop_failure_detection_ is flipped and the
    //     owning hpx::future<void> is .get()'d before registry_ is torn down,
    //     since the loop dereferences the registry's shadows while running.
    //
    // Per sweep:
    //   - Snapshots currently-joined, ready, non-evicting peers via
    //     registry_->snapshot_peers() (no new registry state was added for
    //     this: the accessor already existed for this purpose).
    //   - Dispatches await_terminal(...) concurrently (async, not sequential
    //     sync calls) for every peer in the sweep, so an unresponsive peer
    //     doesn't block the detection of others and doesn't multiply the
    //     sweep's total duration by peer count.
    //   - Each dispatch is read against a freshly queried epoch (query_state),
    //     since peer_snapshot itself carries no epoch.
    //
    // Fencing semantics:
    //   - hpx::error::future_cancelled on a peer's await_terminal future means
    //     a genuine timeout: no terminal event arrived under the epoch we
    //     waited on. Before fencing, the epoch captured when the wait was
    //     started is re-confirmed against a fresh query_state() call, since the
    //     target's supervision manager may have erased its recorded state for
    //     this epoch in the interim (e.g. remove_target() ran because the peer
    //     left/rejoined); publishing event::failed against a no-longer- open
    //     epoch would otherwise be rejected as an illegal new-epoch opener. If
    //     the epoch is still confirmed open, we bump and publish event::failed
    //     against *our own local copy* of that peer's shadow only; this loop
    //     only needs the local shadow to reflect failure so future
    //     check_admission()/dispatch_work() calls against it observe the fence.
    //   - hpx::error::stale_state means the epoch was already advanced
    //     (typically the reactive eviction path already fired) - no action
    //     needed.
    //   - Any other outcome is a genuinely delivered terminal event - nothing
    //     to do; the reactive path already handles it.
    //   - check_admission() itself is never called from this loop; it remains
    //     exclusively the caller-side gate inside dispatch_work.hpp.
    //
    // Cancellation/shutdown behavior:
    //   - The outer sweep loop and the per-peer dispatch loop both check
    //     stop_failure_detection_ and is_initialized() before doing any more
    //     work, so a call to finalize() is observed promptly rather than only
    //     between full sweeps.
    //   - Sweep completion is polled in short poll_interval slices via
    //     wait_all_for rather than a single blocking wait_all, so finalize() is
    //     bounded by ~poll_interval, not by default_discovery_timeout, even if
    //     a sweep has unresponsive peers in flight.
    //   - If the stop signal fires mid-sweep, outstanding per-peer
    //     continuations are abandoned (not cancelled): they hold their own
    //     reference-counted future shared state independent of the `checks`
    //     vector, so they run to completion safely after this function returns.
    //     This is intentionally allowed: each continuation only touches a
    //     captured-by-value shadow/epoch and calls publish_event against that
    //     local shadow, never reaching into dispatch_state's registry_, so it
    //     cannot race finalize()'s teardown. Consequently, finalize() returning
    //     promptly does not imply all background poll activity has ceased -
    //     only that this loop itself has stopped issuing new sweeps.

    void pace_consecutive_sweeps(std::atomic<bool> const& flag,
        std::chrono::milliseconds const& poll_interval)
    {
        constexpr std::chrono::milliseconds stop_check_slice{20};

        auto remaining = poll_interval;
        while (!needs_stopping(flag) && remaining.count() > 0)
        {
            auto const slice = (std::min) (remaining, stop_check_slice);
            hpx::this_thread::sleep_for(slice);
            remaining -= slice;
        }
    }

    // Track query_state failures during executing the failure detection loop.
    struct peer_tracking
    {
        // Registry-level join epoch, used only to detect that the peer left
        // and rejoined (in which case any stale tracking below must reset).
        // This is distinct from the peer's local dispatch lifecycle epoch
        // (state.epoch) that publish_event/await_terminal operate on -
        // conflating the two causes spurious rejoin resets and lets the
        // failure fence be published against the wrong epoch.
        std::uint64_t join_epoch = 0;

        // Last lifecycle epoch (state.epoch) observed from a successful
        // query_state for this peer. Unset until the first successful query;
        // only a value observed this way is valid to fence against.
        std::optional<std::uint64_t> last_lifecycle_epoch;

        std::uint32_t consecutive_query_failures = 0;
    };

    // Result of probing whether a peer is genuinely silent (no new terminal
    // event/heartbeat) or should not be fenced on (either it became active,
    // or the read itself was untrustworthy).
    enum class silence_status : std::uint8_t
    {
        alive_or_unknown,    // don't fence: alive, or read was untrustworthy
        still_silent
    };

    struct silence_probe
    {
        std::uint64_t seq;    // valid when status == still_silent
        silence_status status;
    };

    // await_terminal only unblocks on a terminal event, so a timeout alone
    // does not distinguish "dead" from "alive but only publishing
    // non-terminal events" (e.g. the periodic event::running heartbeat).
    // Re-query and compare event_sequence_number before concluding genuine
    // silence. Callers reuse the returned seq instead of re-querying.
    silence_probe probe_peer_silence(
        std::uint64_t const last_seq, hpx::id_type const& locality)
    {
        // await_terminal only unblocks on a terminal event, so a timeout alone
        // does not distinguish "dead" from "alive but only publishing
        // non-terminal events" (e.g. the periodic event::running heartbeat).
        // Re-query and compare event_sequence_number before concluding genuine
        // failure.
        hpx::error_code ec(hpx::throwmode::lightweight);
        auto const recheck = hpx::supervision::query_state(locality, ec);
        if (ec || recheck.ec)
        {
            // Can't confirm liveness or death right now; don't fence on an
            // untrustworthy read - try again next sweep.
            return {
                .seq = last_seq, .status = silence_status::alive_or_unknown};
        }

        if (recheck.event_sequence_number != last_seq)
        {
            // Sequence number advanced since we started waiting (e.g. a
            // heartbeat pulse) - the peer is alive, just quiet.
            return {
                .seq = last_seq, .status = silence_status::alive_or_unknown};
        }

        return {.seq = recheck.event_sequence_number,
            .status = silence_status::still_silent};
    }

    bool stalled_after_grace(std::uint64_t last_seq,
        hpx::supervision::server::peer_snapshot const& peer,
        std::uint64_t const epoch,
        std::chrono::milliseconds const& poll_timeout)
    {
        constexpr int max_consecutive_silent_windows = 3;

        // Each retry should only wait long enough to observe a fresh
        // heartbeat/terminal event, not the full detection window - otherwise
        // fencing is delayed by up to max_consecutive_silent_windows *
        // poll_timeout instead of one detection window.
        auto const grace_interval =
            poll_timeout / max_consecutive_silent_windows;

        for (int attempt = 0; attempt < max_consecutive_silent_windows;
            ++attempt)
        {
            auto const [seq, status] =
                probe_peer_silence(last_seq, peer.peer_locality);
            if (status == silence_status::alive_or_unknown)
                return false;

            last_seq = seq;

            hpx::error_code ec2(hpx::throwmode::lightweight);
            hpx::supervision::await_terminal(hpx::launch::sync,
                peer.peer_locality, epoch, grace_interval, ec2);

            if (!ec2 || get_error(ec2) != hpx::error::future_cancelled)
            {
                // peer became responsive during the grace window
                return false;
            }

            // still silent -> loop again for one more grace
        }

        // Close the observation gap left by the loop above.
        auto const [final_seq, final_status] =
            probe_peer_silence(last_seq, peer.peer_locality);
        return final_status == silence_status::still_silent;
    }

    void failure_detection_loop()
    {
        constexpr std::chrono::milliseconds poll_interval{200};
        constexpr std::uint32_t max_consecutive_query_failures = 3;

        dispatch_state& ds = get_dispatch_state();

        // Consecutive query_state failure count per peer. Persists across
        // sweeps (unlike everything else in this loop) so a peer that is
        // genuinely unreachable - not just transiently slow - can be
        // distinguished from a one-off blip and eventually fenced, instead of
        // perpetually deferring judgment via continue/return.
        std::unordered_map<hpx::id_type, peer_tracking> query_failures;

        // Outer loop: keep polling as long as we haven't been asked to stop and
        // the dispatcher is still in the "active" state. is_initialized()
        // guards against racing finalize() that's already in progress.
        while (!needs_stopping(
            ds.stop_failure_detection_, std::memory_order_acquire))
        {
            auto const poll_timeout = current_failure_detection_poll_timeout();

            // Snapshot of currently-joined, ready, non-evicting peers. Uses the
            // sync overload so this call blocks inline rather than returning a
            // future we'd have to chain off of.
            hpx::error_code peers_ec(hpx::throwmode::lightweight);
            auto const peers =
                ds.registry_.snapshot_peers(hpx::launch::sync, peers_ec);
            if (peers_ec)
            {
                // A transient failure here (e.g. registry AGAS lookup hiccup)
                // must not terminate the detection loop retry on the next sweep
                hpx::this_thread::sleep_for(poll_interval);
                continue;
            }

            std::vector<hpx::future<void>> checks;
            checks.reserve(peers.size());

            for (auto const& peer : peers)
            {
                if (needs_stopping(ds.stop_failure_detection_))
                    break;

                // peer_snapshot carries no epoch of its own
                hpx::error_code ec(hpx::throwmode::lightweight);
                auto const state =
                    hpx::supervision::query_state(peer.peer_locality, ec);

                auto [it, _] = query_failures.try_emplace(peer.peer_locality,
                    peer_tracking{.join_epoch = peer.join_epoch,
                        .last_lifecycle_epoch = std::nullopt,
                        .consecutive_query_failures = 0});
                auto& [join_epoch, last_lifecycle_epoch, failures] = it->second;

                // A pre-existing tracking entry from an earlier join epoch
                // (e.g. the peer left and rejoined) must not carry over its
                // stale failure count or lifecycle epoch.
                if (join_epoch != peer.join_epoch)
                {
                    join_epoch = peer.join_epoch;
                    last_lifecycle_epoch = std::nullopt;
                    failures = 0;
                }

                if (ec)
                {
                    // Genuine query failure (transport/RPC error): the peer may
                    // be unreachable. Guard against sustained unreachability
                    // with a best-effort fence attempt once the threshold is
                    // hit - if the peer is genuinely gone, this publish may
                    // itself fail, which is fine: reactive eviction/registry
                    // cleanup is the fallback for that case.
                    if (++failures >= max_consecutive_query_failures)
                    {
                        // Only fence if a valid lifecycle epoch has ever been
                        // observed for this peer - otherwise there is no epoch
                        // to fence against and the publish would be rejected as
                        // an invalid transition anyway.
                        if (last_lifecycle_epoch.has_value())
                        {
                            hpx::error_code ec1(hpx::throwmode::lightweight);
                            hpx::supervision::publish_event(peer.peer_locality,
                                hpx::supervision::event::failed,
                                *last_lifecycle_epoch, ec1);
                        }
                        query_failures.erase(peer.peer_locality);
                    }
                    continue;
                }

                if (state.ec)
                {
                    // The peer's own supervision manager is alive and reachable
                    // but has not recorded any lifecycle state for this
                    // target/epoch yet (e.g. it just rejoined and hasn't
                    // published `started`). There is no epoch open to fence,
                    // and attempting `event::failed` here is guaranteed to be
                    // rejected as an invalid transition (bad_parameter). Treat
                    // this as a no-op/reset rather than accumulating toward a
                    // fence.
                    failures = 0;
                    continue;
                }

                // update failure tracking
                last_lifecycle_epoch = state.epoch;
                failures = 0;

                auto cont = [peer, epoch = state.epoch,
                                seq = state.event_sequence_number,
                                poll_timeout](auto&& f) {
                    hpx::detail::try_catch_exception_ptr<hpx::exception>(
                        [&]() {
                            // success: real terminal event delivered
                            f.get();
                        },
                        [&](hpx::exception const& e) {
                            if (e.get_error() != hpx::error::future_cancelled)
                                return;

                            if (!stalled_after_grace(
                                    seq, peer, epoch, poll_timeout))
                                return;    // not stalled (yet)

                            // No progress since the wait started: genuine
                            // stall. Before fencing, re-confirm the target
                            // still has *this* epoch open - remove_target() on
                            // the peer may have erased its states_ entry in the
                            // interim (e.g. the peer left/rejoined), in which
                            // case a `failed` fence would be rejected as an
                            // illegal new-epoch opener. Treat that as a no-op,
                            // mirroring the state.ec branch above.
                            hpx::error_code ec_check(
                                hpx::throwmode::lightweight);
                            auto const recheck = hpx::supervision::query_state(
                                peer.peer_locality, ec_check);
                            if (ec_check || recheck.ec ||
                                recheck.epoch != epoch)
                                return;

                            // No progress since the wait started: genuine
                            // stall. Fence locally only, never touch the peer's
                            // own registry.
                            hpx::error_code ec3(hpx::throwmode::lightweight);
                            hpx::supervision::publish_event(peer.peer_locality,
                                hpx::supervision::event::failed, epoch, ec3);
                        });
                };

                checks.push_back(hpx::supervision::await_terminal(
                    peer.peer_locality, state.epoch, poll_timeout)
                        .then(hpx::launch::async, HPX_MOVE(cont)));
            }

            {
                // Testing support: mark the sweep in flight while we wait
                ds.sweep_in_flight_.store(
                    !checks.empty(), std::memory_order_release);
                auto _ = hpx::experimental::scope_exit([&]() {
                    ds.sweep_in_flight_.store(false, std::memory_order_release);
                });

                // Poll the batch in short slices so the stop flag is rechecked
                // every poll_interval instead of only after the full sweep
                // resolves.
                while (!needs_stopping(ds.stop_failure_detection_) &&
                    hpx::wait_all_for(poll_interval, checks) ==
                        hpx::future_status::timeout)
                {
                    // still waiting on this sweep; recheck stop flag
                }
            }

            // Do not spin: pace consecutive sweeps even when the batch resolves
            // immediately (or is empty).
            pace_consecutive_sweeps(ds.stop_failure_detection_, poll_interval);

            // Prune entries for peers no longer present in the current snapshot
            // (evicted/departed), otherwise this map grows unboundedly over the
            // process lifetime.
            for (auto it = query_failures.begin(); it != query_failures.end();
                /**/)
            {
                bool const still_present =
                    std::ranges::any_of(peers, [&](auto const& p) {
                        return p.peer_locality == it->first;
                    });
                it = still_present ? std::next(it) : query_failures.erase(it);
            }
        }
    }

    // Periodically republishes event::running against this locality so
    // failure_detection_loop() on peers observes a moving
    // event_sequence_number/ timestamp even when no work is being dispatched.
    // Started/stopped the same way failure_detection_loop() is: last step of
    // run_init_sequence()'s success path, joined by finalize() before registry_
    // teardown.
    void heartbeat_loop()
    {
        dispatch_state& ds = get_dispatch_state();

        auto publish_pulse = [&]() {
            {
                std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                if (!ds.registry_)
                    return false;    // torn down concurrently by finalize(); retry
            }

            auto const epoch = ds.epoch_.load(std::memory_order_acquire);

            // Best-effort: a transient failure here must never kill the loop -
            // finalize()'s own event::running/completed publish already handles
            // the terminal transition; this is purely a liveness pulse.
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::supervision::publish_event(
                hpx::find_here(), hpx::supervision::event::running, epoch, ec);
            return true;
        };

        publish_pulse();

        while (!needs_stopping(ds.stop_heartbeat_, std::memory_order_acquire))
        {
            // Do not spin: pace consecutive publish_event calls.
            pace_consecutive_sweeps(
                ds.stop_heartbeat_, current_heartbeat_interval());

            if (needs_stopping(ds.stop_heartbeat_))
            {
                break;
            }

            publish_pulse();
        }
    }
}    // namespace

namespace hpx::supervision::detail {

    // Free function (needs external linkage for the plain action wrapping it
    // below) returning this locality's current dispatch-cycle epoch.
    // dispatch_state is a private, file-local singleton with no component
    // instance of its own to attach an action to, so this mirrors the
    // hpx::detail::get_locality_name()/hpx_get_locality_name_action pattern HPX
    // itself uses for other per-locality state queries that have no natural
    // component to hang off of.
    std::uint64_t current_epoch_impl()
    {
        return get_dispatch_state().epoch_.load(std::memory_order_acquire);
    }
}    // namespace hpx::supervision::detail

HPX_PLAIN_ACTION(hpx::supervision::detail::current_epoch_impl,
    supervision_dispatch_current_epoch_action)

namespace hpx::supervision {

    hpx::future<std::uint64_t> current_epoch(hpx::id_type const& locality)
    {
        return hpx::async<supervision_dispatch_current_epoch_action>(locality);
    }

    std::uint64_t current_epoch(hpx::launch::sync_policy,
        hpx::id_type const& locality, hpx::error_code& ec)
    {
        return current_epoch(locality).get(ec);
    }

    namespace testing {

        std::vector<server::peer_snapshot> local_snapshot_peers()
        {
            dispatch_state const& ds = get_dispatch_state();

            // Same guard failure_detection_loop() itself uses before touching
            // registry_ - returns empty if not currently active.
            if (needs_stopping(ds.stop_failure_detection_))
                return {};

            hpx::supervision::registry local_registry;
            {
                std::scoped_lock<hpx::spinlock> l(
                    const_cast<dispatch_state&>(ds).mtx_);
                local_registry = ds.registry_;
            }
            if (!local_registry)
                return {};
            return local_registry.snapshot_peers(hpx::launch::sync);
        }

        void set_failure_detection_poll_timeout_for_testing(
            hpx::chrono::steady_duration const& timeout)
        {
            auto const timeout_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout.value());
            failure_detection_poll_timeout_ms.store(
                timeout_ms.count(), std::memory_order_relaxed);
        }

        void suspend_heartbeat_for_testing()
        {
            dispatch_state& ds = get_dispatch_state();

            ds.stop_heartbeat_.store(true, std::memory_order_release);

            hpx::future<void> local_heartbeat_task;
            {
                std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                local_heartbeat_task = HPX_MOVE(ds.heartbeat_task_);
            }
            if (local_heartbeat_task.valid())
            {
                local_heartbeat_task.get();
            }
        }

        bool failure_detection_sweep_in_flight_for_testing()
        {
            if (!hpx::supervision::is_initialized())
                return false;
            return get_dispatch_state().sweep_in_flight_.load(
                std::memory_order_acquire);
        }

        void stop_background_loops()
        {
            dispatch_state& ds = get_dispatch_state();

            // Signal both loops to stop as early as possible, mirroring
            // finalize()'s own teardown-start step.
            ds.stop_failure_detection_.store(true, std::memory_order_release);
            ds.stop_heartbeat_.store(true, std::memory_order_release);

            // Join both tasks under the same lock discipline finalize() uses,
            // but stop here: no event::completed publish, no name
            // unregistration, no state_ reset - the locality's last published
            // event is left stale on purpose.
            hpx::future<void> local_heartbeat_task;
            hpx::future<void> local_failure_detection_task;
            {
                std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                local_heartbeat_task = HPX_MOVE(ds.heartbeat_task_);
                local_failure_detection_task =
                    HPX_MOVE(ds.failure_detection_task_);
            }
            if (local_heartbeat_task.valid())
            {
                local_heartbeat_task.wait();
            }
            if (local_failure_detection_task.valid())
            {
                local_failure_detection_task.wait();
            }
        }
    }    // namespace testing

    ////////////////////////////////////////////////////////////////////////////
    hpx::shared_future<registry> init(
        hpx::chrono::steady_duration const& discovery_timeout)
    {
        dispatch_state& ds = get_dispatch_state();

        auto expected = dispatcher_lifecycle_state::uninitialized;
        if (ds.state_.compare_exchange_strong(
                expected, dispatcher_lifecycle_state::initializing))
        {
            // Winner: publish in_flight_init_ before running the sequence so
            // losers observing "initializing" have something to attach to.
            hpx::future<void> f =
                hpx::async(&run_init_sequence, discovery_timeout);

            hpx::shared_future<registry> sf = f.then(hpx::launch::sync,
                [&ds](hpx::future<void>&& result) -> registry {
                    if (result.has_value())
                    {
                        std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                        ds.in_flight_init_ = hpx::shared_future<registry>();

                        ds.state_.store(dispatcher_lifecycle_state::active);

                        // Reset the stop flags in case a prior init/finalize
                        // cycle left it set (dispatch_api can presumably be
                        // re-initialized after finalize()).
                        ds.stop_failure_detection_.store(
                            false, std::memory_order_release);
                        ds.stop_heartbeat_.store(
                            false, std::memory_order_release);

                        // Start the poll loops as the last step of the success
                        // path., after discover_and_join() has returned, so the
                        // loops never race initialization.
                        ds.failure_detection_task_ =
                            hpx::async(&failure_detection_loop);

                        ds.heartbeat_task_ = hpx::async(&heartbeat_loop);
                        return ds.registry_;
                    }

                    std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                    ds.registry_ = hpx::supervision::registry();

                    ds.in_flight_init_ = shared_future<registry>();
                    ds.state_.store(dispatcher_lifecycle_state::uninitialized);

                    result.get();    // rethrow exception, if any
                    return {};
                });

            {
                std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                ds.in_flight_init_ = sf;
            }

            return sf;
        }

        switch (expected)
        {
        case dispatcher_lifecycle_state::active:
            return make_active_handle(ds);

        case dispatcher_lifecycle_state::finalizing:
            return hpx::make_exceptional_future<registry>(HPX_GET_EXCEPTION(
                hpx::error::invalid_status, "hpx::supervision::init",
                "cannot initialize while a concurrent finalize() is in "
                "progress"));

        case dispatcher_lifecycle_state::initializing:
        default:
            // The picked 30s timeout is long enough that a normal CAS-publish
            // handoff will never legitimately hit it, but also short enough
            // that a genuinely stuck/deadlocked winner still eventually
            // surfaces an error instead of hanging forever
            auto const timeout =
                hpx::chrono::steady_duration(std::chrono::seconds(30))
                    .from_now();

            // Attach to the in-flight init. NOTE: narrow, documented spin
            // window between the winner's CAS and its in_flight_init_ publish.
            while (std::chrono::steady_clock::now() < timeout)
            {
                {
                    std::scoped_lock<hpx::spinlock> l(ds.mtx_);
                    if (ds.in_flight_init_.valid())
                    {
                        return ds.in_flight_init_;
                    }
                }

                auto const current = ds.state_.load(std::memory_order_acquire);
                if (current == dispatcher_lifecycle_state::active)
                {
                    return make_active_handle(ds);
                }
                if (current != dispatcher_lifecycle_state::initializing)
                {
                    break;
                }
                hpx::this_thread::yield();
            }

            return hpx::make_exceptional_future<registry>(
                HPX_GET_EXCEPTION(hpx::error::invalid_status,
                    hpx::throwmode::lightweight, "hpx::supervision::init",
                    "timed out attaching to a concurrent init() call; retry"));
        }
    }
    ////////////////////////////////////////////////////////////////////////////
    registry init(hpx::launch::sync_policy,
        hpx::chrono::steady_duration const& discovery_timeout)
    {
        return init(discovery_timeout).get();
    }

    void finalize()
    {
        dispatch_state& ds = get_dispatch_state();

        if (auto expected = dispatcher_lifecycle_state::active;
            !ds.state_.compare_exchange_strong(
                expected, dispatcher_lifecycle_state::finalizing))
        {
            // Documented no-op: uninitialized, still initializing, or already
            // finalizing all resolve immediately with zero side effects - no
            // event published, no name touched, no component destroyed.
            return;
        }

        // Signal the loops to stop as early as possible, before any teardown of
        // the state the loops still reference.
        ds.stop_failure_detection_.store(true, std::memory_order_release);
        ds.stop_heartbeat_.store(true, std::memory_order_release);

        // Join the task before releasing registry_ ownership - the loop
        // dereferences registry_'s shadows, so this must complete first to
        // avoid a dangling reference during extraction/publish. A failure
        // inside the loop must not abort finalization.
        hpx::future<void> local_heartbeat_task;
        hpx::future<void> local_failure_detection_task;
        {
            std::scoped_lock<hpx::spinlock> l(ds.mtx_);
            local_heartbeat_task = HPX_MOVE(ds.heartbeat_task_);
            local_failure_detection_task = HPX_MOVE(ds.failure_detection_task_);
        }
        if (local_heartbeat_task.valid())
        {
            local_heartbeat_task.wait();
        }
        if (local_failure_detection_task.valid())
        {
            local_failure_detection_task.wait();
        }

        // Winner: extract the owned registry under the lock so no other thread
        // can observe a half-torn-down registry_.
        hpx::supervision::registry local_registry;

        {
            std::scoped_lock<hpx::spinlock> l(ds.mtx_);
            local_registry = HPX_MOVE(ds.registry_);

            // Drop any settled handle from the completed init cycle so a later
            // cycle's attaching caller can never observe it.
            ds.in_flight_init_ =
                hpx::shared_future<hpx::supervision::registry>();
        }

        HPX_ASSERT(local_registry);    // active implies registry_ was set

        // First make sure the current state has advanced to running as otherwise
        // the publish_event call below will fail.
        auto const epoch = ds.epoch_.load(std::memory_order_acquire);

        std::exception_ptr publish_failure;
        hpx::detail::try_catch_exception_ptr(
            [&]() {
                auto const here = hpx::find_here();
                if (auto const state = hpx::supervision::query_state(here);
                    state.last_event == hpx::supervision::event::started)
                {
                    hpx::supervision::publish_event(
                        here, hpx::supervision::event::running, epoch);
                }
                hpx::supervision::publish_event(
                    here, hpx::supervision::event::completed, epoch);
            },
            [&](std::exception_ptr const& e) {
                // Best-effort: still unregister/re-arm below even if event
                // publication failed, so a transient failure never permanently
                // strands the lifecycle at `finalizing`.
                publish_failure = e;
            });

        // Unregister registry name before releasing ownership, so no new peer
        // can discover either component mid-teardown.
        hpx::error_code ec2(hpx::throwmode::lightweight);
        local_registry.unregister_name(hpx::launch::sync, ec2);

        // Let the client go out of scope here: client_'s default
        // manage_lifetime=true means each destructor releases this locality's
        // ownership of the underlying component.
        local_registry = hpx::supervision::registry();

        // Re-arm: a subsequent init() call can now start a fresh registry pair
        // from scratch.
        ds.state_.store(dispatcher_lifecycle_state::uninitialized,
            std::memory_order_release);

        if (publish_failure)
        {
            std::rethrow_exception(publish_failure);
        }
    }

    bool is_initialized() noexcept
    {
        dispatch_state const& ds = get_dispatch_state();
        return ds.state_.load(std::memory_order_acquire) ==
            dispatcher_lifecycle_state::active;
    }

    ////////////////////////////////////////////////////////////////////////////
    hpx::future<lifecycle_state> query_state(registry const& handle)
    {
        if (!handle)
        {
            return hpx::make_exceptional_future<lifecycle_state>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::supervision::query_state",
                    "the given registry is not valid"));
        }
        auto const here = hpx::find_here();
        return hpx::supervision::query_state(here, here);
    }

    lifecycle_state query_state(
        hpx::launch::sync_policy, registry const& handle, hpx::error_code& ec)
    {
        return query_state(handle).get(ec);
    }

    hpx::future<lifecycle_state> query_state(
        registry const&, discovered_peer const& peer)
    {
        if (!peer.locality)
        {
            return hpx::make_exceptional_future<lifecycle_state>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::supervision::query_state",
                    "the given peer does not hold a valid locality"));
        }

        // peer.locality is passed as both the locality to contact and the
        // target to query: a locality's own lifecycle events are always
        // published under its own id (see run_init_sequence()'s
        // publish_event(here, ...) and the query_state(registry const&)
        // overload above, which does the same with hpx::find_here() for both
        // arguments). There is no separate sentinel/target identity to
        // resolve here - this simply performs that same self-query remotely,
        // against peer.locality's own supervision manager.
        return hpx::supervision::query_state(peer.locality, peer.locality);
    }

    lifecycle_state query_state(hpx::launch::sync_policy,
        registry const& handle, discovered_peer const& peer,
        hpx::error_code& ec)
    {
        return query_state(handle, peer).get(ec);
    }

    hpx::future<publish_result> publish_event(
        registry const& handle, event const ev, std::uint64_t const epoch)
    {
        if (!handle)
        {
            return hpx::make_exceptional_future<publish_result>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::supervision::publish_event",
                    "the given registry is not valid"));
        }

        auto const here = hpx::find_here();
        return hpx::supervision::publish_event(here, here, ev, epoch);
    }

    publish_result publish_event(hpx::launch::sync_policy,
        registry const& handle, event const ev, std::uint64_t const epoch,
        hpx::error_code& ec)
    {
        return publish_event(handle, ev, epoch).get(ec);
    }
}    // namespace hpx::supervision
