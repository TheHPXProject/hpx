//  Copyright (c) 2014 Anuj R. Sharma
//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/partitioned_vector/partitioned_vector.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/containers/partitioned_vector/export_definitions.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_decl.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_fwd.hpp>
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
/// \cond NOINTERNAL
namespace hpx::server {

    ///////////////////////////////////////////////////////////////////////////
    struct partitioned_vector_config_data
    {
        // Each partition is described by its corresponding client object, its
        // size, and locality id.
        struct partition_data
        {
            partition_data() = default;

            partition_data(id_type const& part, std::size_t first,
                std::size_t size, std::uint32_t locality_id)
              : partition_(part)
              , first_(first)
              , size_(size)
              , locality_id_(locality_id)
            {
            }

            id_type const& get_id() const
            {
                return partition_;
            }

            hpx::id_type partition_;
            std::size_t first_ = 0;
            std::size_t size_ = 0;
            std::uint32_t locality_id_ = naming::invalid_locality_id;

        private:
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned)
            {
                // clang-format off
                ar & partition_ & first_ & size_ & locality_id_;
                // clang-format on
            }
        };

        partitioned_vector_config_data() = default;

        partitioned_vector_config_data(
            std::size_t size, std::vector<partition_data>&& partitions)
          : size_(size)
          , partitions_(HPX_MOVE(partitions))
        {
        }

        std::size_t size_ = 0;
        std::vector<partition_data> partitions_;

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & size_ & partitions_;
        }
    };
}    // namespace hpx::server

HPX_DISTRIBUTED_METADATA_DECLARATION(
    hpx::server::partitioned_vector_config_data,
    hpx_server_partitioned_vector_config_data)

/// \endcond

namespace hpx {

