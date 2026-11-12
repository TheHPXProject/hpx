//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

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

        hpx::future<bool> result = hpx::async(
            make_policy(0, hpx::threads::thread_execution_hint::none), [] {
                std::atomic<bool> blocker_started(false);
                std::atomic<bool> release_blocker(false);
                hpx::future<void> blocker = hpx::async(
                    make_policy(1, hpx::threads::thread_execution_hint::none),
                    [&] {
                        blocker_started.store(true, std::memory_order_release);
                        while (!release_blocker.load(std::memory_order_acquire))
                        {
                            hpx::this_thread::yield();
                        }
                    });

                while (!blocker_started.load(std::memory_order_acquire))
                {
                    hpx::this_thread::yield();
                }

                std::size_t const parent_worker = hpx::get_worker_thread_num();
                std::atomic<std::size_t> continuation_worker(
                    static_cast<std::size_t>(-1));

                hpx::launch continuation_policy = make_policy(
                    1, hpx::threads::thread_execution_hint::run_as_child);
                continuation_policy.set_priority(
                    hpx::threads::thread_priority::normal);
                hpx::future<void> continuation = hpx::make_ready_future().then(
                    continuation_policy,
                    [&continuation_worker](hpx::future<void>&& predecessor) {
                        predecessor.get();
                        continuation_worker.store(hpx::get_worker_thread_num(),
                            std::memory_order_release);
                    });

                hpx::future_status const status =
                    continuation.wait_for(std::chrono::milliseconds(250));
                if (status != hpx::future_status::ready)
                {
                    release_blocker.store(true, std::memory_order_release);
                    blocker.get();
                    continuation.get();
                    return false;
                }

                continuation.get();
                bool const ran_inline =
                    continuation_worker.load(std::memory_order_acquire) ==
                    parent_worker;
                release_blocker.store(true, std::memory_order_release);
                blocker.get();
                return ran_inline;
            });

        HPX_TEST(result.get());
    }

    void test_expired_wait_does_not_run_child()
    {
        hpx::future<bool> result = hpx::async(
            make_policy(0, hpx::threads::thread_execution_hint::none), [] {
                std::atomic<bool> blocker_started(false);
                std::atomic<bool> release_blocker(false);
                std::atomic<bool> continuation_started(false);
                hpx::future<void> blocker = hpx::async(
                    make_policy(1, hpx::threads::thread_execution_hint::none),
                    [&] {
                        blocker_started.store(true, std::memory_order_release);
                        while (!release_blocker.load(std::memory_order_acquire))
                        {
                            hpx::this_thread::yield();
                        }
                    });

                while (!blocker_started.load(std::memory_order_acquire))
                {
                    hpx::this_thread::yield();
                }

                hpx::launch continuation_policy = make_policy(
                    1, hpx::threads::thread_execution_hint::run_as_child);
                continuation_policy.set_priority(
                    hpx::threads::thread_priority::normal);
                hpx::future<void> continuation = hpx::make_ready_future().then(
                    continuation_policy, [&](hpx::future<void>&& predecessor) {
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
                blocker.get();
                return timed_out_without_running;
            });

        HPX_TEST(result.get());
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
    hpx::program_options::options_description desc(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc.add_options()("scheduler",
        hpx::program_options::value<std::string>()->default_value("default"),
        "default (as selected by --hpx:queuing) | shared-priority");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc;
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const& vm) {
        if (vm["scheduler"].as<std::string>() == "shared-priority")
        {
            rp.create_thread_pool(
                "default", hpx::resource::scheduling_policy::shared_priority);
        }
    };

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
