//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file partitioned_vector_local_view_iterator.hpp

#pragma once

#include <hpx/assert.hpp>
#include <hpx/components/containers/partitioned_vector/detail/view_element.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <utility>

namespace hpx {

    template <typename DataType, typename BaseIter>
    class partitioned_vector_local_view_iterator
      : public hpx::util::iterator_adaptor<
            partitioned_vector_local_view_iterator<DataType, BaseIter>,
            BaseIter, DataType, std::forward_iterator_tag, DataType&>
    {
    private:
        using base_type = hpx::util::iterator_adaptor<
            partitioned_vector_local_view_iterator<DataType, BaseIter>,
            BaseIter, DataType, std::forward_iterator_tag, DataType&>;

    public:
        partitioned_vector_local_view_iterator() = default;

        explicit partitioned_vector_local_view_iterator(
            BaseIter&& it, BaseIter&& end)
          : base_type(HPX_FORWARD(BaseIter, it))
          , end_(HPX_FORWARD(BaseIter, end))
        {
            satisfy_predicate();
        }

        bool is_at_end() const
        {
            return this->base_reference() == end_;
        }

    private:
        friend class hpx::util::iterator_core_access;

        template <typename, typename>
        friend class const_partitioned_vector_local_view_iterator;

        DataType& dereference() const
        {
            HPX_ASSERT(!is_at_end());
            return this->base_reference()->data();
        }

        void increment()
        {
            ++(this->base_reference());
            satisfy_predicate();
        }

        void satisfy_predicate()
        {
            while (this->base_reference() != end_ &&
                !this->base_reference()->is_owned_by_current_thread())
            {
                ++(this->base_reference());
            }
        }

    private:
        BaseIter end_;
    };

    template <typename DataType, typename BaseIter>
    class const_partitioned_vector_local_view_iterator
      : public hpx::util::iterator_adaptor<
            const_partitioned_vector_local_view_iterator<DataType, BaseIter>,
            BaseIter, DataType, std::forward_iterator_tag, DataType const&>
    {
    private:
        using base_type = hpx::util::iterator_adaptor<
            const_partitioned_vector_local_view_iterator<DataType, BaseIter>,
            BaseIter, DataType, std::forward_iterator_tag, DataType const&>;

    public:
        const_partitioned_vector_local_view_iterator() = default;

        template <typename RightBaseIter>
        explicit const_partitioned_vector_local_view_iterator(
            partitioned_vector_local_view_iterator<DataType,
                RightBaseIter> const& other)
          : base_type(other.base())
          , end_(other.end_)
        {
        }

        explicit const_partitioned_vector_local_view_iterator(
            BaseIter&& it, BaseIter&& end)
          : base_type(HPX_FORWARD(BaseIter, it))
          , end_(HPX_FORWARD(BaseIter, end))
        {
            satisfy_predicate();
        }

        bool is_at_end() const
        {
            return this->base_reference() == end_;
        }

    private:
        friend class hpx::util::iterator_core_access;

        DataType const& dereference() const
        {
            HPX_ASSERT(!is_at_end());
            return this->base_reference()->data();
        }

        void increment()
        {
            ++(this->base_reference());
            satisfy_predicate();
        }

        void satisfy_predicate()
        {
            while (this->base_reference() != end_ &&
                !this->base_reference()->is_owned_by_current_thread())
            {
                ++(this->base_reference());
            }
        }

    private:
        BaseIter end_;
    };
}    // namespace hpx