    /// hpx::partitioned_vector is a sequence container that encapsulates
    /// dynamic size arrays.
    ///
    /// \note A hpx::partitioned_vector does not store all elements in a
    ///       contiguous block of memory. Memory is contiguous inside each of
    ///       the segmented partitions only.
    ///
    /// The hpx::partitioned_vector is a segmented data structure which is a
    /// collection of one
    /// or more hpx::server::partitioned_vector_partitions. The hpx::partitioned_vector
    /// stores the global
    /// ids of each hpx::server::partitioned_vector_partition and the size of each
    /// hpx::server::partitioned_vector_partition.
    ///
    /// The storage of the vector is handled automatically, being expanded and
    /// contracted as needed. Vectors usually occupy more space than static arrays,
    /// because more memory is allocated to handle future growth. This way a vector
    /// does not need to reallocate each time an element is inserted, but only when
    /// the additional memory is exhausted.
    ///
    ///  This contains the client side implementation of the
    ///  hpx::partitioned_vector. This
    ///  class defines the synchronous and asynchronous APIs for each of the
    ///  exposed functionalities.
    ///
    /// \tparam T   The type of the elements. The requirements that are imposed
    ///             on the elements depend on the actual operations performed
    ///             on the container. Generally, it is required that element type
    ///             is a complete type and meets the requirements of Erasable,
    ///             but many member functions impose stricter requirements.
    ///
    template <typename T, typename Data>
    class partitioned_vector
      : public hpx::components::client_base<partitioned_vector<T, Data>,
            hpx::components::server::distributed_metadata_base<
                server::partitioned_vector_config_data>>
    {
    public:
        using allocator_type = detail::extract_allocator_type_t<T, Data>;

        using size_type = typename Data::size_type;
        using difference_type = typename Data::difference_type;

        using value_type = T;
        using reference = T;
        using const_reference = T const;

#if defined(HPX_NATIVE_MIC)
        using pointer = T*;
        using const_pointer = T const*;
#else
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer =
            typename std::allocator_traits<allocator_type>::const_pointer;
#endif

    private:
        using base_type = hpx::components::client_base<partitioned_vector,
            hpx::components::server::distributed_metadata_base<
                server::partitioned_vector_config_data>>;

        using partitioned_vector_partition_server =
            hpx::server::partitioned_vector<T, Data>;
        using partitioned_vector_partition_client =
            hpx::partitioned_vector_partition<T, Data>;

        struct partition_data
          : server::partitioned_vector_config_data::partition_data
        {
            using base_type =
                server::partitioned_vector_config_data::partition_data;

            partition_data() = default;

            partition_data(id_type const& part, std::size_t first,
                std::size_t size, std::uint32_t locality_id)
              : base_type(part, first, size, locality_id)
            {
            }

            partition_data(base_type&& base) noexcept
              : base_type(HPX_MOVE(base))
            {
            }

            std::shared_ptr<partitioned_vector_partition_server> local_data_;
        };

        // The list of partitions belonging to this vector.
        // Each partition is described by its corresponding client object, its
        // size, and locality id.
        using partitions_vector_type = std::vector<partition_data>;

        size_type size_;    // overall size of the vector

        // This is the vector representing the base_index and corresponding
        // global ID's of the underlying partitioned_vector_partitions.
        partitions_vector_type partitions_;

    public:
        using iterator = segmented::vector_iterator<T, Data>;
        using const_iterator = segmented::const_vector_iterator<T, Data>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using local_iterator = segmented::local_vector_iterator<T, Data>;
        using const_local_iterator =
            segmented::const_local_vector_iterator<T, Data>;

        using segment_iterator = segmented::segment_vector_iterator<T, Data,
            typename partitions_vector_type::iterator>;
        using const_segment_iterator =
            segmented::const_segment_vector_iterator<T, Data,
                typename partitions_vector_type::const_iterator>;

        using local_segment_iterator =
            segmented::local_segment_vector_iterator<T, Data,
                typename partitions_vector_type::iterator>;
        using const_local_segment_iterator =
            segmented::local_segment_vector_iterator<T, Data,
                typename partitions_vector_type::const_iterator>;

        using partition_data_type = partition_data;

    private:
        friend class segmented::vector_iterator<T, Data>;
        friend class segmented::const_vector_iterator<T, Data>;

        friend class segmented::segment_vector_iterator<T, Data,
            typename partitions_vector_type::iterator>;
        friend class segmented::const_segment_vector_iterator<T, Data,
            typename partitions_vector_type::const_iterator>;

        std::size_t get_partition_size(std::size_t partnum) const;
        std::size_t get_global_index_part(
            std::size_t partnum, size_type local_index) const;

        ///////////////////////////////////////////////////////////////////////
        // Connect this vector to the existing vector using the given symbolic
        // name.
        void get_data_helper(
            id_type id, server::partitioned_vector_config_data&& data);

        // this will be called by the base class once the registered id becomes
        // available
        future<void> connect_to_helper(id_type id);

    public:
        future<void> connect_to(std::string symbolic_name);
        void connect_to(launch::sync_policy, std::string symbolic_name);

        // Register this vector with AGAS using the given symbolic name
        future<void> register_as(std::string symbolic_name);

        void register_as(launch::sync_policy, std::string symbolic_name);

        // construct from id
        partitioned_vector(future<id_type>&& f);

        void copy_data_from(partitioned_vector const& rhs);

    public:
        // Return partition sizes
        std::vector<std::size_t> get_partition_sizes() const;

        // Return partition localities
        std::vector<hpx::id_type> get_partition_localities() const;

        // Return the sequence number of the segment corresponding to the given
        // global index
        std::size_t get_partition(size_type global_index) const;

        // Return the local index inside the given segment
        std::size_t get_local_index(
            size_type partnum, size_type global_index) const;

        // Return the local indices inside the segment corresponding to the
        // given global indices
        std::vector<size_type> get_local_indices(
            std::vector<size_type> indices) const;

        // Return the global index corresponding to the local index inside the
        // given segment.
        template <typename SegmentIter>
        std::size_t get_global_index(
            SegmentIter const& it, size_type local_index) const
        {
            return get_global_index_part(
                it.base() - partitions_.cbegin(), local_index);
        }

        template <typename SegmentIter>
        std::size_t get_partition(SegmentIter const& it) const
        {
            return std::distance(partitions_.begin(), it.base());
        }

        // Return the local iterator referencing an element inside a segment
        // based on the given global index.
        local_iterator get_local_iterator(size_type global_index);
        const_local_iterator get_local_iterator(size_type global_index) const;

        // Return the segment iterator referencing a segment based on the given
        // global index.
        segment_iterator get_segment_iterator(size_type global_index);
        const_segment_iterator get_segment_iterator(
            size_type global_index) const;

    protected:
        /// \cond NOINTERNAL
        using bulk_locality_result =
            std::pair<hpx::id_type, std::vector<hpx::id_type>>;
        /// \endcond

        template <typename DistPolicy>
        static hpx::future<std::vector<bulk_locality_result>> create_helper1(
            DistPolicy const& policy, std::size_t count,
            std::vector<std::size_t> const& sizes);

        template <typename DistPolicy>
        static hpx::future<std::vector<bulk_locality_result>> create_helper2(
            DistPolicy const& policy, std::size_t count,
            std::vector<std::size_t> const& sizes, T const& val);

        // This function is called when we are creating the vector. It
        // initializes the partitions based on the give parameters.
        template <typename DistPolicy, typename Create>
        void create(DistPolicy const& policy, Create&& creator);

        template <typename DistPolicy>
        void create(DistPolicy const& policy);

        template <typename DistPolicy>
        void create(T const& val, DistPolicy const& policy);

        // Perform a deep copy from the given vector
        void copy_from(partitioned_vector const& rhs);

        partitioned_vector(partitioned_vector const& rhs, bool make_unmanaged)
          : base_type(rhs.get_id(), make_unmanaged)
          , size_(rhs.size_)
          , partitions_(rhs.partitions_)
        {
            if (make_unmanaged)
            {
                for (auto& part : partitions_)
                {
                    part.partition_.make_unmanaged();
                }
            }
        }

        explicit partitioned_vector(hpx::id_type id, std::size_t size,
            partitions_vector_type&& partitions)
          : base_type(HPX_MOVE(id))
          , size_(size)
          , partitions_(partitions)
        {
        }

    public:
        static partitioned_vector create_from(hpx::id_type id, std::size_t size,
            partitions_vector_type&& partitions)
        {
            std::uint32_t locality_id = get_locality_id();
            for (auto& p : partitions)
            {
                if (p.locality_id_ == locality_id && !p.local_data_)
                {
                    p.local_data_ =
                        hpx::get_ptr<partitioned_vector_partition_server>(
                            hpx::launch::sync, p.partition_);
                }
            }

            return partitioned_vector(HPX_MOVE(id), size, HPX_MOVE(partitions));
        }

        /// Default Constructor which creates hpx::partitioned_vector with
        /// \a num_partitions = 0 and \a partition_size = 0. Hence, the overall
        /// size of the vector is 0.
        ///
        partitioned_vector();

        /// Constructor which creates hpx::partitioned_vector with the given
        /// overall \a size
        ///
        /// \param size             The overall size of the vector
        ///
        explicit partitioned_vector(size_type size);

        /// Constructor which creates and initializes vector with the given
        /// \a where all elements are initialized with \a val.
        ///
        /// \param size             The overall size of the vector
        /// \param val              Default value for the elements in vector
        ///
        partitioned_vector(size_type size, T const& val);

        /// Constructor which creates and initializes vector of a size as given
        /// by the range, where all elements are initialized with the values
        /// from the given range [begin, end) and using the given distribution
        /// policy.
        ///
        /// \param begin            Start of range to use for initializing the
        ///                         new instance.
        /// \param end              End of range to use for initializing the
        ///                         new instance.
        ///
        partitioned_vector(typename Data::const_iterator begin,
            typename Data::const_iterator end);

        /// Constructor which creates vector of \a size using the given
        /// distribution policy.
        ///
        /// \param policy           The distribution policy to use
        ///
        template <typename DistPolicy>
        explicit partitioned_vector(DistPolicy const& policy,
            std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>* =
                nullptr);

        /// Constructor which creates and initializes vector of \a size using
        /// the given distribution policy.
        ///
        /// \param size             The overall size of the vector
        /// \param policy           The distribution policy to use
        ///
        template <typename DistPolicy>
        partitioned_vector(size_type size, DistPolicy const& policy,
            std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>* =
                nullptr);

        /// Constructor which creates and initializes vector of \a size, where
        /// all elements are initialized with \a val and using the given
        /// distribution policy.
        ///
        /// \param size             The overall size of the vector
        /// \param val              Default value for the elements in vector
        /// \param policy           The distribution policy to use
        ///
        template <typename DistPolicy>
        partitioned_vector(size_type size, T const& val,
            DistPolicy const& policy,
            std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>* =
                nullptr);

        /// Constructor which creates and initializes vector of a size as given
        /// by the range, where all elements are initialized with the values
        /// from the given range [begin, end) and using the given distribution
        /// policy.
        ///
        /// \param begin            Start of range to use for initializing the
        ///                         new instance.
        /// \param end              End of range to use for initializing the
        ///                         new instance.
        /// \param policy           The distribution policy to use
        ///
        template <typename DistPolicy>
        partitioned_vector(typename Data::const_iterator begin,
            typename Data::const_iterator end, DistPolicy const& policy,
            std::enable_if_t<traits::is_distribution_policy_v<DistPolicy>>* =
                nullptr);

        /// Copy construction performs a deep copy of the right hand side
        /// vector.
        partitioned_vector(partitioned_vector const& rhs)
          : base_type()
          , size_(0)
        {
            if (rhs.size_ != 0)
                copy_from(rhs);
        }

        partitioned_vector(partitioned_vector&& rhs) noexcept
          : base_type(HPX_MOVE(static_cast<base_type&&>(rhs)))
          , size_(rhs.size_)
          , partitions_(HPX_MOVE(rhs.partitions_))
        {
            rhs.size_ = 0;
        }

        /// \brief Array subscript operator. This does not throw any exception.
        ///
        /// \param pos Position of the element in the vector [Note the first
        ///            position in the partition is 0]
        ///
        /// \return Returns a proxy object which represents the indexed element.
        ///         A (possibly remote) access operation is performed only once
        ///         this proxy instance is used.
        ///
        segmented::detail::vector_value_proxy<T, Data> operator[](size_type pos)
        {
            return segmented::detail::vector_value_proxy<T, Data>(*this, pos);
        }

        /// \brief Array subscript operator. This does not throw any exception.
        ///
        /// \param pos Position of the element in the vector [Note the first
        ///            position in the partition is 0]
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        /// \note This function does not return a reference to the actual
        ///       element but a copy of its value.
        ///
        T operator[](size_type pos) const
        {
            return get_value(launch::sync, pos);
        }

        /// Copy assignment operator, performs deep copy of the right hand side
        /// vector.
        ///
        /// \param rhs    This the hpx::partitioned_vector object which is to
        ///               be copied
        ///
        partitioned_vector& operator=(partitioned_vector const& rhs)
        {
            if (this != &rhs && rhs.size_ != 0)
                copy_from(rhs);
            return *this;
        }

        partitioned_vector& operator=(partitioned_vector&& rhs) noexcept
        {
            if (this != &rhs)
            {
                this->base_type::operator=(static_cast<base_type&&>(rhs));

                size_ = rhs.size_;
                partitions_ = HPX_MOVE(rhs.partitions_);

                rhs.size_ = 0;
            }
            return *this;
        }

        // Create reference to rhs partitioned vector
        partitioned_vector ref(bool make_unmanaged = true) const
        {
            return {*this, make_unmanaged};
        }

        ///////////////////////////////////////////////////////////////////////
        // Capacity related APIs in vector class

        /// \brief Compute the size as the number of elements it contains.
        ///
        /// \return Return the number of elements in the vector
        ///
        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return size_;
        }

