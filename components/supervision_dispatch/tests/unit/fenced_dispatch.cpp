//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision_dispatch/dispatch_work.hpp>

#include <atomic>
#include <cstdint>

// Fixture: a plain action whose invocation count is observable, used as the
// Action wrapped by fenced_action<Action>.

std::atomic<int> invocation_count{0};

int count_invocation()
{
    return invocation_count.fetch_add(1, std::memory_order_relaxed) + 1;
}
HPX_PLAIN_ACTION(count_invocation, count_invocation_action);

int get_invocation_count()
{
    return invocation_count.load(std::memory_order_acquire);
}
HPX_PLAIN_ACTION(get_invocation_count, get_invocation_count_action);

void reset_invocation_count()
{
    invocation_count.store(0, std::memory_order_release);
}
HPX_PLAIN_ACTION(reset_invocation_count, reset_invocation_count_action);

// ============================================================================
// Test Cases: Fenced Dispatch
// ============================================================================

namespace {

    // Reach `running` via a legal path: started -> running.
    void reach_running_at_epoch(hpx::id_type const& locality,
        hpx::id_type const& target, std::uint64_t const epoch)
    {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::started, epoch);
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::running, epoch);
    }
}    // namespace

// No fence anywhere: dispatch_work() resolves normally, wrapped action runs
// exactly once.
void test_dispatch_success(hpx::id_type const& locality)
{
    hpx::sync(reset_invocation_count_action(), locality);

    constexpr std::uint64_t epoch = 0;

    hpx::future<int> f = hpx::supervision::dispatch_work(
        count_invocation_action(), locality, epoch);

    HPX_TEST(f.get() == 1);
    HPX_TEST_EQ(hpx::sync(get_invocation_count_action(), locality), 1);

    hpx::supervision::remove_target(locality);
}

// Client-side early-out: target already latched a terminal event before
// dispatch_work() is called. Wrapped action must never dispatch.
void test_dispatch_client_side_fence()
{
    hpx::id_type const locality = hpx::find_here();
    hpx::sync(reset_invocation_count_action(), locality);

    hpx::id_type const target = hpx::find_here();
    constexpr std::uint64_t epoch = 0;

    reach_running_at_epoch(locality, target, epoch);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, epoch);

    hpx::future<int> f = hpx::supervision::dispatch_work(
        count_invocation_action(), target, epoch);

    bool caught = false;
    try
    {
        f.get();
    }
    catch (hpx::exception const& e)
    {
        caught = true;
        HPX_TEST(hpx::get_error(e) == hpx::error::target_fenced);
    }
    HPX_TEST(caught);
    HPX_TEST_EQ(hpx::sync(get_invocation_count_action(), locality), 0);

    hpx::supervision::remove_target(locality, target);
}

// Server-side race: client-side check_admission() returns admitted, but the
// target latches a terminal event before the authoritative re-check inside
// invoke_fenced_action runs. Driven deterministically by publishing between
// the two checks, then calling invoke_fenced_action() directly.
void test_dispatch_server_side_race()
{
    invocation_count = 0;

    hpx::id_type const& target = hpx::find_here();
    constexpr std::uint64_t epoch = 0;

    HPX_TEST(hpx::supervision::check_admission(target, epoch) ==
        hpx::supervision::dispatch_outcome::admitted);

    reach_running_at_epoch(hpx::find_here(), target, epoch);
    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(), target,
        hpx::supervision::event::completed, epoch);

    bool caught = false;
    try
    {
        hpx::supervision::invoke_fenced_action(
            count_invocation_action(), target, epoch);
    }
    catch (hpx::exception const& e)
    {
        caught = true;
        HPX_TEST(hpx::get_error(e) == hpx::error::target_fenced);
    }
    HPX_TEST(caught);
    HPX_TEST_EQ(invocation_count.load(), 0);

    hpx::supervision::remove_target(target);
}

// Terminal latch is scoped to the epoch it was recorded under: dispatch
// under a different epoch is admitted -- guards against an overly-broad
// re-check that ignores epoch.
void test_dispatch_epoch_mismatch_not_fenced()
{
    hpx::id_type const locality = hpx::find_here();
    hpx::sync(reset_invocation_count_action(), locality);

    hpx::id_type const target = hpx::find_here();
    constexpr std::uint64_t fenced_epoch = 1;
    constexpr std::uint64_t other_epoch = 2;

    reach_running_at_epoch(locality, target, fenced_epoch);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, fenced_epoch);

    hpx::future<int> f =
        hpx::supervision::dispatch_work<count_invocation_action>(
            target, other_epoch);

    HPX_TEST(f.get() == 1);
    HPX_TEST_EQ(hpx::sync(get_invocation_count_action(), locality), 1);

    hpx::supervision::remove_target(locality, target);
}

// Both fenced paths (client-side, server-side) must report the same error code
// so callers only need a single catch clause. Assert on
// hpx::error::target_fenced only, never on message text.
void test_fenced_error_code_consistent_across_paths(
    hpx::id_type const& remote_locality)
{
    HPX_TEST_NEQ(hpx::find_here(), remote_locality);

    hpx::sync(reset_invocation_count_action(), remote_locality);

    hpx::id_type const client_side_target = hpx::find_here();
    hpx::id_type const& server_side_target = remote_locality;
    constexpr std::uint64_t epoch = 0;

    reach_running_at_epoch(hpx::find_here(), client_side_target, epoch);
    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        client_side_target, hpx::supervision::event::completed, epoch);

    hpx::error client_side_error{};
    try
    {
        hpx::supervision::dispatch_work<count_invocation_action>(
            client_side_target, epoch)
            .get();
    }
    catch (hpx::exception const& e)
    {
        client_side_error = hpx::get_error(e);
    }

    HPX_TEST(hpx::supervision::check_admission(server_side_target, epoch) ==
        hpx::supervision::dispatch_outcome::admitted);

    reach_running_at_epoch(hpx::find_here(), server_side_target, epoch);
    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        server_side_target, hpx::supervision::event::completed, epoch);

    hpx::error server_side_error{};
    try
    {
        hpx::supervision::invoke_fenced_action(
            count_invocation_action(), server_side_target, epoch);
    }
    catch (hpx::exception const& e)
    {
        server_side_error = hpx::get_error(e);
    }

    HPX_TEST(client_side_error == hpx::error::target_fenced);
    HPX_TEST(server_side_error == hpx::error::target_fenced);
    HPX_TEST_EQ(hpx::sync(get_invocation_count_action(), remote_locality), 0);

    hpx::supervision::remove_target(hpx::find_here(), client_side_target);
    hpx::supervision::remove_target(hpx::find_here(), server_side_target);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        test_dispatch_success(locality);
    }

    test_dispatch_client_side_fence();
    test_dispatch_server_side_race();
    test_dispatch_epoch_mismatch_not_fenced();

    for (auto const& locality : hpx::find_remote_localities())
    {
        test_fenced_error_code_consistent_across_paths(locality);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
