//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check / usage example: unlike plain_worker.cpp - where
// dispatch_work()'s target is always the peer's own locality, reached through a
// plain action - and unlike a naive variant where every locality creates its
// own component instance, this example enforces a strict root/worker model.
// Only locality 0 (the root) ever calls hpx::new_<worker_client>(...), and it
// always places the new instance on the *peer's* locality, never its own. The
// worker never instantiates anything; it only walks init() -> query_state() ->
// publish_event(running) -> discover_and_join() -> barrier -> (wait) ->
// publish_event(completed) -> barrier -> finalize(), purely to establish and
// publish a fenceable join_epoch for the root to dispatch against. This shows
// that dispatch_work()'s fencing is a property of the *target's owning
// locality*, independent of who created the component or where the caller runs.
//
// Synchronization between the root and each worker is peer-to-peer only: one
// hpx::distributed::barrier per {root, worker} pair, reused for both rendezvous
// points below via two wait() calls on the same object -- never the job-wide
// hpx::distributed::barrier::synchronize() that plain_worker.cpp uses -- so
// pacing between any given pair is independent of every other pair. Run with
// two localities so the root actually has a peer to dispatch work to.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter across two
    // localities booting concurrently; short enough to keep this example
    // fast (mirrors the timeout used by plain_worker.cpp).
    constexpr std::chrono::milliseconds worker_discovery_timeout{5000};
}    // namespace

// ============================================================================
// Component: the unit of work the root dispatches to an instance it creates
// on the worker's locality. Unlike plain_worker.cpp's do_work(), this is a
// per-instance component action, because the root -- not dispatch_work() --
// decides which instance to target, by creating it itself.
// ============================================================================

struct worker_server : hpx::components::component_base<worker_server>
{
    // Incremented once per successful do_work() invocation on this instance;
    // read back afterwards (via get_work_count()) to confirm the root's
    // fenced dispatch actually ran the action exactly once.
    //
    // Note: the relaxed/acquire memory_order tags here and on
    // get_work_count() below are not what makes the readback safe -- they
    // don't establish any cross-locality ordering by themselves. The actual
    // happens-before edge is the RPC round-trip of the root's
    // dispatch_work<do_work_action>(...).get() call completing (see
    // run_root_role() below) before get_work_count() is ever invoked.
    void do_work(int /* arg */)
    {
        work_count_.fetch_add(1, std::memory_order_relaxed);
    }

    // Verification helper: lets the root read back how many times this
    // instance actually executed do_work(), rather than just trusting that
    // dispatch_work().get() returning without exception means the work ran.
    // Called directly by the root -- bypassing dispatch_work() -- since this
    // is a test-only sanity check, not part of the fenced work itself.
    int get_work_count() const
    {
        return work_count_.load(std::memory_order_acquire);
    }

    HPX_DEFINE_COMPONENT_ACTION(worker_server, do_work, do_work_action)
    HPX_DEFINE_COMPONENT_ACTION(
        worker_server, get_work_count, get_work_count_action)

private:
    std::atomic<int> work_count_{0};
};

using worker_server_type = hpx::components::component<worker_server>;
HPX_REGISTER_COMPONENT(worker_server_type, worker_server)

using do_work_action = worker_server::do_work_action;
HPX_REGISTER_ACTION_DECLARATION(do_work_action)
HPX_REGISTER_ACTION(do_work_action)

using get_work_count_action = worker_server::get_work_count_action;
HPX_REGISTER_ACTION_DECLARATION(get_work_count_action)
HPX_REGISTER_ACTION(get_work_count_action)

