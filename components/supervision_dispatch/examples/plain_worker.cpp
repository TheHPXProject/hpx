//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check / usage example: a role that actively dispatches work to its
// peers walks through init() -> publish_event(running) ->
// discover_and_join(handle, ...) -> barrier -> per-peer dispatch_work() ->
// publish_event(completed) -> barrier -> finalize(). Run with two localities so
// each side actually has a peer to dispatch work to.
//
// \note init() alone -- without ever calling discover_and_join() or
//       dispatch_work() -- is sufficient for a "shadow-less" role: one that
//       only needs to be discoverable/supervised itself but never dispatches
//       work of its own to peers. Such a role can stop right after init(); the
//       discovery/dispatch sequence below is only needed for a role like this
//       one that actively fans work out to its peers.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter across two
    // localities booting concurrently; short enough to keep this example
    // fast (mirrors the timeout used by fan_out_join.cpp).
    constexpr std::chrono::milliseconds worker_discovery_timeout{5000};

    // Incremented once per successful do_work() invocation on this locality;
    // read back afterwards (via get_work_count_action below) to confirm each
    // dispatched peer actually ran the action exactly once.
    std::atomic<int> work_count{0};
}    // namespace

// The unit of work a joined peer dispatches to this worker. This is a plain
// action - like every other Action used with dispatch_work() in this example -
// rather than a per-instance component action, because dispatch_work() reaches
// its target through the raw locality id carried by discovered_peer::locality/
// joined_peer::target, not through a distinct component-instance id. The int
// argument stands in for whatever payload a real worker action would take; here
// it just exercises dispatch_work()'s forwarding of extra arguments.
void do_work(int /* arg */)
{
    work_count.fetch_add(1, std::memory_order_relaxed);
}
HPX_PLAIN_ACTION(do_work, do_work_action)

// Verification helper: lets the dispatching locality (0) read back how many
// times each peer actually executed do_work(), rather than just trusting that
// dispatch_work().get() returning without exception means the work ran.
int get_work_count()
{
    return work_count.load(std::memory_order_acquire);
}
HPX_PLAIN_ACTION(get_work_count, get_work_count_action)

// ============================================================================
// Worker role
// ============================================================================

// Walks a dispatch-capable worker role through its full lifecycle: advertise
// this locality's worker capability, init() the supervision-dispatch runtime,
// publish `running`, discover and join every peer, dispatch do_work() to each
// of them, publish `completed`, and finalize().
void run_worker_role()
{
    // Registers this locality as a supervised participant and returns the
    // handle used for every subsequent supervision_dispatch call below.
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

    // Pull the epoch init() just established (via run_init_sequence()'s
    // local_sentinel.start(sync, new_epoch)) rather than assuming epoch 0; this
    // is the Task A handle-based query_state() overload -- there is no separate
    // current_epoch() accessor.
    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    // Announce this locality has entered its active/working phase for the
    // current epoch, so peers observing our state see `running` rather than a
    // stale/default value before any work has started.
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    // Discover every other participating locality and join them, returning one
    // discovered_peer per remote locality (each carrying its own locality id
    // and join epoch for use with dispatch_work()/query_state()).
    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_and_join(handle, worker_discovery_timeout);

    if (peers.size() != hpx::find_remote_localities().size())
    {
        std::cerr << "Failed to join all peers\n";

        hpx::supervision::finalize();
        hpx::terminate();
    }

    // First sync point: every locality has published `running` and joined its
    // peers before any locality starts dispatching work to them.
    hpx::distributed::barrier::synchronize();

    // Only locality 0 fans work out; the peers are purely receivers in this
    // example. This keeps the illustration simple while still exercising a real
    // many-target dispatch_work() fan-out.
    if (hpx::get_locality_id() == 0)
    {
        for (hpx::supervision::discovered_peer const& peer : peers)
        {
            try
            {
                // dispatch_work(Action, discovered_peer const&, Ts&&...)
                // resolves to dispatch_work<Action>(peer.locality,
                // peer.join_epoch, arg...) under the hood, so this call is
                // automatically fenced against the peer's own join epoch.
                hpx::supervision::dispatch_work(do_work_action(), peer,
                    static_cast<int>(hpx::get_locality_id()))
                    .get();
            }
            catch (hpx::exception const& e)
            {
                if (hpx::get_error(e) == hpx::error::target_fenced)
                {
                    // The peer already latched a terminal event for this epoch
                    // (e.g. it finished, or failed, before we got to it) --
                    // skip it rather than treating this as an error.
                    continue;
                }
                throw;
            }
        }

        // Every peer we successfully dispatched to must have actually run
        // do_work() exactly once.
        for (hpx::supervision::discovered_peer const& peer : peers)
        {
            int const count = hpx::sync(get_work_count_action(), peer.locality);
            hpx::util::format_to(
                std::cout, "peer {} did work {} times", peer.locality, count);
        }
    }

    // Announce completion for the same epoch we started `running` under, so
    // observers see a coherent running -> completed transition rather than a
    // jump across epochs.
    //
    // Note: finalize() will also publish event::completed for this same
    // epoch during teardown. This early publish is intentional - it lets
    // peers observe the running -> completed transition before this
    // locality reaches the second barrier, rather than only after
    // finalize() runs. The later publish from finalize() is a harmless
    // no-op re-affirmation of the already-latched terminal state.
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

    // Second sync point: every locality has published `completed` before any
    // locality tears its supervision state down.
    hpx::distributed::barrier::synchronize();

    // Tear down this locality's supervision state; takes no arguments, as
    // established for this API surface.
    hpx::supervision::finalize();
}

// Main Entry Point
int hpx_main()
{
    run_worker_role();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
