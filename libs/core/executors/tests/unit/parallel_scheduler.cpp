// Copyright (c) 2025 Sai Charan Arvapally
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

#if defined(HPX_HAVE_STDEXEC)
// Include stdexec async_scope for stop token testing
#include <exec/async_scope.hpp>

int hpx_main(int, char*[])
{
    // Type and Concept Tests
    // parallel_scheduler models scheduler concept
    {
        auto sched = ex::get_parallel_scheduler();
        static_assert(ex::scheduler<decltype(sched)>,
            "parallel_scheduler must model scheduler");
    }

    // parallel_scheduler is not default constructible
    {
        static_assert(!std::is_default_constructible_v<ex::parallel_scheduler>,
            "parallel_scheduler should not be default constructible");
        static_assert(std::is_destructible_v<ex::parallel_scheduler>,
            "parallel_scheduler should be destructible");
    }

    // parallel_scheduler is copyable and movable
    {
        static_assert(std::is_copy_constructible_v<ex::parallel_scheduler>,
            "parallel_scheduler should be copy constructible");
        static_assert(std::is_move_constructible_v<ex::parallel_scheduler>,
            "parallel_scheduler should be move constructible");
        static_assert(
            std::is_nothrow_copy_constructible_v<ex::parallel_scheduler>,
            "copy constructor should be noexcept");
        static_assert(
            std::is_nothrow_move_constructible_v<ex::parallel_scheduler>,
            "move constructor should be noexcept");
        static_assert(std::is_nothrow_copy_assignable_v<ex::parallel_scheduler>,
            "copy assignment should be noexcept");
        static_assert(std::is_nothrow_move_assignable_v<ex::parallel_scheduler>,
            "move assignment should be noexcept");
    }

    // A copied scheduler is equal to the original
    {
        auto sched1 = ex::get_parallel_scheduler();
        auto sched2 = sched1;
        HPX_TEST(sched1 == sched2);
    }

    // Two schedulers from get_parallel_scheduler() are equal
    {
        auto sched1 = ex::get_parallel_scheduler();
        auto sched2 = ex::get_parallel_scheduler();
        HPX_TEST(sched1 == sched2);
    }

    // schedule() produces a sender
    {
        auto snd = ex::schedule(ex::get_parallel_scheduler());
        using sender_t = decltype(snd);

        static_assert(
            ex::sender<sender_t>, "schedule() result must model sender");
        static_assert(ex::sender_of<sender_t, ex::set_value_t()>,
            "schedule() result must be sender_of<set_value_t()>");
        static_assert(ex::sender_of<sender_t, ex::set_stopped_t()>,
            "schedule() result must be sender_of<set_stopped_t()>");
    }

    // Trivial schedule task (bare sync_wait, no then)
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        ex::sync_wait(ex::schedule(sched));
    }

    // Simple schedule runs on worker thread (not main thread)
    {
        std::thread::id this_id = std::this_thread::get_id();
        std::thread::id pool_id{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto snd = ex::then(
            ex::schedule(sched), [&] { pool_id = std::this_thread::get_id(); });

        ex::sync_wait(std::move(snd));

        HPX_TEST(pool_id != std::thread::id{});
        HPX_TEST_NEQ(this_id, pool_id);
    }

    // Forward progress guarantee is parallel
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        HPX_TEST(ex::get_forward_progress_guarantee(sched) ==
            ex::forward_progress_guarantee::parallel);
    }

    // get_completion_scheduler returns the scheduler
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        HPX_TEST(ex::get_completion_scheduler<ex::set_value_t>(
                     ex::get_env(ex::schedule(sched))) == sched);
    }

    // Chain task: two then calls execute on same thread
    {
        std::thread::id this_id = std::this_thread::get_id();
        std::thread::id pool_id{};
        std::thread::id pool_id2{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto snd = ex::then(
            ex::schedule(sched), [&] { pool_id = std::this_thread::get_id(); });
        auto snd2 = ex::then(
            std::move(snd), [&] { pool_id2 = std::this_thread::get_id(); });

        ex::sync_wait(std::move(snd2));

        HPX_TEST(pool_id != std::thread::id{});
        HPX_TEST_NEQ(this_id, pool_id);
        HPX_TEST(pool_id == pool_id2);
    }

    // P2079R10 example: schedule + then chain with values
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        auto begin = ex::schedule(sched);
        auto hi = ex::then(begin, [] { return 13; });
        auto add_42 = ex::then(hi, [](int arg) { return arg + 42; });
        auto [i] = ex::sync_wait(add_42).value();
        HPX_TEST_EQ(i, 55);
    }

    // Error propagation
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        bool caught_error = false;

        auto snd = ex::schedule(sched) |
            ex::then([] -> int { throw std::runtime_error("test error"); });

        try
        {
            ex::sync_wait(std::move(snd));
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            caught_error = true;
            HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
        }
        HPX_TEST(caught_error);
    }

    // when_all with multiple senders
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto s1 = ex::schedule(sched) | ex::then([] { return 1; });
        auto s2 = ex::schedule(sched) | ex::then([] { return 2; });
        auto s3 = ex::schedule(sched) | ex::then([] { return 3; });

        auto [r1, r2, r3] = ex::sync_wait(ex::when_all(s1, s2, s3)).value();
        HPX_TEST_EQ(r1, 1);
        HPX_TEST_EQ(r2, 2);
        HPX_TEST_EQ(r3, 3);
    }

    // Simple bulk task
    {
        std::thread::id this_id = std::this_thread::get_id();
        constexpr std::size_t num_tasks = 16;
        std::thread::id pool_ids[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk(
            ex::schedule(sched), ex::par, num_tasks, [&](unsigned long id) {
                pool_ids[id] = std::this_thread::get_id();
            });

        ex::sync_wait(std::move(bulk_snd));

        for (auto pool_id : pool_ids)
        {
            HPX_TEST(pool_id != std::thread::id{});
            HPX_TEST_NEQ(this_id, pool_id);
        }
    }

    // Bulk chaining with value propagation
    {
        std::thread::id this_id = std::this_thread::get_id();
        constexpr std::size_t num_tasks = 16;
        std::thread::id pool_id{};
        std::thread::id propagated_pool_ids[num_tasks]{};
        std::thread::id pool_ids[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto snd = ex::then(ex::schedule(sched), [&] {
            pool_id = std::this_thread::get_id();
            return pool_id;
        });

        auto bulk_snd = ex::bulk(std::move(snd), ex::par, num_tasks,
            [&](unsigned long id, std::thread::id propagated_pool_id) {
                propagated_pool_ids[id] = propagated_pool_id;
                pool_ids[id] = std::this_thread::get_id();
            });

        std::optional<std::tuple<std::thread::id>> res =
            ex::sync_wait(std::move(bulk_snd));

        // first schedule ran on a different thread
        HPX_TEST(pool_id != std::thread::id{});
        HPX_TEST_NEQ(this_id, pool_id);

        // bulk items ran and propagated the received value
        for (std::size_t i = 0; i < num_tasks; ++i)
        {
            HPX_TEST(pool_ids[i] != std::thread::id{});
            HPX_TEST(propagated_pool_ids[i] == pool_id);
            HPX_TEST_NEQ(this_id, pool_ids[i]);
        }

        // result of bulk is the same as the first schedule
        HPX_TEST(res.has_value());
        HPX_TEST(std::get<0>(res.value()) == pool_id);
    }

    // Bulk error handling
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        bool caught_error = false;

        auto bulk_snd =
            ex::bulk(ex::schedule(sched), ex::par, 20, [](std::size_t i) {
                if (i == 10)
                    throw std::runtime_error("Bulk error");
            });

        try
        {
            ex::sync_wait(std::move(bulk_snd));
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            caught_error = true;
            HPX_TEST(
                std::string(e.what()).find("Bulk error") != std::string::npos);
        }
        HPX_TEST(caught_error);
    }

    // Simple bulk_chunked task
    {
        std::thread::id this_id = std::this_thread::get_id();
        constexpr std::size_t num_tasks = 16;
        std::thread::id pool_ids[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::par,
            num_tasks, [&](unsigned long b, unsigned long e) {
                for (unsigned long id = b; id < e; ++id)
                    pool_ids[id] = std::this_thread::get_id();
            });

        ex::sync_wait(std::move(bulk_snd));

        for (auto pool_id : pool_ids)
        {
            HPX_TEST(pool_id != std::thread::id{});
            HPX_TEST_NEQ(this_id, pool_id);
        }
    }

    // bulk_chunked performs chunking (with large shape)
    {
        std::atomic<bool> has_chunking{false};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::par, 10000,
            [&](unsigned long b, unsigned long e) {
                if ((e - b) > 1)
                    has_chunking = true;
            });

        ex::sync_wait(std::move(bulk_snd));
        HPX_TEST(has_chunking.load());
    }

    // bulk_chunked covers the entire range
    {
        constexpr std::size_t num_tasks = 200;
        bool covered[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::par,
            num_tasks, [&](unsigned long b, unsigned long e) {
                for (auto i = b; i < e; ++i)
                    covered[i] = true;
            });

        ex::sync_wait(std::move(bulk_snd));

        for (std::size_t i = 0; i < num_tasks; ++i)
        {
            HPX_TEST(covered[i]);
        }
    }

    // bulk_chunked with seq doesn't do chunking (single chunk)
    {
        constexpr std::size_t num_tasks = 200;
        std::atomic<int> execution_count{0};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::seq,
            num_tasks, [&](std::size_t b, std::size_t e) {
                HPX_TEST_EQ(b, std::size_t(0));
                HPX_TEST_EQ(e, num_tasks);
                execution_count++;
            });

        ex::sync_wait(std::move(bulk_snd));

        // Per P2079R10 reference: seq should produce exactly 1 chunk
        // with b==0, e==num_tasks.
        HPX_TEST_EQ(execution_count.load(), 1);
    }

    // Simple bulk_unchunked task
    {
        std::thread::id this_id = std::this_thread::get_id();
        constexpr std::size_t num_tasks = 16;
        std::thread::id pool_ids[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(
            ex::schedule(sched), ex::par, num_tasks, [&](unsigned long id) {
                pool_ids[id] = std::this_thread::get_id();
            });

        ex::sync_wait(std::move(bulk_snd));

        for (auto pool_id : pool_ids)
        {
            HPX_TEST(pool_id != std::thread::id{});
            HPX_TEST_NEQ(this_id, pool_id);
        }
    }

    // bulk_unchunked with seq runs everything on one thread
    {
        constexpr std::size_t num_tasks = 16;
        std::thread::id pool_ids[num_tasks]{};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(
            ex::schedule(sched), ex::seq, num_tasks, [&](unsigned long id) {
                pool_ids[id] = std::this_thread::get_id();
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            });

        ex::sync_wait(std::move(bulk_snd));

        for (auto pool_id : pool_ids)
        {
            HPX_TEST(pool_id != std::thread::id{});
            // Per P2079R10 reference: all should be on same thread with seq.
            HPX_TEST(pool_id == pool_ids[0]);
        }
    }

    // bulk with par_unseq)
    {
        constexpr std::size_t num_tasks = 128;
        std::atomic<std::size_t> count{0};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk(
            ex::schedule(sched), ex::par_unseq, num_tasks, [&](std::size_t) {
                count.fetch_add(1, std::memory_order_relaxed);
            });

        ex::sync_wait(std::move(bulk_snd));
        HPX_TEST_EQ(count.load(), num_tasks);
    }

    // Stop token support test (P2079R10 requirement)
    {
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();
        experimental::execution::async_scope scope;
        scope.request_stop();
        HPX_TEST(scope.get_stop_source().stop_requested());

        bool called = false;
        auto snd = ex::then(ex::schedule(sched), [&called] { called = true; });

        scope.spawn(std::move(snd));
        ex::sync_wait(scope.on_empty());

        HPX_TEST(!called);
    }

    // Test set_value_t completion scheduler query
    {
        auto sched = ex::get_parallel_scheduler();
        auto snd = ex::schedule(sched);
        auto env = ex::get_env(snd);

        // Query the completion scheduler for set_value_t
        auto completion_sched =
            ex::get_completion_scheduler<ex::set_value_t>(env);
        HPX_TEST_EQ(completion_sched, sched);
    }

    // Test that set_stopped_t IS now exposed (per project decision / Isidoros)
    {
        auto sched = ex::get_parallel_scheduler();
        auto snd = ex::schedule(sched);
        auto env = ex::get_env(snd);

        auto stopped_sched =
            ex::get_completion_scheduler<ex::set_stopped_t>(env);
        HPX_TEST_EQ(stopped_sched, sched);
    }

    // Test receiver double-move safety: if execute() throws, receiver is still valid
    {
        auto sched = ex::get_parallel_scheduler();
        auto snd = ex::schedule(sched) | ex::then([]() { return 42; });

        // This should complete successfully without double-move issues
        ex::sync_wait(std::move(snd));
    }

    // Test bulk_unchunked with completes_on pattern
    {
        auto sched = ex::get_parallel_scheduler();
        std::vector<int> v(10, 0);

        auto snd = ex::schedule(sched) | ex::then([&v]() { return 77; }) |
            ex::bulk_unchunked(
                ex::par, 10, [&v](std::size_t i, int val) { v[i] = val; });

        ex::sync_wait(std::move(snd));

        // All elements should be set to 77
        for (int i = 0; i < 10; ++i)
        {
            HPX_TEST_EQ(v[i], 77);
        }
    }

    // Test bulk_unchunked with multiple value arguments
    {
        auto sched = ex::get_parallel_scheduler();
        std::vector<int> v(10, 0);

        auto snd = ex::schedule(sched) | ex::then([]() { return 88; }) |
            ex::bulk_unchunked(
                ex::par, 10, [&v](std::size_t i, int val) { v[i] = val; });

        ex::sync_wait(std::move(snd));

        // All elements should be set to 88
        for (int i = 0; i < 10; ++i)
        {
            HPX_TEST_EQ(v[i], 88);
        }
    }

    // Test sequential bulk with completes_on
    {
        auto sched = ex::get_parallel_scheduler();
        std::vector<int> v(5, 0);
        std::set<std::thread::id> thread_ids;

        auto snd = ex::schedule(sched) | ex::then([&v]() { return 55; }) |
            ex::bulk_chunked(ex::seq, 5,
                [&v, &thread_ids](std::size_t begin, std::size_t end, int val) {
                    for (std::size_t i = begin; i < end; ++i)
                        v[i] = val;
                    thread_ids.insert(std::this_thread::get_id());
                });

        ex::sync_wait(std::move(snd));

        // All elements should be set to 55
        for (int i = 0; i < 5; ++i)
        {
            HPX_TEST_EQ(v[i], 55);
        }
        // Sequential execution should use only 1 thread
        HPX_TEST_EQ(thread_ids.size(), std::size_t(1));
    }

    // Unchunked internal chunking: large shape covers entire range
    {
        constexpr std::size_t n = 100000;
        auto sched = ex::get_parallel_scheduler();
        std::vector<std::atomic<int>> flags(n);
        for (auto& f : flags)
            f.store(0, std::memory_order_relaxed);

        auto snd = ex::bulk_unchunked(
            ex::schedule(sched), ex::par, n, [&](std::size_t i) {
                flags[i].fetch_add(1, std::memory_order_relaxed);
            });

        ex::sync_wait(std::move(snd));

        for (std::size_t i = 0; i < n; ++i)
        {
            HPX_TEST_EQ(flags[i].load(), 1);
        }
    }

    // Unchunked internal chunking: value propagation with large shape
    {
        constexpr std::size_t n = 50000;
        auto sched = ex::get_parallel_scheduler();
        std::vector<int> results(n, 0);

        auto snd = ex::schedule(sched) | ex::then([]() { return 7; }) |
            ex::bulk_unchunked(ex::par, n,
                [&](std::size_t i, int val) { results[i] = val + 1; });

        auto [passthrough] = ex::sync_wait(std::move(snd)).value();
        HPX_TEST_EQ(passthrough, 7);

        for (std::size_t i = 0; i < n; ++i)
        {
            HPX_TEST_EQ(results[i], 8);
        }
    }

    // Unchunked + bulk large shape covers entire range
    {
        constexpr std::size_t n = 100000;
        auto sched = ex::get_parallel_scheduler();
        std::vector<std::atomic<int>> flags(n);
        for (auto& f : flags)
            f.store(0, std::memory_order_relaxed);

        auto snd =
            ex::bulk(ex::schedule(sched), ex::par, n, [&](std::size_t i) {
                flags[i].fetch_add(1, std::memory_order_relaxed);
            });

        ex::sync_wait(std::move(snd));

        for (std::size_t i = 0; i < n; ++i)
        {
            HPX_TEST_EQ(flags[i].load(), 1);
        }
    }

    // Chained bulk: bulk -> then -> bulk (composability via sender chaining)
    {
        constexpr std::size_t n = 256;
        auto sched = ex::get_parallel_scheduler();
        std::vector<std::atomic<int>> phase1(n);
        std::vector<std::atomic<int>> phase2(n);
        for (auto& p : phase1)
            p.store(0, std::memory_order_relaxed);
        for (auto& p : phase2)
            p.store(0, std::memory_order_relaxed);

        auto snd = ex::bulk(ex::schedule(sched), ex::par, n,
                       [&](std::size_t i) {
                           phase1[i].store(1, std::memory_order_relaxed);
                       }) |
            ex::bulk(ex::par, n, [&](std::size_t i) {
                phase2[i].store(phase1[i].load(std::memory_order_relaxed) + 1,
                    std::memory_order_relaxed);
            });

        ex::sync_wait(std::move(snd));

        for (std::size_t i = 0; i < n; ++i)
        {
            HPX_TEST_EQ(phase1[i].load(), 1);
            HPX_TEST_EQ(phase2[i].load(), 2);
        }
    }

    // Mixed bulk variants chained: bulk_chunked -> bulk_unchunked
    {
        constexpr std::size_t n = 200;
        auto sched = ex::get_parallel_scheduler();
        std::vector<std::atomic<int>> results(n);
        for (auto& r : results)
            r.store(0, std::memory_order_relaxed);

        auto snd =
            ex::bulk_chunked(ex::schedule(sched), ex::par, n,
                [&](std::size_t begin, std::size_t end) {
                    for (std::size_t i = begin; i < end; ++i)
                        results[i].fetch_add(10, std::memory_order_relaxed);
                }) |
            ex::bulk_unchunked(ex::par, n, [&](std::size_t i) {
                results[i].fetch_add(1, std::memory_order_relaxed);
            });

        ex::sync_wait(std::move(snd));

        for (std::size_t i = 0; i < n; ++i)
        {
            HPX_TEST_EQ(results[i].load(), 11);
        }
    }

    // P2079R10 Replaceability API tests

    // Backend via shared_ptr: two schedulers from get_parallel_scheduler share backend
    {
        auto sched1 = ex::get_parallel_scheduler();
        auto sched2 = ex::get_parallel_scheduler();
        HPX_TEST(sched1 == sched2);

        // Both share the same backend pointer
        HPX_TEST(sched1.get_backend() == sched2.get_backend());
    }

    // Backend provides underlying scheduler (default HPX backend)
    {
        auto sched = ex::get_parallel_scheduler();
        auto const* underlying = sched.get_underlying_scheduler();
        HPX_TEST(underlying != nullptr);
    }

    // Backend provides PU mask (default HPX backend)
    {
        auto sched = ex::get_parallel_scheduler();
        auto const* pu_mask = sched.get_pu_mask();
        HPX_TEST(pu_mask != nullptr);
    }

    // query_parallel_scheduler_backend returns a valid backend
    {
        auto backend = ex::query_parallel_scheduler_backend();
        HPX_TEST(backend != nullptr);
    }

    // Custom backend: schedule completes via proxy
    {
        struct counting_backend final : ex::parallel_scheduler_backend
        {
            std::atomic<int>& schedule_count;

            explicit counting_backend(std::atomic<int>& count)
              : schedule_count(count)
            {
            }

            void schedule(ex::parallel_scheduler_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                schedule_count.fetch_add(1, std::memory_order_relaxed);
                proxy.set_value();
            }

            void schedule_bulk_chunked(std::size_t count,
                ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                for (std::size_t b = 0; b < count; b += 64)
                {
                    auto e = (std::min) (b + std::size_t(64), count);
                    proxy.execute(b, e);
                }
                proxy.set_value();
            }

            void schedule_bulk_unchunked(std::size_t count,
                ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                for (std::size_t i = 0; i < count; ++i)
                    proxy.execute(i, i + 1);
                proxy.set_value();
            }

            bool equal_to(ex::parallel_scheduler_backend const& other)
                const noexcept override
            {
                return this == &other;
            }
        };

        std::atomic<int> count{0};
        auto backend = std::make_shared<counting_backend>(count);
        ex::parallel_scheduler sched(backend);

        // schedule through custom backend
        auto snd = ex::schedule(sched) | ex::then([] { return 99; });
        auto [val] = ex::sync_wait(std::move(snd)).value();
        HPX_TEST_EQ(val, 99);
        HPX_TEST(count.load() > 0);
    }

    // Custom backend equality: same pointer => equal
    {
        struct dummy_backend final : ex::parallel_scheduler_backend
        {
            void schedule(ex::parallel_scheduler_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                proxy.set_value();
            }
            void schedule_bulk_chunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                proxy.set_value();
            }
            void schedule_bulk_unchunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                proxy.set_value();
            }
            bool equal_to(ex::parallel_scheduler_backend const& other)
                const noexcept override
            {
                return this == &other;
            }
        };

        auto b1 = std::make_shared<dummy_backend>();
        auto b2 = std::make_shared<dummy_backend>();

        ex::parallel_scheduler s1(b1);
        ex::parallel_scheduler s2(b1);    // same backend
        ex::parallel_scheduler s3(b2);    // different backend

        HPX_TEST(s1 == s2);
        HPX_TEST(!(s1 == s3));
    }

    // Default backend: schedulers from different get_parallel_scheduler() calls
    // share the same backend and are equal
    {
        auto s1 = ex::get_parallel_scheduler();
        auto s2 = ex::get_parallel_scheduler();
        HPX_TEST(s1 == s2);
        HPX_TEST(s1.get_backend().get() == s2.get_backend().get());
    }

    // set_parallel_scheduler_backend() actually replaces the live backend
    {
        struct marker_backend final : ex::parallel_scheduler_backend
        {
            std::atomic<int>& hit;
            explicit marker_backend(std::atomic<int>& h)
              : hit(h)
            {
            }

            void schedule(ex::parallel_scheduler_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                hit.fetch_add(1, std::memory_order_relaxed);
                p.set_value();
            }
            void schedule_bulk_chunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                p.set_value();
            }
            void schedule_bulk_unchunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                p.set_value();
            }
            bool equal_to(
                ex::parallel_scheduler_backend const& o) const noexcept override
            {
                return this == &o;
            }
        };

        std::atomic<int> hit{0};
        auto orig = ex::query_parallel_scheduler_backend();

        // Install the marker backend
        ex::set_parallel_scheduler_backend(
            std::make_shared<marker_backend>(hit));

        // get_parallel_scheduler() must now use the marker backend
        auto sched = ex::get_parallel_scheduler();
        ex::sync_wait(ex::schedule(sched));
        HPX_TEST(hit.load() > 0);

        // Restore the original backend so other tests are unaffected
        ex::set_parallel_scheduler_backend(orig);
        HPX_TEST(ex::get_parallel_scheduler() == ex::get_parallel_scheduler());
    }

    // Virtual bulk dispatch: custom backend that implements bulk via
    // schedule_bulk_chunked. This verifies that the parallel_bulk_dispatch_sender
    // correctly routes through the virtual path when get_underlying_scheduler()
    // returns nullptr.
    {
        struct bulk_counting_backend final : ex::parallel_scheduler_backend
        {
            std::atomic<int>& schedule_hits;
            std::atomic<int>& bulk_hits;

            bulk_counting_backend(
                std::atomic<int>& sched, std::atomic<int>& bulk)
              : schedule_hits(sched)
              , bulk_hits(bulk)
            {
            }

            void schedule(ex::parallel_scheduler_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                schedule_hits.fetch_add(1, std::memory_order_relaxed);
                p.set_value();
            }
            void schedule_bulk_chunked(std::size_t count,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                bulk_hits.fetch_add(1, std::memory_order_relaxed);
                // Execute all elements in one chunk
                if (count > 0)
                    p.execute(0, count);
                p.set_value();
            }
            void schedule_bulk_unchunked(std::size_t count,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                bulk_hits.fetch_add(1, std::memory_order_relaxed);
                for (std::size_t i = 0; i < count; ++i)
                    p.execute(i, i + 1);
                p.set_value();
            }
            bool equal_to(
                ex::parallel_scheduler_backend const& o) const noexcept override
            {
                return this == &o;
            }
            // Returns nullptr: triggers virtual dispatch path
        };

        std::atomic<int> sched_hits{0};
        std::atomic<int> bulk_hits{0};
        auto b = std::make_shared<bulk_counting_backend>(sched_hits, bulk_hits);
        ex::parallel_scheduler sched(b);

        // Bulk operation through virtual dispatch
        std::vector<int> results(10, 0);
        auto bulk_snd = ex::schedule(sched) |
            stdexec::bulk(stdexec::par, 10,
                [&results](std::size_t i) { results[i] = 42; });
        ex::sync_wait(std::move(bulk_snd));

        // Verify: schedule was called (for the child sender) and
        // bulk was dispatched through the backend
        HPX_TEST(sched_hits.load() > 0);
        HPX_TEST(bulk_hits.load() > 0);
        for (int i = 0; i < 10; ++i)
        {
            HPX_TEST_EQ(results[i], 42);
        }
    }

    // stop_requested() on the proxy: returns false when no stop is in flight.
    // The backend can call this to poll for cancellation during schedule().
    {
        bool proxy_saw_stop = false;

        struct stop_check_backend final : ex::parallel_scheduler_backend
        {
            bool& saw_;
            explicit stop_check_backend(bool& b)
              : saw_(b)
            {
            }

            void schedule(ex::parallel_scheduler_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                // No stop has been requested; proxy must report false.
                saw_ = p.stop_requested();
                p.set_value();
            }
            void schedule_bulk_chunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                p.set_value();
            }
            void schedule_bulk_unchunked(std::size_t,
                ex::parallel_scheduler_bulk_item_receiver_proxy& p,
                std::span<std::byte>) noexcept override
            {
                p.set_value();
            }
            bool equal_to(
                ex::parallel_scheduler_backend const& o) const noexcept override
            {
                return this == &o;
            }
        };

        auto b = std::make_shared<stop_check_backend>(proxy_saw_stop);
        ex::parallel_scheduler sched(b);
        ex::sync_wait(ex::schedule(sched));
        HPX_TEST(!proxy_saw_stop);
    }

    // ========================================================================
    // P3804R2 VERIFICATION TESTS
    // ========================================================================
    // These tests verify the P3804R2 specification for execution policy
    // handling in bulk operations. P3804R2 clarifies that:
    // - seq policy: Backend receives count=1, executes all work sequentially
    // - par policy: Backend receives count=shape, distributes work in parallel

    // P3804R2: bulk_chunked with seq policy calls f(0, shape) exactly once
    {
        constexpr std::size_t num_tasks = 200;
        std::atomic<int> execution_count{0};
        std::size_t observed_begin = 999;
        std::size_t observed_end = 999;
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::seq,
            num_tasks, [&](std::size_t b, std::size_t e) {
                observed_begin = b;
                observed_end = e;
                execution_count++;
            });

        ex::sync_wait(std::move(bulk_snd));

        // P3804R2 3.7: seq policy should produce exactly 1 call
        // with f(0, shape, args...)
        HPX_TEST_EQ(execution_count.load(), 1);
        HPX_TEST_EQ(observed_begin, std::size_t(0));
        HPX_TEST_EQ(observed_end, num_tasks);
    }

    // P3804R2: bulk_chunked with par policy creates multiple chunks
    {
        constexpr std::size_t num_tasks = 10000;
        std::atomic<int> chunk_count{0};
        std::atomic<bool> has_chunking{false};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_chunked(ex::schedule(sched), ex::par,
            num_tasks, [&](std::size_t b, std::size_t e) {
                chunk_count++;
                if ((e - b) > 1)
                    has_chunking = true;
            });

        ex::sync_wait(std::move(bulk_snd));

        // P3804R2 3.7: par policy should create multiple chunks
        HPX_TEST(chunk_count.load() > 1);
        HPX_TEST(has_chunking.load());
    }

    // P3804R2: bulk_unchunked with seq executes all items on same thread
    {
        constexpr std::size_t num_tasks = 50;
        std::thread::id pool_ids[num_tasks];
        std::atomic<int> execution_count{0};
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(
            ex::schedule(sched), ex::seq, num_tasks, [&](std::size_t id) {
                pool_ids[id] = std::this_thread::get_id();
                execution_count++;
            });

        ex::sync_wait(std::move(bulk_snd));

        // P3804R2 3.7: seq policy should execute sequentially
        // All items should execute on the same thread
        HPX_TEST_EQ(execution_count.load(), static_cast<int>(num_tasks));
        std::thread::id first_thread = pool_ids[0];
        for (std::size_t i = 1; i < num_tasks; ++i)
        {
            HPX_TEST_EQ(pool_ids[i], first_thread);
        }
    }

    // P3804R2: bulk_unchunked with par uses multiple threads
    {
        constexpr std::size_t num_tasks = 200;
        std::thread::id pool_ids[num_tasks];
        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(ex::schedule(sched), ex::par,
            num_tasks,
            [&](std::size_t id) { pool_ids[id] = std::this_thread::get_id(); });

        ex::sync_wait(std::move(bulk_snd));

        // P3804R2 3.7: par policy should use multiple threads
        std::set<std::thread::id> unique_threads;
        for (auto tid : pool_ids)
        {
            unique_threads.insert(tid);
        }
        HPX_TEST(unique_threads.size() > 1);
    }

    // P3804R2: Verify all elements are processed exactly once with seq
    {
        constexpr std::size_t num_tasks = 100;
        std::atomic<int> counters[num_tasks];
        for (auto& c : counters)
            c.store(0);

        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(ex::schedule(sched), ex::seq,
            num_tasks, [&](std::size_t id) { counters[id]++; });

        ex::sync_wait(std::move(bulk_snd));

        // Every element should be processed exactly once
        for (std::size_t i = 0; i < num_tasks; ++i)
        {
            HPX_TEST_EQ(counters[i].load(), 1);
        }
    }

    // P3804R2: Verify all elements are processed exactly once with par
    {
        constexpr std::size_t num_tasks = 1000;
        std::atomic<int> counters[num_tasks];
        for (auto& c : counters)
            c.store(0);

        ex::parallel_scheduler sched = ex::get_parallel_scheduler();

        auto bulk_snd = ex::bulk_unchunked(ex::schedule(sched), ex::par,
            num_tasks, [&](std::size_t id) { counters[id]++; });

        ex::sync_wait(std::move(bulk_snd));

        // Every element should be processed exactly once
        for (std::size_t i = 0; i < num_tasks; ++i)
        {
            HPX_TEST_EQ(counters[i].load(), 1);
        }
    }

    return hpx::local::finalize();
}
#else
int hpx_main(int, char*[])
{
    // parallel_scheduler requires HPX_HAVE_STDEXEC
    return hpx::local::finalize();
}
#endif

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
