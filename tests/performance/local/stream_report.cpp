//  Copyright (c) 2015 Thomas Heller
//  Copyright (c) 2015-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This code is based on the STREAM benchmark:
// https://www.cs.virginia.edu/stream/ref.html
//
// We adopted the code and HPXifyed it.
//

#if defined(HPX_MSVC_NVCC)
// NVCC causes an ICE in MSVC if this is not defined
#define BOOST_NO_CXX11_ALLOCATOR
#endif

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/compute.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/thread.hpp>
#include <hpx/version.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

bool csv = false;
bool header = false;

///////////////////////////////////////////////////////////////////////////////
std::string get_executor_name(std::size_t executor)
{
    switch (executor)
    {
    case 0:
        return "parallel_executor";
    case 1:
        return "block_executor";
    case 2:
        return "parallel_executor";
    case 3:
        return "fork_join_executor";
    case 4:
        return "scheduler_executor";
    case 5:
        return "concurrent_executor";
    default:
        return "no-executor";
    }
}

///////////////////////////////////////////////////////////////////////////////
std::string get_allocator_name(std::size_t allocator)
{
    switch (allocator)
    {
    case 0:
        return "std_allocator";
    case 1:
        return "block_allocator";
    case 2:
        return "default_parallel_policy_allocator";
    case 3:
        return "default_fork_join_policy_allocator";
    case 4:
        return "default_scheduler_executor_allocator";
    case 5:
        return "gpu allocator";
    default:
        return "no-allocator";
    }
}

///////////////////////////////////////////////////////////////////////////////
hpx::threads::topology& retrieve_topology()
{
    static hpx::threads::topology& topo = hpx::threads::create_topology();
    return topo;
}

///////////////////////////////////////////////////////////////////////////////
double mysecond()
{
    return static_cast<double>(hpx::chrono::high_resolution_clock::now()) *
        1e-9;
}

int checktick()
{
    static std::size_t const M = 20;
    double timesfound[M];

    // Collect a sequence of M unique time values from the system.
    for (std::size_t i = 0; i < M; i++)
    {
        double const t1 = mysecond();
        double t2;
        while (((t2 = mysecond()) - t1) < 1.0E-6)
            ;
        timesfound[i] = t2;
    }

    // Determine the minimum difference between these M values.
    // This result will be our estimate (in microseconds) for the
    // clock granularity.
    int minDelta = 1000000;
    for (std::size_t i = 1; i < M; i++)
    {
        int Delta = (int) (1.0E6 * (timesfound[i] - timesfound[i - 1]));
        minDelta = (std::min) (minDelta, (std::max) (Delta, 0));
    }

    return (minDelta);
}

