//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that the parallel algorithms chunk a loop whose body was given a
// time (timed_task) by that time: task_size_chunk_size gets the run time of
// an element in picoseconds, so a body of 0.25 ns runs a loop on a quarter of
// the cores a nano body (1 ns) would, and one of 0.75 ns on three quarters. The
// parameters record the cores and the chunk size the partitioner reports
// (collect_execution_parameters). The test runs on 4 worker threads, with a
// minimal chunk time of 1 us and one chunk per core: a core is given 1000
// nano iterations.
//
// The builtin arithmetic function objects and the algorithms that call no
// function get the run time of the bytes they read and write per element, at
// copy_bytes_per_ns (16 by default, so a double moved costs 0.5 ns): copy and
// negate 16 bytes (0.5 ns), adding two doubles into a third 24 bytes
// (0.75 ns), fill 8 bytes (0.25 ns).

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;
using hpx::chrono::picoseconds;

namespace {

    struct recorded
    {
        std::size_t cores = 0;
        std::size_t chunk_size = 0;
    };

    struct recording_parameters
      : hpx::execution::experimental::task_size_chunk_size
    {
        explicit recording_parameters(recorded* out) noexcept
          : task_size_chunk_size(1us, 1)
          , out_(out)
        {
        }

        template <typename Executor>
        void collect_execution_parameters(Executor&&, std::size_t,
            std::size_t const cores, std::size_t,
            std::size_t const chunk_size) const noexcept
        {
            out_->cores = cores;
            out_->chunk_size = chunk_size;
        }

        recorded* out_;
    };
}    // namespace

template <>
struct hpx::execution::experimental::is_executor_parameters<
    recording_parameters> : std::true_type
{
};

namespace {

    constexpr std::size_t count = 4000;

    template <typename Run>
    void check(Run&& run, std::size_t const cores, std::size_t const chunk_size)
    {
        std::vector<double> v(count, 0.0);
        recorded result;
        auto policy = hpx::execution::par.with(recording_parameters(&result));
        run(policy, v);

        HPX_TEST_EQ(result.cores, cores);
        HPX_TEST_EQ(result.chunk_size, chunk_size);

        std::size_t wrong = 0;
        for (double const d : v)
        {
            wrong += d != 1.0 ? 1 : 0;
        }
        HPX_TEST_EQ(wrong, std::size_t(0));
    }

    // the cores and the chunk size the partitioner reports for run
    template <typename Run>
    void check_split(
        Run&& run, std::size_t const cores, std::size_t const chunk_size)
    {
        recorded result;
        auto policy = hpx::execution::par.with(recording_parameters(&result));
        run(policy);

        HPX_TEST_EQ(result.cores, cores);
        HPX_TEST_EQ(result.chunk_size, chunk_size);
    }

    void test_byte_rates()
    {
        std::vector<double> a(count, 1.0), b(count, 2.0), c(count, 0.0);

        // adding two doubles into a third: 24 bytes, 0.75 ns
        check_split(
            [&](auto& policy) {
                hpx::transform(policy, a.begin(), a.end(), b.begin(), c.begin(),
                    std::plus<>());
            },
            3, 1334);
        HPX_TEST_EQ(c[count - 1], 3.0);

        // a negation and a copy of a double: 16 bytes, 0.5 ns
        check_split(
            [&](auto& policy) {
                hpx::transform(
                    policy, a.begin(), a.end(), c.begin(), std::negate<>());
            },
            2, 2000);
        check_split(
            [&](auto& policy) {
                hpx::copy(policy, a.begin(), a.end(), c.begin());
            },
            2, 2000);

        // fill writes 8 bytes, 0.25 ns
        check_split(
            [&](auto& policy) { hpx::fill(policy, c.begin(), c.end(), 4.0); },
            1, count);
        HPX_TEST_EQ(c[0], 4.0);
    }

    auto const increment = [](double& d) { d += 1.0; };

