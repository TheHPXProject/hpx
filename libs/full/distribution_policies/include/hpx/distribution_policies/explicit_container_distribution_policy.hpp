//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/distribution_policies/default_distribution_policy.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // This class specifies the block chunking policy parameters to use for the
    // partitioning of the data in a hpx::partitioned_vector
    HPX_CXX_EXPORT struct explicit_container_distribution_policy
      : components::default_distribution_policy
    {
        explicit_container_distribution_policy() = default;

        explicit_container_distribution_policy operator()(
            std::vector<std::size_t> sizes) const
        {
            return {HPX_MOVE(sizes), get_localities()};
        }

        explicit_container_distribution_policy operator()(
            std::vector<std::size_t> sizes, hpx::id_type locality) const
        {
            return {HPX_MOVE(sizes), HPX_MOVE(locality)};
        }

        explicit_container_distribution_policy operator()(
            std::vector<std::size_t> sizes,
            std::vector<id_type> localities) const
        {
            return {HPX_MOVE(sizes), HPX_MOVE(localities)};
        }

        ///////////////////////////////////////////////////////////////////////
        [[nodiscard]] std::size_t get_num_partitions() const noexcept
        {
            return sizes_.size();
        }

        [[nodiscard]] std::vector<hpx::id_type> get_localities() const
        {
            if (!localities_)
            {
                // use this locality, if this object was default constructed
                return std::vector<id_type>(1,
                    naming::get_id_from_locality_id(agas::get_locality_id()));
            }

            HPX_ASSERT(!localities_->empty());
            return *localities_;
        }

        [[nodiscard]] std::vector<std::size_t> get_sizes() const
        {
            return sizes_;
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & localities_ & sizes_;
            // clang-format on
        }

        explicit_container_distribution_policy(
            std::vector<std::size_t> sizes, std::vector<id_type> localities)
          : components::default_distribution_policy(HPX_MOVE(localities))
          , sizes_(HPX_MOVE(sizes))
        {
        }

        explicit_container_distribution_policy(
            std::vector<std::size_t> sizes, hpx::id_type locality)
          : components::default_distribution_policy(HPX_MOVE(locality))
          , sizes_(HPX_MOVE(sizes))
        {
        }

        // number of chunks to create
        std::vector<std::size_t> sizes_;
    };

    HPX_CXX_EXPORT
    HPX_EXPORT extern explicit_container_distribution_policy const
        explicit_container_layout;

    ///////////////////////////////////////////////////////////////////////////
    namespace traits {

        template <>
        struct is_distribution_policy<explicit_container_distribution_policy>
          : std::true_type
        {
        };

        template <>
        struct num_container_partitions<explicit_container_distribution_policy>
        {
            static std::size_t call(
                explicit_container_distribution_policy const& policy)
            {
                return policy.get_num_partitions();
            }
        };

        template <>
        struct container_partition_sizes<explicit_container_distribution_policy>
        {
            static std::vector<std::size_t> call(
                explicit_container_distribution_policy const& policy,
                std::size_t)
            {
                return policy.get_sizes();
            }
        };
    }    // namespace traits
}    // namespace hpx
