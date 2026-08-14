//  Copyright (c) 2014-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/unordered/unordered_map.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/containers/unordered/partition_unordered_map_component.hpp>
#include <hpx/components/containers/unordered/unordered_map_segmented_iterator.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/// The hpx::unordered_map and its API's are defined here.
///
/// The hpx::unordered_map is a segmented data structure which is a collection
/// of one or more hpx::partition_unordered_maps. The hpx::unordered_map stores
/// the global IDs of each hpx::partition_unordered_maps.

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace server {
    ///////////////////////////////////////////////////////////////////////////
    struct unordered_map_config_data
    {
        // Each partition is described by it's corresponding client object, its
        // size, and locality id.
        struct partition_data
        {
            partition_data()
              : locality_id_(naming::invalid_locality_id)
            {
            }

            partition_data(id_type const& part, std::uint32_t locality_id)
              : partition_(make_ready_future(part).share())
              , locality_id_(locality_id)
            {
            }

            id_type get_id() const
            {
                return partition_.get();
            }

            hpx::shared_future<id_type> partition_;
            std::uint32_t locality_id_;

        private:
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned)
            {
                ar & partition_ & locality_id_;
            }
        };

        unordered_map_config_data() {}

        unordered_map_config_data(std::vector<partition_data> const& partitions)
          : partitions_(partitions)
        {
        }

        unordered_map_config_data(std::vector<partition_data>&& partitions)
          : partitions_(HPX_MOVE(partitions))
        {
        }

        std::vector<partition_data> partitions_;

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & partitions_;
        }
    };
}}    // namespace hpx::server

HPX_DISTRIBUTED_METADATA_DECLARATION(hpx::server::unordered_map_config_data,
    hpx_server_unordered_map_config_data)

///////////////////////////////////////////////////////////////////////////////
namespace hpx {
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        template <typename Key, typename T, typename Hash, typename KeyEqual>
        struct unordered_map_value_proxy
        {
            unordered_map_value_proxy(
                hpx::unordered_map<Key, T, Hash, KeyEqual>& um, Key const& key)
              : um_(um)
              , key_(key)
            {
            }

            operator T() const
            {
                return um_.get_value(launch::sync, key_);
            }

            template <typename T_>
            unordered_map_value_proxy& operator=(T_&& value)
            {
                um_.set_value(launch::sync, key_, HPX_FORWARD(T_, value));
                return *this;
            }

            hpx::unordered_map<Key, T, Hash, KeyEqual>& um_;
            Key const& key_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Hash, typename IsEmpty = std::is_empty<Hash>>
        struct unordered_hasher
        {
            unordered_hasher()
              : hasher_()
            {
            }

            explicit unordered_hasher(Hash const& hasher)
              : hasher_(hasher)
            {
            }

            template <typename Key>
            std::size_t operator()(Key const& key) const
            {
                return hasher_(key);
            }

            Hash hasher_;
        };

        template <typename Hash>
        struct unordered_hasher<Hash, std::true_type>
        {
            unordered_hasher() = default;

            explicit unordered_hasher(Hash const&) {}

