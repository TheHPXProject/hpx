//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/move.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/util/low_level.hpp>
#include <hpx/parallel/util/range.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::util {

    // \brief Compare the elements pointed by it1 and it2, and if they
    //        are equals, compare their position, doing a stable comparison
    //
    // \param [in] it1 : iterator to the first element
    // \param [in] pos1 : position of the object pointed by it1
    // \param [in] it2 : iterator to the second element
    // \param [in] pos2 : position of the element pointed by it2
    // \param [in] comp : comparison object
    // \return result of the comparison
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
        typename Compare>
    bool less_range(Iter it1, std::uint32_t pos1, Sent it2, std::uint32_t pos2,
        Compare comp)
    {
        if (comp(*it1, *it2))
        {
            return true;
        }
        if (pos2 < pos1)
        {
            return false;
        }
        return !comp(*it2, *it1);
    }

    // \brief Merge four ranges
    // \param [in] dest: range where move the elements merged. Their size must be
    //                   greater or equal than the sum of the sizes of the ranges
    //                   in the array R
    // \param [in] R : array of ranges to merge
    // \param [in] nrange_input : number of ranges in R
    // \param [in] comp : comparison object
    // \return range with all the elements move with the size adjusted
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    util::range<Iter1, Sent1> full_merge4(util::range<Iter1, Sent1>& rdest,
        util::range<Iter2, Sent2> vrange_input[4], std::uint32_t nrange_input,
        Compare comp)
    {
        using range1_t = util::range<Iter1, Sent1>;
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        std::size_t ndest = 0;
        std::uint32_t i = 0;
        while (i < nrange_input)
        {
            if (vrange_input[i].size() != 0)
            {
                ndest += vrange_input[i++].size();
            }
            else
            {
                for (std::uint32_t k = i + 1; k < nrange_input; ++k)
                {
                    vrange_input[k - 1] = vrange_input[k];
                }
                --nrange_input;
            }
        }

        if (nrange_input == 0)
        {
            return range1_t(rdest.begin(), rdest.begin());
        }
        if (nrange_input == 1)
        {
            return init_move(rdest, vrange_input[0]);
        }
        if (nrange_input == 2)
        {
            return full_merge(rdest, vrange_input[0], vrange_input[1], comp);
        }

        // Initial sort
        std::uint32_t pos[4] = {0, 1, 2, 3};
        std::uint32_t npos = nrange_input;

        if (less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        if (less_range(vrange_input[pos[2]].begin(), pos[2],
                vrange_input[pos[1]].begin(), pos[1], comp))
        {
            std::swap(pos[1], pos[2]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[3]].begin(), pos[3],
                vrange_input[pos[2]].begin(), pos[2], comp))
        {
            std::swap(pos[3], pos[2]);
        }

        if (less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[2]].begin(), pos[2],
                vrange_input[pos[1]].begin(), pos[1], comp))
        {
            std::swap(pos[1], pos[2]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        Iter1 it_dest = rdest.begin();
        while (npos > 2)
        {
            auto& r = vrange_input[pos[0]];

            *it_dest++ = std::ranges::iter_move(r.begin());
            r = util::range<Iter2, Sent2>(r.begin() + 1, r.end());

            if (r.size() == 0)
            {
                pos[0] = pos[1];
                pos[1] = pos[2];
                pos[2] = pos[3];
                --npos;
            }
            else
            {
                if (less_range(vrange_input[pos[1]].begin(), pos[1],
                        vrange_input[pos[0]].begin(), pos[0], comp))
                {
                    std::swap(pos[0], pos[1]);
                    if (less_range(vrange_input[pos[2]].begin(), pos[2],
                            vrange_input[pos[1]].begin(), pos[1], comp))
                    {
                        std::swap(pos[1], pos[2]);
                        if (npos == 4 &&
                            less_range(vrange_input[pos[3]].begin(), pos[3],
                                vrange_input[pos[2]].begin(), pos[2], comp))
                        {
                            std::swap(pos[2], pos[3]);
                        }
                    }
                }
            }
        }

        range1_t raux1(rdest.begin(), it_dest);
        range1_t raux2(it_dest, rdest.end());
        if (pos[0] < pos[1])
        {
            return concat(raux1,
                full_merge(
                    raux2, vrange_input[pos[0]], vrange_input[pos[1]], comp));
        }

        return concat(raux1,
            full_merge(
                raux2, vrange_input[pos[1]], vrange_input[pos[0]], comp));
    }

    // \brief Merge four ranges and put the result in uninitialized memory
    // \param [in] dest: range where create and move the elements merged. Their
    //                   size must be greater or equal than the sum of the sizes
    //                   of the ranges in the array R
    // \param [in] R : array of ranges to merge
    // \param [in] nrange_input : number of ranges in vrange_input
    // \param [in] comp : comparison object
    // \return range with all the elements move with the size adjusted
    HPX_CXX_CORE_EXPORT template <typename Value, typename Iter, typename Sent,
        typename Compare>
    util::range<Value*> uninit_full_merge4(util::range<Value*> const& dest,
        util::range<Iter, Sent> vrange_input[4], std::uint32_t nrange_input,
        Compare comp)
    {
        using value_type = hpx::traits::iter_value_t<Iter>;

        static_assert(
            std::is_same_v<value_type, Value>, "Incompatible iterators\n");

        std::size_t ndest = 0;
        std::uint32_t i = 0;
        while (i < nrange_input)
        {
            if (vrange_input[i].size() != 0)
            {
                ndest += vrange_input[i++].size();
            }
            else
            {
                for (std::uint32_t k = i + 1; k < nrange_input; ++k)
                {
                    vrange_input[k - 1] = vrange_input[k];
                }
                --nrange_input;
            }
        }

        if (nrange_input == 0)
        {
            return util::range<value_type*>(dest.begin(), dest.begin());
        }
        if (nrange_input == 1)
        {
            return uninit_move(dest, vrange_input[0]);
        }
        if (nrange_input == 2)
        {
            return uninit_full_merge(
                dest, vrange_input[0], vrange_input[1], comp);
        }

        // Initial sort
        std::uint32_t pos[4] = {0, 1, 2, 3};
        std::uint32_t npos = nrange_input;

        if (less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        if (less_range(vrange_input[pos[2]].begin(), pos[2],
                vrange_input[pos[1]].begin(), pos[1], comp))
        {
            std::swap(pos[1], pos[2]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[3]].begin(), pos[3],
                vrange_input[pos[2]].begin(), pos[2], comp))
        {
            std::swap(pos[3], pos[2]);
        }

        if (less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[2]].begin(), pos[2],
                vrange_input[pos[1]].begin(), pos[1], comp))
        {
            std::swap(pos[1], pos[2]);
        }

        if (npos == 4 &&
            less_range(vrange_input[pos[1]].begin(), pos[1],
                vrange_input[pos[0]].begin(), pos[0], comp))
        {
            std::swap(pos[0], pos[1]);
        }

        value_type* it_dest = dest.begin();
        while (npos > 2)
        {
            auto& r = vrange_input[pos[0]];

            util::construct_object(
                &*it_dest++, std::ranges::iter_move(r.begin()));
            r = util::range<Iter, Sent>(r.begin() + 1, r.end());

            if (r.size() == 0)
            {
                pos[0] = pos[1];
                pos[1] = pos[2];
                pos[2] = pos[3];
                --npos;
            }
            else
            {
                if (less_range(vrange_input[pos[1]].begin(), pos[1],
                        vrange_input[pos[0]].begin(), pos[0], comp))
                {
                    std::swap(pos[0], pos[1]);
                    if (less_range(vrange_input[pos[2]].begin(), pos[2],
                            vrange_input[pos[1]].begin(), pos[1], comp))
                    {
                        std::swap(pos[1], pos[2]);
                        if (npos == 4 &&
                            less_range(vrange_input[pos[3]].begin(), pos[3],
                                vrange_input[pos[2]].begin(), pos[2], comp))
                        {
                            std::swap(pos[2], pos[3]);
                        }
                    }
                }
            }
        }

        util::range<value_type*> raux1(dest.begin(), it_dest);
        util::range<value_type*> raux2(it_dest, dest.end());
        if (pos[0] < pos[1])
        {
            return concat(raux1,
                uninit_full_merge(
                    raux2, vrange_input[pos[0]], vrange_input[pos[1]], comp));
        }

        return concat(raux1,
            uninit_full_merge(
                raux2, vrange_input[pos[1]], vrange_input[pos[0]], comp));
    }
}    // namespace hpx::parallel::util
