//  Copyright (c) 2018-2020 Mikael Simberg
//  Copyright (c) 2018-2019 John Biddiscombe
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/chrono.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::async;
using hpx::future;
using hpx::post;

using hpx::chrono::high_resolution_timer;

// global vars we stick here to make printouts easy for plotting
static std::string queuing = "default";
static std::size_t numa_sensitive = 0;
static std::uint64_t num_threads = 1;
static std::string info_string = "";

///////////////////////////////////////////////////////////////////////////////
// we use globals here to prevent the delay from being optimized away
double global_scratch = 0;
std::uint64_t num_iterations = 0;

///////////////////////////////////////////////////////////////////////////////
double null_function() noexcept
{
    if (num_iterations > 0)
    {
        int const array_size = 4096;
        std::array<double, array_size> dummy;
        for (std::uint64_t i = 0; i < num_iterations; ++i)
        {
            for (std::uint64_t j = 0; j < array_size; ++j)
            {
                dummy[j] = 1.0 /
                    (2.0 * static_cast<double>(i) * static_cast<double>(j) +
                        1.0);
            }
        }
        return dummy[0];
    }
    return 0.0;
}

struct scratcher
{
    void operator()(future<double> r) const
    {
        global_scratch += r.get();
    }
};

void measure_function_futures_create_thread_hierarchical_placement(
    std::uint64_t count, int const repetitions)
{
    auto sched = hpx::threads::get_self_id_data()->get_scheduler_base();

    if (std::string("core-shared_priority_queue_scheduler") ==
        sched->get_description())
    {
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                sched->get_parent_pool()->get_pool_name());
        sched->add_remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::assign_work_thread_parent,
            hpx::threads::policies::scheduler_mode::enable_stealing |
                hpx::threads::policies::scheduler_mode::enable_stealing_numa |
                hpx::threads::policies::scheduler_mode::
                    assign_work_round_robin |
                hpx::threads::policies::scheduler_mode::steal_after_local |
                hpx::threads::policies::scheduler_mode::
                    steal_high_priority_first,
            full_mask);
    }
    auto const desc = hpx::threads::thread_description();
    auto prio = hpx::threads::thread_priority::normal;
    auto stack_size = hpx::threads::thread_stacksize::small_;
    auto num_threads = hpx::get_num_worker_threads();
    hpx::error_code ec;

    hpx::util::perftests_report(
        "future overhead - create_thread_hierarchical - latch", "no-executor",
        repetitions, [&]() -> void {
            hpx::latch l(static_cast<std::int64_t>(count));

            auto const func = [&l]() {
                null_function();
                l.count_down(1);
            };
            auto const thread_func =
                hpx::threads::detail::thread_function_nullary<decltype(func)>{
                    func};
            for (std::size_t t = 0; t < num_threads; ++t)
            {
                auto const hint = hpx::threads::thread_schedule_hint(
                    static_cast<std::int16_t>(t));
                auto spawn_func = [&thread_func, sched, hint, t, count,
                                      num_threads, desc, prio, stack_size]() {
                    std::uint64_t const count_start = t * count / num_threads;
                    std::uint64_t const count_end =
                        (t + 1) * count / num_threads;
                    hpx::error_code ec;
                    for (std::uint64_t i = count_start; i < count_end; ++i)
                    {
                        hpx::threads::thread_init_data init(
                            hpx::threads::thread_function_type(thread_func),
                            desc, prio, hint, stack_size,
                            hpx::threads::thread_schedule_state::pending, false,
                            sched);
                        sched->create_thread(init, nullptr, ec);
                    }
                };

                // different versions of clang-format disagree
                // clang-format off
                auto const thread_spawn_func =
                    hpx::threads::detail::thread_function_nullary<
                        decltype(spawn_func)>{spawn_func};
                // clang-format on

                hpx::threads::thread_init_data init(
                    hpx::threads::thread_function_type(thread_spawn_func), desc,
                    prio, hint, stack_size,
                    hpx::threads::thread_schedule_state::pending, false, sched);
                sched->create_thread(init, nullptr, ec);
            }
            l.wait();
        });
    hpx::util::perftests_print_times();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    {
        if (vm.count("hpx:queuing"))
            queuing = vm["hpx:queuing"].as<std::string>();

        if (vm.count("hpx:numa-sensitive"))
            numa_sensitive = 1;
        else
            numa_sensitive = 0;

        bool test_all = (vm.count("test-all") > 0);
        int const repetitions = vm["test_count"].as<int>();

        if (vm.count("info"))
            info_string = vm["info"].as<std::string>();

        num_threads = hpx::get_num_worker_threads();

        num_iterations = vm["delay-iterations"].as<std::uint64_t>();

        std::uint64_t const count = vm["futures"].as<std::uint64_t>();

        hpx::util::perftests_init(vm, "future_overhead_report");

        if (HPX_UNLIKELY(0 == count))
            throw std::logic_error("error: count of 0 futures specified\n");

        if (test_all)
        {
            measure_function_futures_create_thread_hierarchical_placement(
                count, repetitions);
        }
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()("futures",
        value<std::uint64_t>()->default_value(500000),
        "number of futures to invoke")

        ("delay-iterations", value<std::uint64_t>()->default_value(0),
         "number of iterations in the delay loop")

        ("test-all", "run all benchmarks")
        ("test_count", value<int>()->default_value(100),
         "number of repetitions of the full benchmark (default: 100)")

        ("info", value<std::string>()->default_value("no-info"),
         "extra info for plot output (e.g. branch name)");
    // clang-format on

    // Initialize and run HPX.
    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
