//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit tests for the dual-queue scheduler backend
// (hpx::threads::policies::lockfree_fifo_double_backend), which provides
// push-to-passive semantics, an active/passive parity toggle, and distinct
// steal=true vs. steal=false pop control flow.

#include <hpx/config.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using backend_type =
    hpx::threads::policies::lockfree_fifo_double_backend<std::uint64_t>;

///////////////////////////////////////////////////////////////////////////////
// 1. Single-threaded FIFO ordering with steal=false.
void test_single_threaded_fifo_order_steal_false()
{
    backend_type backend(16);

    constexpr std::uint64_t count = 8;
    for (std::uint64_t i = 0; i != count; ++i)
    {
        HPX_TEST(backend.push(i));
    }

    // The very first pop(false) call finds the (initially empty) active queue
    // exhausted and toggles parity so that the queue which had been receiving
    // pushes becomes active. From that point on, items must come out in the
    // order they were pushed (FIFO).
    for (std::uint64_t i = 0; i != count; ++i)
    {
        std::uint64_t val = 0;
        HPX_TEST(backend.pop(val, /*steal=*/false));
        HPX_TEST_EQ(val, i);
    }

    std::uint64_t val = 0;
    HPX_TEST(!backend.pop(val, /*steal=*/false));
}

///////////////////////////////////////////////////////////////////////////////
// 2. empty() requires both queues to be empty.
void test_empty_requires_both_queues_empty()
{
    backend_type backend(16);

    HPX_TEST(backend.empty());

    HPX_TEST(backend.push(static_cast<std::uint64_t>(42)));
    HPX_TEST(!backend.empty());

    // Toggle parity so that the pushed item ends up in what is now considered
    // the active queue, while the (still empty) passive queue is the other one.
    // empty() must not be fooled by inspecting only one of the two queues.
    std::uint64_t val = 0;
    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(42));

    HPX_TEST(backend.empty());
}

///////////////////////////////////////////////////////////////////////////////
// 3. Passive-push / parity-toggle correctness: push always lands on the
//    current passive queue, and pop(steal=false) drains the active queue, only
//    reaching the passive queue (by toggling parity) once active is exhausted.
void test_passive_push_and_parity_toggle()
{
    backend_type backend(16);

    HPX_TEST(backend.push(static_cast<std::uint64_t>(1)));
    HPX_TEST(backend.push(static_cast<std::uint64_t>(2)));
    HPX_TEST(backend.push(static_cast<std::uint64_t>(3)));

    std::uint64_t val = 0;

    // Active queue is empty -> retries fail -> toggle -> pop from what is now
    // active (was passive, holds 1, 2, 3) -> first item pushed.
    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(1));

    // A push here must land in the *new* passive queue and must not interfere
    // with the remaining items (2, 3) in the now-active queue.
    HPX_TEST(backend.push(static_cast<std::uint64_t>(4)));

    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(2));

    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(3));

    // Active is exhausted again -> toggle -> pop from the queue holding 4.
    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(4));

    HPX_TEST(backend.empty());
}

///////////////////////////////////////////////////////////////////////////////
// 4. steal=true pop order: passive is tried first, falling back to active,
//    without losing or duplicating items.
void test_steal_pop_order()
{
    backend_type backend(16);

    std::vector<std::uint64_t> expected;
    for (std::uint64_t i = 0; i != 5; ++i)
    {
        HPX_TEST(backend.push(i));
        expected.push_back(i);
    }

    std::vector<std::uint64_t> popped;
    std::uint64_t val = 0;

    // Force a toggle so that item 0 is consumed and items 1..4 remain in what
    // is now the active queue, while the passive queue is empty.
    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(0));
    popped.push_back(val);

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    // Passive is empty, so steal=true must fall back to active and pop from its
    // opposite (left) end, i.e. the most recently pushed remaining item.
    HPX_TEST(backend.pop(val, /*steal=*/true));
    HPX_TEST_EQ(val, std::uint64_t(4));
    popped.push_back(val);

    // New pushes land on the (empty) passive queue and must be preferred by
    // steal=true over the remaining active items.
    HPX_TEST(backend.push(std::uint64_t(99)));
    expected.push_back(99);

    HPX_TEST(backend.pop(val, /*steal=*/true));
    HPX_TEST_EQ(val, std::uint64_t(99));
    popped.push_back(val);
#endif

    // Drain everything else via steal=true; no items may be lost or duplicated
    // regardless of the exact order they come out in. Without 128-bit atomics
    // the backend falls back to a single-ended queue, so both the passive-first
    // and active-fallback pops are plain FIFO.
    while (backend.pop(val, /*steal=*/true))
    {
        popped.push_back(val);
    }

    HPX_TEST(backend.empty());

    std::ranges::sort(popped);
    std::ranges::sort(expected);
    HPX_TEST(popped == expected);
}

///////////////////////////////////////////////////////////////////////////////
// 5. Boundary behavior of the retry-then-toggle control flow in
//    pop(steal=false): a pop must not spuriously succeed while both queues are
//    empty, and must succeed via the parity toggle as soon as the (previously
//    passive) queue holds an item once active is exhausted.
void test_retry_then_toggle_boundary()
{
    backend_type backend(16);

    std::uint64_t val = 0;

    // Both queues empty: repeated pop(false) calls must consistently
    // report failure.
    for (int i = 0; i != 10; ++i)
    {
        HPX_TEST(!backend.pop(val, /*steal=*/false));
    }

    // Active is empty, passive holds exactly one item: pop(false) must still
    // succeed after exhausting the retries and toggling parity.
    HPX_TEST(backend.push(static_cast<std::uint64_t>(7)));
    HPX_TEST(backend.pop(val, /*steal=*/false));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(7));

    HPX_TEST(!backend.pop(val, /*steal=*/false));
}

