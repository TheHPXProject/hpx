//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

static std::uint64_t iterations = 20000;
static std::uint64_t repetitions = 5;
static std::uint64_t volatile sink = 0;

struct run_loop_guard
{
    ex::run_loop loop;
    hpx::thread worker;

    run_loop_guard()
      : worker([this]() { loop.run(); })
    {
    }

    ~run_loop_guard()
    {
        loop.finish();
        worker.join();
    }

    auto scheduler() noexcept
    {
        return loop.get_scheduler();
    }
};

template <typename F>
void measure(char const* name, char const* exec, std::uint64_t count,
    std::uint64_t rep, F&& f)
{
    hpx::util::perftests_report(name, exec, rep, [&]() {
        for (std::uint64_t i = 0; i < count; ++i)
        {
            f(i);
        }
    });
}

void benchmark_ready_future_get(std::uint64_t count, std::uint64_t rep)
{
    measure("future boundary", "ready_future.get()", count, rep,
        [](std::uint64_t i) {
            auto f = hpx::make_ready_future<std::uint64_t>(i);
            sink = sink + f.get();
        });
}

void benchmark_keep_future(std::uint64_t count, std::uint64_t rep,
    ex::run_loop::run_loop_scheduler const& sched)
{
    measure("future boundary", "keep_future + continues_on", count, rep,
        [&](std::uint64_t i) {
            auto f = hpx::make_ready_future<std::uint64_t>(i);

            auto result = tt::sync_wait(
                ex::keep_future(std::move(f)) | ex::continues_on(sched));
            HPX_ASSERT(result);
            auto& r = hpx::get<0>(*result);
            sink = sink + r.get();
        });
}

void benchmark_make_future(std::uint64_t count, std::uint64_t rep,
    ex::run_loop::run_loop_scheduler const& sched)
{
    measure("future boundary", "make_future(sched, schedule|then)", count, rep,
        [&](std::uint64_t i) {
            auto fut = ex::make_future(
                sched, ex::schedule(sched) | ex::then([i]() { return i + 1; }));
            sink = sink + fut.get();
        });
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    iterations = vm["iterations"].as<std::uint64_t>();
    repetitions = vm["repetitions"].as<std::uint64_t>();
    hpx::util::perftests_init(vm);

    if (iterations == 0 || repetitions == 0)
    {
        std::cerr << "iterations and repetitions must be non-zero\n"
                  << std::flush;
        hpx::local::finalize();
        return -1;
    }

    {
        run_loop_guard guard;
        auto const sched = guard.scheduler();

        benchmark_ready_future_get(iterations, repetitions);
        benchmark_keep_future(iterations, repetitions, sched);
        benchmark_make_future(iterations, repetitions, sched);

        hpx::util::perftests_print_times();
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("iterations", value<std::uint64_t>()->default_value(20000),
            "number of benchmark iterations per repetition")
        ("repetitions", value<std::uint64_t>()->default_value(5),
            "number of benchmark repetitions");
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all"};

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#endif
