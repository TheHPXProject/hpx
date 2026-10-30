//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file task_size_chunk_size.hpp
/// \page hpx::execution::experimental::task_size_chunk_size
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/timing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <type_traits>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    /// Loop iterations are divided into chunks based on the task size of the
    /// loop body (see \a hpx::task_size). The run time a size stands for (see
    /// \a hpx::task_size_duration) is taken as the run time of one iteration,
    /// nothing is measured. The loop runs on as many cores as can be given
    /// \a chunks_per_core chunks of at least \a min_chunk_time each, at least
    /// on one, and then on the calling thread if it is only one chunk.
    ///
    /// A loop body without a size is chunked as with the default parameters.
    ///
    HPX_CXX_CORE_EXPORT struct task_size_chunk_size
    {
    public:
        /// Construct a \a task_size_chunk_size executor parameters object
        ///
        /// \param min_chunk_time  [in] The minimal run time of a chunk.
        /// \param chunks_per_core [in] The number of chunks each core should
        ///                        get for load balancing.
        ///
        /// \note Default constructed \a task_size_chunk_size executor
        ///       parameters use 5 microseconds as the minimal run time of a
        ///       chunk and 4 chunks per core.
        ///
        explicit task_size_chunk_size(
            hpx::chrono::steady_duration const& min_chunk_time =
                std::chrono::microseconds(5),
            std::size_t const chunks_per_core = 4) noexcept
          : min_chunk_time_(min_chunk_time.value())
          , chunks_per_core_(chunks_per_core == 0 ? 1 : chunks_per_core)
        {
        }

        /// \cond NOINTERNAL
        // This executor parameters type takes the run time of one iteration
        // from the task size of the loop body.
        using uses_task_size = std::true_type;

        // The run time of one iteration as given by the size of the loop body,
        // f is not invoked. null_duration if the size is not known.
        template <typename Executor, typename F>
        static hpx::chrono::steady_duration measure_iteration(
            Executor&&, F&& f, std::size_t) noexcept
        {
            auto const size =
                hpx::traits::get_task_size<std::decay_t<F>>::call(f);
            if (size == hpx::threads::task_size::unknown)
            {
                return hpx::chrono::null_duration;
            }
            return hpx::threads::task_size_duration(size);
        }

        // As many cores as can be given chunks_per_core chunks of at least
        // min_chunk_time each, at least one.
        template <typename Executor>
        std::size_t processing_units_count(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const count) const noexcept
        {
            std::size_t const available_pus =
                hpx::execution::experimental::processing_units_count(
                    exec, iteration_duration, count);

            auto const per_iteration = iteration_duration.value();
            if (per_iteration == per_iteration.zero())
            {
                return available_pus;
            }

            // the iterations one core needs, computed without multiplying by
            // count, which could overflow
            auto const per_core =
                min_chunk_time_ * static_cast<rep>(chunks_per_core_);
            auto const iterations_per_core =
                static_cast<std::size_t>(per_core / per_iteration);
            std::size_t const cores =
                iterations_per_core == 0 ? count : count / iterations_per_core;
            return std::clamp(cores, std::size_t(1),
                (std::max) (available_pus, std::size_t(1)));
        }

        // chunks_per_core chunks per core, none shorter than min_chunk_time.
        template <typename Executor>
        std::size_t get_chunk_size(Executor&&,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const cores, std::size_t const count) const noexcept
        {
            auto const per_iteration = iteration_duration.value();
            if (per_iteration == per_iteration.zero() || count == 0)
            {
                return 0;    // use the default
            }

            // a single core runs the whole loop as one chunk, on the calling
            // thread
            if (cores <= 1)
            {
                return count;
            }

            std::size_t const num_chunks =
                chunks_per_core_ * (std::max) (cores, std::size_t(1));
            std::size_t const balanced = (count + num_chunks - 1) / num_chunks;

            // iterations that run for at least min_chunk_time, rounding up
            auto const min_iterations = static_cast<std::size_t>(
                (min_chunk_time_ + per_iteration - duration(1)) /
                per_iteration);

            return (std::min) (count, (std::max) (balanced, min_iterations));
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        using duration = hpx::chrono::steady_clock::duration;
        using rep = duration::rep;

        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & min_chunk_time_ & chunks_per_core_;
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        duration min_chunk_time_;
        std::size_t chunks_per_core_;
        /// \endcond
    };

    /// \cond NOINTERNAL
    template <>
    struct is_executor_parameters<
        hpx::execution::experimental::task_size_chunk_size> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
