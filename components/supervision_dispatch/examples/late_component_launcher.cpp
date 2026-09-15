//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The root side of the supervision_dispatch "late component" demo: this binary
// is the only one of the family that ever runs standalone (as locality 0). It
// spawns late_component_worker/late_component_failing_worker/
// late_component_aborting_worker on demand via
// hpx::components::process::execute() -- mirroring the connect-back pattern
// used by launch_process.cpp
// (libs/full/runtime_components/tests/unit/components/launch_process.cpp) and
// os_assigned_parcelport_port_7406.cpp
// (libs/full/agas/tests/regressions/os_assigned_parcelport_port_7406.cpp) --
// each spawned worker then joins this root under supervision_dispatch and polls
// until root goes silent or it idles out (see late_component_worker.cpp for
// that side of the story).
//
// Task 4a: spawn_worker()/worker_slot/task are the scaffolding every later task
// builds on: spawn_worker() launches one worker process, waits for it to
// connect and join, creates a late_component::test_client instance on its
// locality (the actual unit of dispatchable work -- see
// late_component_worker.hpp), and returns a worker_slot bundling the peer, the
// instance, and everything needed to fence/dispatch against it later.
// worker_slot deliberately does not embed a raw queued-task counter: whether a
// slot still has queued work is always recomputed by scanning `queue` itself
// (see the "any_of" check below), so nothing needs to be kept in sync across a
// respawn.
//
// Task 4b: the dispatch loop below pops tasks off `queue`, dispatches each via
// hpx::supervision::dispatch_work<set_message_action>() fenced against the
// task's worker_slot, and reacts to hpx::error::target_fenced by dropping the
// fenced slot (never reusing its epoch); if HPX_HAVE_FORCE_DISCONNECT is
// enabled, it then purges the fenced locality's leftover AGAS/connection-cache
// state via hpx::force_disconnect() before respawning a fresh worker in the
// slot's place (same variant, brand-new locality/component/epoch), and retrying
// the task -- up to a capped number of retries, past which the task is dropped
// with a logged failure instead (each of detect/purge/relaunch logs its own
// line to make the sequence observable). On success, if no other queued task
// still targets the same slot, a per-worker shutdown signal is sent and the
// slot is retired.
//
// Explicit non-goals, deferred to Task 4c:
//   - No full-queue-drain detection across *all* worker slots.
//   - No error handling for spawn_worker() itself throwing (e.g. process launch
//     failure) -- this is assumed to succeed throughout.
//
// Shutdown-signal convention introduced here: there is no supervision_dispatch
// primitive that lets one peer forcibly latch a terminal event for another
// (publish_event() in dispatch_api.hpp always targets the caller's *own* handle
// -- see its doc comments), and late_component_worker.cpp's own poll loop is
// explicitly local-only (idle-timeout/root-fencing, never a message from root
// -- see the file comment there). So "send the per-worker shutdown signal"
// below means: dispatch a well-known sentinel payload
// (shutdown_sentinel_message) via the ordinary set_message_action to the
// worker_slot's component_id, purely as an observable "root considers this
// worker done" marker (visible via get_message()), and otherwise drop the slot
// from root's own active set. It does not, by itself, hasten the spawned worker
// process's own exit -- that remains governed entirely by its own idle-timeout
// as documented in late_component_worker.cpp.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch.hpp>

