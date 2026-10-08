//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file partitioned_vector_local_view.hpp

#pragma once

#include <hpx/components/containers/partitioned_vector/partitioned_vector_local_view_iterator.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_view.hpp>

#include <cstddef>

///////////////////////////////////////////////////////////////////////////////
/// \cond NOINTERNAL

namespace hpx {
    template <typename T, std::size_t N, typename Data>
    struct partitioned_vector_local_view
      : public hpx::partitioned_vector_view<T, N, Data>
    {
    private:
        using base_type = hpx::partitioned_vector_view<T, N, Data>;
        using base_iterator = typename base_type::iterator;
        using const_base_iterator = typename base_type::const_iterator;

    public:
        using value_type = T;
        using iterator =
            typename hpx::partitioned_vector_local_view_iterator<Data,
                base_iterator>;
        using const_iterator =
            typename hpx::const_partitioned_vector_local_view_iterator<Data,
                const_base_iterator>;

        explicit partitioned_vector_local_view(base_type const& global_pview)
          : base_type(global_pview)
        {
        }

        // Iterator interfaces
        iterator begin()
        {
            base_type& base(*this);
            return iterator(base.begin(), base.end());
        }

        iterator end()
        {
            base_type& base(*this);
            return iterator(base.end(), base.end());
        }

        const_iterator begin() const
        {
            base_type const& base(*this);
            return const_iterator(base.begin(), base.end());
        }

        const_iterator end() const
        {
            base_type const& base(*this);
            return const_iterator(base.end(), base.end());
        }

        const_iterator cbegin() const
        {
            base_type const& base(*this);
            return const_iterator(base.begin(), base.end());
        }

        const_iterator cend() const
        {
            base_type const& base(*this);
            return const_iterator(base.end(), base.end());
        }
    };

    template <typename T, std::size_t N, typename Data>
    partitioned_vector_local_view<T, N, Data> local_view(
        hpx::partitioned_vector_view<T, N, Data> const& base)
    {
        return partitioned_vector_local_view<T, N, Data>(base);
    }
}    // namespace hpx
/// \endcond