        /// \brief Compute the information about the underlying partitions.
        ///
        /// \return Return the partitions
        ///
        constexpr partitions_vector_type const& partitions() const
        {
            return partitions_;
        }

        //
        //  Element access APIs in vector class
        //

        /// Returns the element at position \a pos in the vector container.
        ///
        /// \param pos Position of the element in the vector
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        T get_value(launch::sync_policy, size_type pos) const
        {
            auto part = get_partition(pos);
            return get_value(launch::sync, part, get_local_index(part, pos));
        }

        /// Returns the element at position \a pos in the vector container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        T get_value(launch::sync_policy, size_type part, size_type pos) const
        {
            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                return part_data.local_data_->get_value(pos);
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.get_value(launch::sync, pos);
        }

        /// Returns the element at position \a pos in the vector container
        /// asynchronously.
        ///
        /// \param pos Position of the element in the vector
        ///
        /// \return Returns the hpx::future to value of the element at position
        ///         represented by \a pos.
        ///
        future<T> get_value(size_type pos) const
        {
            auto part = get_partition(pos);
            return get_value(part, get_local_index(part, pos));
        }

        /// Returns the element at position \a pos in the given partition in
        /// the vector container asynchronously.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        ///
        /// \return Returns the hpx::future to value of the element at position
        ///         represented by \a pos.
        ///
        future<T> get_value(size_type part, size_type pos) const
        {
            if (partitions_[part].local_data_)
            {
                return make_ready_future(
                    partitions_[part].local_data_->get_value(pos));
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.get_value(pos);
        }

        /// Returns the elements at the positions \a pos from the given
        /// partition in the vector container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        std::vector<T> get_values(launch::sync_policy, size_type part,
            std::vector<size_type> const& pos = {}) const
        {
            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
                return part_data.local_data_->get_values(pos);

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.get_values(launch::sync, pos);
        }

