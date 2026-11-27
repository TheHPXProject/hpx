//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A smoke test of hpx_tune's chunk area: run the tool as a process of its
// own, for a small pool and few regions, and check what it wrote. The file
// has to hold a section per executor, one for the pool size measured and one
// for the pool sizes that weren't, and the HPX runtime started with it
// (--hpx:config) has to give each executor that minimal chunk time
// (get_min_chunk_time). Loops chunked by task_size_chunk_size, attached to
// the policy so that it doesn't depend on HPX_WITH_TASK_SIZE_DEFAULT_PARAMETERS,
// with a body that has a task size, have to get their cores and chunks from
// these values: without both, the values have no effect. The report has to
// hold the chunk area's measurements. A rule the tool doesn't know makes it
// fail without writing a file. The test checks the form of the files and
// that HPX takes their values, not whether the values are good for the
// machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;

namespace {

    std::string const chunk_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_chunk.ini";
    std::string const report_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_chunk.json";
    std::string const refused_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_chunk_refused.ini";

    // the pool size the tool measures; the runtime of this test may have
    // another, as the tool also writes the value for the sizes not measured
    constexpr std::size_t pool_size = 2;

    // run hpx_tune with the given arguments, return its exit code
    int run_tool(std::string const& arguments)
    {
        std::string command =
            "\"" + std::string(HPX_TUNE_EXECUTABLE) + "\" " + arguments;
#if defined(_WIN32)
        // cmd.exe drops the first and the last quote of a command that
        // starts with one
        command = "\"" + command + "\"";
#endif
        std::cout << "running: " << command << std::endl;
        return std::system(command.c_str());
    }

    std::string read_file(std::string const& name)
    {
        std::ifstream in(name);
        return std::string(std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>());
    }

    std::vector<std::string> const executors = {"parallel", "fork_join",
        "suspending_fork_join", "shared_suspending_fork_join"};

    std::string key(std::string const& executor, std::string const& size)
    {
        return "hpx.task_size_chunk_size." + executor +
            (size.empty() ? "" : "." + size) + ".min_chunk_time_us";
    }

    // the file, read with HPX's ini reader: both sections of each executor,
    // with positive values; parallel_executor keeps HPX's default
    void check_file()
    {
        hpx::util::section file;
        file.read(chunk_file);
        for (std::string const& executor : executors)
        {
            for (std::string const& size :
                std::vector<std::string>{std::to_string(pool_size), ""})
            {
                std::string const value =
                    file.get_entry(key(executor, size), "");
                HPX_TEST_MSG(!value.empty(), key(executor, size).c_str());
                HPX_TEST_MSG(value.empty() || std::stod(value) > 0.0,
                    key(executor, size).c_str());
            }
        }
        HPX_TEST_EQ(
            file.get_entry(key("parallel", ""), ""), std::string("2.50"));
    }

    // the report: the chunk area's part, with the options it ran with
    void check_report()
    {
        std::string const json = read_file(report_file);
        for (char const* member : {"\"areas\": {\"chunk\": {",
                 "\"pool_sizes\": [2]", "\"regions\": 50", "\"runs\": 1",
                 "\"rule\": \"fitted\"", "\"chunks_per_core\": ",
                 "\"measured_region_times\": [{\"run\": 0, ",
                 "\"region_times\": [{\"executor\": \"parallel\", ",
                 "\"costs\": [{\"executor\": \"parallel\", ",
                 "\"settings\": [{\"key\": \"hpx.task_size_chunk_size."})
        {
            HPX_TEST_MSG(json.find(member) != std::string::npos, member);
        }
    }