// Client wrapper -- only the root ever constructs one of these, via
// hpx::new_<worker_client>(peer.locality), which places the instance on the
// *peer's* locality rather than the root's own.
struct worker_client
  : hpx::components::client_base<worker_client, worker_server>
{
    using base_type =
        hpx::components::client_base<worker_client, worker_server>;

    explicit worker_client(hpx::id_type const& id)
      : base_type(id)
    {
    }

    worker_client(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
    {
    }

    // Bypasses dispatch_work() on purpose -- see get_work_count() above.
    [[nodiscard]] int get_work_count() const
    {
        return hpx::async<get_work_count_action>(this->get_id()).get();
    }
};

// ============================================================================
// Worker role -- never instantiates anything; only establishes a fenceable
// join_epoch for the root to dispatch against.
// ============================================================================

void run_worker_role()
{
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_and_join(handle, worker_discovery_timeout);

    if (peers.size() != hpx::find_remote_localities().size())
    {
        std::cerr << "Failed to join all peers\n";

        hpx::supervision::finalize();
        hpx::terminate();
    }

    // Single barrier scoped to {root, this worker}, reused below for both
    // rendezvous points via two wait() calls -- never the job-wide
    // hpx::distributed::barrier::synchronize().
    std::size_t const own_locality = hpx::get_locality_id();
    std::string const barrier_name =
        "/component_worker/" + std::to_string(own_locality);
    hpx::distributed::barrier const b(barrier_name,
        std::vector<std::size_t>{static_cast<std::size_t>(0), own_locality},
        own_locality);

    // First rendezvous: signals to the root that this worker has joined and is
    // ready for the root to create an instance here and dispatch to it.
    b.wait();

    // Second rendezvous, same barrier object: signals teardown-readiness.
    b.wait();

    // Publishing the "completed" terminal event here, before the second
    // rendezvous below, is what opens a race window against the root's fenced
    // dispatch_work<do_work_action>(...) call: if this event latches on this
    // locality's supervision registry before the root's dispatch is
    // fenced-checked against it, the root's call fails with
    // hpx::error::target_fenced (see the catch block in run_root_role() below),
    // which is what the b.wait() there guards against.
    //
    // The root creates a worker_client instance on this locality, dispatches
    // do_work() to it, and verifies the result entirely on its own - this
    // worker has nothing further to do until teardown.
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

    hpx::supervision::finalize();
}

// ============================================================================
// Root role -- the only role that ever calls hpx::new_<worker_client>(), and
// it always places the new instance on the *peer's* locality.
// ============================================================================

void run_root_role()
{
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_and_join(handle, worker_discovery_timeout);

    if (peers.size() != hpx::find_remote_localities().size())
    {
        std::cerr << "Failed to join all peers\n";

        hpx::supervision::finalize();
        hpx::terminate();
    }

    for (hpx::supervision::discovered_peer const& peer : peers)
    {
        std::size_t const peer_locality =
            hpx::naming::get_locality_id_from_id(peer.locality);

        // Same single, reusable, pairwise barrier the worker constructs
        // above -- identical name/ranks on both sides, opposite rank.
        std::string const barrier_name =
            "/component_worker/" + std::to_string(peer_locality);
        hpx::distributed::barrier b(barrier_name,
            std::vector<std::size_t>{
                static_cast<std::size_t>(0), peer_locality},
            0);

        // First rendezvous: wait until this peer has joined before creating
        // an instance on its locality.
        b.wait();

        // Only the root ever creates a component instance, and it places it on
        // the *peer's* locality -- never its own -- to show that
        // dispatch_work()'s fencing keys off the target's owning locality,
        // independent of who created the component or where the caller runs.
        auto instance = hpx::new_<worker_client>(peer.locality);
        try
        {
            // dispatch_work<Action>(target, epoch, ts...) is fenced against
            // peer.join_epoch regardless of target being a component instance
            // id rather than a raw locality id -- the peer's locality owns both
            // the instance and the supervision state consulted by the
            // authoritative re-check.
            hpx::supervision::dispatch_work<do_work_action>(instance.get_id(),
                peer.join_epoch, static_cast<int>(hpx::get_locality_id()))
                .get();
        }
        catch (hpx::exception const& e)
        {
            if (hpx::get_error(e) == hpx::error::target_fenced)
            {
                // Even on this path, the worker's second b.wait() below is
                // unconditional (see run_worker_role() above), so we still have
                // to satisfy this pairwise rendezvous before moving on to the
                // next peer - otherwise the worker blocks forever waiting for a
                // root-side participant that never shows up.
                b.wait();

                // The peer already latched a terminal event for this epoch
                // (e.g. it finished, or failed, before we got to it) - skip it
                // rather than treating this as an error.
                continue;
            }
            throw;
        }

        // Verification bypasses dispatch_work() on purpose: this reads back the
        // instance's state directly, as a test-only sanity check, not as part
        // of the fenced work itself. No additional gating (e.g. waiting on a
        // supervision event) is needed here: dispatch_work<do_work_action>(...)
        // above only returns after do_work() has actually completed on the
        // peer, so that .get() already establishes the happens-before edge this
        // readback relies on.
        int const count = instance.get_work_count();
        hpx::util::format_to(
            std::cout, "peer {} did work {} times\n", peer_locality, count);

        // Second rendezvous, same barrier object: signals teardown-readiness
        // for this pair before moving on to the next peer.
        b.wait();
    }

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

    hpx::supervision::finalize();
}

// Main Entry Point
int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        run_root_role();
    }
    else
    {
        run_worker_role();
    }

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