        /// Asynchronously returns the elements at the positions \a pos from
        /// the given partition in the vector container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Positions of the elements in the vector
        ///
        /// \return Returns the hpx::future to values of the elements at the
        ///         given positions represented by \a pos.
        ///
        future<std::vector<T>> get_values(
            size_type part, std::vector<size_type> const& pos = {}) const
        {
            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                return make_ready_future(
                    part_data.local_data_->get_values(pos));
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.get_values(pos);
        }

        /// Returns the elements at the positions \a pos in the vector
        /// container.
        ///
        /// \param pos   Global position of the element in the vector
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        future<std::vector<T>> get_values(
            std::vector<size_type> const& pos) const
        {
            // check if position vector is empty
            // the following code needs at least one element.
            if (pos.empty())
                return make_ready_future(std::vector<T>());

            // current partition index of the block
            size_type part_cur = get_partition(pos[0]);

            // iterator to the begin of current block
            auto part_begin = pos.begin();

            // vector holding futures of the values for all blocks
            std::vector<future<std::vector<T>>> part_values_future;
            for (auto it = pos.begin(); it != pos.end(); ++it)
            {
                // get the partition of the current position
                size_type part = get_partition(*it);

                // if the partition of the current position is the same
                // as the rest of the current block go to next position
                if (part == part_cur)
                    continue;

                // if the partition of the current position is NOT the same
                // as the positions before the block ends here

                // this is the end of a block containing indexes ('pos')
                // of the same partition ('part').
                // get async values for this block
                part_values_future.push_back(get_values(part_cur,
                    get_local_indices(std::vector<size_type>(part_begin, it))));

                // reset block variables to start a new one from here
                part_cur = part;
                part_begin = it;
            }

            // the end of the vector is also an end of a block
            // get async values for this block
            part_values_future.push_back(get_values(part_cur,
                get_local_indices(
                    std::vector<size_type>(part_begin, pos.end()))));

            // This helper function unwraps the vectors from each partition
            // and merge them to one vector
            auto merge_func =
                [&pos](std::vector<future<std::vector<T>>>&& part_values_f)
                -> std::vector<T> {
                std::vector<T> values;
                values.reserve(pos.size());

                for (future<std::vector<T>>& part_f : part_values_f)
                {
                    std::vector<T> part_values = part_f.get();
                    std::move(part_values.begin(), part_values.end(),
                        std::back_inserter(values));
                }
                return values;
            };

            // when all values are here merge them to one vector
            // and return a future to this vector
            return dataflow(
                launch::async, merge_func, HPX_MOVE(part_values_future));
        }