    template <typename Body>
    auto for_each_with(Body body)
    {
        return [body](auto& policy, std::vector<double>& v) {
            hpx::for_each(policy, v.begin(), v.end(), body);
        };
    }

    // The rates by footprint per core (step 3): the runtime builds them
    // from hpx.task_size_chunk_size.copy_bytes_per_ns_l1 to _memory, set
    // below for data in memory only; with a rate of 64 bytes per ns for
    // footprints of up to 1 MB per core, copying 4000 doubles (64 KB) takes
    // 0.125 ns per element and stays on one core, where it takes two at the
    // default rate.
    void test_memory_rates()
    {
        namespace exd = hpx::execution::experimental::detail;
        constexpr std::size_t any_footprint =
            (std::numeric_limits<std::size_t>::max)();

        // from the configuration: data in memory at 8 bytes per ns, the
        // caches' shares (where the topology has caches) at
        // copy_bytes_per_ns (16)
        HPX_TEST_EQ(exd::get_copy_bytes_per_ns(any_footprint), std::size_t(8));
        if (hpx::threads::get_topology().get_cache_info(0, 1).size != 0)
        {
            HPX_TEST_EQ(exd::get_copy_bytes_per_ns(1), std::size_t(16));
        }

        exd::set_memory_rates(
            {{std::size_t(1) << 20, 64}, {any_footprint, 16}});
        HPX_TEST_EQ(exd::get_copy_bytes_per_ns(1000), std::size_t(64));
        HPX_TEST_EQ(exd::get_copy_bytes_per_ns(2 << 20), std::size_t(16));

        std::vector<double> a(count, 1.0), c(count, 0.0);
        auto const copy = [&](auto& policy) {
            hpx::copy(policy, a.begin(), a.end(), c.begin());
        };
        check_split(copy, 1, count);

        // without rates, copy_bytes_per_ns for every footprint
        exd::set_memory_rates({});
        HPX_TEST_EQ(exd::get_copy_bytes_per_ns(1000), std::size_t(16));
        check_split(copy, 2, 2000);
    }

    int hpx_main(int, char**)
    {
        // a nano body: 4000 iterations of 1 ns, 4 cores of 1000
        check(for_each_with(hpx::nano_task(increment)), 4, 1000);

        // 0.25 ns: a core takes 4000 iterations, one core
        check(for_each_with(hpx::timed_task(increment, picoseconds(250))), 1,
            count);

        // 0.5 ns: a core takes 2000 iterations, two cores
        check(for_each_with(hpx::timed_task(increment, picoseconds(500))), 2,
            2000);

        // 0.75 ns: a core takes 1333 iterations, three cores, chunks of
        // at least 1 us (1334 iterations)
        check(for_each_with(hpx::timed_task(increment, picoseconds(750))), 3,
            1334);

        // through a wrapper that forwards the time
        check(for_each_with(hpx::bind_front(
                  hpx::timed_task(increment, picoseconds(250)))),
            1, count);

        // through task_size_of_iteration: transform
        check(
            [](auto& policy, std::vector<double>& v) {
                hpx::transform(policy, v.begin(), v.end(), v.begin(),
                    hpx::timed_task(
                        [](double d) { return d + 1.0; }, picoseconds(250)));
            },
            1, count);

        // through for_loop's wrapper
        check(
            [](auto& policy, std::vector<double>& v) {
                hpx::experimental::for_loop(policy, std::size_t(0), v.size(),
                    hpx::timed_task([&v](std::size_t i) { v[i] += 1.0; },
                        picoseconds(250)));
            },
            1, count);

        test_byte_rates();
        test_memory_rates();

        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    // a rate for data in memory (see test_memory_rates)
    hpx::local::init_params params;
    params.cfg = {"hpx.task_size_chunk_size.copy_bytes_per_ns_memory=8"};
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, params), 0);
    return hpx::util::report_errors();
}
