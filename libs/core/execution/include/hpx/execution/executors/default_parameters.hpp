//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/default_parameters.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/executors/execution_parameters_fwd.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/timing.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    /// Loop iterations are divided into pieces of size \a chunk_size and then
    /// assigned to threads. If \a chunk_size is not specified, the iterations
    /// are evenly (if possible) divided contiguously among the threads.
    ///
    /// \note This executor parameters type is equivalent to OpenMP's STATIC
    ///       scheduling directive.
    ///
    HPX_CXX_CORE_EXPORT struct default_parameters
    {
        /// Construct a \a default_parameters executor parameters object
        ///
        /// \note By default, the number of loop iterations is determined from
        ///       the number of available cores and the overall number of loop
        ///       iterations to schedule.
        ///
        default_parameters() = default;

        /// \cond NOINTERNAL
        template <typename Executor>
        std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const&, std::size_t const cores,
            std::size_t const num_iterations) const
        {
            // Make sure the internal round-robin counter of the executor is
            // reset
            hpx::execution::experimental::reset_thread_distribution(
                *this, HPX_FORWARD(Executor, exec));

            if (cores == 1)
            {
                return num_iterations;
            }

            // Return a chunk size that ensures that each core ends up with the
            // same number of chunks the sizes of which are equal (except for
            // the last chunk, which may be smaller by not more than the number
            // of chunks in terms of elements).
            std::size_t const cores_times_4 = 4 * cores;    // -V112
            std::size_t chunk_size = num_iterations / cores_times_4;

            // we should not consider more chunks than we have elements
            auto const max_chunks = (std::min) (cores_times_4, num_iterations);

            // we should not make chunks smaller than what's determined by
            // the max chunk size
            chunk_size = (std::max) (chunk_size,
                (num_iterations + max_chunks - 1) / max_chunks);

            HPX_ASSERT(chunk_size * cores_times_4 >= num_iterations);

            return chunk_size;
        }
        /// \endcond
    };

    /// \cond NOINTERNAL
    template <>
    struct is_executor_parameters<default_parameters> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