        /// Returns the elements at the positions \a pos
        /// in the vector container.
        ///
        /// \param pos   Global position of the element in the vector
        ///
        /// \return Returns the value of the element at position represented by
        ///         \a pos.
        ///
        std::vector<T> get_values(
            launch::sync_policy, std::vector<size_type> const& pos) const
        {
            return get_values(pos).get();
        }

        // //FRONT (never throws exception)
        // /** @brief Access the value of first element in the vector.
        //  *
        //  *  Calling the function on empty container cause undefined behavior.
        //  *
        //  * @return Return the value of the first element in the vector
        //  */
        // VALUE_TYPE front() const
        // {
        //     return partitioned_vector_partition_stub::front_async(
        //                             (partitions_.front().first).get()
        //                                           ).get();
        // }//end of front_value
        //
        // /** @brief Asynchronous API for front().
        //  *
        //  *  Calling the function on empty container cause undefined behavior.
        //  *
        //  * @return Return the hpx::future to return value of front()
        //  */
        // hpx::future< VALUE_TYPE > front_async() const
        // {
        //     return partitioned_vector_partition_stub::front_async(
        //                             (partitions_.front().first).get()
        //                                           );
        // }//end of front_async
        //
        // //BACK (never throws exception)
        // /** @brief Access the value of last element in the vector.
        //  *
        //  *  Calling the function on empty container cause undefined behavior.
        //  *
        //  * @return Return the value of the last element in the vector
        //  */
        // VALUE_TYPE back() const
        // {
        //     // As the LAST pair is there and then decrement operator to that
        //     // LAST is undefined hence used the end() function rather than back()
        //     return partitioned_vector_partition_stub::back_async(
        //                     ((partitions_.end() - 2)->first).get()
        //                                          ).get();
        // }//end of back_value
        //
        // /** @brief Asynchronous API for back().
        //  *
        //  *  Calling the function on empty container cause undefined behavior.
        //  *
        //  * @return Return hpx::future to the return value of back()
        //  */
        // hpx::future< VALUE_TYPE > back_async() const
        // {
        //     //As the LAST pair is there
        //     return partitioned_vector_partition_stub::back_async(
        //                     ((partitions_.end() - 2)->first).get()
        //                                          );
        // }//end of back_async
        //
        // //
        // // Modifier component action
        // //
        //
        // //ASSIGN
        // /** @brief Assigns new contents to each partition, replacing its
        //  *          current contents and modifying each partition size
        //  *          accordingly.
        //  *
        //  *  @param n     New size of each partition
        //  *  @param val   Value to fill the partition with
        //  *
        //  *  @exception hpx::invalid_vector_error If the \a n is equal to zero
        //  *              then it throw \a hpx::invalid_vector_error exception.
        //  */
        // void assign(size_type n, VALUE_TYPE const& val)
        // {
        //     if(n == 0)
        //         HPX_THROW_EXCEPTION(
        //             hpx::invalid_vector_error,
        //             "assign",
        //             "Invalid Vector: new_partition_size should be greater than zero"
        //                             );
        //
        //     std::vector<future<void>> assign_lazy_sync;
        //     for (partition_description_type const& p,
        //         util::make_iterator_range(partitions_.begin(),
        //                                    partitions_.end() - 1)
        //         )
        //     {
        //         assign_lazy_sync.push_back(
        //             partitioned_vector_partition_stub::assign_async(
        //                  (p.first).get(), n, val)
        //                                   );
        //     }
        //     hpx::wait_all(assign_lazy_sync);
        //     adjust_base_index(partitions_.begin(),
        //                       partitions_.end() - 1,
        //                       n);
        // }//End of assign
        //
        // /** @brief Asynchronous API for assign().
        //  *
        //  *  @param n     New size of each partition
        //  *  @param val   Value to fill the partition with
        //  *
        //  *  @exception hpx::invalid_vector_error If the \a n is equal to zero
        //  *              then it throw \a hpx::invalid_vector_error exception.
        //  *
        //  *  @return This return the hpx::future of type void [The void return
        //  *           type can help to check whether the action is completed or
        //  *           not]
        //  */
        // future<void> assign_async(size_type n, VALUE_TYPE const& val)
        // {
        //     return hpx::async(launch::async,
        //                       &vector::assign,
        //                       this,
        //                       n,
        //                       val
        //                       );
        // }
        //
        // //PUSH_BACK
        // /** @brief Add new element at the end of vector. The added element
        //  *          contain the \a val as value.
        //  *
        //  *  The value is added to the back to the last partition.
        //  *
        //  *  @param val Value to be copied to new element
        //  */
        // void push_back(VALUE_TYPE const& val)
        // {
        //     partitioned_vector_partition_stub::push_back_async(
        //                     ((partitions_.end() - 2 )->first).get(),
        //                                         val
        //                                         ).get();
        // }

