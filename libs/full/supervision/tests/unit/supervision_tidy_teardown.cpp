//  Copyright (c) 2026 Anshuman Agrawal
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
#include <cstddef>
#include <memory>

namespace {

    // Shared with the activity callbacks, which may outlive the test body.
    struct blocked_callback_state
    {
        std::atomic<std::size_t> entered{0};
        std::atomic<std::size_t> finished{0};
        std::atomic<bool> release{false};
    };

    // Blocks the worker running the activity callback until the test releases
    // it. The upper bound keeps a regressed unbounded wait from hanging the
    // whole test executable; the tests assert that the expected call returns
    // long before the bound expires.
    void block_until_released(blocked_callback_state& state)
    {
        state.entered.fetch_add(1, std::memory_order_acq_rel);

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!state.release.load(std::memory_order_acquire) &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        state.finished.fetch_add(1, std::memory_order_acq_rel);
    }
}    // namespace

// A local activity observer's callback has to complete before tidy() returns:
// a local agent cannot become unreachable, so the teardown escape must not
// apply to it. This is the regression guard for the local half of the wait
// loop in fire_activity_event().
void test_tidy_waits_for_local_activity_agent()
{
    hpx::id_type const locality = hpx::find_here();

    auto state = std::make_shared<blocked_callback_state>();

    auto const handle =
        hpx::supervision::register_activity_observer(hpx::launch::sync,
            locality, [state](hpx::supervision::activity_notification const&) {
                block_until_released(*state);
                return true;
            });
    HPX_TEST_NEQ(handle, hpx::invalid_id);

    hpx::id_type const target = make_test_target();

    // Admit one notification and leave its callback blocked.
    auto publish = hpx::async([target]() {
        hpx::supervision::publish_event(
            target, hpx::supervision::event::started);
    });

    while (state->entered.load(std::memory_order_acquire) == 0)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto tidy = hpx::async([]() {
        hpx::error_code ec;
        hpx::supervision::get_supervision_manager().tidy(ec);
        return ec;
    });

    // Longer than the 100ms wait slice: a tidy() that abandons local agents
    // would already have returned by now.
    hpx::this_thread::sleep_for(std::chrono::milliseconds(300));
    HPX_TEST(!tidy.is_ready());

    state->release.store(true, std::memory_order_release);

    while (state->finished.load(std::memory_order_acquire) <
        state->entered.load(std::memory_order_acquire))
    {
        hpx::this_thread::yield();
    }

    publish.get();

    hpx::error_code ec = tidy.get();
    HPX_TEST(!ec);

    hpx::error_code unregister_ec;
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, handle, unregister_ec);
    HPX_TEST(!unregister_ec);
}

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(test_tidy_waits_for_local_activity_agent);

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
