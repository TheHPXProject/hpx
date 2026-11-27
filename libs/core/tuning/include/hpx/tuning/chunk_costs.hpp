//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file chunk_costs.hpp
/// The costs of a parallel region of the executors, and the minimal run time
/// of a chunk that task_size_chunk_size uses for their loops
/// (hpx.task_size_chunk_size.<executor>[.<pool size>].min_chunk_time_us, see
/// hpx::execution::experimental::get_min_chunk_time).

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/statistics.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace hpx::tuning {

    /// The executors whose regions are measured, by the names under which
    /// their minimal chunk time is configured: parallel_executor,
    /// fork_join_executor, suspending_fork_join_executor and
    /// shared_suspending_fork_join_executor.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<std::string>
    chunk_executors();

    /// The time of a parallel region of one executor on a pool of workers,
    /// for one number of indices: one bulk_sync_execute with a trivial body,
    /// regions back to back, in microseconds per region.
    HPX_CXX_CORE_EXPORT struct region_time
    {
        std::string executor;
        std::size_t pool_size = 0;
        std::size_t indices = 0;
        summary time_us;
    };

    /// How the regions are measured.
    HPX_CXX_CORE_EXPORT struct chunk_cost_options
    {
        /// the regions timed together for one sample
        std::size_t regions = 2000;
        /// the samples of each executor and number of indices
        std::size_t repeats = 21;
        /// the numbers of indices, as multiples of the pool size
        std::vector<std::size_t> multiples = {1, 4, 16};
    };

    /// Measure the region times of the executors of chunk_executors() on all
    /// worker threads of the running runtime's default pool. Each executor
    /// and number of indices first runs one sample's regions untimed, to
    /// warm up. Must be called on an HPX thread.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<region_time>
    measure_region_times(chunk_cost_options const& options = {});

    /// A region time as one line of text, and back (for a child process that
    /// measures and its parent): "<executor> <pool size> <indices> <count>
    /// <min> <q1> <median> <q3> <p90> <max>". from_line returns nothing for
    /// a line that isn't one.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_line(
        region_time const& time);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::optional<region_time> from_line(
        std::string const& line);

    /// The costs of a region of one executor on a pool of workers: a region
    /// of c chunks on n workers takes about region_us + (c / n) * chunk_us
    /// beyond its work, fitted by least squares to the medians of its region
    /// times. residual_us is the largest distance of a median from the fit,
    /// time_at_pool_size_us the median for as many indices as workers.
    HPX_CXX_CORE_EXPORT struct chunk_cost
    {
        std::string executor;
        std::size_t pool_size = 0;
        double region_us = 0.0;
        double chunk_us = 0.0;
        double residual_us = 0.0;
        double time_at_pool_size_us = 0.0;
        /// the largest (q3 - q1) / median of the region times
        double spread = 0.0;
    };

    /// Fit the costs of each executor and pool size of the given region
    /// times (those with fewer than two numbers of indices are left out).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<chunk_cost> fit_chunk_costs(
        std::vector<region_time> const& times);

    /// How the minimal run time of a chunk follows from the costs.
    HPX_CXX_CORE_EXPORT enum class chunk_rule {
        /// f * (chunk_us + region_us / n), with f such that
        /// parallel_executor gets the default
        fitted,
        /// default * time_at_pool_size_us / that of parallel_executor
        ratio
    };

    /// The name of a rule ("fitted", "ratio"), and the rule of a name.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* to_string(
        chunk_rule rule) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::optional<chunk_rule>
    chunk_rule_from_string(std::string const& name) noexcept;

    /// The sections with the minimal run time of a chunk for each executor
    /// and pool size of the given costs, by the given rule, relative to the
    /// default minimal run time (in microseconds) that parallel_executor
    /// keeps: hpx.task_size_chunk_size.<executor>.<pool size> for each pool
    /// size, and hpx.task_size_chunk_size.<executor> with the value of the
    /// largest pool size, for the pool sizes that weren't measured. A pool
    /// size without parallel_executor's costs is left out. The settings of
    /// parallel_executor are marked as the default.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<ini_section> chunk_sections(
        std::vector<chunk_cost> const& costs, double default_min_chunk_time_us,
        chunk_rule rule);
}    // namespace hpx::tuning