        /// Copy the value of \a val in the element at position \a pos in
        /// the vector container.
        ///
        /// \param pos   Position of the element in the vector
        /// \param val   The value to be copied
        ///
        template <typename T_>
        void set_value(launch::sync_policy, size_type pos, T_&& val)
        {
            auto part = get_partition(pos);
            return set_value(launch::sync, part, get_local_index(part, pos),
                HPX_FORWARD(T_, val));
        }

        /// Copy the value of \a val in the element at position \a pos in
        /// the vector container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the partition
        /// \param val   The value to be copied
        ///
        template <typename T_>
        void set_value(
            launch::sync_policy, size_type part, size_type pos, T_&& val)
        {
            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                part_data.local_data_->set_value(pos, HPX_FORWARD(T_, val));
            }
            else
            {
                partitioned_vector_partition_client partition(
                    partitions_[part].partition_, true);
                partition.set_value(launch::sync, pos, HPX_FORWARD(T_, val));
            }
        }

        /// Asynchronous set the element at position \a pos of the partition
        /// \a part to the given value \a val.
        ///
        /// \param pos   Position of the element in the vector
        /// \param val   The value to be copied
        ///
        /// \return This returns the hpx::future of type void which gets ready
        ///         once the operation is finished.
        ///
        template <typename T_>
        future<void> set_value(size_type pos, T_&& val)
        {
            auto part = get_partition(pos);
            return set_value(
                part, get_local_index(part, pos), HPX_FORWARD(T_, val));
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
        future<void> set_value(size_type part, size_type pos, T_&& val)
        {
            partition_data const& part_data = partitions_[part];
            if (part_data.local_data_)
            {
                part_data.local_data_->set_value(pos, HPX_FORWARD(T_, val));
                return make_ready_future();
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.set_value(pos, HPX_FORWARD(T_, val));
        }

        /// Copy the values of \a val to the elements at positions \a pos in
        /// the partition \part of the vector container.
        ///
        /// \param part  Sequence number of the partition
        /// \param pos   Position of the element in the vector
        /// \param val   The value to be copied
        ///
        void set_values(launch::sync_policy, size_type part,
            std::vector<size_type> const& pos, std::vector<T> const& val)
        {
            HPX_ASSERT(pos.empty() || pos.size() == val.size());

            if (partitions_[part].local_data_)
            {
                partitions_[part].local_data_->set_values(pos, val);
                return;
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.set_values(pos, val).get();
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
        future<void> set_values(size_type part,
            std::vector<size_type> const& pos, std::vector<T> const& val)
        {
            HPX_ASSERT(pos.empty() || pos.size() == val.size());

            if (partitions_[part].local_data_)
            {
                partitions_[part].local_data_->set_values(pos, val);
                return make_ready_future();
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.set_values(pos, val);
        }

        /// Asynchronously set the element at position \a pos
        /// to the given value \a val.
        ///
        /// \param pos   Global position of the element in the vector
        /// \param val   The value to be copied
        ///
        /// \return This returns the hpx::future of type void which gets ready
        ///         once the operation is finished.
        ///
        future<void> set_values(
            std::vector<size_type> const& pos, std::vector<T> const& val)
        {
            HPX_ASSERT(pos.size() == val.size());

            // check if position vector is empty
            // the following code needs at least one element.
            if (pos.empty())
                return make_ready_future();

            // partition index of the current block
            size_type part_cur = get_partition(pos[0]);

            // iterator to the begin of current block
            typename std::vector<size_type>::const_iterator pos_block_begin =
                pos.begin();
            typename std::vector<T>::const_iterator val_block_begin =
                val.begin();

            // vector holding futures of the state for all blocks
            std::vector<future<void>> part_futures;

            // going through the position vector
            typename std::vector<size_type>::const_iterator pos_it =
                pos.begin();
            typename std::vector<T>::const_iterator val_it = val.begin();
            for (/**/; pos_it != pos.end(); ++pos_it, ++val_it)
            {
                // get the partition of the current position
                size_type part = get_partition(*pos_it);

                // if the partition of the current position is the same
                // as the rest of the current block go to next position
                if (part == part_cur)
                    continue;

                // if the partition of the current position is NOT the same
                // as the positions before the block ends here

                // this is the end of a block containing indexes ('pos')
                // of the same partition ('part').
                // set asynchronous values for this block
                part_futures.push_back(set_values(part_cur,
                    get_local_indices(
                        std::vector<size_type>(pos_block_begin, pos_it)),
                    std::vector<T>(val_block_begin, val_it)));

                // reset block variables to start a new one from here
                part_cur = part;
                pos_block_begin = pos_it;
                val_block_begin = val_it;
            }

            // the end of the vector is also an end of a block
            // get asynchronous values for this block
            part_futures.push_back(set_values(part_cur,
                get_local_indices(
                    std::vector<size_type>(pos_block_begin, pos.end())),
                std::vector<T>(val_block_begin, val.end())));

            return hpx::when_all(part_futures);
        }

        void set_values(launch::sync_policy, std::vector<size_type> const& pos,
            std::vector<T> const& val)
        {
            return set_values(pos, val).get();
        }

        template <typename F, typename... Ts>
        future<util::invoke_result_t<F, T, Ts...>> apply_on(
            size_type part, size_type pos, F f, Ts... ts) const
        {
            if (partitions_[part].local_data_)
            {
                return make_ready_future(partitions_[part].local_data_->apply(
                    pos, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
            }

            partitioned_vector_partition_client partition(
                partitions_[part].partition_, true);
            return partition.apply(
                pos, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        auto apply(std::size_t pos, F f, Ts... ts)
        {
            auto part = get_partition(pos);
            return apply_on(part, get_local_index(part, pos), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        auto apply(launch::sync_policy, std::size_t pos, F f, Ts... ts)
        {
            auto part = get_partition(pos);
            return apply_on(part, get_local_index(part, pos), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...)
                .get();
        }

        // //CLEAR
        // // TODO if number of partitions is kept constant every time then
        // // clear should modify (clear each partitioned_vector_partition
        // // one by one).
        //   void clear()
        //   {
        //       //It is keeping one gid hence iterator does not go
        //       //in an invalid state
        //       partitions_.erase(partitions_.begin() + 1,
        //                                  partitions_.end()-1);
        //       partitioned_vector_partition_stub::clear_async(
        //          (partitions_[0].second).get())
        //               .get();
        //       HPX_ASSERT(partitions_.size() > 1);
        //       //As this function changes the size we should have LAST always.
        //   }

        ///////////////////////////////////////////////////////////////////////
        /// Return the iterator at the beginning of the first segment located
        /// on the given locality.
        iterator begin()
        {
            return iterator(this, get_global_index(segment_cbegin(), 0));
        }

        /// \brief Return the const_iterator at the beginning of the vector.
        const_iterator begin() const
        {
            return const_iterator(this, get_global_index(segment_cbegin(), 0));
        }

        /// \brief Return the const_iterator at the beginning of the vector.
        const_iterator cbegin() const
        {
            return const_iterator(this, get_global_index(segment_cbegin(), 0));
        }

        /// \brief Return the iterator at the end of the vector.
        iterator end()
        {
            return iterator(this, get_global_index(segment_cend(), 0));
        }

        /// \brief Return the const_iterator at the end of the vector.
        const_iterator end() const
        {
            return const_iterator(this, get_global_index(segment_cend(), 0));
        }

        /// \brief Return the const_iterator at the end of the vector.
        const_iterator cend() const
        {
            return const_iterator(this, get_global_index(segment_cend(), 0));
        }

        ///////////////////////////////////////////////////////////////////////
        /// Return the iterator at the beginning of the first partition of the
        /// vector on the given locality.
        iterator begin(std::uint32_t id)
        {
            return iterator(this, get_global_index(segment_begin(id), 0));
        }

        /// Return the iterator at the beginning of the first partition of the
        /// vector on the given locality.
        const_iterator begin(std::uint32_t id) const
        {
            return const_iterator(
                this, get_global_index(segment_cbegin(id), 0));
        }

        /// Return the iterator at the beginning of the first partition of the
        /// vector on the given locality.
        const_iterator cbegin(std::uint32_t id) const
        {
            return const_iterator(
                this, get_global_index(segment_cbegin(id), 0));
        }

        /// Return the iterator at the end of the last partition of the
        /// vector on the given locality.
        iterator end(std::uint32_t id)
        {
            return iterator(this, get_global_index(segment_end(id), 0));
        }

        /// Return the iterator at the end of the last partition of the
        /// vector on the given locality.
        const_iterator end(std::uint32_t id) const
        {
            return const_iterator(this, get_global_index(segment_cend(id), 0));
        }

        /// Return the iterator at the end of the last partition of the
        /// vector on the given locality.
        const_iterator cend(std::uint32_t id) const
        {
            return const_iterator(this, get_global_index(segment_cend(id), 0));
        }

        ///////////////////////////////////////////////////////////////////////
        /// Return the iterator at the beginning of the first segment located
        /// on the given locality.
        iterator begin(id_type const& id)
        {
            HPX_ASSERT(naming::is_locality(id));
            return begin(naming::get_locality_id_from_id(id));
        }

        /// Return the iterator at the beginning of the first segment located
        /// on the given locality.
        const_iterator begin(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return begin(naming::get_locality_id_from_id(id));
        }

        /// Return the iterator at the beginning of the first segment located
        /// on the given locality.
        const_iterator cbegin(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return cbegin(naming::get_locality_id_from_id(id));
        }

        /// Return the iterator at the end of the last segment located
        /// on the given locality.
        iterator end(id_type const& id)
        {
            HPX_ASSERT(naming::is_locality(id));
            return end(naming::get_locality_id_from_id(id));
        }

        /// Return the iterator at the end of the last segment located
        /// on the given locality.
        const_iterator end(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return end(naming::get_locality_id_from_id(id));
        }

        /// Return the iterator at the end of the last segment located
        /// on the given locality.
        const_iterator cend(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return cend(naming::get_locality_id_from_id(id));
        }

        ///////////////////////////////////////////////////////////////////////
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
            return segment_cend();
        }

        const_segment_iterator segment_cend() const
        {
            return const_segment_iterator(partitions_.cend(), this);
        }

        ///////////////////////////////////////////////////////////////////////
        // Return local segment iterator
        local_segment_iterator segment_begin(std::uint32_t id)
        {
            return local_segment_iterator(
                partitions_.begin(), partitions_.end(), id);
        }

        const_local_segment_iterator segment_begin(std::uint32_t id) const
        {
            return segment_cbegin(id);
        }

        const_local_segment_iterator segment_cbegin(std::uint32_t id) const
        {
            return const_local_segment_iterator(
                partitions_.cbegin(), partitions_.cend(), id);
        }

        local_segment_iterator segment_end(std::uint32_t id)
        {
            local_segment_iterator it = segment_begin(id);
            it.unsatisfy_predicate();
            return it;
        }

        const_local_segment_iterator segment_end(std::uint32_t id) const
        {
            const_local_segment_iterator it = segment_begin(id);
            it.unsatisfy_predicate();
            return it;
        }

        const_local_segment_iterator segment_cend(std::uint32_t id) const
        {
            const_local_segment_iterator it = segment_cbegin(id);
            it.unsatisfy_predicate();
            return it;
        }

        ///////////////////////////////////////////////////////////////////////
        local_segment_iterator segment_begin(id_type const& id)
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_begin(naming::get_locality_id_from_id(id));
        }

        const_local_segment_iterator segment_begin(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_begin(naming::get_locality_id_from_id(id));
        }

        const_local_segment_iterator segment_cbegin(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_cbegin(naming::get_locality_id_from_id(id));
        }

        local_segment_iterator segment_end(id_type const& id)
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_end(naming::get_locality_id_from_id(id));
        }

        const_local_segment_iterator segment_end(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_end(naming::get_locality_id_from_id(id));
        }

        const_local_segment_iterator segment_cend(id_type const& id) const
        {
            HPX_ASSERT(naming::is_locality(id));
            return segment_cend(naming::get_locality_id_from_id(id));
        }
    };

    // serialization of partitioned_vector requires special handling
    template <typename T, typename Data>
    struct traits::needs_reference_semantics<partitioned_vector<T, Data>>
      : std::true_type
    {
    };
}    // namespace hpx
