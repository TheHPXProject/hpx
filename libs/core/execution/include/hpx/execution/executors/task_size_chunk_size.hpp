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
#include <hpx/execution/detail/task_size_chunk_size_defaults.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution/executors/get_min_chunk_time.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/timing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

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
        /// with the default minimal run time of a chunk and the default
        /// number of chunks per core.
        ///
        /// \note The defaults are 2.5 microseconds as the minimal run time of
        ///       a chunk and 8 chunks per core. They can be changed at startup
        ///       with the configuration entries
        ///       \a hpx.task_size_chunk_size.min_chunk_time_us (in
        ///       microseconds) and \a hpx.task_size_chunk_size.chunks_per_core.
        ///       The defaults are read when a loop is chunked, so an object
        ///       created before the runtime has started (for instance as part
        ///       of a global execution policy) uses the configured ones. An
        ///       executor can ask for a minimal run time of its own (see
        ///       \a hpx::execution::experimental::get_min_chunk_time), which
        ///       replaces the default one for its loops.
        ///
        constexpr task_size_chunk_size() noexcept = default;

        /// Construct a \a task_size_chunk_size executor parameters object
        ///
        /// \param min_chunk_time  [in] The minimal run time of a chunk.
        /// \param chunks_per_core [in] The number of chunks each core should
        ///                        get for load balancing, 0 stands for the
        ///                        default number of chunks per core.
        ///
        explicit task_size_chunk_size(
            hpx::chrono::steady_duration const& min_chunk_time,
            std::size_t const chunks_per_core = 0) noexcept
          : min_chunk_time_(min_chunk_time.value())
          , chunks_per_core_(chunks_per_core)
        {
        }

        /// \cond NOINTERNAL
        // This executor parameters type takes the run time of one iteration
        // from the task size of the loop body.
        using uses_task_size = std::true_type;

        // The run time of one iteration as given by the loop body: from the
        // bytes it reads and writes if it gives them (the library's
        // operations on builtin elements) at the rate for the loop's
        // footprint per core, else its time if it gives one (see
        // hpx::timed_task), else the time its size stands for; f is not
        // invoked. null_duration if none is known. Not noexcept: the
        // executor's processing_units_count throws if the runtime isn't
        // running.
        template <typename Executor, typename F>
        hpx::chrono::steady_duration measure_iteration(
            Executor&& exec, F&& f, std::size_t const count) const
        {
            if (std::size_t const bytes =
                    hpx::traits::get_task_bytes<std::decay_t<F>>::call(f);
                bytes != 0 && count != 0)
            {
                // the time of an element if each core touches the given
                // footprint
                auto const time_for = [bytes](std::size_t const footprint) {
                    std::size_t const rate =
                        detail::get_copy_bytes_per_ns(footprint);
                    return hpx::chrono::picoseconds(
                        static_cast<hpx::chrono::picoseconds::rep>(
                            (bytes * 1000 + rate) / (2 * rate)));
                };

                // the whole loop on one core decides whether it runs on more
                // than one, each core's share of it how fast it runs there
                hpx::chrono::picoseconds time = time_for(count * bytes);
                std::size_t cores = processing_units_count(exec, time, count);
                if (cores > 1)
                {
                    time = time_for(count * bytes / cores);
                }

                // The algorithm gives the loop the cores this time gives
                // (processing_units_count). A faster time gives fewer cores,
                // each with a larger share, which may not fit the level the
                // time was taken for: take the time of the larger share until
                // the cores don't drop any more. The loop then runs on at
                // least the cores its time was taken for, so the time is
                // never too short. The cores drop on every round, so this
                // ends.
                while (cores > 1)
                {
                    std::size_t const next =
                        processing_units_count(exec, time, count);
                    if (next >= cores)
                    {
                        break;
                    }
                    cores = next;
                    time = time_for(count * bytes / cores);
                }
                return time;
            }
            if (auto const time =
                    hpx::traits::get_task_time<std::decay_t<F>>::call(f);
                time.count() > 0)
            {
                return time;
            }
            auto const size =
                hpx::traits::get_task_size<std::decay_t<F>>::call(f);
            if (size == hpx::threads::task_size::unknown)
            {
                return hpx::chrono::null_duration;
            }
            return hpx::threads::task_size_duration(size);
        }

        // As many cores as can be given chunks_per_core chunks of at least
        // min_chunk_time each, at least one. Not noexcept: the executor's
        // processing_units_count throws if the runtime isn't running.
        template <typename Executor>
        std::size_t processing_units_count(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const count) const
        {
            std::size_t const available_pus =
                hpx::execution::experimental::processing_units_count(
                    exec, iteration_duration, count);

            if (!iteration_duration)
            {
                return available_pus;
            }
            // in picoseconds: an iteration may run for less than a
            // nanosecond
            auto const per_iteration = iteration_duration.as_picoseconds();

            // the iterations one core needs, computed without multiplying by
            // count, which could overflow
            auto const [min_chunk_time, chunks_per_core] = get_values(exec);
            std::size_t const iterations_per_core = scaled_quotient(
                std::chrono::duration_cast<hpx::chrono::picoseconds>(
                    min_chunk_time)
                    .count(),
                static_cast<ps_rep>(chunks_per_core), per_iteration.count());
            std::size_t const cores =
                iterations_per_core == 0 ? count : count / iterations_per_core;
            return std::clamp(cores, std::size_t(1),
                (std::max) (available_pus, std::size_t(1)));
        }

        // chunks_per_core chunks per core, none shorter than min_chunk_time.
        template <typename Executor>
        std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const cores, std::size_t const count) const noexcept
        {
            if (!iteration_duration || count == 0)
            {
                return 0;    // use the default
            }
            auto const per_iteration = iteration_duration.as_picoseconds();

            // a single core runs the whole loop as one chunk, on the calling
            // thread
            if (cores <= 1)
            {
                return count;
            }

            auto const [min_chunk_time, chunks_per_core] = get_values(exec);
            std::size_t const num_chunks =
                chunks_per_core * (std::max) (cores, std::size_t(1));
            std::size_t const balanced = (count + num_chunks - 1) / num_chunks;

            // iterations that run for at least min_chunk_time, rounding up
            // (without adding to the times, which can be the largest number
            // of picoseconds)
            auto const min_time =
                std::chrono::duration_cast<hpx::chrono::picoseconds>(
                    min_chunk_time);
            auto const min_iterations =
                static_cast<std::size_t>(min_time / per_iteration) +
                (min_time % per_iteration != hpx::chrono::picoseconds(0) ? 1 :
                                                                           0);

            return (std::min) (count, (std::max) (balanced, min_iterations));
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        using duration = hpx::chrono::steady_clock::duration;
        using rep = duration::rep;
        using ps_rep = hpx::chrono::picoseconds::rep;

        // total * factor / part, rounded down, for a total and a factor of at
        // least zero and a positive part, without forming total * factor,
        // which can exceed the range of picoseconds (a minimal chunk time of
        // hours and 1024 chunks per core); saturated at the largest
        // std::size_t
        static constexpr std::size_t scaled_quotient(
            ps_rep const total, ps_rep const factor, ps_rep const part) noexcept
        {
            constexpr ps_rep max = (std::numeric_limits<ps_rep>::max)();
            if (part <= 0)
            {
                return (std::numeric_limits<std::size_t>::max)();
            }
            ps_rep const quotient = total / part;
            ps_rep const remainder = total % part;
            if (factor != 0 && quotient > max / factor)
            {
                return (std::numeric_limits<std::size_t>::max)();
            }
            // remainder < part, so remainder * factor / part < factor
            ps_rep const extra = remainder <= max / (factor == 0 ? 1 : factor) ?
                remainder * factor / part :
                static_cast<ps_rep>(static_cast<long double>(remainder) *
                    static_cast<long double>(factor) /
                    static_cast<long double>(part));
            return static_cast<std::size_t>(quotient * factor + extra);
        }

        // The minimal run time of a chunk and the number of chunks per core,
        // for the ones that were not given the executor's minimal run time
        // (the current default unless it asks for another one) and the
        // current default number of chunks per core.
        template <typename Executor>
        std::pair<duration, std::size_t> get_values(
            Executor const& exec) const noexcept
        {
            if (min_chunk_time_ >= duration::zero() && chunks_per_core_ != 0)
            {
                return {min_chunk_time_, chunks_per_core_};
            }

            return {min_chunk_time_ >= duration::zero() ?
                    min_chunk_time_ :
                    hpx::chrono::steady_duration(
                        hpx::execution::experimental::get_min_chunk_time(exec))
                        .value(),
                chunks_per_core_ != 0 ?
                    chunks_per_core_ :
                    detail::get_task_size_chunk_size_defaults()
                        .chunks_per_core};
        }

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
        // a negative minimal run time and 0 chunks per core stand for the
        // defaults
        duration min_chunk_time_ = duration(-1);
        std::size_t chunks_per_core_ = 0;
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