            template <typename Key>
            std::size_t operator()(Key const& key) const
            {
                return Hash()(key);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename KeyEqual, typename IsEmpty = std::is_empty<KeyEqual>>
        struct unordered_comparator
        {
            unordered_comparator()
              : equal_()
            {
            }

            explicit unordered_comparator(KeyEqual const& equal)
              : equal_(equal)
            {
            }

            template <typename Key>
            bool operator()(Key const& lhs, Key const& rhs) const
            {
                return equal_(lhs, rhs);
            }

            KeyEqual equal_;
        };

        template <typename KeyEqual>
        struct unordered_comparator<KeyEqual, std::true_type>
        {
            unordered_comparator() = default;

            explicit unordered_comparator(KeyEqual const&) {}

            template <typename Key>
            bool operator()(Key const& lhs, Key const& rhs) const
            {
                return KeyEqual()(lhs, rhs);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Hash, typename KeyEqual>
        struct unordered_base
        {
            unordered_base() {}

            unordered_base(Hash const& hasher, KeyEqual const& equal)
              : hasher_(hasher)
              , equal_(equal)
            {
            }

            unordered_hasher<Hash> hasher_;
            unordered_comparator<KeyEqual> equal_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// This is the unordered_map class which defines hpx::unordered_map
    /// functionality.
    ///
    ///  This contains the client side implementation of the hpx::unordered_map.
    ///  This class defines the synchronous and asynchronous API's for each of
    ///  the exposed functionalities.
    ///
    template <typename Key, typename T, typename Hash, typename KeyEqual>
    class unordered_map
      : hpx::components::client_base<unordered_map<Key, T, Hash, KeyEqual>,
            hpx::components::server::distributed_metadata_base<
                server::unordered_map_config_data>>
      , detail::unordered_base<Hash, KeyEqual>
    {
    public:
        typedef std::allocator<T> allocator_type;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        typedef T value_type;
        typedef T reference;
        typedef T const const_reference;

#if defined(HPX_NATIVE_MIC)
        typedef T* pointer;
        typedef T const* const_pointer;
#else
        typedef typename std::allocator_traits<allocator_type>::pointer pointer;
        typedef typename std::allocator_traits<allocator_type>::const_pointer
            const_pointer;
#endif

    private:
        typedef hpx::components::client_base<unordered_map,
            hpx::components::server::distributed_metadata_base<
                server::unordered_map_config_data>>
            base_type;
        typedef detail::unordered_base<Hash, KeyEqual> hash_base_type;

        typedef hpx::server::partition_unordered_map<Key, T, Hash, KeyEqual>
            partition_unordered_map_server;
        typedef hpx::partition_unordered_map<Key, T, Hash, KeyEqual>
            partition_unordered_map_client;

        struct partition_data
          : server::unordered_map_config_data::partition_data
        {
            typedef server::unordered_map_config_data::partition_data base_type;

            partition_data(id_type const& part, std::uint32_t locality_id)
              : base_type(part, locality_id)
            {
            }

            partition_data(base_type&& base)
              : base_type(HPX_MOVE(base))
            {
            }

            hpx::future<typename partition_unordered_map_server::data_type>
            get_data() const
            {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
                typedef typename partition_unordered_map_server::
                    get_copied_data_action action_type;
                return hpx::async<action_type>(this->partition_.get());
#else
                HPX_ASSERT(false);
                return hpx::make_ready_future(
                    typename partition_unordered_map_server::data_type{});
#endif
            }

            hpx::future<void> set_data(
                typename partition_unordered_map_server::data_type&& d)
            {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
                typedef typename partition_unordered_map_server::
                    set_copied_data_action action_type;
                return hpx::async<action_type>(
                    this->partition_.get(), HPX_MOVE(d));
#else
                HPX_ASSERT(false);
                HPX_UNUSED(d);
                return hpx::make_ready_future();
#endif
            }

            std::shared_ptr<partition_unordered_map_server> local_data_;
        };

        // The list of partitions belonging to this unordered_map.
        //
        // Each partition is described by it's corresponding client object, its
        // size, and locality id.
        typedef std::vector<partition_data> partitions_vector_type;

        // This is the vector representing the base_index and corresponding
        // global ID's of the underlying partitioned_vector_partitions.
        partitions_vector_type partitions_;

        ///////////////////////////////////////////////////////////////////////
        // Connect this unordered_map to the existing unordered_mapusing the
        // given symbolic name.
        void get_data_helper(id_type id, server::unordered_map_config_data data)
        {
            partitions_.clear();
            partitions_.reserve(data.partitions_.size());

            std::move(data.partitions_.begin(), data.partitions_.end(),
                std::back_inserter(partitions_));

            base_type::reset(HPX_MOVE(id));
        }

        // this will be called by the base class once the registered id becomes
        // available
        future<void> connect_to_helper(id_type id)
        {
            typedef typename base_type::server_component_type::get_action act;

            return async(act(), id).then(
                [=, this](
                    future<server::unordered_map_config_data>&& f) -> void {
                    get_data_helper(id, f.get());
                });
        }

        ///////////////////////////////////////////////////////////////////////
        std::size_t get_partition(Key const& key) const
        {
            return this->hasher_(key) % partitions_.size();
        }

        std::vector<hpx::id_type> get_partition_ids() const
        {
            std::vector<hpx::id_type> ids;
            ids.reserve(partitions_.size());
            for (partition_data const& pd : partitions_)
            {
                ids.push_back(pd.get_id());
            }
            return ids;
        }

        ///////////////////////////////////////////////////////////////////////
        struct get_ptr_helper
        {
            std::size_t loc;
            partitions_vector_type& partitions;

            void operator()(
                future<std::shared_ptr<partition_unordered_map_server>>&& f)
                const
            {
                partitions[loc].local_data_ = f.get();
            }
        };

        /// \cond NOINTERNAL
        typedef std::pair<hpx::id_type, std::vector<hpx::id_type>>
            bulk_locality_result;
        /// \endcond

        void init(std::vector<bulk_locality_result> const& ids)
        {
            std::uint32_t this_locality = get_locality_id();
            std::vector<future<void>> ptrs;

            std::size_t l = 0;
            for (bulk_locality_result const& r : ids)
            {
                using naming::get_locality_id_from_id;
                std::uint32_t locality = get_locality_id_from_id(r.first);

                for (hpx::id_type const& id : r.second)
                {
                    partitions_.push_back(partition_data(id, locality));
                    if (locality == this_locality)
                    {
                        ptrs.push_back(
                            get_ptr<partition_unordered_map_server>(id).then(
                                get_ptr_helper{l, partitions_}));
                    }
                    ++l;
                }
            }

            hpx::wait_all(ptrs);
        }

        ///////////////////////////////////////////////////////////////////////
        // default construct partitions
        template <typename DistPolicy>
        void create(DistPolicy const& policy)
        {
            typedef partition_unordered_map_server component_type;

            std::size_t num_parts =
                traits::num_container_partitions<DistPolicy>::call(policy);

            // create as many partitions as required
            hpx::future<std::vector<bulk_locality_result>> f =
                policy.template bulk_create<false, component_type>(num_parts);

            // now initialize our data structures
            init(f.get());
        }

        // This function is called when we are creating the unordered_map. It
        // initializes the partitions based on the give parameters.
        template <typename DistPolicy>
        void create(DistPolicy const& policy, std::size_t bucket_count,
            Hash const& hash, KeyEqual const& equal)
        {
            typedef partition_unordered_map_server component_type;

            std::size_t num_parts =
                traits::num_container_partitions<DistPolicy>::call(policy);

            // create as many partitions as required
            hpx::future<std::vector<bulk_locality_result>> f =
                policy.template bulk_create<false, component_type>(
                    num_parts, bucket_count, hash, equal);

            // now initialize our data structures
            init(f.get());
        }

        // Perform a deep copy from the given unordered_map
        void copy_from(unordered_map const& rhs)
        {
            typedef
                typename partitions_vector_type::const_iterator const_iterator;

            std::vector<future<id_type>> objs;
            const_iterator end = rhs.partitions_.end();
            for (const_iterator it = rhs.partitions_.begin(); it != end; ++it)
            {
                typedef partition_unordered_map_server component_type;
                objs.push_back(hpx::components::copy<component_type>(
                    it->partition_.get()));
            }
            hpx::wait_all(objs);

            std::uint32_t this_locality = get_locality_id();
            std::vector<future<void>> ptrs;

            partitions_vector_type partitions;
            partitions.reserve(rhs.partitions_.size());
            for (std::size_t i = 0; i != rhs.partitions_.size(); ++i)
            {
                std::uint32_t locality = rhs.partitions_[i].locality_id_;

                partitions.push_back(partition_data(objs[i].get(), locality));

                if (locality == this_locality)
                {
                    ptrs.push_back(get_ptr<partition_unordered_map_server>(
                        partitions[i].partition_.get())
                            .then(get_ptr_helper{i, partitions}));
                }
            }

            hpx::wait_all(ptrs);

            std::swap(partitions_, partitions);
        }

    public:
        future<void> connect_to(std::string const& symbolic_name)
        {
            this->base_type::connect_to(symbolic_name);
            return this->base_type::share().then(
                [=, this](shared_future<id_type>&& f) -> hpx::future<void> {
                    return connect_to_helper(f.get());
                });
        }

        // Register this unordered_map with AGAS using the given symbolic name
        future<void> register_as(std::string const& symbolic_name)
        {
            std::vector<server::unordered_map_config_data::partition_data>
                partitions;
            partitions.reserve(partitions_.size());

            std::copy(partitions_.begin(), partitions_.end(),
                std::back_inserter(partitions));

            server::unordered_map_config_data data(HPX_MOVE(partitions));
            this->base_type::reset(
                hpx::new_<components::server::distributed_metadata_base<
                    server::unordered_map_config_data>>(
                    hpx::find_here(), HPX_MOVE(data)));

            return this->base_type::register_as(symbolic_name);
        }

    public:
        /// Default Constructor which create hpx::unordered_map with
        /// \a num_partitions = 1 and \a partition_size = 0. Hence overall size
        /// of the unordered_map is 0.
        unordered_map()
        {
            create(container_layout);
        }

        template <typename DistPolicy>
        unordered_map(DistPolicy const& policy,
            typename std::enable_if<
                traits::is_distribution_policy<DistPolicy>::value>::type* =
                nullptr)
        {
            create(policy);
        }

        explicit unordered_map(std::size_t bucket_count,
            Hash const& hash = Hash(), KeyEqual const& equal = KeyEqual())
          : hash_base_type(hash, equal)
        {
            create(hpx::container_layout, bucket_count, hash, equal);
        }

        template <typename DistPolicy>
        unordered_map(std::size_t bucket_count, DistPolicy const& policy,
            typename std::enable_if<
                traits::is_distribution_policy<DistPolicy>::value>::type* =
                nullptr)
        {
            create(policy, bucket_count, Hash(), KeyEqual());
        }

        template <typename DistPolicy>
        unordered_map(std::size_t bucket_count, Hash const& hash,
            DistPolicy const& policy,
            typename std::enable_if<
                traits::is_distribution_policy<DistPolicy>::value>::type* =
                nullptr)
          : hash_base_type(hash, KeyEqual())
        {
            create(policy, bucket_count, hash, KeyEqual());
        }

        template <typename DistPolicy>
        unordered_map(std::size_t bucket_count, Hash const& hash,
            KeyEqual const& equal, DistPolicy const& policy,
            typename std::enable_if<
                traits::is_distribution_policy<DistPolicy>::value>::type* =
                nullptr)
          : hash_base_type(hash, equal)
        {
            create(policy, bucket_count, hash, equal);
        }

        unordered_map(unordered_map const& rhs)
          : hash_base_type(rhs)
        {
            copy_from(rhs);
        }

        unordered_map(unordered_map&& rhs) noexcept
          : base_type(HPX_MOVE(static_cast<base_type&&>(rhs)))
          , hash_base_type(HPX_MOVE(static_cast<hash_base_type&&>(rhs)))
          , partitions_(HPX_MOVE(rhs.partitions_))
        {
        }

        unordered_map& operator=(unordered_map const& rhs)
        {
            if (this != &rhs)
                copy_from(rhs);
            return *this;
        }
        unordered_map& operator=(unordered_map&& rhs) noexcept
        {
            if (this != &rhs)
            {
                this->base_type::operator=(
                    HPX_MOVE(static_cast<base_type&&>(rhs)));
                this->hash_base_type::operator=(
                    HPX_MOVE(static_cast<hash_base_type&&>(rhs)));

                partitions_ = HPX_MOVE(rhs.partitions_);
            }
            return *this;
        }

        // the type every partition stores its data in
        typedef typename partition_unordered_map_server::data_type
            partition_data_type;

        std::size_t get_num_partitions() const
        {
            return partitions_.size();
        }

        /// \brief Array subscript operator. This does not throw any exception.
        ///
        /// \param pos Position of the element in the unordered_map
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        /// \note The non-const version of is operator returns a proxy object
        ///       instead of a real reference to the element.
        ///
        detail::unordered_map_value_proxy<Key, T, Hash, KeyEqual> operator[](
            Key const& pos)
        {
            return detail::unordered_map_value_proxy<Key, T, Hash, KeyEqual>(
                *this, pos);
        }
        T operator[](Key const& pos) const
        {
            return get_value(launch::sync, pos);
        }

        /// Returns the element at position \a pos in the unordered_map container.
        ///
        /// \param pos Position of the element in the unordered_map
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        T get_value(
            launch::sync_policy, Key const& pos, bool erase = false) const
        {
            return get_value(launch::sync, get_partition(pos), pos, erase);
        }

        /// Returns the element at position \a pos in the unordered_map container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        T get_value(launch::sync_policy, size_type part, Key const& pos,
            bool erase = false) const
        {
            HPX_ASSERT(part < partitions_.size());

            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
                return part_data.local_data_->get_value(pos, erase);

            return partition_unordered_map_client(part_data.partition_)
                .get_value(launch::sync, pos, erase);
        }

        /// Returns the element at position \a pos in the unordered_map container
        /// asynchronously.
        ///
        /// \param pos Position of the element in the unordered_map
        ///
        /// \return Returns the hpx::future to value of the element at position
        ///         represented by \a pos.
        ///
        future<T> get_value(Key const& pos, bool erase = false) const
        {
            return get_value(get_partition(pos), pos, erase);
        }

        /// Returns the element at position \a pos in the given partition in
        /// the unordered_map container asynchronously.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        ///
        /// \return Returns the hpx::future to value of the element at position
        ///         represented by \a pos.
        ///
        future<T> get_value(
            size_type part, Key const& pos, bool erase = false) const
        {
            HPX_ASSERT(part < partitions_.size());

            if (partitions_[part].local_data_)
            {
                return make_ready_future(
                    partitions_[part].local_data_->get_value(pos, erase));
            }

            return partition_unordered_map_client(partitions_[part].partition_)
                .get_value(pos, erase);
        }

        /// Copy the value of \a val in the element at position \a pos in
        /// the unordered_map container.
        ///
        /// \param pos   Position of the element in the unordered_map
        /// \param val   The value to be copied
        ///
        template <typename T_>
        void set_value(launch::sync_policy, Key const& pos, T_&& val)
        {
            return set_value(
                launch::sync, get_partition(pos), pos, HPX_FORWARD(T_, val));
        }

        /// Copy the value of \a val in the element at position \a pos in
        /// the unordered_map container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        /// \param val   The value to be copied
        ///
        template <typename T_>
        void set_value(
            launch::sync_policy, size_type part, Key const& pos, T_&& val)
        {
            HPX_ASSERT(part < partitions_.size());

            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                part_data.local_data_->set_value(pos, HPX_FORWARD(T_, val));
            }
            else
            {
                partition_unordered_map_client(part_data.partition_)
                    .set_value(launch::sync, pos, HPX_FORWARD(T_, val));
            }
        }

        /// Asynchronous set the element at position \a pos of the partition
        /// \a part to the given value \a val.
        ///
        /// \param pos   Position of the element in the unordered_map
        /// \param val   The value to be copied
        ///
        /// \return This returns the hpx::future of type void which gets ready
        ///         once the operation is finished.
        ///
        template <typename T_>
        future<void> set_value(Key const& pos, T_&& val)
        {
            return set_value(get_partition(pos), pos, HPX_FORWARD(T_, val));
        }

        /// Asynchronously set the element at position \a pos in
        /// the partition \part to the given value \a val.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        /// \param val   The value to be copied
        ///
        /// \return This returns the hpx::future of type void which gets ready
        ///         once the operation is finished.
        ///
        template <typename T_>
        future<void> set_value(size_type part, Key const& pos, T_&& val)
        {
            HPX_ASSERT(part < partitions_.size());

            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                part_data.local_data_->set_value(pos, HPX_FORWARD(T_, val));
                return make_ready_future();
            }

            return partition_unordered_map_client(part_data.partition_)
                .set_value(pos, HPX_FORWARD(T_, val));
        }

        /// Asynchronously compute the size of the unordered_map.
        ///
        /// \return Return the number of elements in the unordered_map
        ///
        hpx::future<std::size_t> size_async() const
        {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
            std::vector<hpx::id_type> ids = get_partition_ids();
            if (ids.empty())
                return make_ready_future(std::size_t(0));

            return hpx::lcos::reduce<
                typename partition_unordered_map_server::size_action>(
                ids, std::plus<std::size_t>());
#else
            HPX_ASSERT(false);
            return hpx::make_ready_future(std::size_t{});
#endif
        }

        /// Compute the size compute the size of the unordered_map.
        ///
        /// \return Return the number of elements in the unordered_map
        ///
        std::size_t size() const
        {
            return size_async().get();
        }

        /// Erase all values with the given key from the partition_unordered_map
        /// container.
        ///
        /// \param key   Key of the element in the partition_unordered_map
        ///
        /// \return Returns the number of elements erased
        ///
        std::size_t erase(launch::sync_policy, Key const& key)
        {
            return erase(key).get();
        }

        std::size_t erase(launch::sync_policy, size_type part, Key const& key)
        {
            HPX_ASSERT(part < partitions_.size());

            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
                return part_data.local_data_->erase(key);

            return partition_unordered_map_client(part_data.partition_)
                .erase(launch::sync, key);
        }

        /// Erase all values with the given key from the partition_unordered_map
        /// container.
        ///
        /// \param key  Key of the element in the partition_unordered_map
        ///
        /// \return This returns the hpx::future containing the number of
        ///         elements erased
        ///
        future<std::size_t> erase(Key const& key)
        {
            return erase(get_partition(key), key);
        }

        future<std::size_t> erase(size_type part, Key const& key)
        {
            HPX_ASSERT(part < partitions_.size());

            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
                return make_ready_future(part_data.local_data_->erase(key));

            return partition_unordered_map_client(part_data.partition_)
                .erase(key);
        }

        ///////////////////////////////////////////////////////////////////////
        typedef segmented::segment_unordered_map_iterator<Key, T, Hash,
            KeyEqual, typename partitions_vector_type::iterator>
            segment_iterator;
        typedef segmented::const_segment_unordered_map_iterator<Key, T, Hash,
            KeyEqual, typename partitions_vector_type::const_iterator>
            const_segment_iterator;

        // Return global segment iterator
        segment_iterator segment_begin()
        {
            return segment_iterator(partitions_.begin(), this);
        }

        const_segment_iterator segment_begin() const
        {
            return const_segment_iterator(partitions_.cbegin(), this);
        }

        const_segment_iterator segment_cbegin() const    //-V524
        {
            return const_segment_iterator(partitions_.cbegin(), this);
        }

        segment_iterator segment_end()
        {
            return segment_iterator(partitions_.end(), this);
        }

        const_segment_iterator segment_end() const
        {
            return const_segment_iterator(partitions_.cend(), this);
        }

        const_segment_iterator segment_cend() const    //-V524
        {
            return const_segment_iterator(partitions_.cend(), this);
        }
    };
}    // namespace hpx
