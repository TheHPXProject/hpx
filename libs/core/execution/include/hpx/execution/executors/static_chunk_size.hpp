//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/static_chunk_size.hpp
/// \page hpx::execution::experimental::static_chunk_size
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/serialization.hpp>
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
    HPX_CXX_CORE_EXPORT struct static_chunk_size
    {
        /// Construct a \a static_chunk_size executor parameters object
        ///
        /// \note By default, the number of loop iterations is determined from
        ///       the number of available cores and the overall number of loop
        ///       iterations to schedule.
        ///
        static_chunk_size() = default;

        /// Construct a \a static_chunk_size executor parameters object
        ///
        /// \param chunk_size   [in] The optional chunk size to use as the
        ///                     number of loop iterations to run on a single
        ///                     thread.
        ///
        constexpr explicit static_chunk_size(
            std::size_t const chunk_size) noexcept
          : chunk_size_(chunk_size)
        {
        }

        /// \cond NOINTERNAL
        template <typename Executor>
        std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const&, std::size_t const cores,
            std::size_t const num_iterations) const
        {
            // Make sure the internal round-robin counter of the executor is
            // reset
            hpx::execution::experimental::reset_thread_distribution(
                *this, exec);

            // use the given chunk size if given
            if (chunk_size_ != 0)
            {
                return chunk_size_;
            }

            if (cores == 1)
            {
                return num_iterations;
            }

            // Return a chunk size that ensures that each core ends up with the
            // same number of chunks the sizes of which are equal (except for
            // the last chunk, which may be smaller by not more than the number
            // of chunks in terms of elements).
            std::size_t const cores_times_4 = 4 * cores;    // -V112
            std::size_t chunk_size =
                (num_iterations + cores_times_4 - 1) / cores_times_4;

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

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & chunk_size_;
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        std::size_t chunk_size_ = 0;
        /// \endcond
    };
}    // namespace hpx::execution::experimental

/// \cond NOINTERNAL
template <>
struct hpx::execution::experimental::is_executor_parameters<
    hpx::execution::experimental::static_chunk_size> : std::true_type
{
};
/// \endcond

namespace hpx::execution {

    using static_chunk_size HPX_DEPRECATED_V(1, 9,
        "hpx::execution::static_chunk_size is deprecated, use "
        "hpx::execution::experimental::static_chunk_size instead") =
        hpx::execution::experimental::static_chunk_size;
}    // namespace hpx::execution
