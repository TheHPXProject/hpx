//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/partitioned_vector/partitioned_vector_view.hpp

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_segmented_iterator.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_view_iterator.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
/// \cond NOINTERNAL

namespace hpx {
    template <typename T, std::size_t N, typename Data = std::vector<T>>
    struct partitioned_vector_view
    {
    private:
        // Type aliases
        using pvector_iterator = hpx::segmented::vector_iterator<T, Data>;
        using const_pvector_iterator =
            hpx::segmented::const_vector_iterator<T, Data>;
        using segment_iterator = typename pvector_iterator::segment_iterator;
        using const_segment_iterator =
            typename const_pvector_iterator::segment_iterator;
        using traits =
            typename hpx::traits::segmented_iterator_traits<pvector_iterator>;
        using list_type = std::initializer_list<std::size_t>;

    public:
        using iterator =
            typename hpx::segmented::partitioned_vector_view_iterator<T, N,
                Data>;
        using const_iterator =
            typename hpx::segmented::const_partitioned_vector_view_iterator<T,
                N, Data>;

        // Minimal dummy construction
        explicit partitioned_vector_view(hpx::lcos::spmd_block const& block)
          : block_(block)
        {
        }

        explicit partitioned_vector_view(hpx::lcos::spmd_block const& block,
            pvector_iterator&& v_begin, pvector_iterator&& v_last,
            list_type&& sw_sizes, list_type&& hw_sizes = {})
          : partitioned_vector_view(block,
                traits::segment(HPX_FORWARD(pvector_iterator, v_begin)),
                traits::segment(HPX_FORWARD(pvector_iterator, v_last)),
                HPX_FORWARD(list_type, sw_sizes),
                HPX_FORWARD(list_type, hw_sizes))
        {
        }

        explicit partitioned_vector_view(hpx::lcos::spmd_block const& block,
            segment_iterator&& begin, segment_iterator&& last,
            list_type sw_sizes, list_type hw_sizes = {})
          : begin_(begin)
          , end_(begin)
          , cbegin_(begin)
          , cend_(begin)
          , block_(block)
        {
            using indices = typename hpx::util::make_index_pack<N>::type;

            // Physical sizes is equal to logical sizes if physical sizes are
            // not defined
            list_type& hw_sizes_ = hw_sizes.size() ? hw_sizes : sw_sizes;

            // Check that sizes of the view are valid regarding its dimension
            HPX_ASSERT_MSG(sw_sizes.size() == N,
                "Defined co-sizes must match the partitioned_vector_view "
                "dimension");

            // Generate two mixed radix basis
            fill_basis(hw_sizes_, hw_basis_, indices());
            fill_basis(sw_sizes, sw_basis_, indices());

            // Compute the needed size for the described view
            std::ptrdiff_t limit = 0;
            std::size_t idx = 0;
            for (std::size_t const& i : sw_sizes)
            {
                limit += (i - 1) * hw_basis_[idx];
                idx++;
            }

            // Check that combined sizes doesn't overflow the used space
            HPX_ASSERT_MSG(limit <= std::distance(begin, last),
                "Space dedicated to the described partitioned_vector_view "
                "is too small");
            HPX_UNUSED(last);

            // Update end_
            end_ += hw_basis_[N - 1] * sw_sizes.begin()[N - 1];
            cend_ += hw_basis_[N - 1] * sw_sizes.begin()[N - 1];
        }

    private:
        // Update view basis from the partitioned_vector_view sizes
        template <std::size_t... I>
        void fill_basis(list_type const& sizes,
            std::array<std::size_t, N + 1>& basis,
            hpx::util::index_pack<I...>) const
        {
            basis[0] = 1;

            std::size_t tmp = 1;
            auto in = sizes.begin();

            (void) std::initializer_list<int>{
                (static_cast<void>(basis[I + 1] = tmp *= (*in), in++), 0)...};
        }

        template <typename... I>
        std::size_t offset_solver(I... index) const
        {
            // Check that the subscript is valid regarding the view dimension
            static_assert(sizeof...(I) == N,
                "Subscript must match the partitioned_vector_view "
                "dimension");

            // Check that all the elements are of integral type
            static_assert(
                util::all_of<typename std::is_integral<I>::type...>::value,
                "One or more elements in subscript is not integral");

            std::size_t offset = 0;
            std::size_t i = 0;

            (void) std::initializer_list<int>{
                (static_cast<void>(
                     offset += ((std::size_t) index) * hw_basis_[i++]),
                    0)...};

            // Check that the solved index doesn't overflow the used space
            HPX_ASSERT_MSG(offset < hw_basis_.back(),
                "*Invalid partitioned_vector_view subscript");

            return offset;
        }

    public:
        // Subsrcript operator
        template <typename... I>
        hpx::detail::view_element<T, Data> operator()(I... index) const
        {
            std::size_t offset = offset_solver(index...);
            return hpx::detail::view_element<T, Data>(
                block_, begin_, end_, begin_ + offset);
        }

        // Iterator interfaces
        iterator begin()
        {
            return iterator(block_, begin_, end_, sw_basis_, hw_basis_, 0);
        }

        iterator end()
        {
            return iterator(
                block_, end_, end_, sw_basis_, hw_basis_, sw_basis_.back());
        }

        const_iterator begin() const
        {
            return const_iterator(
                block_, cbegin_, cend_, sw_basis_, hw_basis_, 0);
        }

        const_iterator end() const
        {
            return const_iterator(
                block_, cend_, cend_, sw_basis_, hw_basis_, sw_basis_.back());
        }

        const_iterator cbegin() const
        {
            return const_iterator(
                block_, cbegin_, cend_, sw_basis_, hw_basis_, 0);
        }

        const_iterator cend() const
        {
            return const_iterator(
                block_, cend_, cend_, sw_basis_, hw_basis_, sw_basis_.back());
        }

    private:
        std::array<std::size_t, N + 1> sw_basis_, hw_basis_;
        segment_iterator begin_, end_;
        const_segment_iterator cbegin_, cend_;
        std::reference_wrapper<hpx::lcos::spmd_block const> block_;
    };
}    // namespace hpx
