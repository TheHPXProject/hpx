//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The root side of the supervision_dispatch "late component" demo: this binary
// is the only one of the family that ever runs standalone (as locality 0). It
// spawns late_component_worker/late_component_failing_worker on demand via
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
// fenced slot (never reusing its epoch), respawning a fresh worker in its place
// (same variant, brand-new locality/component/epoch), and retrying the task --
// up to a capped number of retries, past which the task is dropped with a
// logged failure instead. On success, if no other queued task still targets the
// same slot, a per-worker shutdown signal is sent and the slot is retired.
//
// Explicit non-goals, deferred to Task 4c:
//   - No full-queue-drain detection across *all* worker slots.
//   - No publish_event(handle, event::completed, ...)/handle.finalize() for the
//     root's own lifecycle.
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
        late_component::server::test_client::set_message_action;

    // Copies this process's current environment, exactly as launch_process.cpp
    // and os_assigned_parcelport_port_7406.cpp do, so it can be handed to
    // process::set_env() with a few HPX_* entries appended for the spawned
    // worker below.
    inline int get_arraylen(char** arr)
    {
        int count = 0;
        if (nullptr != arr)
        {
            while (nullptr != arr[count])
                ++count;
        }
        return count;
    }

    std::vector<std::string> get_environment()
    {
        std::vector<std::string> env;
#if defined(HPX_WINDOWS)
        int len = get_arraylen(_environ);
        std::copy(&_environ[0], &_environ[len], std::back_inserter(env));
#elif defined(linux) || defined(__linux) || defined(__linux__) ||              \
    defined(__AIX__) || defined(__APPLE__) || defined(__FreeBSD__)
        int len = get_arraylen(environ);
        std::copy(&environ[0], &environ[len], std::back_inserter(env));
#else
#error "Don't know, how to access the execution environment on this platform"
#endif
        return env;
    }

    // ========================================================================
    // Task 4a: spawn_worker()/worker_slot/task scaffolding.
    // ========================================================================

    // Everything root needs to fence/dispatch against, and eventually retire,
    // one spawned worker: the joined peer (locality + join_epoch, for
    // dispatch_work()'s fencing), the late_component::test_client instance root
    // created on the worker's locality (the actual dispatch target), and enough
    // bookkeeping (use_failing_variant, the process handle) to respawn an
    // equivalent replacement later.
    struct worker_slot
    {
        hpx::supervision::discovered_peer peer;

        // Kept alongside component_id purely to keep the underlying instance
        // referenced (client_base's managed id_type would keep it alive via
        // component_id alone, but holding the client itself here also lets the
        // shutdown-signal convention call set_message() directly, see below).
        late_component::test_client component;
        hpx::id_type component_id;

        bool use_failing_variant;

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

    // Launches one late_component_worker or late_component_failing_worker
    // process (depending on \p use_failing_variant), waits for it to connect
    // back as a new locality and join \p handle, creates a
    // late_component::test_client instance on its locality, and returns the
    // resulting worker_slot. Every call produces a brand-new locality,
    // component instance, and join epoch - never a reused one, which is
    // exactly what a post-fencing respawn (see run_launcher() below) needs.
    //
    // Assumes success throughout (see the "Explicit non-goals" paragraph in the
    // file comment above): a process launch failure here is not handled and
    // simply propagates out of this function.
    worker_slot spawn_worker(hpx::supervision::supervision_handle const& handle,
        bool use_failing_variant)
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

        fs::path const exe = base_dir /
            (use_failing_variant ?
                    "late_component_failing_worker" HPX_EXECUTABLE_EXTENSION :
                    "late_component_worker" HPX_EXECUTABLE_EXTENSION);

        std::vector<std::string> args;
        args.push_back(fs::to_string(exe));
        args.push_back("--hpx:ignore-batch-env");
        args.push_back("--hpx:threads=1");

        // Force use of the TCP parcelport, mirroring launch_process.cpp/
        // os_assigned_parcelport_port_7406.cpp.
        args.push_back("--hpx:ini=hpx.parcel.tcp.priority=1000");
        args.push_back("--hpx:ini=hpx.parcel.bootstrap=tcp");

        std::vector<std::string> env = get_environment();

        std::string const address =
            hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS);

        env.push_back("HPX_AGAS_SERVER_ADDRESS=" + address);
        env.push_back("HPX_AGAS_SERVER_PORT=" +
            hpx::get_config_entry(
                "hpx.agas.port", std::to_string(HPX_INITIAL_IP_PORT)));

        // Let the operating system choose the parcelport port (mirroring
        // os_assigned_parcelport_port_7406.cpp) rather than tracking
        // previously-used ports ourselves -- every respawn needs a fresh,
        // unused port, and an unbounded number of respawns can happen over this
        // root's lifetime.
        env.push_back("HPX_PARCEL_SERVER_ADDRESS=" + address);
        env.push_back("HPX_PARCEL_SERVER_PORT=0");

        std::string const latch_name =
            "late_component_launcher/spawn/" + std::to_string(spawn_id);
        env.push_back("HPX_ON_STARTUP_WAIT_ON_LATCH=" + latch_name);

        std::vector<hpx::id_type> const localities_before =
            hpx::find_remote_localities();

        process::child c = process::execute(hpx::find_here(),
            process::run_exe(fs::to_string(exe)), process::set_args(args),
            process::set_env(env),
            process::start_in_dir(fs::to_string(base_dir)),
            process::throw_on_error(), process::wait_on_latch(latch_name));

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
            .use_failing_variant = use_failing_variant,
            .child = std::move(c)};
    }

    // ========================================================================
    // Task 4b: fencing-aware dispatch loop.
    // ========================================================================

    void run_launcher(hpx::supervision::supervision_handle const& handle)
    {
        std::vector<std::optional<worker_slot>> worker_slots;

        // Seed two workers -- one normal, one that departs mid-epoch without
        // finalize() (see late_component_failing_worker.cpp) - so the
        // fencing/retry/respawn path below is actually exercised rather than
        // only ever taking the success path.
        worker_slots.push_back(
            spawn_worker(handle, /* use_failing_variant */ false));
        worker_slots.push_back(
            spawn_worker(handle, /* use_failing_variant */ true));

        std::deque<task> queue;
        queue.push_back(task{0, "hello from root: message 1 for worker 0", 0});
        queue.push_back(task{0, "hello from root: message 2 for worker 0", 0});
        queue.push_back(task{1, "hello from root: message 1 for worker 1", 0});

        while (!queue.empty())
        {
            task current = std::move(queue.front());
            queue.pop_front();

            // Step 2: the slot this task targets may already have been dropped
            // (e.g. by an earlier task's fencing that gave up instead of
            // requeuing) - respawn before dispatching in that case.
            if (!worker_slots[current.worker_slot_index])
            {
                // Which variant a since-dropped slot used is not tracked once
                // the slot itself is gone; default back to the non-failing
                // worker for this path.
                worker_slots[current.worker_slot_index] =
                    spawn_worker(handle, /* use_failing_variant */ false);
            }

            try
            {
                worker_slot const& slot =
                    *worker_slots[current.worker_slot_index];

                hpx::supervision::dispatch_work<set_message_action>(
                    slot.component_id, slot.peer.join_epoch, current.payload)
                    .get();
            }
            catch (hpx::exception const& e)
            {
                if (hpx::get_error(e) != hpx::error::target_fenced)
                {
                    throw;
                }

                // Step 4: fenced - drop the slot. worker_slot.peer.join_epoch
                // is fixed for that peer's lifetime, so this slot must never be
                // reused; a respawn always produces a brand-new worker_slot
                // with a fresh epoch.
                bool const use_failing_variant =
                    worker_slots[current.worker_slot_index]
                        ->use_failing_variant;
                worker_slots[current.worker_slot_index] = std::nullopt;

                ++current.retries;
                if (current.retries > max_task_retries)
                {
                    std::cerr << "late_component_launcher: dropping task "
                                 "after "
                              << current.retries
                              << " retries (payload: " << current.payload
                              << ")\n";
                    continue;
                }

                worker_slots[current.worker_slot_index] =
                    spawn_worker(handle, use_failing_variant);

                // Retry before newer tasks.
                queue.push_front(std::move(current));
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
    hpx::supervision::supervision_handle const handle =
        hpx::supervision::init(hpx::launch::sync, root_discovery_timeout);

    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    run_launcher(handle);

    // Root's own event::completed publication/finalize() is Task 4c/ completion
    // territory, not this task's concern (see the "Explicit non-goals"
    // paragraph in the file comment above); this binary's own HPX runtime
    // shutdown below is independent of that.
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Mirrors launch_process.cpp: this locality expects other localities (the
    // workers spawned by spawn_worker() above) to dynamically connect to it
    // over its lifetime.
    std::vector<std::string> const cfg = {
        "hpx.expect_connecting_localities!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
