//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// thread_queue::get_next_threads must not hand out more threads than
// requested, in every configuration (including HPX_HAVE_THREAD_QUEUE_WAITTIME,
// whose dequeue loop used to ignore the limit). When stealing, the minimum
// number of pending threads applies to all of them, not to the number of
// threads requested.

#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

// use the queue type of the scheduler running this test, so that threads
// released below are handed back to their queue through it
using queue_type =
    hpx::threads::policies::local_priority_queue_scheduler<>::thread_queue_type;

constexpr std::size_t num_threads = 16;
constexpr std::size_t max_items = 5;

int hpx_main(queue_type& queue)
{
    auto* scheduler = hpx::threads::get_self_id_data()->get_scheduler_base();

    // all threads have to run before they can be destroyed
    hpx::latch done(static_cast<std::ptrdiff_t>(num_threads + 1));

    {
        for (std::size_t i = 0; i != num_threads; ++i)
        {
            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function_nullary(
                    [&done] { done.count_down(1); }),
                "thread_queue_get_next_threads",
                hpx::threads::thread_priority::normal,
                hpx::threads::thread_schedule_hint(),
                hpx::threads::thread_stacksize::small_,
                hpx::threads::thread_schedule_state::pending, true, scheduler);

            hpx::threads::thread_id_ref_type id;
            queue.create_thread(data, &id, hpx::throws);
            HPX_TEST(id);
        }
        HPX_TEST_EQ(queue.get_pending_queue_length(),
            static_cast<std::int64_t>(num_threads));

        // steal fewer threads than are available
        std::vector<hpx::threads::thread_id_ref_type> taken;
        taken.reserve(max_items);
        std::size_t const n = queue.get_next_threads(
            std::back_inserter(taken), max_items, true, true);

        HPX_TEST_EQ(n, max_items);
        HPX_TEST_EQ(taken.size(), max_items);
        HPX_TEST_EQ(queue.get_pending_queue_length(),
            static_cast<std::int64_t>(num_threads - max_items));

        // fewer threads than the minimum are left, they can't be stolen
        std::vector<hpx::threads::thread_id_ref_type> rest;
        HPX_TEST_EQ(queue.get_next_threads(
                        std::back_inserter(rest), num_threads, true, true),
            std::size_t(0));
        HPX_TEST(rest.empty());

        // the rest is still there
        std::size_t const m = queue.get_next_threads(
            std::back_inserter(rest), num_threads, false, true);

        HPX_TEST_EQ(m, num_threads - max_items);
        HPX_TEST_EQ(rest.size(), num_threads - max_items);
        HPX_TEST_EQ(queue.get_pending_queue_length(), std::int64_t(0));

        // run the threads through the scheduler, they are handed back to
        // this queue once they have terminated
        for (auto& thrd : taken)
        {
            scheduler->schedule_thread(
                HPX_MOVE(thrd), hpx::threads::thread_schedule_hint(), true);
        }
        for (auto& thrd : rest)
        {
            scheduler->schedule_thread(
                HPX_MOVE(thrd), hpx::threads::thread_schedule_hint(), true);
        }
        taken.clear();
        rest.clear();
        done.arrive_and_wait();
    }

    return hpx::local::finalize();
}

int main(int, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=2"};

    std::string const queuing = "--hpx:queuing=local-priority-fifo";
    std::vector<char*> av = {argv[0], const_cast<char*>(queuing.c_str())};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    // stealing is allowed only while all threads are pending
    hpx::threads::policies::thread_queue_init_parameters const parameters(
        HPX_THREAD_QUEUE_MAX_THREAD_COUNT,
        static_cast<std::int64_t>(num_threads));

    // the queue has to outlive the runtime: a worker hands a terminated thread
    // back to the queue and may still access it after the thread count has
    // dropped to zero
    queue_type queue(parameters);

    HPX_TEST_EQ(hpx::local::init([&] { return hpx_main(queue); },
                    static_cast<int>(av.size()), av.data(), init_args),
        0);

    // all threads have terminated, no worker accesses the queue anymore
    HPX_TEST_EQ(queue.get_thread_count(), std::int64_t(0));
    HPX_TEST(queue.cleanup_terminated(true));

    return hpx::util::report_errors();
}
