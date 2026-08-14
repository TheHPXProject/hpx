//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/collect_chunking_parameters.hpp
/// \page hpx::execution::experimental::collect_chunking_parameters
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

    /// Collected execution parameters
    HPX_CXX_CORE_EXPORT struct chunking_parameters
    {
        std::size_t num_elements;
        std::size_t num_cores;
        std::size_t num_chunks;
        std::size_t chunk_size;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            ar & num_elements & num_cores & num_chunks & chunk_size;
        }
    };

    /// Collect various parameters used for running a parallel algorithm
    HPX_CXX_CORE_EXPORT struct collect_chunking_parameters
    {
        explicit constexpr collect_chunking_parameters(
            chunking_parameters& exec_params) noexcept
          : exec_params_(&exec_params)
        {
        }

        /// \cond NOINTERNAL
        template <typename Executor>
        void collect_execution_parameters(Executor&&,
            std::size_t const num_elements, std::size_t const num_cores,
            std::size_t const num_chunks,
            std::size_t const chunk_size) const noexcept
        {
            exec_params_->num_elements = num_elements;
            exec_params_->num_cores = num_cores;
            exec_params_->num_chunks = num_chunks;
            exec_params_->chunk_size = chunk_size;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & *exec_params_;
            // clang-format on
        }

        chunking_parameters* exec_params_;
        /// \endcond
    };

    /// \cond NOINTERNAL
    template <>
    struct is_executor_parameters<collect_chunking_parameters> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
