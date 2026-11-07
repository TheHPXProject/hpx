// Copyright (c) 2026 Shivansh Singh
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/executors/task_scheduler.hpp>
#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>

namespace ex = hpx::execution::experimental;

namespace {

    struct custom_mock_scheduler
    {
        int id = 0;

        struct sender;
        sender schedule() const noexcept;

        ex::forward_progress_guarantee query(
            ex::get_forward_progress_guarantee_t) const noexcept
        {
            return ex::forward_progress_guarantee::concurrent;
        }

        template <typename F>
        void execute(F&& f) const
        {
            HPX_FORWARD(F, f)();
        }

        friend bool operator==(custom_mock_scheduler const& lhs,
            custom_mock_scheduler const& rhs) noexcept
        {
            return lhs.id == rhs.id;
        }
    };

    struct custom_mock_scheduler::sender
    {
        using sender_concept = ex::sender_t;
        using completion_signatures =
            ex::completion_signatures<ex::set_value_t(),
                ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>;

        template <typename Receiver>
        struct operation_state
        {
            std::decay_t<Receiver> receiver;

            void start() & noexcept
            {
                ex::set_value(HPX_MOVE(receiver));
            }
        };

        template <typename Receiver>
        operation_state<Receiver> connect(Receiver&& r) const
        {
            return {HPX_FORWARD(Receiver, r)};
        }

        struct env
        {
            custom_mock_scheduler sched;

            auto query(
                ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept
            {
                return sched;
            }
        };

        env get_env() const noexcept
        {
            return {custom_mock_scheduler{}};
        }
    };

    inline custom_mock_scheduler::sender custom_mock_scheduler::schedule()
        const noexcept
    {
        return {};
    }

    struct custom_mock_backend final : ex::parallel_scheduler_backend
    {
        int id = 0;

        explicit custom_mock_backend(int val = 0) noexcept
          : id(val)
        {
        }

        void schedule(ex::parallel_scheduler_receiver_proxy& proxy,
            std::span<std::byte>) noexcept override
        {
            proxy.set_value();
        }

        void schedule_bulk_chunked(std::size_t count,
            ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
            std::span<std::byte>) noexcept override
        {
            if (count > 0)
            {
                proxy.execute(0, count);
            }
            proxy.set_value();
        }

        void schedule_bulk_unchunked(std::size_t count,
            ex::parallel_scheduler_bulk_item_receiver_proxy& proxy,
            std::span<std::byte>) noexcept override
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                proxy.execute(i, i + 1);
            }
            proxy.set_value();
        }

        bool equal_to(
            ex::parallel_scheduler_backend const& other) const noexcept override
        {
            auto const* p = dynamic_cast<custom_mock_backend const*>(&other);
            return p != nullptr && p->id == id;
        }

        ex::forward_progress_guarantee get_forward_progress_guarantee()
            const noexcept override
        {
            return ex::forward_progress_guarantee::concurrent;
        }
    };

}    // namespace

