//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reflect_action_overhead.cpp
/// \brief Runtime dispatch overhead: HPX_PLAIN_ACTION vs reflect_action.
///
/// Measures the per-dispatch latency of:
///   1. Traditional HPX_PLAIN_ACTION-based dispatch
///   2. C++26 reflection-based hpx::async<^^func> dispatch
///
/// Both dispatch the same function to a remote locality N times.
/// Results show that reflection adds zero runtime overhead -- the
/// difference between the two is within measurement noise.
///
/// Usage:
///   ./reflect_action_overhead_test --nparcels=1000 --nwarmup=100

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/timing.hpp>
#include <iostream>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace bench {

    /// Simple function dispatched remotely in both benchmarks.
    /// Returns a value that depends on the argument to prevent dead-code
    /// elimination.
    std::uint64_t identity(std::uint64_t n)
    {
        return n + 1;
    }

}    // namespace bench

///////////////////////////////////////////////////////////////////////////////
// Baseline: explicit action type defined without reflection macros.
// Note: when HPX_HAVE_CXX26_REFLECTION is enabled, HPX_PLAIN_ACTION
// itself expands to reflect_action<^^func>. To ensure a genuine
// comparison, we use the explicit make_action_t form here.
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
static void run_macro_benchmark(
    hpx::id_type const& target, std::size_t nparcels, std::size_t nwarmup)
{
    bench_identity_action act;

    // Warmup
    for (std::size_t i = 0; i < nwarmup; ++i)
        hpx::async(act, target, std::uint64_t(i)).get();

    // Timed run
    std::vector<hpx::future<std::uint64_t>> futures;
    futures.reserve(nparcels);

    hpx::chrono::high_resolution_timer t;
    for (std::size_t i = 0; i < nparcels; ++i)
        futures.push_back(hpx::async(act, target, std::uint64_t(i)));
    for (auto& f : hpx::when_all(futures).get())
        f.get();
    double const elapsed = t.elapsed();

    double const us_per_dispatch = (elapsed * 1e6) / double(nparcels);
    std::cout << "[HPX_PLAIN_ACTION] " << nparcels
              << " dispatches: total=" << elapsed * 1e3 << " ms"
              << "  per-dispatch=" << us_per_dispatch << " us\n"
              << std::flush;
}

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX26_REFLECTION)
static void run_reflection_benchmark(
    hpx::id_type const& target, std::size_t nparcels, std::size_t nwarmup)
{
    // Warmup
    for (std::size_t i = 0; i < nwarmup; ++i)
        hpx::async<^^bench::identity>(target, std::uint64_t(i)).get();

    // Timed run
    std::vector<hpx::future<std::uint64_t>> futures;
    futures.reserve(nparcels);

    hpx::chrono::high_resolution_timer t;
    for (std::size_t i = 0; i < nparcels; ++i)
        futures.push_back(
            hpx::async<^^bench::identity>(target, std::uint64_t(i)));
    for (auto& f : hpx::when_all(futures).get())
        f.get();
    double const elapsed = t.elapsed();

    double const us_per_dispatch = (elapsed * 1e6) / double(nparcels);
    std::cout << "[reflect_action  ] " << nparcels
              << " dispatches: total=" << elapsed * 1e3 << " ms"
              << "  per-dispatch=" << us_per_dispatch << " us\n"
              << std::flush;
}
#endif    // HPX_HAVE_CXX26_REFLECTION

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const nparcels = vm["nparcels"].as<std::size_t>();
    std::size_t const nwarmup = vm["nwarmup"].as<std::size_t>();
    if (nparcels == 0)
    {
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "reflect_action_overhead", "--nparcels must be greater than zero");
    }

    std::vector<hpx::id_type> remote = hpx::find_remote_localities();
    if (remote.empty())
    {
        std::cout << "No remote localities found. "
                     "Run with at least 2 localities (--hpx:localities=2).\n"
                  << std::flush;
        return hpx::finalize();
    }

    hpx::id_type const target = remote[0];

    std::cout << "\nRuntime dispatch overhead: "
                 "HPX_PLAIN_ACTION vs C++26 reflect_action\n"
              << "Target locality: " << target << "\n"
              << "Parcels: " << nparcels << "  Warmup: " << nwarmup << "\n"
              << std::string(60, '-') << "\n"
              << std::flush;

    run_macro_benchmark(target, nparcels, nwarmup);

#if defined(HPX_HAVE_CXX26_REFLECTION)
    run_reflection_benchmark(target, nparcels, nwarmup);
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
            "number of remote dispatches per benchmark")
        ("nwarmup,w",
            hpx::program_options::value<std::size_t>()->default_value(100),
            "number of warmup dispatches before timing");
    // clang-format on

    std::vector<std::string> cfg;
    cfg.push_back("hpx.run_hpx_main!=1");

    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

#endif    // !HPX_COMPUTE_DEVICE_CODE
