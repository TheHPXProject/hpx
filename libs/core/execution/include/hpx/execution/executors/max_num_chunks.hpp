//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/max_num_chunks.hpp
/// \page hpx::execution::experimental::max_num_chunks
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    /// Loop iterations are divided into not more than \a num_chunks partitions
    /// that are assigned to threads. If \a num_chunks is not specified, the
    /// number of chunks is determined based on the number of available cores.
    ///
    HPX_CXX_CORE_EXPORT struct max_num_chunks
    {
        /// Construct a \a max_num_chunks executor parameters object
        ///
        /// \note By default, the number of chunks is determined from the number
        ///       of available cores.
        ///
        max_num_chunks() = default;

        /// Construct a \a max_num_chunks executor parameters object
        ///
        /// \param num_chunks   [in] The optional number of chunks to use to run
        ///                     on a single thread.
        ///
        constexpr explicit max_num_chunks(std::size_t const num_chunks) noexcept
          : num_chunks_(num_chunks)
        {
        }

        /// \cond NOINTERNAL
        template <typename Executor>
        std::size_t maximal_number_of_chunks(
            Executor&&, std::size_t const cores, std::size_t) const
        {
            // use the given number of chunks if given
            if (num_chunks_ != 0)
            {
                return num_chunks_;
            }

            if (cores == 1)
            {
                return 1;
            }

            // Return a number of chunks that that leads to at most 4 chunks per
            // core.
            return cores * 4;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & num_chunks_;
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        std::size_t num_chunks_ = 0;
        /// \endcond
    };
}    // namespace hpx::execution::experimental

/// \cond NOINTERNAL
template <>
struct hpx::execution::experimental::is_executor_parameters<
    hpx::execution::experimental::max_num_chunks> : std::true_type
{
};
/// \endcond
