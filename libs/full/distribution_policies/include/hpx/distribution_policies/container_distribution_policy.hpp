//  Copyright (c) 2014 Bibek Ghimire
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
    HPX_CXX_EXPORT struct container_distribution_policy
      : components::default_distribution_policy
    {
        container_distribution_policy() = default;

        container_distribution_policy operator()(
            std::size_t num_partitions) const
        {
            return {num_partitions, get_localities()};
        }

        container_distribution_policy operator()(
            hpx::id_type const& locality) const
        {
            return {locality};
        }

        container_distribution_policy operator()(
            std::vector<id_type> const& localities) const
        {
            if (num_partitions_ != static_cast<std::size_t>(-1))
            {
                return {num_partitions_, localities};
            }
            return {localities.size(), localities};
        }

        container_distribution_policy operator()(
            std::vector<id_type>&& localities) const
        {
            if (num_partitions_ != static_cast<std::size_t>(-1))
            {
                return {num_partitions_, HPX_MOVE(localities)};
            }
            return {localities.size(), HPX_MOVE(localities)};
        }

        container_distribution_policy operator()(std::size_t num_partitions,
            std::vector<id_type> const& localities) const
        {
            return {num_partitions, localities};
        }

        container_distribution_policy operator()(
            std::size_t num_partitions, std::vector<id_type>&& localities) const
        {
            return {num_partitions, HPX_MOVE(localities)};
        }

        ///////////////////////////////////////////////////////////////////////
        [[nodiscard]] std::size_t get_num_partitions() const
        {
            if (localities_)
            {
                std::size_t const num_parts =
                    (num_partitions_ == static_cast<std::size_t>(-1)) ?
                    localities_->size() :
                    num_partitions_;
                return (std::max) (num_parts, static_cast<std::size_t>(1));
            }
            return static_cast<std::size_t>(1);
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

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & localities_ & num_partitions_;
            // clang-format on
        }

        container_distribution_policy(
            std::size_t num_partitions, std::vector<id_type> const& localities)
          : components::default_distribution_policy(localities)
          , num_partitions_(num_partitions)
        {
        }

        container_distribution_policy(
            std::size_t num_partitions, std::vector<id_type>&& localities)
          : components::default_distribution_policy(HPX_MOVE(localities))
          , num_partitions_(num_partitions)
        {
        }

        container_distribution_policy(hpx::id_type const& locality)
          : components::default_distribution_policy(locality)
        {
        }

        // number of chunks to create
        std::size_t num_partitions_ = static_cast<std::size_t>(-1);
    };

    HPX_CXX_EXPORT HPX_EXPORT extern container_distribution_policy const
        container_layout;

    ///////////////////////////////////////////////////////////////////////////
    namespace traits {

        template <>
        struct is_distribution_policy<container_distribution_policy>
          : std::true_type
        {
        };

        template <>
        struct num_container_partitions<container_distribution_policy>
        {
            static std::size_t call(container_distribution_policy const& policy)
            {
                return policy.get_num_partitions();
            }
        };
    }    // namespace traits
}    // namespace hpx
