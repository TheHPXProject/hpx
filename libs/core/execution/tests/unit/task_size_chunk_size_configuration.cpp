//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The defaults of task_size_chunk_size come from the configuration of the
// runtime, and from the library if the configuration gives none. An executor
// can have a minimal run time of a chunk of its own.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

using namespace std::chrono_literals;
namespace ex = hpx::execution::experimental;

int hpx_main_configured()
{
    auto const defaults = ex::detail::get_task_size_chunk_size_defaults();
    HPX_TEST(defaults.min_chunk_time == 20us);
    HPX_TEST_EQ(defaults.chunks_per_core, 2u);
    HPX_TEST_EQ(defaults.copy_bytes_per_ns, 32u);

    hpx::execution::parallel_executor exec;
    std::size_t const available = ex::processing_units_count(exec);

    // 20 us and 2 chunks per core: a core needs 40 iterations of 1 us
    ex::task_size_chunk_size const configured;
    HPX_TEST_EQ(configured.processing_units_count(exec, 1us, 80),
        (std::min) (std::size_t(2), available));

    // 1000 iterations in 2 chunks for each of 4 cores
    HPX_TEST_EQ(configured.get_chunk_size(exec, 1us, 4, 1000), 125u);

    // chunks are not shorter than 20 us
    HPX_TEST_EQ(configured.get_chunk_size(exec, 1us, 4, 100), 20u);

    // given values win, a chunks_per_core of 0 stands for the default
    ex::task_size_chunk_size const given(5us, 4);
    HPX_TEST_EQ(given.get_chunk_size(exec, 1us, 4, 1000), 63u);

    ex::task_size_chunk_size const given_time(5us);
    HPX_TEST_EQ(given_time.get_chunk_size(exec, 1us, 4, 1000), 125u);
    HPX_TEST_EQ(given_time.get_chunk_size(exec, 1us, 4, 20), 5u);

    return hpx::local::finalize();
}

int hpx_main_executors()
{
    // configured for every number of worker threads, and for some
    HPX_TEST(ex::detail::get_executor_min_chunk_time("fork_join", 2) == 250ns);
    HPX_TEST(ex::detail::get_executor_min_chunk_time("fork_join", 3) == 500ns);
    HPX_TEST(ex::detail::get_executor_min_chunk_time("fork_join", 4) == 750ns);
    HPX_TEST(ex::detail::get_executor_min_chunk_time("parallel", 2) == 10us);
    HPX_TEST(ex::detail::get_executor_min_chunk_time("other", 2) == 2500ns);

    // the executors ask for the values configured for them and their number
    // of worker threads, an executor without a value of its own gets the
    // default
    hpx::execution::parallel_executor exec;
    HPX_TEST(ex::get_min_chunk_time(exec) == 10us);

    hpx::execution::experimental::fork_join_executor fork_join;
    std::size_t const num_threads =
        hpx::threads::count(ex::get_processing_units_mask(fork_join));
    std::chrono::nanoseconds const fork_join_time =
        num_threads == 2 ? 250ns : (num_threads == 4 ? 750ns : 500ns);
    HPX_TEST(ex::get_min_chunk_time(fork_join) == fork_join_time);

    hpx::execution::sequenced_executor seq;
    HPX_TEST(ex::get_min_chunk_time(seq) == 2500ns);

    ex::task_size_chunk_size const configured;

    // chunks are not shorter than 10 us on the parallel_executor ...
    HPX_TEST_EQ(configured.get_chunk_size(exec, 1us, 4, 100), 10u);

    // ... and fork_join_time on the fork_join_executor (2.5 us, the default,
    // would make it one chunk of all 20 iterations)
    HPX_TEST_EQ(configured.get_chunk_size(fork_join, 100ns, 2, 20),
        static_cast<std::size_t>((fork_join_time + 99ns) / 100ns));

    // a given minimal run time wins over the executor's
    ex::task_size_chunk_size const given(1us);
    HPX_TEST_EQ(given.get_chunk_size(fork_join, 100ns, 2, 20), 10u);

    return hpx::local::finalize();
}

int hpx_main_compiled()
{
    // a runtime started without the configuration entries uses the compiled
    // defaults again
    auto const defaults = ex::detail::get_task_size_chunk_size_defaults();
    HPX_TEST(defaults.min_chunk_time == 2500ns);
    HPX_TEST_EQ(defaults.chunks_per_core, 8u);
    HPX_TEST_EQ(defaults.copy_bytes_per_ns, 16u);
    HPX_TEST(ex::detail::get_executor_min_chunk_time("fork_join", 2) ==
        defaults.min_chunk_time);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    {
        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.task_size_chunk_size.min_chunk_time_us=20",
            "hpx.task_size_chunk_size.chunks_per_core=2",
            "hpx.task_size_chunk_size.copy_bytes_per_ns=32"};

        HPX_TEST_EQ_MSG(
            hpx::local::init(hpx_main_configured, argc, argv, init_args), 0,
            "HPX main exited with non-zero status");
    }

    {
        hpx::local::init_params init_args;
        init_args.cfg = {
            "hpx.task_size_chunk_size.fork_join.min_chunk_time_us!=0.5",
            "hpx.task_size_chunk_size.fork_join.2.min_chunk_time_us!=0.25",
            "hpx.task_size_chunk_size.fork_join.4.min_chunk_time_us!=0.75",
            "hpx.task_size_chunk_size.fork_join.x.min_chunk_time_us!=1",
            "hpx.task_size_chunk_size.parallel.min_chunk_time_us!=10"};

        HPX_TEST_EQ_MSG(
            hpx::local::init(hpx_main_executors, argc, argv, init_args), 0,
            "HPX main exited with non-zero status");
    }

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main_compiled, argc, argv), 0,
        "HPX main exited with non-zero status");

    // configured values out of range are ignored, the compiled defaults are
    // used instead
    std::vector<std::vector<std::string>> const out_of_range = {
        {"hpx.task_size_chunk_size.min_chunk_time_us=-1",
            "hpx.task_size_chunk_size.chunks_per_core=1025"},
        {"hpx.task_size_chunk_size.chunks_per_core=0"},
        {"hpx.task_size_chunk_size.copy_bytes_per_ns=0"},
        {"hpx.task_size_chunk_size.copy_bytes_per_ns=4097"},
        {"hpx.task_size_chunk_size.min_chunk_time_us=2000000"},
        {"hpx.task_size_chunk_size.min_chunk_time_us=nan"},
        {"hpx.task_size_chunk_size.min_chunk_time_us=inf"},
        {"hpx.task_size_chunk_size.fork_join.min_chunk_time_us!=-1",
            "hpx.task_size_chunk_size.fork_join.2.min_chunk_time_us!=nan"}};
    for (auto const& cfg : out_of_range)
    {
        hpx::local::init_params init_args;
        init_args.cfg = cfg;

        HPX_TEST_EQ_MSG(
            hpx::local::init(hpx_main_compiled, argc, argv, init_args), 0,
            "HPX main exited with non-zero status");
    }

    return hpx::util::report_errors();
}
