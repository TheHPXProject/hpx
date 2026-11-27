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
    /// for one number of indices: one parallel loop (for_loop) with a
    /// trivial body and a chunk per index, loops back to back, in
    /// microseconds per loop. The loop is what task_size_chunk_size chunks,
    /// so its time includes what the algorithm adds to the executor's
    /// region. cores is the number of workers the loop runs on: the pool
    /// size, or fewer for a loop that task_size_chunk_size runs on fewer
    /// cores (the algorithm narrows the executor with
    /// with_processing_units_count; an executor without it runs such a
    /// loop's indices on all its workers).
    HPX_CXX_CORE_EXPORT struct region_time
    {
        std::string executor;
        std::size_t pool_size = 0;
        std::size_t cores = 0;
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
        /// the numbers of indices, as multiples of the region's workers
        std::vector<std::size_t> multiples = {1, 4, 16};
        /// the numbers of workers of the narrow regions, besides those on
        /// the whole pool; a number not below the pool size is left out
        std::vector<std::size_t> narrow_cores = {1, 2};
    };

    /// Measure the region times of the executors of chunk_executors() on all
    /// worker threads of the running runtime's default pool, and on the
    /// narrow numbers of them. Must be called on an HPX thread.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<region_time>
    measure_region_times(chunk_cost_options const& options = {});

    /// A region time as one line of text, and back (for a child process that
    /// measures and its parent): "<executor> <pool size> <cores> <indices>
    /// <count> <min> <q1> <median> <q3> <p90> <max>". from_line returns
    /// nothing for a line that isn't one.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_line(
        region_time const& time);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::optional<region_time> from_line(
        std::string const& line);

    /// The region times of several runs (children of the tool, each with a
    /// runtime of its own) as one per executor, pool size and number of
    /// indices: the run's time whose median is the median of the runs'
    /// medians (the lower one of the two in the middle for an even number
    /// of runs), for each number of workers. The runs differ more than the
    /// samples of one run do, so
    /// that one slow or fast run doesn't move the result. The times come
    /// in the order of their first appearance.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<region_time> median_of_runs(
        std::vector<region_time> const& times);

    /// The costs of a region of one executor on a pool of workers, for the
    /// region's number of workers (cores): a region of c chunks on that many
    /// workers takes about region_us + (c / cores) * chunk_us beyond its
    /// work, fitted by least squares to the medians of its region times.
    /// residual_us is the largest distance of a median from the fit,
    /// time_at_pool_size_us the median for as many indices as workers.
    HPX_CXX_CORE_EXPORT struct chunk_cost
    {
        std::string executor;
        std::size_t pool_size = 0;
        std::size_t cores = 0;
        double region_us = 0.0;
        double chunk_us = 0.0;
        double residual_us = 0.0;
        double time_at_pool_size_us = 0.0;
        /// the largest (q3 - q1) / median of the region times
        double spread = 0.0;
    };

    /// Fit the costs of each executor, pool size and number of workers of
    /// the given region times (those with fewer than two numbers of indices
    /// are left out).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<chunk_cost> fit_chunk_costs(
        std::vector<region_time> const& times);

    /// How the minimal run time of a chunk follows from the costs.
    ///
    /// The minimal run time matters only for loops too small to give every
    /// core chunks_per_core chunks of that run time: task_size_chunk_size
    /// then runs them on fewer cores, each with chunks_per_core chunks. Such
    /// a loop costs a worker region_us + chunks_per_core * chunk_us besides
    /// its chunks, chunk_us + region_us / chunks_per_core for each chunk.
    /// An executor's chunks may be as much shorter than parallel_executor's
    /// as this cost is smaller, for the same share of the time.
    ///
    /// Both rules have a floor: a loop that task_size_chunk_size moves from
    /// one core (the calling thread, without a region) to two runs half of
    /// its work on each, and pays the region on two workers and its chunks.
    /// That pays only if the minimal run time is at least chunk_us +
    /// region_us / chunks_per_core of the region on two workers, so an
    /// executor's value is never below it.
    HPX_CXX_CORE_EXPORT enum class chunk_rule {
        /// f * (chunk_us + region_us / chunks_per_core), with f such that
        /// parallel_executor gets the default
        fitted,
        /// default * time_at_pool_size_us / that of parallel_executor (the
        /// region times for one chunk per worker, without the fit)
        ratio
    };

    /// The name of a rule ("fitted", "ratio"), and the rule of a name.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* to_string(
        chunk_rule rule) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::optional<chunk_rule>
    chunk_rule_from_string(std::string const& name) noexcept;

    /// The sections with the minimal run time of a chunk for each executor
    /// and pool size of the given costs (those on the whole pool), by the
    /// given rule and with the floor of the costs on two workers, relative
    /// to the
    /// default minimal run time (in microseconds) that parallel_executor
    /// keeps: hpx.task_size_chunk_size.<executor>.<pool size> for each pool
    /// size, and hpx.task_size_chunk_size.<executor> with the value of the
    /// largest pool size, for the pool sizes that weren't measured. A pool
    /// size without parallel_executor's costs is left out. The settings of
    /// parallel_executor are marked as the default. chunks_per_core is that
    /// of task_size_chunk_size (hpx.task_size_chunk_size.chunks_per_core);
    /// 0 is taken as 1.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<ini_section> chunk_sections(
        std::vector<chunk_cost> const& costs, double default_min_chunk_time_us,
        chunk_rule rule, std::size_t chunks_per_core);
}    // namespace hpx::tuning
