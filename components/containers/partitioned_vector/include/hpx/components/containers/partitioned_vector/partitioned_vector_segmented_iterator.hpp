//  Copyright (c) 2014 Anuj R. Sharma
//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http:// ww.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file partitioned_vector_segmented_iterator.hpp
/// \brief This file contains the implementation of iterators for hpx::partitioned_vector.

// The idea for these iterators is taken from
// http://lafstern.org/matt/segmented.pdf.

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_decl.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::segmented {

    ///////////////////////////////////////////////////////////////////////////
    // This class wraps plain a partitioned_vector<>::iterator or
    // partitioned_vector<>::const_iterator
    template <typename T, typename Data, typename BaseIter>
    class local_raw_vector_iterator
      : public hpx::util::iterator_adaptor<
            local_raw_vector_iterator<T, Data, BaseIter>, BaseIter>
    {
    private:
        using base_type = hpx::util::iterator_adaptor<
            segmented::local_raw_vector_iterator<T, Data, BaseIter>, BaseIter>;
        using base_iterator = BaseIter;

    public:
        using local_iterator = segmented::local_vector_iterator<T, Data>;
        using local_const_iterator =
            segmented::const_local_vector_iterator<T, Data>;

        local_raw_vector_iterator() = default;

        local_raw_vector_iterator(base_iterator const& it,
            std::shared_ptr<server::partitioned_vector<T, Data>> data)
          : base_type(it)
          , data_(HPX_MOVE(data))
        {
        }

        local_iterator remote()
        {
            HPX_ASSERT(data_);
            std::size_t local_index =
                std::distance(data_->begin(), this->base());
            return local_iterator(
                partitioned_vector_partition<T, Data>(data_->get_id()),
                local_index, data_);
        }
        local_const_iterator remote() const
        {
            HPX_ASSERT(data_);
            std::size_t local_index =
                std::distance(data_->begin(), this->base());
            return local_const_iterator(
                partitioned_vector_partition<T, Data>(data_->get_id()),
                local_index, data_);
        }

    private:
        std::shared_ptr<server::partitioned_vector<T, Data>> data_;
    };

    template <typename T, typename Data, typename BaseIter>
    class const_local_raw_vector_iterator
      : public hpx::util::iterator_adaptor<
            segmented::const_local_raw_vector_iterator<T, Data, BaseIter>,
            BaseIter>
    {
    private:
        using base_type = hpx::util::iterator_adaptor<
            segmented::const_local_raw_vector_iterator<T, Data, BaseIter>,
            BaseIter>;
        using base_iterator = BaseIter;

    public:
        using local_iterator = segmented::const_local_vector_iterator<T, Data>;
        using local_const_iterator =
            segmented::const_local_vector_iterator<T, Data>;

        const_local_raw_vector_iterator() = default;

        const_local_raw_vector_iterator(base_iterator const& it,
            std::shared_ptr<server::partitioned_vector<T, Data>> data)
          : base_type(it)
          , data_(HPX_MOVE(data))
        {
        }

        local_const_iterator remote()
        {
            HPX_ASSERT(data_);
            std::size_t local_index =
                std::distance(data_->cbegin(), this->base());
            return local_const_iterator(
                partitioned_vector_partition<T, Data>(data_->get_id()),
                local_index, data_);
        }
        local_const_iterator remote() const
        {
            HPX_ASSERT(data_);
            std::size_t local_index =
                std::distance(data_->cbegin(), this->base());
            return local_const_iterator(
                partitioned_vector_partition<T, Data>(data_->get_id()),
                local_index, data_);
        }

    private:
        std::shared_ptr<server::partitioned_vector<T, Data>> data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename Data>
        struct local_vector_value_proxy
        {
            explicit constexpr local_vector_value_proxy(
                local_vector_iterator<T, Data> const& it) noexcept
              : it_(it)
            {
            }

            local_vector_value_proxy(local_vector_value_proxy const&) = default;
            local_vector_value_proxy(local_vector_value_proxy&&) = default;

            local_vector_value_proxy& operator=(
                local_vector_value_proxy const&) = default;
            local_vector_value_proxy& operator=(
                local_vector_value_proxy&&) = default;

            ~local_vector_value_proxy() = default;

            operator T() const
            {
                if (!it_.get_data())
                {
                    return it_.get_partition().get_value(
                        launch::sync, it_.get_local_index());
                }
                return *(it_.get_data()->begin() + it_.get_local_index());
            }

            T& local_get()
            {
                HPX_ASSERT(it_.get_data());
                return *(it_.get_data()->begin() + it_.get_local_index());
            }

            T const& local_get() const
            {
                HPX_ASSERT(it_.get_data());
                return *(it_.get_data()->begin() + it_.get_local_index());
            }

            template <typename T_,
                typename Enable = std::enable_if_t<!std::is_same_v<
                    std::decay_t<T_>, local_vector_value_proxy>>>
            local_vector_value_proxy const& operator=(T_&& value) const
            {
                if (!it_.get_data())
                {
                    it_.get_partition().set_value(launch::sync,
                        it_.get_local_index(), HPX_FORWARD(T_, value));
                }
                else
                {
                    *(it_.get_data()->begin() + it_.get_local_index()) =
                        HPX_FORWARD(T_, value);
                }
                return *this;
            }

        private:
            local_vector_iterator<T, Data> const& it_;
        };

        template <typename T, typename Data>
        struct const_local_vector_value_proxy
        {
            explicit constexpr const_local_vector_value_proxy(
                const_local_vector_iterator<T, Data> const& it) noexcept
              : it_(it)
            {
            }

            const_local_vector_value_proxy(
                const_local_vector_value_proxy const&) = default;
            const_local_vector_value_proxy(
                const_local_vector_value_proxy&&) = default;

            const_local_vector_value_proxy& operator=(
                const_local_vector_value_proxy const&) = default;
            const_local_vector_value_proxy& operator=(
                const_local_vector_value_proxy&&) = default;

            ~const_local_vector_value_proxy() = default;

            operator T() const
            {
                if (!it_.get_data())
                {
                    return it_.get_partition().get_value(
                        launch::sync, it_.get_local_index());
                }
                return *(it_.get_data()->begin() + it_.get_local_index());
            }

            T const& local_get() const
            {
                HPX_ASSERT(it_.get_data());
                return *(it_.get_data()->begin() + it_.get_local_index());
            }

        private:
            const_local_vector_iterator<T, Data> const& it_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename Data>
        struct vector_value_proxy
        {
            explicit constexpr vector_value_proxy(
                hpx::partitioned_vector<T, Data>& v,
                std::size_t const index) noexcept
              : v_(&v)
              , index_(index)
            {
            }

            vector_value_proxy(vector_value_proxy const&) = default;
            vector_value_proxy(vector_value_proxy&&) = default;

            vector_value_proxy& operator=(vector_value_proxy const&) = default;
            vector_value_proxy& operator=(vector_value_proxy&&) = default;

            ~vector_value_proxy() = default;

            operator T() const
            {
                return v_->get_value(launch::sync, index_);
            }

            template <typename T_,
                typename Enable = std::enable_if_t<
                    !std::is_same_v<std::decay_t<T_>, vector_value_proxy>>>
            vector_value_proxy const& operator=(T_&& value) const
            {
                v_->set_value(launch::sync, index_, HPX_FORWARD(T_, value));
                return *this;
            }

        private:
            partitioned_vector<T, Data>* v_;
            std::size_t index_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// This class implements the local iterator functionality for the
    /// partitioned backend of a hpx::partitioned_vector.
    template <typename T, typename Data>
    class local_vector_iterator
      : public hpx::util::iterator_facade<
            segmented::local_vector_iterator<T, Data>, T,
            std::random_access_iterator_tag,
            segmented::detail::local_vector_value_proxy<T, Data>>
    {
    private:
        using base_type = hpx::util::iterator_facade<
            segmented::local_vector_iterator<T, Data>, T,
            std::random_access_iterator_tag,
            segmented::detail::local_vector_value_proxy<T, Data>>;

    public:
        using size_type = std::size_t;

        // constructors
        local_vector_iterator() = default;

        local_vector_iterator(hpx::id_type partition,
            size_type const local_index,
            std::shared_ptr<server::partitioned_vector<T, Data>> data)
          : partition_(
                partitioned_vector_partition<T, Data>(HPX_MOVE(partition)))
          , local_index_(local_index)
          , data_(HPX_MOVE(data))
        {
        }

        local_vector_iterator(partitioned_vector_partition<T, Data> partition,
            size_type const local_index,
            std::shared_ptr<server::partitioned_vector<T, Data>> data) noexcept
          : partition_(HPX_MOVE(partition))
          , local_index_(local_index)
          , data_(HPX_MOVE(data))
        {
        }

        using local_raw_iterator = segmented::local_raw_vector_iterator<T, Data,
            typename Data::iterator>;
        using local_raw_const_iterator =
            segmented::const_local_raw_vector_iterator<T, Data,
                typename Data::const_iterator>;

        ///////////////////////////////////////////////////////////////////////
        local_raw_iterator local()
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return local_raw_iterator(
                data_->begin() + local_index_, data_);    //-V522
        }
        local_raw_const_iterator local() const
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return local_raw_iterator(
                data_->cbegin() + local_index_, data_);    //-V522
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned /* version */)
        {
            ar & partition_ & local_index_;
        }

    protected:
        friend class hpx::util::iterator_core_access;

        bool equal(local_vector_iterator const& other) const
        {
            return partition_ == other.partition_ &&
                local_index_ == other.local_index_;
        }

        [[nodiscard]] typename base_type::reference dereference() const noexcept
        {
            return segmented::detail::local_vector_value_proxy<T, Data>(*this);
        }

        void increment() noexcept
        {
            ++local_index_;
        }

        void decrement() noexcept
        {
            --local_index_;
        }

        void advance(std::ptrdiff_t const n) noexcept
        {
            local_index_ += n;
        }

        std::ptrdiff_t distance_to(
            local_vector_iterator const& other) const noexcept
        {
            HPX_ASSERT(partition_ == other.partition_);
            return other.local_index_ - local_index_;
        }

    public:
        partitioned_vector_partition<T, Data>& get_partition() noexcept
        {
            return partition_;
        }
        partitioned_vector_partition<T, Data> get_partition() const noexcept
        {
            return partition_;
        }

        [[nodiscard]] size_type get_local_index() const noexcept
        {
            return local_index_;
        }

        std::shared_ptr<server::partitioned_vector<T, Data>>& get_data()
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return data_;
        }
        std::shared_ptr<server::partitioned_vector<T, Data>> const& get_data()
            const
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return data_;
        }

    protected:
        // refer to a partition of the vector
        partitioned_vector_partition<T, Data> partition_{};

        // local position in the referenced partition
        size_type local_index_ = static_cast<size_type>(-1);

        // caching address of component
        mutable std::shared_ptr<server::partitioned_vector<T, Data>> data_{};
    };

    template <typename T, typename Data>
    class const_local_vector_iterator
      : public hpx::util::iterator_facade<const_local_vector_iterator<T, Data>,
            T const, std::random_access_iterator_tag,
            segmented::detail::const_local_vector_value_proxy<T, Data>>
    {
        using base_type =
            hpx::util::iterator_facade<const_local_vector_iterator<T, Data>,
                T const, std::random_access_iterator_tag,
                segmented::detail::const_local_vector_value_proxy<T, Data>>;

    public:
        using size_type = std::size_t;

        // constructors
        const_local_vector_iterator() = default;

        const_local_vector_iterator(hpx::id_type partition,
            size_type const local_index,
            std::shared_ptr<server::partitioned_vector<T, Data>> data)
          : partition_(
                partitioned_vector_partition<T, Data>(HPX_MOVE(partition)))
          , local_index_(local_index)
          , data_(HPX_MOVE(data))
        {
        }

        const_local_vector_iterator(
            partitioned_vector_partition<T, Data> partition,
            size_type const local_index,
            std::shared_ptr<server::partitioned_vector<T, Data>> data) noexcept
          : partition_(HPX_MOVE(partition))
          , local_index_(local_index)
          , data_(HPX_MOVE(data))
        {
        }

        const_local_vector_iterator(local_vector_iterator<T, Data> const& it)
          : partition_(it.get_partition())
          , local_index_(it.get_local_index())
          , data_(it.get_data())
        {
        }

        using local_raw_iterator = segmented::const_local_raw_vector_iterator<T,
            Data, typename Data::const_iterator>;
        using local_raw_const_iterator = local_raw_iterator;

        ///////////////////////////////////////////////////////////////////////
        local_raw_iterator local()
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return local_raw_iterator(
                data_->cbegin() + local_index_, data_);    //-V522
        }
        local_raw_const_iterator local() const
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return local_raw_const_iterator(
                data_->cbegin() + local_index_, data_);    //-V522
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned /* version */)
        {
            ar & partition_ & local_index_;
        }

    protected:
        friend class hpx::util::iterator_core_access;

        bool equal(const_local_vector_iterator const& other) const
        {
            return partition_ == other.partition_ &&
                local_index_ == other.local_index_;
        }

        [[nodiscard]] typename base_type::reference dereference() const noexcept
        {
            return segmented::detail::const_local_vector_value_proxy<T, Data>(
                *this);
        }

        void increment() noexcept
        {
            ++local_index_;
        }

        void decrement() noexcept
        {
            --local_index_;
        }

        void advance(std::ptrdiff_t const n) noexcept
        {
            local_index_ += n;
        }

        std::ptrdiff_t distance_to(
            const_local_vector_iterator const& other) const noexcept
        {
            HPX_ASSERT(partition_ == other.partition_);
            return other.local_index_ - local_index_;
        }

    public:
        partitioned_vector_partition<T, Data> get_partition() const
        {
            return partition_;
        }
        [[nodiscard]] size_type get_local_index() const noexcept
        {
            return local_index_;
        }

        std::shared_ptr<server::partitioned_vector<T, Data>>& get_data()
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return data_;
        }
        std::shared_ptr<server::partitioned_vector<T, Data>> const& get_data()
            const
        {
            if (!data_ && partition_)
            {
                data_ = partition_.get_ptr();
            }
            return data_;
        }

    protected:
        // refer to a partition of the vector
        partitioned_vector_partition<T, Data> partition_{};

        // local position in the referenced partition
        size_type local_index_ = static_cast<size_type>(-1);

        // caching address of component
        mutable std::shared_ptr<server::partitioned_vector<T, Data>> data_{};
    };

    ///////////////////////////////////////////////////////////////////////////
    /// This class implement the segmented iterator for the
    /// hpx::partitioned_vector.
    template <typename T, typename Data, typename BaseIter>
    class segment_vector_iterator
      : public hpx::util::iterator_adaptor<
            segment_vector_iterator<T, Data, BaseIter>, BaseIter>
    {
        using base_type = hpx::util::iterator_adaptor<
            segment_vector_iterator<T, Data, BaseIter>, BaseIter>;

    public:
        segment_vector_iterator() = default;

        explicit segment_vector_iterator(
            BaseIter const& it, partitioned_vector<T, Data>* data = nullptr)
          : base_type(it)
          , data_(data)
        {
        }

        [[nodiscard]] partitioned_vector<T, Data>* get_data() noexcept
        {
            return data_;
        }
        [[nodiscard]] partitioned_vector<T, Data> const* get_data()
            const noexcept
        {
            return data_;
        }

        [[nodiscard]] bool is_at_end() const
        {
            return data_ == nullptr ||
                this->base_type::base_reference() == data_->partitions_.end();
        }

    private:
        partitioned_vector<T, Data>* data_ = nullptr;
    };

    template <typename T, typename Data, typename BaseIter>
    class const_segment_vector_iterator
      : public hpx::util::iterator_adaptor<
            const_segment_vector_iterator<T, Data, BaseIter>, BaseIter>
    {
        using base_type = hpx::util::iterator_adaptor<
            const_segment_vector_iterator<T, Data, BaseIter>, BaseIter>;

    public:
        const_segment_vector_iterator() = default;

        template <typename RightBaseIter>
        explicit const_segment_vector_iterator(
            segment_vector_iterator<T, Data, RightBaseIter> const& o)
          : base_type(o.base())
          , data_(o.get_data())
        {
        }

        explicit const_segment_vector_iterator(BaseIter const& it,
            partitioned_vector<T, Data> const* data = nullptr)
          : base_type(it)
          , data_(data)
        {
        }

        [[nodiscard]] partitioned_vector<T, Data> const* get_data()
            const noexcept
        {
            return data_;
        }

        [[nodiscard]] bool is_at_end() const
        {
            return data_ == nullptr ||
                this->base_type::base_reference() == data_->partitions_.end();
        }

    private:
        partitioned_vector<T, Data> const* data_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename BaseIterator>
        struct is_requested_locality
        {
            using reference =
                typename std::iterator_traits<BaseIterator>::reference;

            is_requested_locality() = default;

            explicit constexpr is_requested_locality(
                std::uint32_t const locality_id) noexcept
              : locality_id_(locality_id)
            {
            }

            constexpr bool operator()(reference val) const noexcept
            {
                return locality_id_ == naming::invalid_locality_id ||
                    locality_id_ == val.locality_id_;
            }

            std::uint32_t locality_id_ = naming::invalid_locality_id;
        };
    }    // namespace detail

    /// This class implement the local segmented iterator for the
    /// hpx::partitioned_vector.
    template <typename T, typename Data, typename BaseIter>
    class local_segment_vector_iterator
      : public hpx::util::iterator_adaptor<
            local_segment_vector_iterator<T, Data, BaseIter>, BaseIter, Data,
            std::forward_iterator_tag>
    {
        using base_type = hpx::util::iterator_adaptor<
            local_segment_vector_iterator<T, Data, BaseIter>, BaseIter, Data,
            std::forward_iterator_tag>;
        using predicate = detail::is_requested_locality<BaseIter>;

    public:
        local_segment_vector_iterator() = default;

        explicit local_segment_vector_iterator(BaseIter const& end)
          : base_type(end)
          , predicate_()
          , end_(end)
        {
        }

        local_segment_vector_iterator(
            BaseIter const& it, BaseIter const& end, std::uint32_t locality_id)
          : base_type(it)
          , predicate_(locality_id)
          , end_(end)
        {
            satisfy_predicate();
        }

        [[nodiscard]] bool is_at_end() const
        {
            return !data_ || this->base() == end_;
        }

        // increment until predicate is not satisfied anymore
        void unsatisfy_predicate()
        {
            while (this->base() != end_ && predicate_(*this->base()))
            {
                ++(this->base_reference());
            }

            if (this->base() != end_)
            {
                data_ = this->base()->local_data_;
            }
            else
            {
                data_.reset();
            }
        }

    private:
        friend class hpx::util::iterator_core_access;

        [[nodiscard]] typename base_type::reference dereference() const
        {
            HPX_ASSERT(!is_at_end());
            return data_->get_data();
        }

        void increment()
        {
            ++(this->base_reference());

            if (this->base() != end_)
            {
                data_ = this->base()->local_data_;
            }
            else
            {
                data_.reset();
            }
        }

        void satisfy_predicate()
        {
            while (this->base() != end_ && !predicate_(*this->base()))
                ++(this->base_reference());

            if (this->base() != end_)
            {
                data_ = this->base()->local_data_;
            }
            else
            {
                data_.reset();
            }
        }

        std::shared_ptr<server::partitioned_vector<T, Data>> data_;
        predicate predicate_;
        BaseIter end_;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// This class implements the (global) iterator functionality for
    /// hpx::partitioned_vector.
    template <typename T, typename Data = std::vector<T>>
    class vector_iterator
      : public hpx::util::iterator_facade<vector_iterator<T, Data>, T,
            std::random_access_iterator_tag,
            detail::vector_value_proxy<T, Data>>
    {
    private:
        using base_type = hpx::util::iterator_facade<vector_iterator<T, Data>,
            T, std::random_access_iterator_tag,
            detail::vector_value_proxy<T, Data>>;

    public:
        using size_type = std::size_t;
        using segment_iterator =
            typename partitioned_vector<T, Data>::segment_iterator;
        using local_segment_iterator =
            typename partitioned_vector<T, Data>::local_segment_iterator;
        using local_iterator =
            typename partitioned_vector<T, Data>::local_iterator;

        // disable use of brackets_proxy in iterator_facade
        using use_brackets_proxy = std::false_type;

        // constructors
        vector_iterator() = default;

        vector_iterator(partitioned_vector<T, Data>* data,
            size_type const global_index) noexcept
          : data_(data)
          , global_index_(global_index)
        {
        }

        [[nodiscard]] partitioned_vector<T, Data>* get_data() noexcept
        {
            return data_;
        }
        [[nodiscard]] partitioned_vector<T, Data> const* get_data()
            const noexcept
        {
            return data_;
        }

        [[nodiscard]] size_type get_global_index() const noexcept
        {
            return global_index_;
        }

    protected:
        friend class hpx::util::iterator_core_access;

        bool equal(vector_iterator const& other) const
        {
            return data_ == other.data_ && global_index_ == other.global_index_;
        }

        [[nodiscard]] typename base_type::reference dereference() const
        {
            HPX_ASSERT(data_);
            return segmented::detail::vector_value_proxy<T, Data>(
                *data_, global_index_);
        }

        void increment() noexcept
        {
            HPX_ASSERT(data_);
            ++global_index_;
        }

        void decrement() noexcept
        {
            HPX_ASSERT(data_);
            --global_index_;
        }

        void advance(std::ptrdiff_t const n) noexcept
        {
            HPX_ASSERT(data_);
            global_index_ += n;
        }

        std::ptrdiff_t distance_to(vector_iterator const& other) const noexcept
        {
            HPX_ASSERT(data_ && other.data_);
            HPX_ASSERT(data_ == other.data_);
            return other.global_index_ - global_index_;
        }

        // refer to the vector
        partitioned_vector<T, Data>* data_ = nullptr;

        // global position in the referenced vector
        size_type global_index_ = static_cast<size_type>(-1);
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data = std::vector<T>>
    class const_vector_iterator
      : public hpx::util::iterator_facade<const_vector_iterator<T, Data>,
            T const, std::random_access_iterator_tag, T const>
    {
    private:
        using base_type =
            hpx::util::iterator_facade<const_vector_iterator<T, Data>, T const,
                std::random_access_iterator_tag, T const>;

    public:
        using size_type = std::size_t;
        using segment_iterator =
            typename partitioned_vector<T, Data>::const_segment_iterator;
        using local_segment_iterator =
            typename partitioned_vector<T, Data>::const_local_segment_iterator;
        using local_iterator =
            typename partitioned_vector<T, Data>::const_local_iterator;

        // disable use of brackets_proxy in iterator_facade
        using use_brackets_proxy = std::false_type;

        // constructors
        const_vector_iterator() = default;

        const_vector_iterator(partitioned_vector<T, Data> const* data,
            size_type const global_index) noexcept
          : data_(data)
          , global_index_(global_index)
        {
        }

        const_vector_iterator(vector_iterator<T, Data> const& it) noexcept
          : data_(it.data_)
          , global_index_(it.global_index_)
        {
        }

        partitioned_vector<T, Data> const* get_data() const noexcept
        {
            return data_;
        }
        [[nodiscard]] size_type get_global_index() const noexcept
        {
            return global_index_;
        }

    protected:
        friend class hpx::util::iterator_core_access;

        bool equal(const_vector_iterator const& other) const noexcept
        {
            return data_ == other.data_ && global_index_ == other.global_index_;
        }

        [[nodiscard]] typename base_type::reference dereference() const
        {
            HPX_ASSERT(data_);
            return data_->get_value(launch::sync, global_index_);
        }

        void increment() noexcept
        {
            HPX_ASSERT(data_);
            ++global_index_;
        }

        void decrement() noexcept
        {
            HPX_ASSERT(data_);
            --global_index_;
        }

        void advance(std::ptrdiff_t const n)
        {
            HPX_ASSERT(data_);
            global_index_ += n;
        }

        std::ptrdiff_t distance_to(
            const_vector_iterator const& other) const noexcept
        {
            HPX_ASSERT(data_ && other.data_);
            HPX_ASSERT(data_ == other.data_);
            return other.global_index_ - global_index_;
        }

        // refer to the vector
        partitioned_vector<T, Data> const* data_ = nullptr;

        // global position in the referenced vector
        size_type global_index_ = static_cast<size_type>(-1);
    };
}    // namespace hpx::segmented

///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    template <typename T, typename Data>
    struct segmented_iterator_traits<segmented::vector_iterator<T, Data>>
    {
        using is_segmented_iterator = std::true_type;

        using iterator = segmented::vector_iterator<T, Data>;
        using segment_iterator = typename iterator::segment_iterator;
        using local_segment_iterator =
            typename iterator::local_segment_iterator;
        using local_iterator = typename iterator::local_iterator;

        using local_raw_iterator = typename local_iterator::local_raw_iterator;

        //  Conceptually this function is supposed to denote which segment
        //  the iterator is currently pointing to (i.e. just global iterator).
        static segment_iterator segment(iterator iter)
        {
            return iter.get_data()->get_segment_iterator(
                iter.get_global_index());
        }

        //  This function should return which is the current segment and
        //  the exact position to which local iterator is pointing.
        static local_iterator local(iterator iter)
        {
            HPX_ASSERT(iter.get_data());    // avoid dereferencing end iterator
            return iter.get_data()->get_local_iterator(iter.get_global_index());
        }

        //  Build a full iterator from the segment and local iterators
        static iterator compose(
            segment_iterator seg_iter, local_iterator const& local_iter)
        {
            partitioned_vector<T, Data>* data = seg_iter.get_data();
            std::size_t index = local_iter.get_local_index();
            return iterator(data, data->get_global_index(seg_iter, index));
        }

        //  This function should return the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator seg_iter)
        {
            std::size_t offset = 0;
            if (seg_iter.is_at_end())
            {
                // return iterator to the end of last segment
                --seg_iter;
                offset = seg_iter.base()->size_;
            }

            return local_iterator(seg_iter.base()->partition_, offset,
                seg_iter.base()->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator seg_iter)
        {
            if (seg_iter.is_at_end())
            {
                --seg_iter;    // return iterator to the end of last segment
            }

            auto& base = seg_iter.base();
            return local_iterator(
                base->partition_, base->size_, base->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  beginning of the partition data.
        static local_raw_iterator begin(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(
                seg_iter->begin(), seg_iter.base()->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  end of the partition data.
        static local_raw_iterator end(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(
                seg_iter->end(), seg_iter.base()->local_data_);
        }

        // Extract the base id for the segment referenced by the given segment
        // iterator.
        static id_type get_id(segment_iterator const& iter)
        {
            return iter->get_id();
        }
    };

    template <typename T, typename Data>
    struct segmented_iterator_traits<segmented::const_vector_iterator<T, Data>>
    {
        using is_segmented_iterator = std::true_type;

        using iterator = segmented::const_vector_iterator<T, Data>;
        using segment_iterator = typename iterator::segment_iterator;
        using local_segment_iterator =
            typename iterator::local_segment_iterator;
        using local_iterator = typename iterator::local_iterator;

        using local_raw_iterator = typename local_iterator::local_raw_iterator;

        //  Conceptually this function is supposed to denote which segment
        //  the iterator is currently pointing to (i.e. just global iterator).
        static segment_iterator segment(iterator const& iter)
        {
            return iter.get_data()->get_segment_iterator(
                iter.get_global_index());
        }

        //  This function should return which is the current segment and
        //  the exact position to which local iterator is pointing.
        static local_iterator local(iterator const& iter)
        {
            HPX_ASSERT(iter.get_data());    // avoid dereferencing end iterator
            return iter.get_data()->get_local_iterator(iter.get_global_index());
        }

        //  Build a full iterator from the segment and local iterators
        static iterator compose(
            segment_iterator const& seg_iter, local_iterator const& local_iter)
        {
            partitioned_vector<T, Data> const* data = seg_iter.get_data();
            std::size_t index = local_iter.get_local_index();
            return iterator(data, data->get_global_index(seg_iter, index));
        }

        //  This function should return the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator seg_iter)
        {
            std::size_t offset = 0;
            if (seg_iter.is_at_end())
            {
                // return iterator to the end of last segment
                --seg_iter;
                offset = seg_iter.base()->size_;
            }

            return local_iterator(seg_iter.base()->partition_, offset,
                seg_iter.base()->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator seg_iter)
        {
            if (seg_iter.is_at_end())
            {
                --seg_iter;    // return iterator to the end of last segment
            }

            auto& base = seg_iter.base();
            return local_iterator(
                base->partition_, base->size_, base->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  beginning of the partition data.
        static local_raw_iterator begin(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(
                seg_iter->cbegin(), seg_iter.base()->local_data_);
        }

        //  This function should return the local iterator which is at the
        //  end of the partition data.
        static local_raw_iterator end(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(
                seg_iter->cend(), seg_iter.base()->local_data_);
        }

        // Extract the base id for the segment referenced by the given segment
        // iterator.
        static id_type get_id(segment_iterator const& iter)
        {
            return iter->get_id();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Some 'remote' iterators need to be mapped before being applied to the
    // local algorithms.
    template <typename T, typename Data>
    struct segmented_local_iterator_traits<
        segmented::local_vector_iterator<T, Data>>
    {
        using is_segmented_local_iterator = std::true_type;

        using iterator = segmented::vector_iterator<T, Data>;
        using local_iterator = segmented::local_vector_iterator<T, Data>;
        using local_raw_iterator = typename local_iterator::local_raw_iterator;

        // Extract base iterator from local_iterator
        static local_raw_iterator local(local_iterator it)
        {
            return it.local();
        }

        // Construct remote local_iterator from local_raw_iterator
        static local_iterator remote(local_raw_iterator it)
        {
            return it.remote();
        }
    };

    template <typename T, typename Data>
    struct segmented_local_iterator_traits<
        segmented::const_local_vector_iterator<T, Data>>
    {
        using is_segmented_local_iterator = std::true_type;

        using iterator = segmented::const_vector_iterator<T, Data>;
        using local_iterator = segmented::const_local_vector_iterator<T, Data>;
        using local_raw_iterator = typename local_iterator::local_raw_iterator;

        // Extract base iterator from local_iterator
        static local_raw_iterator local(local_iterator it)
        {
            return it.local();
        }

        // Construct remote local_iterator from local_raw_iterator
        static local_iterator remote(local_raw_iterator it)
        {
            return it.remote();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Data>
    struct is_value_proxy<
        hpx::segmented::detail::local_vector_value_proxy<T, Data>>
      : std::true_type
    {
    };

    template <typename T, typename Data>
    struct proxy_value<
        hpx::segmented::detail::local_vector_value_proxy<T, Data>>
    {
        using type = T;
    };

    template <typename T, typename Data>
    struct is_value_proxy<hpx::segmented::detail::vector_value_proxy<T, Data>>
      : std::true_type
    {
    };

    template <typename T, typename Data>
    struct proxy_value<hpx::segmented::detail::vector_value_proxy<T, Data>>
    {
        using type = T;
    };
}    // namespace hpx::traits