template <typename Vector>
void check_results(std::size_t iterations, Vector const& a_res,
    Vector const& b_res, Vector const& c_res)
{
    std::vector<STREAM_TYPE> a(a_res.size());
    std::vector<STREAM_TYPE> b(b_res.size());
    std::vector<STREAM_TYPE> c(c_res.size());

    hpx::copy(hpx::execution::par, a_res.begin(), a_res.end(), a.begin());
    hpx::copy(hpx::execution::par, b_res.begin(), b_res.end(), b.begin());
    hpx::copy(hpx::execution::par, c_res.begin(), c_res.end(), c.begin());

    STREAM_TYPE aj, bj, cj, scalar;
    STREAM_TYPE aSumErr, bSumErr, cSumErr;
    STREAM_TYPE aAvgErr, bAvgErr, cAvgErr;
    double epsilon;
    int ierr, err;

    /* reproduce initialization */
    aj = 1.0;
    bj = 2.0;
    cj = 0.0;
    /* now execute timing loop */
    scalar = 3.0;
    for (std::size_t k = 0; k < iterations; k++)
    {
        cj = aj;
        bj = scalar * cj;
        cj = aj + bj;
        aj = bj + scalar * cj;
    }

    /* accumulate deltas between observed and expected results */
    aSumErr = 0.0;
    bSumErr = 0.0;
    cSumErr = 0.0;
    for (std::size_t j = 0; j < a.size(); j++)
    {
        aSumErr += std::abs(a[j] - aj);
        bSumErr += std::abs(b[j] - bj);
        cSumErr += std::abs(c[j] - cj);
    }
    aAvgErr = aSumErr / (STREAM_TYPE) a.size();
    bAvgErr = bSumErr / (STREAM_TYPE) a.size();
    cAvgErr = cSumErr / (STREAM_TYPE) a.size();

    if (sizeof(STREAM_TYPE) == 4)
    {
        epsilon = 1.e-6;
    }
    else if (sizeof(STREAM_TYPE) == 8)
    {
        epsilon = 1.e-13;
    }
    else
    {
        hpx::util::format_to(std::cout, "WEIRD: sizeof(STREAM_TYPE) = {}\n",
            sizeof(STREAM_TYPE));
        epsilon = 1.e-6;
    }

    err = 0;
    if (std::abs(aAvgErr / aj) > epsilon)
    {
        err++;
        hpx::util::format_to(std::cout,
            "Failed Validation on array a[], AvgRelAbsErr > epsilon ({})\n",
            epsilon);
        hpx::util::format_to(std::cout,
            "     Expected Value: {}, AvgAbsErr: {}, AvgRelAbsErr: {}\n", aj,
            aAvgErr, std::abs(aAvgErr) / aj);
        ierr = 0;
        for (std::size_t j = 0; j < a.size(); j++)
        {
            if (std::abs(a[j] / aj - 1.0) > epsilon)
            {
                ierr++;
#ifdef VERBOSE
                if (ierr < 10)
                {
                    hpx::util::format_to(std::cout,
                        "         array a: index: {}, expected: {}, "
                        "observed: {}, relative error: {}\n",
                        (unsigned long) j, aj, a[j],
                        (double) std::abs((aj - a[j]) / aAvgErr));
                }
#endif
            }
        }
        hpx::util::format_to(
            std::cout, "     For array a[], {} errors were found.\n", ierr);
    }
    if (std::abs(bAvgErr / bj) > epsilon)
    {
        err++;
        hpx::util::format_to(std::cout,
            "Failed Validation on array b[], AvgRelAbsErr > epsilon ({})\n",
            epsilon);
        hpx::util::format_to(std::cout,
            "     Expected Value: {}, AvgAbsErr: {}, AvgRelAbsErr: {}\n", bj,
            bAvgErr, std::abs(bAvgErr) / bj);
        hpx::util::format_to(
            std::cout, "     AvgRelAbsErr > Epsilon ({})\n", epsilon);
        ierr = 0;
        for (std::size_t j = 0; j < a.size(); j++)
        {
            if (std::abs(b[j] / bj - 1.0) > epsilon)
            {
                ierr++;
#ifdef VERBOSE
                if (ierr < 10)
                {
                    hpx::util::format_to(std::cout,
                        "         array b: index: {}, expected: {}, "
                        "observed: {}, relative error: {}\n",
                        (unsigned long) j, bj, b[j],
                        (double) std::abs((bj - b[j]) / bAvgErr));
                }
#endif
            }
        }
        hpx::util::format_to(
            std::cout, "     For array b[], {} errors were found.\n", ierr);
    }
    if (std::abs(cAvgErr / cj) > epsilon)
    {
        err++;
        hpx::util::format_to(std::cout,
            "Failed Validation on array c[], AvgRelAbsErr > epsilon ({})\n",
            epsilon);
        hpx::util::format_to(std::cout,
            "     Expected Value: {}, AvgAbsErr: {}, AvgRelAbsErr: {}\n", cj,
            cAvgErr, std::abs(cAvgErr) / cj);
        hpx::util::format_to(
            std::cout, "     AvgRelAbsErr > Epsilon ({})\n", epsilon);
        ierr = 0;
        for (std::size_t j = 0; j < a.size(); j++)
        {
            if (std::abs(c[j] / cj - 1.0) > epsilon)
            {
                ierr++;
#ifdef VERBOSE
                if (ierr < 10)
                {
                    hpx::util::format_to(std::cout,
                        "         array c: index: {}, expected: {}, "
                        "observed: {}, relative error: {}\n",
                        (unsigned long) j, cj, c[j],
                        (double) std::abs((cj - c[j]) / cAvgErr));
                }
#endif
            }
        }
        hpx::util::format_to(
            std::cout, "     For array c[], {} errors were found.\n", ierr);
    }
    if (err == 0)
    {
        if (!csv)
        {
            hpx::util::format_to(std::cout,
                "Solution Validates: avg error less than {} on all three "
                "arrays\n",
                epsilon);
        }
    }
#ifdef VERBOSE
    hpx::util::format_to(std::cout, "Results Validation Verbose Results:\n");
    hpx::util::format_to(
        std::cout, "    Expected a(1), b(1), c(1): {} {} {}\n", aj, bj, cj);
    hpx::util::format_to(std::cout, "    Observed a(1), b(1), c(1): {} {} {}\n",
        a[1], b[1], c[1]);
    hpx::util::format_to(std::cout, "    Rel Errors on a, b, c:     {} {} {}\n",
        (double) std::abs(aAvgErr / aj), (double) std::abs(bAvgErr / bj),
        (double) std::abs(cAvgErr / cj));
#endif
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct multiply_step
{
    explicit constexpr multiply_step(T factor) noexcept
      : factor_(factor)
    {
    }

    // FIXME : call operator of multiply_step is momentarily defined with
    //         a generic parameter to allow the host_side invoke_result<>
    //         (used in invoke()) to get the return type

    template <typename U>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T operator()(U val) const noexcept
    {
        return val * factor_;
    }

    T factor_;
};

template <typename T>
struct add_step
{
    // FIXME : call operator of add_step is momentarily defined with
    //         generic parameters to allow the host_side invoke_result<>
    //         (used in invoke()) to get the return type

    template <typename U>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T operator()(
        U val1, U val2) const noexcept
    {
        return val1 + val2;
    }
};

template <typename T>
struct triad_step
{
    explicit constexpr triad_step(T factor) noexcept
      : factor_(factor)
    {
    }

    // FIXME : call operator of triad_step is momentarily defined with
    //         generic parameters to allow the host_side invoke_result<>
    //         (used in invoke()) to get the return type

    template <typename U>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T operator()(
        U val1, U val2) const noexcept
    {
        return val1 + val2 * factor_;
    }

    T factor_;
};

///////////////////////////////////////////////////////////////////////////////
template <typename Allocator, typename Policy>
auto run_benchmark(std::size_t warmup_iterations, std::size_t iterations,
    std::size_t size, Allocator&& alloc, Policy&& policy, std::size_t executor)
{
    std::string exec_name = get_executor_name(executor);
    std::string alloc_name = get_allocator_name(executor);
    // Allocate our data
    using vector_type = hpx::compute::vector<STREAM_TYPE, Allocator>;

    vector_type a(size, alloc);
    vector_type b(size, alloc);
    vector_type c(size, alloc);

    // Initialize arrays
    hpx::fill(policy, a.begin(), a.end(), 1.0);
    hpx::fill(policy, b.begin(), b.end(), 2.0);
    hpx::fill(policy, c.begin(), c.end(), 0.0);

    ///////////////////////////////////////////////////////////////////////////
    // Warmup loop
    double scalar = 3.0;
    for (std::size_t iteration = 0; iteration != warmup_iterations; ++iteration)
    {
        // Copy
        hpx::copy(policy, a.begin(), a.end(), c.begin());

        // Scale
        hpx::transform(policy, c.begin(), c.end(), b.begin(),
            multiply_step<STREAM_TYPE>(scalar));

        // Add
        hpx::ranges::transform(policy, a.begin(), a.end(), b.begin(), b.end(),
            c.begin(), add_step<STREAM_TYPE>());

        // Triad
        hpx::ranges::transform(policy, b.begin(), b.end(), c.begin(), c.end(),
            a.begin(), triad_step<STREAM_TYPE>(scalar));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Reinitialize arrays (if needed)
    hpx::fill(policy, a.begin(), a.end(), 1.0);
    hpx::fill(policy, b.begin(), b.end(), 2.0);
    hpx::fill(policy, c.begin(), c.end(), 0.0);

    // Copy
    hpx::util::perftests_report("stream benchmark - Copy",
        exec_name + "_" + alloc_name, iterations,
        [&]() -> void { hpx::copy(policy, a.begin(), a.end(), c.begin()); });
    // Scale
    hpx::util::perftests_report("Stream benchmark - Scale",
        exec_name + "_" + alloc_name, iterations, [&]() -> void {
            hpx::transform(policy, c.begin(), c.end(), b.begin(),
                multiply_step<STREAM_TYPE>(scalar));
        });
    // Add
    hpx::util::perftests_report("Stream benchmark - Add",
        exec_name + "_" + alloc_name, iterations, [&]() -> void {
            hpx::ranges::transform(policy, a.begin(), a.end(), b.begin(),
                b.end(), c.begin(), add_step<STREAM_TYPE>());
        });
    // Triad
    hpx::util::perftests_report("Stream benchmark - Triad",
        exec_name + "_" + alloc_name, iterations, [&]() -> void {
            hpx::ranges::transform(policy, b.begin(), b.end(), c.begin(),
                c.end(), a.begin(), triad_step<STREAM_TYPE>(scalar));
        });

    // TODO: adapt the check result to work with the new version
    //// Check Results ...
    //check_results(iterations, a, b, c);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    std::size_t iterations = vm["iterations"].as<std::size_t>();
    std::size_t warmup_iterations = vm["warmup_iterations"].as<std::size_t>();
    std::size_t chunk_size = vm["chunk_size"].as<std::size_t>();
    hpx::util::perftests_init(vm, "stream_report");
    std::size_t executor;
    header = vm.count("header") > 0;

    HPX_UNUSED(chunk_size);

    if (vector_size < 1)
    {
        HPX_THROW_EXCEPTION(hpx::error::commandline_option_error, "hpx_main",
            "Invalid vector size, must be at least 1");
    }

    if (iterations < 1)
    {
        HPX_THROW_EXCEPTION(hpx::error::commandline_option_error, "hpx_main",
            "Invalid number of iterations given, must be at least 1");
    }

    {
        {
            // Default parallel policy and allocator with default parallel policy.
            executor = 2;
            auto policy = hpx::execution::par;
            hpx::compute::host::detail::policy_allocator<STREAM_TYPE,
                decltype(policy)>
                alloc(policy);

            run_benchmark<>(warmup_iterations, iterations, vector_size,
                std::move(alloc), std::move(policy), executor);
        }

        {
            // Fork-join executor and allocator with fork-join executor.
            executor = 3;
            using executor_type =
                hpx::execution::experimental::fork_join_executor;
            executor_type exec;
            auto policy = hpx::execution::par.on(exec);
            hpx::compute::host::detail::policy_allocator<STREAM_TYPE,
                decltype(policy)>
                alloc(policy);

            run_benchmark<>(warmup_iterations, iterations, vector_size,
                std::move(alloc), std::move(policy), executor);
        }

        {
            // thread_pool_scheduler used through a scheduler_executor.
            executor = 4;
            using executor_type =
                hpx::execution::experimental::scheduler_executor<
                    hpx::execution::experimental::thread_pool_scheduler>;
            executor_type exec;
            auto policy = hpx::execution::par.on(exec);
            hpx::compute::host::detail::policy_allocator<STREAM_TYPE,
                decltype(policy)>
                alloc(policy);

            run_benchmark<>(warmup_iterations, iterations, vector_size,
                std::move(alloc), std::move(policy), executor);
        }
    }

    hpx::util::perftests_print_times();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        (   "csv", "output results as csv")
        (   "header", "print header for csv results")
        (   "vector_size",
            hpx::program_options::value<std::size_t>()->default_value(1024),
            "size of vector (default: 1024)")
        (   "iterations",
            hpx::program_options::value<std::size_t>()->default_value(10),
            "number of iterations to repeat each test. (default: 10)")
        (   "warmup_iterations",
            hpx::program_options::value<std::size_t>()->default_value(1),
            "number of warmup iterations to perform before timing. (default: 1)")
        (   "chunk_size",
             hpx::program_options::value<std::size_t>()->default_value(0),
            "size of vector (default: 1024)")
        ;
    // clang-format on

    // parse command line here to extract the necessary settings for HPX
    parsed_options opts = command_line_parser(argc, argv)
                              .allow_unregistered()
                              .options(cmdline)
                              .style(command_line_style::unix_style)
                              .run();

    variables_map vm;
    store(opts, vm);

    std::vector<std::string> cfg = {
        "hpx.numa_sensitive=2"    // no-cross NUMA stealing
    };

    hpx::util::perftests_cfg(cmdline);
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
