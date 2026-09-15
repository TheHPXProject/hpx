//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reflect_action_overhead.cpp
/// \brief Runtime dispatch overhead: HPX_PLAIN_ACTION vs reflect_action.
///
/// Measures per-dispatch latency of:
///   1. Traditional make_action_t-based dispatch (baseline)
///   2. C++26 reflection-based hpx::async<^^func> dispatch
///
/// Both paths use the same template helper to avoid code drift.
/// Reports min, median, and stddev over --nruns timed passes.
///
/// Usage:
///   ./reflect_action_overhead_test --nparcels=1000 --nwarmup=100 --nruns=10

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/timing.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace bench {

    /// Simple function dispatched remotely in both benchmarks.
    /// Returns a value that depends on the argument to prevent
    /// dead-code elimination.
    std::uint64_t identity(std::uint64_t n)
    {
        return n + 1;
    }

}    // namespace bench

///////////////////////////////////////////////////////////////////////////////
// Baseline: explicit action type defined without reflection macros.
// When HPX_HAVE_CXX26_REFLECTION is enabled, HPX_PLAIN_ACTION itself
// expands to reflect_action<^^func>. We use make_action_t directly
// to ensure a genuine comparison between the two dispatch paths.
#if defined(HPX_HAVE_CXX26_REFLECTION)
struct bench_identity_action
  : hpx::actions::make_action_t<decltype(&bench::identity), &bench::identity,
        bench_identity_action>
{
};
#else
HPX_PLAIN_ACTION(bench::identity, bench_identity_action)
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief Compute median of a sorted vector.
static double median(std::vector<double>& v)
{
    std::size_t const n = v.size();
    std::sort(v.begin(), v.end());
    if (n % 2 == 0)
        return (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return v[n / 2];
}

/// \brief Compute population standard deviation.
static double stddev(std::vector<double> const& v, double mean)
{
    double sum = 0.0;
    for (double x : v)
        sum += (x - mean) * (x - mean);
    return std::sqrt(sum / double(v.size()));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Template benchmark helper -- shared by both dispatch paths.
///
/// \tparam Dispatcher  Callable: (target, nparcels) ->
///                      vector<future<uint64_t>>
///
/// \param label      Label printed in output.
/// \param target     Remote locality to dispatch to.
/// \param nparcels   Number of dispatches per timed run.
/// \param nwarmup    Number of warmup dispatches before timing.
/// \param nruns      Number of timed repetitions for statistics.
/// \param warmup_fn  Callable: (target, i) -> future<uint64_t> for warmup.
/// \param dispatch   The dispatcher callable.
template <typename WarmupFn, typename DispatchFn>
static void run_benchmark(char const* label, hpx::id_type const& target,
    std::size_t nparcels, std::size_t nwarmup, std::size_t nruns,
    WarmupFn warmup_fn, DispatchFn dispatch_fn)
{
    // Warmup
    for (std::size_t i = 0; i < nwarmup; ++i)
        warmup_fn(target, std::uint64_t(i)).get();

    // Timed runs
    std::vector<double> times_us;
    times_us.reserve(nruns);

    for (std::size_t run = 0; run < nruns; ++run)
    {
        std::vector<hpx::future<std::uint64_t>> futures;
        futures.reserve(nparcels);

        hpx::chrono::high_resolution_timer t;
        for (std::size_t i = 0; i < nparcels; ++i)
            futures.push_back(dispatch_fn(target, std::uint64_t(i)));
        for (auto& f : hpx::when_all(futures).get())
            f.get();

        double const elapsed_us = t.elapsed() * 1e6;
        times_us.push_back(elapsed_us / double(nparcels));
    }

    // Statistics
    double const med = median(times_us);
    double const mn = *std::min_element(times_us.begin(), times_us.end());
    double const mean =
        std::accumulate(times_us.begin(), times_us.end(), 0.0) / double(nruns);
    double const sd = stddev(times_us, mean);

    std::cout << label << nparcels << " dispatches x " << nruns
              << " runs:  min=" << mn << " us  median=" << med
              << " us  stddev=" << sd << " us\n"
              << std::flush;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const nparcels = vm["nparcels"].as<std::size_t>();
    std::size_t const nwarmup = vm["nwarmup"].as<std::size_t>();
    std::size_t const nruns = vm["nruns"].as<std::size_t>();

    if (nparcels == 0)
    {
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "reflect_action_overhead", "--nparcels must be greater than zero");
    }
    if (nruns == 0)
    {
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "reflect_action_overhead", "--nruns must be greater than zero");
    }

    std::vector<hpx::id_type> remote = hpx::find_remote_localities();
    if (remote.empty())
    {
        std::cout << "No remote localities found. "
                     "Run with at least 2 localities "
                     "(--hpx:localities=2).\n"
                  << std::flush;
        return hpx::finalize();
    }

    hpx::id_type const target = remote[0];

    std::cout << "\nRuntime dispatch overhead: "
                 "make_action_t vs C++26 reflect_action\n"
              << "Target locality: " << target << "\n"
              << "Parcels: " << nparcels << "  Warmup: " << nwarmup
              << "  Runs: " << nruns << "\n"
              << std::string(60, '-') << "\n"
              << std::flush;

    // Baseline: make_action_t dispatch
    bench_identity_action act;
    run_benchmark(
        "[make_action_t   ] ", target, nparcels, nwarmup, nruns,
        [&act](hpx::id_type const& t, std::uint64_t i) {
            return hpx::async(act, t, i);
        },
        [&act](hpx::id_type const& t, std::uint64_t i) {
            return hpx::async(act, t, i);
        });

#if defined(HPX_HAVE_CXX26_REFLECTION)
    // Reflection: hpx::async<^^func> dispatch
    run_benchmark(
        "[reflect_action  ] ", target, nparcels, nwarmup, nruns,
        [](hpx::id_type const& t, std::uint64_t i) {
            return hpx::async<^^bench::identity>(t, i);
        },
        [](hpx::id_type const& t, std::uint64_t i) {
            return hpx::async<^^bench::identity>(t, i);
        });

    std::cout << std::string(60, '-') << "\n" << std::flush;
#else
    std::cout << "[reflect_action] skipped -- "
                 "HPX_HAVE_CXX26_REFLECTION not defined.\n"
              << std::flush;
#endif

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    hpx::program_options::options_description cmdline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("nparcels,n",
            hpx::program_options::value<std::size_t>()->default_value(1000),
            "number of remote dispatches per timed run")
        ("nwarmup,w",
            hpx::program_options::value<std::size_t>()->default_value(100),
            "number of warmup dispatches before timing")
        ("nruns,r",
            hpx::program_options::value<std::size_t>()->default_value(10),
            "number of timed repetitions for statistics");
    // clang-format on

    std::vector<std::string> cfg;
    cfg.push_back("hpx.run_hpx_main!=1");

    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

#endif    // !HPX_COMPUTE_DEVICE_CODE