///////////////////////////////////////////////////////////////////////////////
// 6. pop() returns false when both queues are truly empty, for both
//    steal=true and steal=false.
void test_pop_false_when_empty()
{
    backend_type backend(16);
    std::uint64_t val = 0;

    HPX_TEST(!backend.pop(val, /*steal=*/true));
    HPX_TEST(!backend.pop(val, /*steal=*/false));

    HPX_TEST(backend.push(static_cast<std::uint64_t>(1)));
    HPX_TEST(backend.pop(val, /*steal=*/true));
    HPX_TEST_EQ(val, static_cast<std::uint64_t>(1));

    HPX_TEST(!backend.pop(val, /*steal=*/true));
    HPX_TEST(!backend.pop(val, /*steal=*/false));
}

///////////////////////////////////////////////////////////////////////////////
// 7. Concurrent multi-producer / multi-stealer stress test: every pushed
//    item must be popped exactly once, with no lost or duplicated items. Only a
//    single "owner" thread calls pop(steal=false), since the parity toggle is
//    documented as a single-threaded operation; any number of threads may
//    push() concurrently or call pop(steal=true).
void test_concurrent_stress()
{
    constexpr std::uint64_t num_producers = 4;
    constexpr std::uint64_t items_per_producer = 5000;
    constexpr std::uint64_t num_stealers = 3;
    constexpr std::uint64_t total_items = num_producers * items_per_producer;

    backend_type backend(total_items);

    // Bounds how long a producer or a consumer may wait without making progress
    // (i.e. without a successful push or pop) before it is considered stalled.
    // This guards against spinning forever if an item is ever lost, turning a
    // silent CI hang into an actionable test failure.
    constexpr auto stress_stall_timeout = std::chrono::seconds(30);

    std::atomic<bool> producers_done(false);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<std::vector<std::uint64_t>> collected(num_stealers + 1);

    for (std::uint64_t p = 0; p != num_producers; ++p)
    {
        producers.emplace_back([&, p]() {
            auto last_progress = std::chrono::steady_clock::now();
            std::uint64_t const base = p * items_per_producer;
            for (std::uint64_t i = 0; i != items_per_producer; ++i)
            {
                while (!backend.push(base + i))
                {
                    if (std::chrono::steady_clock::now() - last_progress >
                        stress_stall_timeout)
                    {
                        HPX_TEST_MSG(false,
                            std::string("producer thread stalled: no "
                                        "progress for 30s while pushing "
                                        "item ") +
                                std::to_string(base + i));
                        return;
                    }
                }
                std::this_thread::yield();
            }
            last_progress = std::chrono::steady_clock::now();
        });
    }

    auto run_consumer = [&, stress_stall_timeout](bool const steal,
                            std::size_t const collected_index,
                            char const* role) {
        std::uint64_t val = 0;
        auto last_progress = std::chrono::steady_clock::now();
        while (true)
        {
            if (backend.pop(val, steal))
            {
                collected[collected_index].push_back(val);
                last_progress = std::chrono::steady_clock::now();
            }
            else if (producers_done.load(std::memory_order_acquire) &&
                backend.empty())
            {
                break;
            }
            else
            {
                if (std::chrono::steady_clock::now() - last_progress >
                    stress_stall_timeout)
                {
                    HPX_TEST_MSG(false,
                        std::string(role) +
                            " thread stalled: no progress for 30s, "
                            "producers_done=" +
                            std::to_string(producers_done.load()) +
                            ", backend.empty()=" +
                            std::to_string(backend.empty()) +
                            " (possible lost item)");
                    return;
                }
                std::this_thread::yield();
            }
        }
    };

    // Stealer threads: pop(steal=true) only.
    for (std::uint64_t s = 0; s != num_stealers; ++s)
    {
        consumers.emplace_back([&, s]() { run_consumer(true, s, "stealer"); });
    }

    // Single owner thread: pop(steal=false) only.
    consumers.emplace_back(
        [&]() { run_consumer(false, num_stealers, "owner"); });

    for (auto& t : producers)
    {
        t.join();
    }
    producers_done.store(true, std::memory_order_release);

    for (auto& t : consumers)
    {
        t.join();
    }

    HPX_TEST(backend.empty());

    std::vector<std::uint64_t> all;
    for (auto& v : collected)
    {
        all.insert(all.end(), v.begin(), v.end());
    }

    HPX_TEST_EQ(all.size(), static_cast<std::size_t>(total_items));

    std::ranges::sort(all);
    all.erase(std::ranges::unique(all).begin(), all.end());
    HPX_TEST_EQ(all.size(), static_cast<std::size_t>(total_items));

    for (std::uint64_t i = 0; i != total_items; ++i)
    {
        HPX_TEST_EQ(all[i], i);
    }
}

int main()
{
    test_single_threaded_fifo_order_steal_false();
    test_empty_requires_both_queues_empty();
    test_passive_push_and_parity_toggle();
    test_steal_pop_order();
    test_retry_then_toggle_boundary();
    test_pop_false_when_empty();
    test_concurrent_stress();

    return hpx::util::report_errors();
}
