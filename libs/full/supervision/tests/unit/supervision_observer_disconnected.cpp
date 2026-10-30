//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The supervision manager drops an observer whose locality went away instead
// of recording the failed delivery as the target having failed (#7834). It
// tells that apart from the observer's callback throwing by where the error
// was raised: sending to a remote observer that went away fails on the
// manager's own locality, while a callback always throws on the observer's
// locality. These tests make sure a callback throwing
// hpx::error::locality_was_disconnected (say, from a remote call it made
// itself) is not mistaken for its locality going away, for a local observer
// and for a remote one. The observer is created on this locality and the
// target is tracked by each locality in turn.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <atomic>
#include <cstddef>
#include <memory>

namespace {

    // Callback that is alive and well, but throws the same error a send to a
    // locality that went away reports. Counts its invocations.
    hpx::supervision::lifecycle_callback make_throwing_observer(
        std::shared_ptr<std::atomic<std::size_t>> calls)
    {
        return
            [calls = HPX_MOVE(calls)](
                hpx::supervision::lifecycle_event_notification const&) -> bool {
                calls->fetch_add(1, std::memory_order_relaxed);
                HPX_THROW_EXCEPTION(hpx::error::locality_was_disconnected,
                    "make_throwing_observer", "remote call made by callback");
            };
    }

    void check_failed_and_still_registered(hpx::id_type const& locality,
        hpx::id_type const& target, hpx::id_type const& observer_handle)
    {
        auto const state =
            hpx::supervision::query_state(hpx::launch::sync, locality, target);
        HPX_TEST(state.last_event == hpx::supervision::event::failed);
        HPX_TEST(state.ec.value() == hpx::error::locality_was_disconnected);

        // a dropped observer's handle would be rejected here
        hpx::error_code ec;
        hpx::supervision::unregister_observer(
            hpx::launch::sync, locality, observer_handle, ec);
        HPX_TEST(!ec);
    }
}    // namespace

// A live event: the throwing callback is recorded as the target having
// failed, as for any other exception it throws, and stays registered.
void test_throwing_callback_fails_target(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    auto const calls = std::make_shared<std::atomic<std::size_t>>(0);

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, make_throwing_observer(calls));

    auto const result = hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    HPX_TEST(result == hpx::supervision::publish_result::applied);
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    check_failed_and_still_registered(locality, target, observer_handle);
}

// The same for the notification replayed to an observer when it is
// registered for a target that already has state.
void test_throwing_callback_replay_fails_target(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    auto const calls = std::make_shared<std::atomic<std::size_t>>(0);

    reach_running(locality, target);

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, make_throwing_observer(calls));
    HPX_TEST_EQ(calls->load(), static_cast<std::size_t>(1));

    check_failed_and_still_registered(locality, target, observer_handle);
}

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_SUPERVISION_TEST_RUN(test_throwing_callback_fails_target, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_throwing_callback_replay_fails_target, locality);
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
