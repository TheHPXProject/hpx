//  Copyright (c) 2015-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>

namespace hpx::util::functional {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct segmented_iterator_segment
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::segment_iterator;

            template <typename Iter>
            type operator()(Iter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::segment(
                    iter);
            }
        };
    };

    HPX_CXX_EXPORT struct segmented_iterator_local
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::local_iterator;

            template <typename Iter>
            type operator()(Iter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::local(iter);
            }
        };
    };

    HPX_CXX_EXPORT struct segmented_iterator_begin
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::local_iterator;

            template <typename SegIter>
            type operator()(SegIter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::begin(iter);
            }
        };
    };

    HPX_CXX_EXPORT struct segmented_iterator_end
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::local_iterator;

            template <typename SegIter>
            type operator()(SegIter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::end(iter);
            }
        };
    };

    HPX_CXX_EXPORT struct segmented_iterator_local_begin
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::local_raw_iterator;

            template <typename LocalSegIter>
            type operator()(LocalSegIter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::begin(iter);
            }
        };
    };

    HPX_CXX_EXPORT struct segmented_iterator_local_end
    {
        template <typename Iterator>
        struct apply
        {
            using type = typename traits::segmented_iterator_traits<
                Iterator>::local_raw_iterator;

            template <typename LocalSegIter>
            type operator()(LocalSegIter iter) const
            {
                return traits::segmented_iterator_traits<Iterator>::end(iter);
            }
        };
    };
}    // namespace hpx::util::functional
