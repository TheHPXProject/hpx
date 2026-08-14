//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2014 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/parallel/segmented_algorithms/functional/segmented_iterator_helpers.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace functional {

        HPX_CXX_EXPORT struct get_raw_iterator
        {
            template <typename Iterator>
            struct apply
            {
                template <typename SegIter>
                typename segmented_iterator_traits<Iterator>::local_raw_iterator
                operator()(SegIter iter) const
                {
                    return iter.local();
                }
            };
        };

        HPX_CXX_EXPORT struct get_remote_iterator
        {
            template <typename Iterator>
            struct apply
            {
                template <typename SegIter>
                typename segmented_iterator_traits<Iterator>::local_iterator
                operator()(SegIter iter) const
                {
                    return iter.remote();
                }
            };
        };
    }    // namespace functional

    ///////////////////////////////////////////////////////////////////////////
    // A zip_iterator represents a segmented iterator if all zipped iterators
    // are segmented iterators themselves.
    template <typename... Ts>
    struct segmented_iterator_traits<util::zip_iterator<Ts...>,
        std::enable_if_t<util::all_of_v<
            typename segmented_iterator_traits<Ts>::is_segmented_iterator...>>>
    {
        using is_segmented_iterator = std::true_type;

        using iterator = util::zip_iterator<Ts...>;
        using segment_iterator = util::zip_iterator<
            typename segmented_iterator_traits<Ts>::segment_iterator...>;
        using local_segment_iterator = util::zip_iterator<
            typename segmented_iterator_traits<Ts>::local_segment_iterator...>;
        using local_iterator = util::zip_iterator<
            typename segmented_iterator_traits<Ts>::local_iterator...>;
        using local_raw_iterator = util::zip_iterator<
            typename segmented_iterator_traits<Ts>::local_raw_iterator...>;

        //  Conceptually this function is supposed to denote which segment
        //  the iterator is currently pointing to (i.e. just global iterator).
        static segment_iterator segment(iterator iter)
        {
            return segment_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_segment,
                iterator>::call(iter));
        }

        //  This function should specify which is the current segment and
        //  the exact position to which local iterator is pointing.
        static local_iterator local(iterator iter)
        {
            return local_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_local,
                iterator>::call(iter));
        }

        //  This function should specify the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator const& iter)
        {
            return local_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_begin,
                iterator>::call(iter));
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator const& iter)
        {
            return local_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_end,
                iterator>::call(iter));
        }

        //  This function should specify the local iterator which is at the
        //  beginning of the partition data.
        static local_raw_iterator begin(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_local_begin,
                iterator>::call(seg_iter));
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition data.
        static local_raw_iterator end(local_segment_iterator const& seg_iter)
        {
            return local_raw_iterator(functional::lift_zipped_iterators<
                util::functional::segmented_iterator_local_end,
                iterator>::call(seg_iter));
        }

        // Extract the base id for the segment referenced by the given segment
        // iterator.
        static hpx::id_type get_id(segment_iterator const& iter)
        {
            using first_base_iterator = typename hpx::tuple_element<0,
                typename iterator::iterator_tuple_type>::type;
            using traits = segmented_iterator_traits<first_base_iterator>;

            return traits::get_id(hpx::get<0>(iter.get_iterator_tuple()));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    struct segmented_local_iterator_traits<util::zip_iterator<Ts...>,
        std::enable_if_t<
            util::all_of_v<typename segmented_local_iterator_traits<
                Ts>::is_segmented_local_iterator...>>>
    {
        using is_segmented_local_iterator = std::true_type;

        using iterator = util::zip_iterator<
            typename segmented_local_iterator_traits<Ts>::iterator...>;
        using local_iterator = util::zip_iterator<Ts...>;
        using local_raw_iterator =
            util::zip_iterator<typename segmented_local_iterator_traits<
                Ts>::local_raw_iterator...>;

        // Extract base iterator from local_iterator
        static local_raw_iterator local(local_iterator const& iter)
        {
            return local_raw_iterator(
                functional::lift_zipped_iterators<functional::get_raw_iterator,
                    iterator>::call(iter));
        }

        // Construct remote local_iterator from local_raw_iterator
        static local_iterator remote(local_raw_iterator const& iter)
        {
            return local_iterator(functional::lift_zipped_iterators<
                functional::get_remote_iterator, iterator>::call(iter));
        }
    };
}    // namespace hpx::traits
