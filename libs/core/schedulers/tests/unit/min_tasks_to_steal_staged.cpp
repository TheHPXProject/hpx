//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx.thread_queue.min_tasks_to_steal_staged must only keep other workers from
// stealing staged tasks: a worker has to convert its own staged tasks even if
// there are fewer of them than that minimum (this used to hang).

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

constexpr std::ptrdiff_t min_tasks = 10;

struct shared_state
{
    explicit shared_state(std::ptrdiff_t num_tasks)
      : done(num_tasks + 1)
    {
    }

    std::atomic<std::ptrdiff_t> count{0};
    std::atomic<std::ptrdiff_t> ran_elsewhere{0};
    hpx::latch done;
};

// Post fire-and-forget tasks (they are staged) to the given worker and wait
// for all of them. The tasks share ownership of the state, so it stays alive
// until the last of them has returned from count_down.
std::shared_ptr<shared_state> run_tasks(
    std::size_t worker, std::ptrdiff_t num_tasks)
{
    auto state = std::make_shared<shared_state>(num_tasks);

    hpx::execution::parallel_executor exec(
        hpx::threads::thread_schedule_hint(static_cast<std::int16_t>(worker)));
    for (std::ptrdiff_t i = 0; i != num_tasks; ++i)
    {
        hpx::post(exec, [state, worker] {
            if (hpx::get_worker_thread_num() != worker)
                ++state->ran_elsewhere;
            ++state->count;
            state->done.count_down(1);
        });
    }

    state->done.arrive_and_wait();
    return state;
}

int hpx_main()
{
    std::size_t const num_workers = hpx::get_num_worker_threads();

    // Fewer staged tasks than the minimum: the hinted worker has to convert
    // them itself. As min_tasks_to_steal_pending is set as well, no other
    // worker may steal them, neither staged nor pending.
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        auto const state = run_tasks(worker, min_tasks / 3);
        HPX_TEST_EQ(state->count.load(), min_tasks / 3);
        HPX_TEST_EQ(state->ran_elsewhere.load(), 0);
    }

    // More staged tasks than the minimum: these may be stolen, but still all
    // have to run.
    auto const state = run_tasks(0, 3 * min_tasks);
    HPX_TEST_EQ(state->count.load(), 3 * min_tasks);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc.add_options()("scheduler",
        hpx::program_options::value<std::string>()->default_value("default"),
        "default (as selected by --hpx:queuing) | workrequesting");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc;
    init_args.cfg = {"hpx.thread_queue.min_tasks_to_steal_staged=10",
        "hpx.thread_queue.min_tasks_to_steal_pending=10"};

    // select the scheduler through the resource partitioner instead of
    // --hpx:queuing, which may be given on the command line already
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const& vm) {
        if (vm["scheduler"].as<std::string>() == "workrequesting")
        {
            rp.create_thread_pool("default",
                hpx::resource::scheduling_policy::local_workrequesting_fifo);
        }
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
