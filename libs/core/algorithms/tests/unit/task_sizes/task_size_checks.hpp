//  Copyright (c) 2026 Vansh Dobhal
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The checks of the task sizes the algorithms give their worker tasks. A
// scheduler that counts the sizes it is asked to create threads with sees
// them. The tests run on one worker, so that the number of tasks doesn't
// depend on the machine.

#pragma once

#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace task_size_checks {

    using hpx::threads::task_size;

    inline std::array<std::atomic<int>, 5> created_with{};

    using base_scheduler =
        hpx::threads::policies::local_priority_queue_scheduler<>;

    struct counting_scheduler : base_scheduler
    {
        using base_scheduler::base_scheduler;

        void create_thread(hpx::threads::thread_init_data& data,
            hpx::threads::thread_id_ref_type* id, hpx::error_code& ec) override
        {
            ++created_with[static_cast<std::size_t>(
                data.schedulehint.task_size())];
            base_scheduler::create_thread(data, id, ec);
        }
    };

    // Runs f and checks that at least one thread has the given size and none
    // has another known size, or that no thread has a known size if the given
    // size is unknown. Threads created by the runtime itself have no size, so
    // they don't count.
    template <typename F>
    void check_only_size(F&& f, task_size size)
    {
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        task_size const known[] = {task_size::nano, task_size::small_,
            task_size::medium, task_size::large};
        int before[4];
        for (int i = 0; i != 4; ++i)
            before[i] = count(known[i]);

        HPX_FORWARD(F, f)();

        for (int i = 0; i != 4; ++i)
        {
            int const created = count(known[i]) - before[i];
            if (known[i] == size)
                HPX_TEST_LT(0, created);
            else
                HPX_TEST_EQ(created, 0);
        }
    }

    // Creates the default pool with the counting scheduler.
    inline std::unique_ptr<hpx::threads::thread_pool_base> create_pool(
        hpx::threads::thread_pool_init_parameters thread_pool_init,
        hpx::threads::policies::thread_queue_init_parameters thread_queue_init)
    {
        counting_scheduler::init_parameter_type init(
            thread_pool_init.num_threads_, thread_pool_init.affinity_data_,
            std::size_t(-1), thread_queue_init);
        auto scheduler = std::make_unique<counting_scheduler>(init);

        return std::make_unique<
            hpx::threads::detail::scheduled_thread_pool<counting_scheduler>>(
            std::move(scheduler), thread_pool_init);
    }

    // Runs hpx_main on one worker of the counting scheduler.
    inline int run(int (*hpx_main)(), int argc, char* argv[])
    {
        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.os_threads=1"};
        init_args.rp_callback =
            [](auto& rp, hpx::program_options::variables_map const&) {
                rp.create_thread_pool("default", &create_pool);
            };

        HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
        return hpx::util::report_errors();
    }
}    // namespace task_size_checks