#include "late_component_worker.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter for a
    // freshly-spawned worker; mirrors the timeout used by
    // late_component_worker.cpp/late_component_failing_worker.cpp.
    constexpr std::chrono::milliseconds root_discovery_timeout{5000};

    // Number of retries a task gets after a fenced dispatch before it is
    // dropped instead of requeued (see the catch block below).
    constexpr int max_task_retries = 3;

    // See the shutdown-signal convention paragraph in the file comment above.
    constexpr char const* shutdown_sentinel_message =
        "__worker_slot_shutdown__";

    using set_message_action =
        late_component::server::test_server::set_message_action;

    // ========================================================================
    // Task 4a: spawn_worker()/worker_slot/task scaffolding.
    // ========================================================================

    // Which worker binary spawn_worker() launches: the graceful
    // late_component_worker, the mid-epoch-departing
    // late_component_failing_worker, or the late_component_aborting_worker that
    // models a true, unmodeled crash (std::exit(-1), no teardown at all).
    enum class worker_variant : std::uint8_t
    {
        normal = 0,
        failing = 1,
        aborting = 2
    };

    constexpr char const* const worker_paths[] = {
        "late_component_worker" HPX_EXECUTABLE_EXTENSION,
        "late_component_failing_worker" HPX_EXECUTABLE_EXTENSION,
        "late_component_aborting_worker" HPX_EXECUTABLE_EXTENSION,
    };

    // Everything root needs to fence/dispatch against, and eventually retire,
    // one spawned worker: the joined peer (locality + join_epoch, for
    // dispatch_work()'s fencing), the late_component::test_client instance root
    // created on the worker's locality (the actual dispatch target), and enough
    // bookkeeping (variant, the process handle) to respawn an equivalent
    // replacement later.
    struct worker_slot
    {
        hpx::supervision::discovered_peer peer;

        // Kept alongside component_id purely to keep the underlying instance
        // referenced (client_base's managed id_type would keep it alive via
        // component_id alone, but holding the client itself here also lets the
        // shutdown-signal convention call set_message() directly, see below).
        late_component::test_client component;
        hpx::id_type component_id;

        worker_variant variant;

        // Keeps the spawned child process handle alive for the lifetime of this
        // slot; never waited on explicitly - the worker exits on its own
        // (idle-timeout or root-fencing), not because root asks it to.
        hpx::components::process::child child;
    };

    // A unit of dispatchable work: the message to set on `queue`d against some
    // `worker_slot`, addressed by index into the launcher's
    // `std::vector<std::optional<worker_slot>>` (see run_launcher() below) -
    // never by a direct pointer/reference, so a fenced slot can be dropped (set
    // to std::nullopt) without invalidating any other task's index.
    struct task
    {
        std::size_t worker_slot_index;
        std::string payload;
        int retries = 0;
    };

    // Launches one late_component_worker, late_component_failing_worker, or
    // late_component_aborting_worker process (depending on \p variant), waits
    // for it to connect back as a new locality and join \p handle, creates a
    // late_component::test_client instance on its locality, and returns the
    // resulting worker_slot. Every call produces a brand-new locality,
    // component instance, and join epoch - never a reused one, which is
    // exactly what a post-fencing respawn (see run_launcher() below) needs.
    //
    // Assumes success throughout (see the "Explicit non-goals" paragraph in the
    // file comment above): a process launch failure here is not handled and
    // simply propagates out of this function.
    worker_slot spawn_worker(
        hpx::supervision::registry const& handle, worker_variant const variant)
    {
        namespace process = hpx::components::process;
        namespace fs = hpx::filesystem;

        // Every spawn needs a name unique for the lifetime of this root
        // process, both for the startup-latch below and to keep successive
        // spawns of the same variant from colliding with each other.
        static std::atomic<std::uint64_t> spawn_counter{0};
        std::uint64_t const spawn_id =
            spawn_counter.fetch_add(1, std::memory_order_relaxed);

        fs::path base_dir = hpx::util::find_prefix();
        base_dir /= "bin";

        fs::path const exe = base_dir / worker_paths[static_cast<int>(variant)];

        hpx::util::format_to(std::cerr,
            "{}: late_component_launcher: launching: {}\n", hpx::find_here(),
            exe);

        std::vector<std::string> args;
        args.emplace_back("--hpx:threads=1");

        std::vector<hpx::id_type> const localities_before =
            hpx::find_remote_localities();

        process::child c = process::launch_connecting_locality(
            fs::to_string(exe), args, {}, spawn_id);

        // Waits for the spawned locality to actually connect back.
        c.wait();

        // Identify the newly-connected locality by diffing against the remote
        // localities known just before spawning - process::child itself does
        // not hand back the new locality's id.
        std::vector<hpx::id_type> const localities_after =
            hpx::find_remote_localities();

        hpx::id_type new_locality;
        for (hpx::id_type const& candidate : localities_after)
        {
            if (std::ranges::find(localities_before, candidate) ==
                localities_before.end())
            {
                new_locality = candidate;
                break;
            }
        }
        if (!new_locality)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status, "spawn_worker",
                "no newly connected locality was detected");
        }

        std::vector<hpx::supervision::discovered_peer> const peers =
            hpx::supervision::discover_and_join(handle, root_discovery_timeout);

        auto const it = std::ranges::find_if(
            peers, [&](hpx::supervision::discovered_peer const& p) {
                return p.locality == new_locality;
            });
        if (it == peers.end())
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status, "spawn_worker",
                "the spawned worker did not join within the discovery "
                "timeout");
        }

        // Only the root ever creates this instance, and it always places it on
        // the *worker's* locality - never its own - mirroring
        // component_worker.cpp's run_root_role().
        auto component = hpx::new_<late_component::test_client>(new_locality);

        hpx::id_type const component_id = component.get_id();
        return worker_slot{.peer = *it,
            .component = std::move(component),
            .component_id = component_id,
            .variant = variant,
            .child = std::move(c)};
    }

    // ========================================================================
    // Task 4b: fencing-aware dispatch loop.
    // ========================================================================

    void run_launcher(hpx::supervision::registry const& handle)
    {
        std::vector<std::optional<worker_slot>> worker_slots;

        // Parallel to worker_slots, this tracks the variant last spawned at
        // each index even after the slot itself is dropped (set to
        // std::nullopt) - needed so a respawn triggered by the dropped-slot
        // path below (as opposed to the fenced-retry path, which still has
        // the slot's variant available before nulling it) still knows which
        // variant to relaunch instead of defaulting to worker_variant::normal.
        std::vector<worker_variant> slot_variants;

        // Seed three workers -- one normal, one that departs mid-epoch without
        // finalize() (see late_component_failing_worker.cpp), and one that
        // crashes outright via std::exit(-1) (see
        // late_component_aborting_worker.cpp) - so the fencing/retry/respawn
        // path below is exercised against both a clean-ish departure and a
        // true, unmodeled crash, rather than only ever taking the success path.
        worker_slots.emplace_back(spawn_worker(handle, worker_variant::normal));
        slot_variants.emplace_back(worker_variant::normal);
        worker_slots.emplace_back(
            spawn_worker(handle, worker_variant::failing));
        slot_variants.emplace_back(worker_variant::failing);
        worker_slots.emplace_back(
            spawn_worker(handle, worker_variant::aborting));
        slot_variants.emplace_back(worker_variant::aborting);

        std::deque<task> queue;
        queue.push_back(task{.worker_slot_index = 0,
            .payload = "hello from root: message 1 for worker 0",
            .retries = 0});
        queue.push_back(task{.worker_slot_index = 0,
            .payload = "hello from root: message 2 for worker 0",
            .retries = 0});
        queue.push_back(task{.worker_slot_index = 1,
            .payload = "hello from root: message 1 for worker 1",
            .retries = 0});
        queue.push_back(task{.worker_slot_index = 2,
            .payload = "hello from root: message 1 for worker 2",
            .retries = 0});

        while (!queue.empty())
        {
            task current = std::move(queue.front());
            queue.pop_front();

            // Step 2: the slot this task targets may already have been dropped
            // (e.g. by an earlier task's fencing that gave up instead of
            // requeuing) - respawn before dispatching in that case.
            if (!worker_slots[current.worker_slot_index])
            {
                // slot_variants retains the variant last used at this index
                // even though the slot itself is gone, so the respawn below
                // stays consistent with the fenced-retry path's
                // variant-preserving relaunch instead of coercing back to
                // worker_variant::normal.
                worker_slots[current.worker_slot_index] = spawn_worker(
                    handle, slot_variants[current.worker_slot_index]);
            }

            auto locality_is_fenced =
                [&](bool const locality_was_disconnected) {
                    // Step 4: fenced - drop the slot.
                    // worker_slot.peer.join_epoch is fixed for that peer's
                    // lifetime, so this slot must never be reused; a
                    // respawn always produces a brand-new worker_slot with
                    // a fresh epoch.
                    worker_variant const variant =
                        worker_slots[current.worker_slot_index]->variant;

                    // Captured before the slot is nulled out below - once
                    // fenced, this is the only place the peer's locality id
                    // is still reachable.
                    hpx::id_type const fenced_locality =
                        worker_slots[current.worker_slot_index]->peer.locality;

                    worker_slots[current.worker_slot_index] = std::nullopt;

                    // A clean hpx::disconnect() has already purged the locality
                    // by the time dispatch reports locality_was_disconnected. A
                    // fenced worker still needs force_disconnect() before it
                    // can be replaced.
                    if (locality_was_disconnected)
                    {
                        hpx::util::format_to(std::cerr,
                            "{}: late_component_launcher: worker at slot {} "
                            "already disconnected\n",
                            hpx::find_here(), current.worker_slot_index);
                    }
                    else
                    {
                        hpx::util::format_to(std::cerr,
                            "{}: late_component_launcher: detected fenced "
                            "worker at slot {}; purging locality via "
                            "force_disconnect\n",
                            hpx::find_here(), current.worker_slot_index);

                        hpx::error_code ec(hpx::throwmode::lightweight);
                        int const disconnect_result =
                            hpx::force_disconnect(fenced_locality, ec);

                        if (disconnect_result != 0 || ec)
                        {
                            hpx::util::format_to(std::cerr,
                                "{}: late_component_launcher: "
                                "force_disconnect failed for slot {}"
                                "; will retry purge\n",
                                hpx::find_here(), current.worker_slot_index);

                            ++current.retries;
                            if (current.retries > max_task_retries)
                            {
                                hpx::util::format_to(std::cerr,
                                    "{}: late_component_launcher: dropping "
                                    "task after {} retries (payload: {})\n",
                                    hpx::find_here(), current.retries,
                                    current.payload);
                                return;
                            }

                            queue.push_front(std::move(current));
                            return;
                        }

                        hpx::util::format_to(std::cerr,
                            "{}: late_component_launcher: purge complete for "
                            "slot {}\n",
                            hpx::find_here(), current.worker_slot_index);
                    }

                    ++current.retries;
                    if (current.retries > max_task_retries)
                    {
                        hpx::util::format_to(std::cerr,
                            "{}: late_component_launcher: dropping task "
                            "after {} retries (payload: {})\n",
                            hpx::find_here(), current.retries, current.payload);
                        return;
                    }

                    worker_slots[current.worker_slot_index] =
                        spawn_worker(handle, variant);
                    slot_variants[current.worker_slot_index] = variant;
                    hpx::util::format_to(std::cerr,
                        "{}: late_component_launcher: relaunch complete for "
                        "slot {}\n",
                        hpx::find_here(), current.worker_slot_index);

                    // Retry before newer tasks.
                    queue.push_front(std::move(current));
                };

            bool const result =
                hpx::detail::try_catch_exception_ptr<hpx::exception>(
                    [&]() {
                        worker_slot const& slot =
                            *worker_slots[current.worker_slot_index];

                        hpx::supervision::dispatch_work<set_message_action>(
                            slot.component_id, slot.peer.join_epoch,
                            current.payload)
                            .get();

                        return false;
                    },
                    [&](hpx::exception const& e) {
                        auto const err = hpx::get_error(e);
                        if (err != hpx::error::target_fenced &&
                            err != hpx::error::locality_was_disconnected)
                        {
                            throw e;
                        }

                        locality_is_fenced(
                            err == hpx::error::locality_was_disconnected);
                        return true;
                    },
                    [](std::exception_ptr const& ep) {
                        std::rethrow_exception(ep);
                        return true;
                    });

            if (result)
            {
                continue;
            }

            // Step 5: success. If no other queued task still targets this slot,
            // this was its last piece of work - send the per-worker shutdown
            // signal and retire the slot. Recomputed by scanning `queue` rather
            // than a stored counter, so it stays correct across any number of
            // intervening respawns of this same index.
            bool const any_remaining =
                std::ranges::any_of(queue, [&](task const& t) {
                    return t.worker_slot_index == current.worker_slot_index;
                });
            if (!any_remaining)
            {
                worker_slot const& slot =
                    *worker_slots[current.worker_slot_index];

                // See the shutdown-signal convention paragraph in the file
                // comment above: bypasses dispatch_work() on purpose, exactly
                // as component_worker.cpp's get_work_count() verification call
                // does, since this is a root-local bookkeeping signal, not
                // fenced work.
                slot.component.set_message(shutdown_sentinel_message);

                worker_slots[current.worker_slot_index] = std::nullopt;
            }
        }
    }
}    // namespace

int hpx_main()
{
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, root_discovery_timeout);

    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    run_launcher(handle);

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

    hpx::supervision::finalize();

    hpx::util::format_to(std::cerr,
        "{}: late_component_launcher: complete, exiting.\n", hpx::find_here());

    return hpx::finalize();
}

int main(int const argc, char* argv[])
{
    // Mirrors launch_process.cpp: this locality expects other localities (the
    // workers spawned by spawn_worker() above) to dynamically connect to it
    // over its lifetime.
    std::vector<std::string> const cfg = {
        "hpx.expect_connecting_localities!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    auto const result = hpx::init(argc, argv, init_args);

    hpx::util::format_to(
        std::cerr, "late_component_launcher: complete, exited.\n");

    return result;
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