    // the value the file gives an executor of this runtime, as the runtime
    // reads it: its pool size's, else that of the sizes not measured
    std::chrono::nanoseconds expected(std::string const& executor)
    {
        std::string const size = std::to_string(hpx::get_num_worker_threads());
        std::string value = hpx::get_config_entry(key(executor, size), "");
        if (value.empty())
            value = hpx::get_config_entry(key(executor, ""), "");
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double, std::micro>(std::stod(value)));
    }

    // task_size_chunk_size on a loop of the executor, with a body of nano
    // task size (1 ns per iteration): the cores and the chunks follow from
    // the executor's value, and a loop with it attached runs every iteration
    template <typename Executor>
    void check_task_size_control(
        Executor const& exec, std::string const& executor)
    {
        std::chrono::nanoseconds const value = expected(executor);
        HPX_TEST_MSG(ex::get_min_chunk_time(exec) == value, executor.c_str());

        ex::task_size_chunk_size const tscs;
        auto const body = hpx::nano_task([](std::size_t) {});
        auto const iteration = tscs.measure_iteration(exec, body, 1);
        HPX_TEST_MSG(
            iteration.value() == std::chrono::nanoseconds(1), executor.c_str());

        // the iterations of one core: chunks_per_core chunks of the value
        std::size_t const chunks_per_core =
            ex::detail::get_task_size_chunk_size_defaults().chunks_per_core;
        auto const per_core =
            static_cast<std::size_t>(value.count()) * chunks_per_core;
        std::size_t const available = ex::processing_units_count(exec);

        // one core short of two cores' work stays on one, two cores' work
        // takes two
        HPX_TEST_EQ_MSG(
            tscs.processing_units_count(exec, iteration, 2 * per_core - 1),
            std::size_t(1), executor.c_str());
        if (available >= 2)
        {
            HPX_TEST_EQ_MSG(
                tscs.processing_units_count(exec, iteration, 2 * per_core),
                std::size_t(2), executor.c_str());
        }

        // on two cores, a loop of one core's work has chunks of the value
        HPX_TEST_EQ_MSG(tscs.get_chunk_size(exec, iteration, 2, per_core),
            static_cast<std::size_t>(value.count()), executor.c_str());

        // a loop with task_size_chunk_size attached, for some cores
        std::size_t const count = 3 * per_core + 7;
        std::vector<std::atomic<int>> calls(count);
        hpx::experimental::for_loop(hpx::execution::par.on(exec).with(tscs),
            std::size_t(0), count, hpx::nano_task([&](std::size_t const i) {
                calls[i].fetch_add(1, std::memory_order_relaxed);
            }));
        std::size_t wrong = 0;
        for (auto const& c : calls)
        {
            wrong += c.load() != 1 ? 1 : 0;
        }
        HPX_TEST_EQ_MSG(wrong, std::size_t(0), executor.c_str());
    }

    // the runtime started with the file: each executor asks for its value,
    // and the task size control uses it
    int check_with_runtime()
    {
        check_task_size_control(
            hpx::execution::parallel_executor(), "parallel");
        {
            ex::fork_join_executor const exec;
            check_task_size_control(exec, "fork_join");
        }
        {
            ex::suspending_fork_join_executor const exec;
            check_task_size_control(exec, "suspending_fork_join");
        }
        check_task_size_control(ex::shared_suspending_fork_join_executor(),
            "shared_suspending_fork_join");
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    // the chunk area alone, small enough for a test
    std::string const options = "--chunk --chunk-pool-size " +
        std::to_string(pool_size) +
        " --chunk-runs 1 --chunk-regions 50 --repeats 3";
    HPX_TEST_EQ(run_tool(options + " --output \"" + chunk_file +
                    "\" --report \"" + report_file + "\""),
        0);
    check_file();
    check_report();

    // the file through the runtime: accepted with --hpx:config, and its
    // values reach the executors
    std::string const config = "--hpx:config=" + chunk_file;
    std::vector<char*> args(argv, argv + argc);
    args.push_back(const_cast<char*>(config.c_str()));
    args.push_back(nullptr);
    HPX_TEST_EQ(hpx::local::init(check_with_runtime,
                    static_cast<int>(args.size() - 1), args.data()),
        0);

    // a rule the tool doesn't know: it fails and writes no file
    std::remove(refused_file.c_str());
    HPX_TEST_NEQ(run_tool(options + " --chunk-rule no_such_rule --output \"" +
                     refused_file + "\""),
        0);
    HPX_TEST(!std::ifstream(refused_file).good());

    return hpx::util::report_errors();
}