int hpx_main(int, char*[])
{
    // Type and Concept checks
    {
        static_assert(ex::scheduler<ex::task_scheduler>,
            "task_scheduler must model scheduler concept");
        static_assert(std::is_copy_constructible_v<ex::task_scheduler>,
            "task_scheduler must be copy constructible");
        static_assert(std::is_move_constructible_v<ex::task_scheduler>,
            "task_scheduler must be move constructible");
        static_assert(std::is_nothrow_copy_constructible_v<ex::task_scheduler>,
            "task_scheduler copy constructor should be noexcept");
        static_assert(std::is_nothrow_move_constructible_v<ex::task_scheduler>,
            "task_scheduler move constructor should be noexcept");
    }

    // Target 1: Construction from parallel_scheduler (reuses backend)
    {
        ex::parallel_scheduler par_sched = ex::get_parallel_scheduler();
        ex::task_scheduler ts(par_sched);

        HPX_TEST(ts.get_backend() != nullptr);
        HPX_TEST(ts.get_backend() == par_sched.get_backend());
        HPX_TEST_EQ(ts.get_backend().get(), par_sched.get_backend().get());
    }

    // Target 2: Construction from custom backend wrapper (arbitrary scheduler)
    {
        custom_mock_scheduler mock{42};
        ex::task_scheduler ts(mock);

        HPX_TEST(ts.get_backend() != nullptr);

        // Verify backend equality logic through backend wrapper
        custom_mock_scheduler mock_same{42};
        ex::task_scheduler ts_same(mock_same);

        custom_mock_scheduler mock_diff{99};
        ex::task_scheduler ts_diff(mock_diff);

        HPX_TEST(ts.get_backend()->equal_to(*ts_same.get_backend()));
        HPX_TEST(!ts.get_backend()->equal_to(*ts_diff.get_backend()));
    }

    // Target 3: Construction from std::shared_ptr<parallel_scheduler_backend> directly
    {
        auto custom_be = std::make_shared<custom_mock_backend>(123);
        ex::task_scheduler ts(custom_be);

        HPX_TEST(ts.get_backend() == custom_be);
        HPX_TEST_EQ(ts.get_backend().get(), custom_be.get());
    }

    // Target 4: task_scheduler::schedule() smoke test
    {
        // 4a. With default parallel_scheduler backend
        {
            ex::task_scheduler ts(ex::get_parallel_scheduler());
            std::atomic<bool> executed{false};

            auto snd =
                ex::schedule(ts) | ex::then([&]() { executed.store(true); });

            hpx::this_thread::experimental::sync_wait(snd);
            HPX_TEST(executed.load());
        }

        // 4b. With custom backend wrapper
        {
            custom_mock_scheduler mock{1};
            ex::task_scheduler ts(mock);
            std::atomic<bool> executed{false};

            auto snd =
                ex::schedule(ts) | ex::then([&]() { executed.store(true); });

            hpx::this_thread::experimental::sync_wait(snd);
            HPX_TEST(executed.load());
        }

        // 4c. With custom backend directly
        {
            auto custom_be = std::make_shared<custom_mock_backend>(7);
            ex::task_scheduler ts(custom_be);
            std::atomic<bool> executed{false};

            auto snd =
                ex::schedule(ts) | ex::then([&]() { executed.store(true); });

            hpx::this_thread::experimental::sync_wait(snd);
            HPX_TEST(executed.load());
        }
    }

    // Target 5: Equality and inequality operators (backend pointer equality)
    {
        ex::parallel_scheduler par_sched = ex::get_parallel_scheduler();
        ex::task_scheduler ts1(par_sched);
        ex::task_scheduler ts2(par_sched);
        ex::task_scheduler ts1_copy = ts1;

        HPX_TEST(ts1 == ts2);
        HPX_TEST(ts1 == ts1_copy);
        HPX_TEST(!(ts1 != ts2));

        auto be1 = std::make_shared<custom_mock_backend>(1);
        auto be2 = std::make_shared<custom_mock_backend>(1);
        ex::task_scheduler ts_be1(be1);
        ex::task_scheduler ts_be2(be2);

        // Different shared_ptr instances -> operator== returns false
        HPX_TEST(ts_be1 != ts_be2);
        HPX_TEST(!(ts_be1 == ts_be2));
        HPX_TEST(ts1 != ts_be1);
    }

    // Target 6: Forward progress guarantee query
    {
        ex::task_scheduler ts(ex::get_parallel_scheduler());
        auto fpg = ex::get_forward_progress_guarantee(ts);
        HPX_TEST(fpg == ex::forward_progress_guarantee::parallel);

        custom_mock_scheduler mock{10};
        ex::task_scheduler ts_custom(mock);
        auto fpg_custom = ex::get_forward_progress_guarantee(ts_custom);
        HPX_TEST(fpg_custom == ex::forward_progress_guarantee::concurrent);
    }

    // Target 7: Bulk Execution with get_parallel_scheduler()
    {
        ex::task_scheduler ts(ex::get_parallel_scheduler());

        // 7a. bulk_chunked with par
        {
            std::atomic<int> count{0};
            std::set<std::thread::id> thread_ids;
            std::mutex mtx;
            constexpr std::size_t n = 100;

            auto snd = ex::schedule(ts) |
                ex::bulk_chunked(
                    ex::par, n, [&](std::size_t begin, std::size_t end) {
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            thread_ids.insert(std::this_thread::get_id());
                        }
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(100));
                        count.fetch_add(static_cast<int>(end - begin),
                            std::memory_order_relaxed);
                    });

            hpx::this_thread::experimental::sync_wait(snd);

            HPX_TEST_EQ(count.load(), static_cast<int>(n));
            HPX_TEST(thread_ids.size() > 1);
        }

        // 7b. bulk_unchunked with par
        {
            std::atomic<int> count{0};
            std::set<std::thread::id> thread_ids;
            std::mutex mtx;
            constexpr std::size_t n = 100;

            auto snd = ex::schedule(ts) |
                ex::bulk_unchunked(ex::par, n, [&](std::size_t) {
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        thread_ids.insert(std::this_thread::get_id());
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                    count.fetch_add(1, std::memory_order_relaxed);
                });

            hpx::this_thread::experimental::sync_wait(snd);

            HPX_TEST_EQ(count.load(), static_cast<int>(n));
            HPX_TEST(thread_ids.size() > 1);
        }
    }

    // Target 8: Construction from thread_pool_scheduler (real HPX scheduler)
    {
        ex::thread_pool_scheduler tps{};
        ex::task_scheduler ts(tps);

        HPX_TEST(ts.get_backend() != nullptr);

        std::atomic<bool> executed{false};
        auto snd = ex::schedule(ts) | ex::then([&]() { executed.store(true); });

        hpx::this_thread::experimental::sync_wait(snd);
        HPX_TEST(executed.load());

        auto fpg = ex::get_forward_progress_guarantee(ts);
        HPX_TEST(fpg == ex::forward_progress_guarantee::parallel);

        // 8a. bulk_chunked with par
        {
            std::atomic<int> bulk_count{0};
            std::set<std::thread::id> tps_thread_ids;
            std::mutex tps_mtx;
            constexpr std::size_t n = 100;

            auto bulk_snd = ex::schedule(ts) |
                ex::bulk_chunked(
                    ex::par, n, [&](std::size_t begin, std::size_t end) {
                        {
                            std::lock_guard<std::mutex> lock(tps_mtx);
                            tps_thread_ids.insert(std::this_thread::get_id());
                        }
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(100));
                        bulk_count.fetch_add(static_cast<int>(end - begin),
                            std::memory_order_relaxed);
                    });

            hpx::this_thread::experimental::sync_wait(bulk_snd);

            HPX_TEST_EQ(bulk_count.load(), static_cast<int>(n));
            HPX_TEST(tps_thread_ids.size() > 1);
        }

        // 8b. bulk_unchunked with par
        {
            std::atomic<int> bulk_count{0};
            std::set<std::thread::id> tps_thread_ids;
            std::mutex tps_mtx;
            constexpr std::size_t n = 100;

            auto bulk_snd = ex::schedule(ts) |
                ex::bulk_unchunked(ex::par, n, [&](std::size_t) {
                    {
                        std::lock_guard<std::mutex> lock(tps_mtx);
                        tps_thread_ids.insert(std::this_thread::get_id());
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                    bulk_count.fetch_add(1, std::memory_order_relaxed);
                });

            hpx::this_thread::experimental::sync_wait(bulk_snd);

            HPX_TEST_EQ(bulk_count.load(), static_cast<int>(n));
            HPX_TEST(tps_thread_ids.size() > 1);
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
