//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace {

    hpx::launch make_policy(std::size_t worker,
        hpx::threads::thread_execution_hint runs_as_child,
        hpx::threads::thread_stacksize stacksize =
            hpx::threads::thread_stacksize::default_)
    {
        hpx::threads::thread_schedule_hint hint(
            static_cast<std::int16_t>(worker));
        hint.runs_as_child_mode(runs_as_child);

        hpx::launch policy = hpx::launch::async;
        policy.set_priority(hpx::threads::thread_priority::bound);
        policy.set_stacksize(stacksize);
        policy.set_hint(hint);
        return policy;
    }

    void test_launch_properties()
    {
        std::size_t const target = 1;
        hpx::promise<void> predecessor;
        hpx::future<void> result = predecessor.get_future().then(
            make_policy(target, hpx::threads::thread_execution_hint::none,
                hpx::threads::thread_stacksize::medium),
            [target](hpx::future<void>&& predecessor) {
                predecessor.get();
                HPX_TEST_EQ(hpx::get_worker_thread_num(), target);
                HPX_TEST_EQ(hpx::this_thread::get_priority(),
                    hpx::threads::thread_priority::bound);
                HPX_TEST_EQ(hpx::threads::get_self_stacksize_enum(),
                    hpx::threads::thread_stacksize::medium);
            });

        predecessor.set_value();
        result.get();
    }

    void test_run_as_child()
    {
        if (!hpx::threads::detail::get_self_or_default_pool()
                ->get_scheduler()
                ->supports_direct_execution())
        {
            return;
        }

        std::atomic<bool> blocker_started(false);
        std::atomic<bool> release_blocker(false);

        hpx::future<void> blocker = hpx::async(
            make_policy(1, hpx::threads::thread_execution_hint::none), [&] {
                blocker_started.store(true, std::memory_order_release);
                while (!release_blocker.load(std::memory_order_acquire))
                {
                }
            });

        while (!blocker_started.load(std::memory_order_acquire))
        {
            hpx::this_thread::yield();
        }

        hpx::future<bool> result = hpx::async(
            make_policy(0, hpx::threads::thread_execution_hint::none), [] {
                std::size_t const parent_worker = hpx::get_worker_thread_num();
                std::size_t continuation_worker = static_cast<std::size_t>(-1);

                hpx::future<void> continuation = hpx::make_ready_future().then(
                    make_policy(
                        1, hpx::threads::thread_execution_hint::run_as_child),
                    [&](hpx::future<void>&& predecessor) {
                        predecessor.get();
                        continuation_worker = hpx::get_worker_thread_num();
                    });

                hpx::future_status const status =
                    continuation.wait_for(std::chrono::milliseconds(250));
                if (status != hpx::future_status::ready)
                {
                    return false;
                }

                continuation.get();
                return continuation_worker == parent_worker;
            });

        bool const ran_inline = result.get();
        release_blocker.store(true, std::memory_order_release);
        blocker.get();

        HPX_TEST(ran_inline);
    }

    void test_expired_wait_does_not_run_child()
    {
        std::atomic<bool> blocker_started(false);
        std::atomic<bool> release_blocker(false);
        std::atomic<bool> continuation_started(false);

        hpx::future<void> blocker = hpx::async(
            make_policy(1, hpx::threads::thread_execution_hint::none), [&] {
                blocker_started.store(true, std::memory_order_release);
                while (!release_blocker.load(std::memory_order_acquire))
                {
                }
            });

        while (!blocker_started.load(std::memory_order_acquire))
        {
            hpx::this_thread::yield();
        }

        hpx::future<bool> result = hpx::async(
            make_policy(0, hpx::threads::thread_execution_hint::none), [&] {
                hpx::future<void> continuation = hpx::make_ready_future().then(
                    make_policy(
                        1, hpx::threads::thread_execution_hint::run_as_child),
                    [&](hpx::future<void>&& predecessor) {
                        predecessor.get();
                        continuation_started.store(
                            true, std::memory_order_release);
                    });

                hpx::future_status const status =
                    continuation.wait_for(std::chrono::milliseconds(0));
                bool const timed_out_without_running =
                    status == hpx::future_status::timeout &&
                    !continuation_started.load(std::memory_order_acquire);

                release_blocker.store(true, std::memory_order_release);
                continuation.get();
                return timed_out_without_running;
            });

        bool const timed_out_without_running = result.get();
        blocker.get();

        HPX_TEST(timed_out_without_running);
    }
}    // namespace

int hpx_main()
{
    HPX_TEST_EQ(hpx::get_os_thread_count(), std::size_t(2));

    test_launch_properties();
    test_run_as_child();
    test_expired_wait_does_not_run_child();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
