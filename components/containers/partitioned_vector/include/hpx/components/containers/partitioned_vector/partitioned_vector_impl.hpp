//  Copyright (c) 2014 Anuj R. Sharma
//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/distribution_policies/container_distribution_policy.hpp>
#include <hpx/distribution_policies/explicit_container_distribution_policy.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/runtime_distributed/copy_component.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_impl.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_decl.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_segmented_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::size_t
    partitioned_vector<T, Data>::get_partition_size(std::size_t partnum) const
    {
        std::size_t size = partitions_.size();
        if (partnum == size)
        {
            return 0;
        }
        return size == 0 ? 0 : partitions_[partnum].size_;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::size_t
    partitioned_vector<T, Data>::get_global_index_part(
        std::size_t partnum, size_type local_index) const
    {
        std::size_t size = partitions_.size();
        if (partnum == size)
        {
            return size_;
        }
        return size == 0 ? local_index :
                           partitions_[partnum].first_ + local_index;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::get_data_helper(
        id_type id, server::partitioned_vector_config_data&& data)
    {
        partitions_.clear();
        partitions_.reserve(data.partitions_.size());

        size_ = data.size_;
        std::move(data.partitions_.begin(), data.partitions_.end(),
            std::back_inserter(partitions_));

        std::uint32_t this_locality = get_locality_id();
        for (auto& p : partitions_)
        {
            if (p.locality_id_ == this_locality && !p.local_data_)
            {
                p.local_data_ =
                    hpx::get_ptr<partitioned_vector_partition_server>(
                        hpx::launch::sync, p.partition_);
            }
        }

        this->base_type::reset(HPX_MOVE(id));
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT hpx::future<void>
    partitioned_vector<T, Data>::connect_to_helper([[maybe_unused]] id_type id)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using act = components::server::distributed_metadata_base<
            server::partitioned_vector_config_data>::get_action;

        return async(act(), id).then(
            [=, this](
                future<server::partitioned_vector_config_data>&& f) -> void {
                return get_data_helper(id, f.get());
            });
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future();
#endif
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT hpx::future<void>
    partitioned_vector<T, Data>::connect_to(std::string symbolic_name)
    {
        this->base_type::connect_to(HPX_MOVE(symbolic_name));
        return this->base_type::share().then(
            [=, this](shared_future<id_type>&& f) -> hpx::future<void> {
                return connect_to_helper(f.get());
            });
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::connect_to(
        launch::sync_policy, std::string symbolic_name)
    {
        connect_to(HPX_MOVE(symbolic_name)).get();
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT hpx::future<void>
    partitioned_vector<T, Data>::register_as(std::string symbolic_name)
    {
        std::vector<server::partitioned_vector_config_data::partition_data>
            partitions;
        partitions.reserve(partitions_.size());

        std::copy(partitions_.begin(), partitions_.end(),
            std::back_inserter(partitions));

        server::partitioned_vector_config_data data(
            size_, HPX_MOVE(partitions));
        this->base_type::reset(
            hpx::new_<components::server::distributed_metadata_base<
                server::partitioned_vector_config_data>>(
                hpx::find_here(), HPX_MOVE(data)));

        return this->base_type::register_as(HPX_MOVE(symbolic_name));
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::register_as(
        launch::sync_policy, std::string symbolic_name)
    {
        register_as(HPX_MOVE(symbolic_name)).get();
        this->base_type::get();
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(future<id_type>&& f)
    {
        f.share().then(
            [=, this](shared_future<id_type>&& fut) -> hpx::future<void> {
                return connect_to_helper(fut.get());
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::size_t
    partitioned_vector<T, Data>::get_partition(size_type global_index) const
    {
        if (global_index == 0)
        {
            return 0;
        }
        if (global_index == size_)
        {
            return partitions_.size();
        }

        // find approximate partition
        std::size_t num_partitions = partitions_.size();
        std::size_t part = global_index / (size_ / num_partitions + 1);

        HPX_ASSERT(part < num_partitions);

        // if approximation is too high, go backwards
        if (global_index < partitions_[part].first_)
        {
            while (part != 0)
            {
                if (global_index >= partitions_[--part].first_)
                {
                    return part;
                }
            }
            return 0;
        }

        // otherwise find partition that holds the global index by going forward
        for (/**/; part != num_partitions; ++part)
        {
            auto const& partition = partitions_[part];
            if (global_index < partition.first_ + partition.size_)
            {
                return part;
            }
        }
        return num_partitions;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::size_t
    partitioned_vector<T, Data>::get_local_index(
        size_type partnum, size_type global_index) const
    {
        return global_index - partitions_[partnum].first_;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
        std::vector<typename partitioned_vector<T, Data>::size_type>
        partitioned_vector<T, Data>::get_local_indices(
            std::vector<size_type> indices) const
    {
        for (size_type& index : indices)
        {
            index = get_local_index(get_partition(index), index);
        }
        return indices;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
        typename partitioned_vector<T, Data>::local_iterator
        partitioned_vector<T, Data>::get_local_iterator(size_type global_index)
    {
        HPX_ASSERT(global_index != static_cast<std::size_t>(-1));

        std::size_t part = get_partition(global_index);
        if (part == partitions_.size())
        {
            // return an iterator to the end of the last partition
            auto const& back = partitions_.back();
            return local_iterator(
                back.partition_, back.size_, back.local_data_);
        }

        std::size_t local_index = get_local_index(part, global_index);
        HPX_ASSERT(local_index != static_cast<std::size_t>(-1));

        return local_iterator(partitions_[part].partition_, local_index,
            partitions_[part].local_data_);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
        typename partitioned_vector<T, Data>::const_local_iterator
        partitioned_vector<T, Data>::get_local_iterator(
            size_type global_index) const
    {
        HPX_ASSERT(global_index != static_cast<std::size_t>(-1));

        std::size_t part = get_partition(global_index);
        if (part == partitions_.size())
        {
            // return an iterator to the end of the last partition
            auto const& back = partitions_.back();
            return local_iterator(
                back.partition_, back.size_, back.local_data_);
        }

        std::size_t local_index = get_local_index(part, global_index);
        HPX_ASSERT(local_index != static_cast<std::size_t>(-1));

        return const_local_iterator(partitions_[part].partition_, local_index,
            partitions_[part].local_data_);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
        typename partitioned_vector<T, Data>::segment_iterator
        partitioned_vector<T, Data>::get_segment_iterator(
            size_type global_index)
    {
        std::size_t part = get_partition(global_index);
        if (part == partitions_.size())
            return segment_iterator(partitions_.end(), this);

        return segment_iterator(partitions_.begin() + part, this);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
        typename partitioned_vector<T, Data>::const_segment_iterator
        partitioned_vector<T, Data>::get_segment_iterator(
            size_type global_index) const
    {
        std::size_t part = get_partition(global_index);
        if (part == partitions_.size())
            return const_segment_iterator(partitions_.cend(), this);

        return const_segment_iterator(partitions_.cbegin() + part, this);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT hpx::future<
        std::vector<typename partitioned_vector<T, Data>::bulk_locality_result>>
    partitioned_vector<T, Data>::create_helper1(DistPolicy const& policy,
        std::size_t count, std::vector<std::size_t> const& sizes)
    {
        using component_type =
            typename partitioned_vector_partition_client::server_component_type;

        return policy.template bulk_create<true, component_type>(count, sizes);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT hpx::future<
        std::vector<typename partitioned_vector<T, Data>::bulk_locality_result>>
    partitioned_vector<T, Data>::create_helper2(DistPolicy const& policy,
        std::size_t count, std::vector<std::size_t> const& sizes, T const& val)
    {
        using component_type =
            typename partitioned_vector_partition_client::server_component_type;

        return policy.template bulk_create<true, component_type>(
            count, sizes, val);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy, typename Create>
    void partitioned_vector<T, Data>::create(
        DistPolicy const& policy, Create&& creator)
    {
        std::size_t num_parts =
            traits::num_container_partitions<DistPolicy>::call(policy);
        std::vector<std::size_t> part_sizes =
            traits::container_partition_sizes<DistPolicy>::call(policy, size_);

        // create as many partitions as required
        hpx::future<std::vector<bulk_locality_result>> f =
            HPX_FORWARD(Create, creator)(policy, num_parts, part_sizes);

        // now initialize our data structures
        std::uint32_t const this_locality = get_locality_id();

        std::size_t num_part = 0;
        std::size_t allocated_size = 0;

        std::size_t l = 0;

        // Fixing the size of partitions to avoid race conditions between
        // possible reallocations during push back and the continuation
        // to set the local partition data
        partitions_.resize(num_parts);
        for (bulk_locality_result const& r : f.get())
        {
            using naming::get_locality_id_from_id;
            std::uint32_t locality = get_locality_id_from_id(r.first);
            for (hpx::id_type const& id : r.second)
            {
                std::size_t size =
                    (std::min) (part_sizes[l], size_ - allocated_size);
                partitions_[l] =
                    partition_data(id, allocated_size, size, locality);

                if (locality == this_locality)
                {
                    partitions_[l].local_data_ =
                        hpx::get_ptr<partitioned_vector_partition_server>(
                            hpx::launch::sync, id);
                }
                ++l;

                allocated_size += size;
                if (++num_part == num_parts)
                {
                    HPX_ASSERT(allocated_size == size_);

                    // shrink last partition, if appropriate
                    if (size != part_sizes.back())
                    {
                        partitioned_vector_partition_client partition(
                            partitions_[l - 1].partition_, true);
                        partition.resize(size);
                    }
                    break;
                }

                HPX_ASSERT(size == part_sizes[l - 1]);
                HPX_ASSERT(l < num_parts);
            }
        }
        HPX_ASSERT(l == num_parts);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::create(DistPolicy const& policy)
    {
        create(policy, &partitioned_vector::create_helper1<DistPolicy>);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::create(T const& val, DistPolicy const& policy)
    {
        create(policy,
            [&val](DistPolicy const& policy, std::size_t num_parts,
                std::vector<std::size_t> const& part_sizes) {
                return create_helper2(policy, num_parts, part_sizes, val);
            });
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::copy_from(partitioned_vector const& rhs)
    {
        std::vector<future<id_type>> objs;
        objs.reserve(rhs.partitions_.size());

        auto end = rhs.partitions_.end();
        for (auto it = rhs.partitions_.begin(); it != end; ++it)
        {
            using component_type =
                typename partitioned_vector_partition_client::
                    server_component_type;
            objs.push_back(
                hpx::components::copy<component_type>(it->partition_));
        }
        hpx::wait_all(objs);

        std::uint32_t this_locality = get_locality_id();

        partitions_vector_type partitions;
        partitions.reserve(rhs.partitions_.size());

        for (std::size_t i = 0; i != rhs.partitions_.size(); ++i)
        {
            std::uint32_t locality = rhs.partitions_[i].locality_id_;

            partitions.push_back(partition_data(objs[i].get(),
                rhs.partitions_[i].first_, rhs.partitions_[i].size_, locality));

            if (locality == this_locality)
            {
                partitions.back().local_data_ =
                    hpx::get_ptr<partitioned_vector_partition_server>(
                        hpx::launch::sync, partitions.back().partition_);
            }
        }

        size_ = rhs.size_;
        std::swap(partitions_, partitions);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT void
    partitioned_vector<T, Data>::copy_data_from(partitioned_vector const& rhs)
    {
        std::vector<std::size_t> const empty;
        for (std::size_t i = 0; i != rhs.partitions_.size(); ++i)
        {
            auto values = rhs.get_values(hpx::launch::sync, i, empty);
            set_values(hpx::launch::sync, i, empty, HPX_MOVE(values));
        }
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::vector<std::size_t>
    partitioned_vector<T, Data>::get_partition_sizes() const
    {
        std::vector<std::size_t> result;
        result.reserve(partitions_.size());
        for (auto const& part : partitions_)
        {
            result.push_back(part.size_);
        }
        return result;
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT std::vector<hpx::id_type>
    partitioned_vector<T, Data>::get_partition_localities() const
    {
        std::vector<hpx::id_type> result;
        result.reserve(partitions_.size());
        for (auto const& part : partitions_)
        {
            result.push_back(
                hpx::naming::get_locality_from_id(part.partition_));
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector()
      : size_(0)
    {
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(size_type size)
      : size_(size)
    {
        if (size != 0)
            create(hpx::container_layout);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(
        size_type size, T const& val)
      : size_(size)
    {
        if (size != 0)
            create(val, hpx::container_layout);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(
        typename Data::const_iterator begin, typename Data::const_iterator end)
      : size_(std::distance(begin, end))
    {
        if (size_ != 0)
        {
            // create all partitions
            create(hpx::container_layout);

            // fill partitions with their part of the data
            std::vector<std::size_t> const empty;
            std::vector<hpx::future<void>> futures;
            futures.reserve(partitions_.size());
            for (std::size_t i = 0; i != partitions_.size(); ++i)
            {
                HPX_ASSERT(static_cast<std::size_t>(std::distance(
                               begin, end)) >= partitions_[i].size_);
                auto const end_part = std::next(begin, partitions_[i].size_);
                futures.push_back(set_values(i, empty, Data(begin, end_part)));
                begin = end_part;
            }
            HPX_ASSERT(begin == end);
            hpx::wait_all(HPX_MOVE(futures));
        }
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(size_type size,
        DistPolicy const& policy,
        std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>*)
      : size_(size)
    {
        if (size != 0)
            create(policy);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(DistPolicy const&,
        std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>*)
      : size_(0)
    {
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    HPX_PARTITIONED_VECTOR_SPECIALIZATION_EXPORT
    partitioned_vector<T, Data>::partitioned_vector(size_type size,
        T const& val, DistPolicy const& policy,
        std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>>* /*= nullptr*/)
      : size_(size)
    {
        if (size != 0)
            create(val, policy);
    }

    template <typename T, typename Data /*= std::vector<T> */>
    template <typename DistPolicy>
    partitioned_vector<T, Data>::partitioned_vector(
        typename Data::const_iterator begin, typename Data::const_iterator end,
        DistPolicy const& policy,
        std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>>* /*= nullptr*/)
      : size_(std::distance(begin, end))
    {
        if (size_ != 0)
        {
            // create all partitions
            create(policy);

            // fill partitions with their part of the data
            std::vector<std::size_t> const empty;
            std::vector<hpx::future<void>> futures;
            futures.reserve(partitions_.size());
            for (std::size_t i = 0; i != partitions_.size(); ++i)
            {
                HPX_ASSERT(static_cast<std::size_t>(std::distance(
                               begin, end)) >= partitions_[i].size_);
                auto const end_part = std::next(begin, partitions_[i].size_);
                futures.push_back(set_values(i, empty, Data(begin, end_part)));
                begin = end_part;
            }
            HPX_ASSERT(begin == end);
            hpx::wait_all(HPX_MOVE(futures));
        }
    }
}    // namespace hpx
