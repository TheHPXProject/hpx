//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <atomic>
#include <chrono>

// ============================================================================
// Test Cases: await_terminal() waiters pending at shutdown
// ============================================================================

// An await_terminal() waiter that is still pending when the runtime shuts down
// must be invalidated as a pre-shutdown step, while the runtime is still
// active, instead of being left for the supervision manager's destructor to
// break once the runtime has stopped: a continuation attached with
// hpx::launch::async would then fail to register its thread and abort the
// process (see #7825).

std::atomic<bool> continuation_ran{false};
std::atomic<bool> saw_invalid_status{false};
std::atomic<bool> late_waiter_rejected{false};

bool is_invalid_status(hpx::future<hpx::supervision::lifecycle_state> f)
{
    bool caught_invalid_status = false;
    try
    {
        f.get();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        caught_invalid_status = e.get_error() == hpx::error::invalid_status;
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    return caught_invalid_status;
}

// Registers a waiter on a target that never publishes an event, with a timeout
// far beyond this test's run time, so that only shutdown can resolve it.
void test_await_terminal_pending_waiter_cancelled_on_shutdown()
{
    hpx::id_type const target = make_test_target();

    auto f = hpx::supervision::await_terminal(
        hpx::find_here(), target, 0, std::chrono::hours(1));
    HPX_TEST(!f.is_ready());

    // the continuation's own future is not needed, it reports via the flags
    [[maybe_unused]] auto _ = f.then(hpx::launch::async,
        [](hpx::future<hpx::supervision::lifecycle_state> f) {
            saw_invalid_status = is_invalid_status(HPX_MOVE(f));
            continuation_ran = true;
        });
}

// Shutdown functions run after all pre-shutdown functions, so by then the
// waiters must be closed: a new waiter must be rejected right away rather
// than be registered and left pending.
void test_await_terminal_rejected_after_waiters_cancelled()
{
    hpx::register_shutdown_function([] {
        auto f = hpx::supervision::await_terminal(
            hpx::find_here(), make_test_target(), 0, std::chrono::hours(1));
        late_waiter_rejected = f.is_ready() && is_invalid_status(HPX_MOVE(f));
    });
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(
        test_await_terminal_pending_waiter_cancelled_on_shutdown);
    HPX_SUPERVISION_TEST_RUN(
        test_await_terminal_rejected_after_waiters_cancelled);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);

    HPX_TEST(continuation_ran);
    HPX_TEST(saw_invalid_status);
    HPX_TEST(late_waiter_rejected);

    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
