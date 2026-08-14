//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/experimental/sandbox.hpp
/// \page hpx::experimental::sandbox
/// \headerfile hpx/experimental/sandbox.hpp
///
/// \brief HPX Sandbox Laboratory -- environment introspection, parallel
///        benchmarking, and scaling telemetry for single-node environments.
///
/// This header provides a lightweight, header-only toolkit for
/// prototyping and benchmarking HPX parallel code in resource-constrained
/// environments such as Compiler Explorer (Godbolt). It includes:
///
///   - **Environment introspection** via hwloc topology queries
///   - **Automatic sandbox detection** for container / CI environments
///   - **Comparative benchmarking** (sequential vs. parallel) with
///     speedup and efficiency metrics
///   - **Console-friendly telemetry** formatted for fixed-width output
///
/// All functions must be called from within a running HPX runtime
/// (e.g. from an HPX thread).

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iosfwd>
#include <string>
#include <utility>

namespace hpx::experimental::sandbox {

    // --- Data Structures ---

    /// \brief Describes the hardware and runtime environment.
    struct environment_info
    {
        std::size_t cores = 0;          ///< Physical CPU cores
        std::size_t pus = 0;            ///< Processing units (hardware threads)
        std::size_t numa_nodes = 0;     ///< NUMA domains
        std::size_t hpx_workers = 0;    ///< Active HPX worker threads
        bool is_sandbox = false;        ///< Likely constrained environment

        /// \brief Print a formatted environment report.
        ///
        /// \param os  Output stream.
        HPX_EXPORT void print(std::ostream& os) const;
    };

    /// \brief Holds the results of a comparative benchmark.
    struct benchmark_report
    {
        std::string label;              ///< User-provided description
        std::size_t iterations = 0;     ///< Measurement iterations
        double seq_mean_ms = 0.0;       ///< Sequential mean time (ms)
        double par_mean_ms = 0.0;       ///< Parallel mean time (ms)
        double speedup = 0.0;           ///< seq_time / par_time
        double efficiency_pct = 0.0;    ///< (speedup / workers) * 100
        std::size_t num_workers = 0;    ///< HPX workers during measurement

        /// \brief Print a formatted benchmark report.
        ///
        /// \param os  Output stream.
        HPX_EXPORT void print(std::ostream& os) const;
    };

    // --- Environment Detection ---
    namespace detail {

        template <typename F>
        double time_once_ms(F&& fn)
        {
            auto const start = std::chrono::high_resolution_clock::now();
            fn();
            auto const end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double, std::milli>(end - start)
                .count();
        }

        template <typename F>
        double mean_ms(F&& fn, std::size_t iterations)
        {
            fn();    // warmup

            double total = 0.0;
            for (std::size_t i = 0; i < iterations; ++i)
            {
                total += time_once_ms(fn);
            }
            return total / static_cast<double>(iterations);
        }
    }    // namespace detail

    // --- Public API ---

    /// \brief Detect and return information about the current environment.
    HPX_EXPORT environment_info detect_environment();

    /// \brief Print a formatted environment report.
    inline void describe_environment(std::ostream& os)
    {
        detect_environment().print(os);
    }

    /// \brief Measure the mean execution time of a callable.
    template <typename F>
    double measure(F&& fn, std::size_t iterations = 5)
    {
        return detail::mean_ms(HPX_FORWARD(F, fn), iterations);
    }

    /// \brief Compare sequential and parallel execution of the same work.
    template <typename SeqFn, typename ParFn>
    benchmark_report benchmark(std::string label, SeqFn&& seq_fn,
        ParFn&& par_fn, std::size_t iterations = 5)
    {
        benchmark_report report;
        report.label = HPX_MOVE(label);
        report.iterations = iterations;
        report.num_workers = hpx::get_num_worker_threads();
        report.seq_mean_ms =
            detail::mean_ms(HPX_FORWARD(SeqFn, seq_fn), iterations);
        report.par_mean_ms =
            detail::mean_ms(HPX_FORWARD(ParFn, par_fn), iterations);
        if (report.par_mean_ms > 0.0)
        {
            report.speedup = report.seq_mean_ms / report.par_mean_ms;
        }
        if (report.num_workers > 0)
        {
            report.efficiency_pct =
                (report.speedup / static_cast<double>(report.num_workers)) *
                100.0;
        }
        return report;
    }
}    // namespace hpx::experimental::sandbox
