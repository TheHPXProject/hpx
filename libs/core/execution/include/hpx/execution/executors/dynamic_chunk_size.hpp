//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/dynamic_chunk_size.hpp
/// \page hpx::execution::experimental::dynamic_chunk_size
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
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
    /// dynamically scheduled among the threads; when a thread finishes one
    /// chunk, it is dynamically assigned another If \a chunk_size is not
    /// specified, the default chunk size is 1.
    ///
    /// \note This executor parameters type is equivalent to OpenMP's DYNAMIC
    ///       scheduling directive.
    ///
    HPX_CXX_CORE_EXPORT struct dynamic_chunk_size
    {
        /// Construct an \a dynamic_chunk_size executor parameters object
        ///
        /// \note Default constructed \a dynamic_chunk_size executor parameter
        ///       types will use a chunk size of '1'.
        ///
        dynamic_chunk_size() = default;

        /// Construct a \a dynamic_chunk_size executor parameters object
        ///
        /// \param chunk_size   [in] The optional chunk size to use as the
        ///                     number of loop iterations to schedule together.
        ///                     The default chunk size is 1.
        ///
        constexpr explicit dynamic_chunk_size(std::size_t chunk_size) noexcept
          : chunk_size_(chunk_size)
        {
        }

        /// \cond NOINTERNAL
        template <typename Executor>
        constexpr std::size_t get_chunk_size(Executor&&,
            hpx::chrono::steady_duration const&, std::size_t,
            std::size_t) const noexcept
        {
            return chunk_size_;
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
        std::size_t chunk_size_ = 1;
        /// \endcond
    };

    /// \cond NOINTERNAL
    template <>
    struct is_executor_parameters<
        hpx::execution::experimental::dynamic_chunk_size> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental

namespace hpx::execution {

    using dynamic_chunk_size HPX_DEPRECATED_V(1, 9,
        "hpx::execution::dynamic_chunk_size is deprecated, use "
        "hpx::execution::experimental::dynamic_chunk_size instead") =
        hpx::execution::experimental::dynamic_chunk_size;
}
