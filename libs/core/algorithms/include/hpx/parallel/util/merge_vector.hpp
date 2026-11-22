//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/util/merge_four.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace hpx::parallel::util {

    // Merge the ranges in the vector v_input using full_merge4. The v_output
    //        vector is used as auxiliary memory in the internal process
    //        The final results is in the dest range.
    //        All the ranges of v_output are inside the range dest
    // \param [in] dest : range where move the elements merged
    // \param [in] v_input : vector of ranges to merge
    // \param [in] v_output : vector of ranges obtained
    // \param [in] comp : comparison object
    // \return range with all the elements moved
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    void merge_level4(util::range<Iter1, Sent1> dest,
        std::vector<util::range<Iter2, Sent2>>& v_input,
        std::vector<util::range<Iter1, Sent1>>& v_output, Compare comp)
    {
        using range1_t = util::range<Iter1, Sent1>;
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        v_output.clear();
        if (v_input.size() == 0)
        {
            return;
        }
        if (v_input.size() == 1)
        {
            v_output.emplace_back(init_move(dest, v_input[0]));
            return;
        }

        std::size_t nrange = v_input.size();
        std::size_t pos_ini = 0;
        while (pos_ini < v_input.size())
        {
            std::size_t const nmerge = (nrange + 3) >> 2;
            // at most four ranges are merged at a time
            auto const nelem =
                static_cast<std::uint32_t>((nrange + nmerge - 1) / nmerge);
            range1_t rz = full_merge4(dest, &v_input[pos_ini], nelem, comp);
            v_output.emplace_back(rz);
            dest = util::range<Iter1, Sent1>(rz.end(), dest.end());
            pos_ini += nelem;
            nrange -= nelem;
        }
    }

    // Merge the ranges over uninitialized memory,in the vector v_input using
    //        full_merge4. The v_output vector is used as auxiliary memory in the
    //        internal process. The final results is in the dest range.
    //        All the ranges of v_output are inside the range dest
    // \param [in] dest : range where move the elements merged
    // \param [in] v_input : vector of ranges to merge
    // \param [in] v_output : vector of ranges obtained
    // \param [in] comp : comparison object
    // \return range with all the elements moved
    HPX_CXX_CORE_EXPORT template <typename Value, typename Iter, typename Sent,
        typename Compare>
    void uninit_merge_level4(util::range<Value*> dest,
        std::vector<util::range<Iter, Sent>>& v_input,
        std::vector<util::range<Value*>>& v_output, Compare comp)
    {
        using range1_t = util::range<Value*>;
        using type1 = hpx::traits::iter_value_t<Iter>;

        static_assert(std::is_same_v<type1, Value>, "Incompatible iterators\n");

        v_output.clear();
        if (v_input.size() == 0)
        {
            return;
        }
        if (v_input.size() == 1)
        {
            v_output.emplace_back(uninit_move(dest, v_input[0]));
            return;
        }

        std::size_t nrange = v_input.size();
        std::size_t pos_ini = 0;
        while (pos_ini < v_input.size())
        {
            std::size_t const nmerge = (nrange + 3) >> 2;
            // at most four ranges are merged at a time
            auto const nelem =
                static_cast<std::uint32_t>((nrange + nmerge - 1) / nmerge);
            range1_t rz =
                uninit_full_merge4(dest, &v_input[pos_ini], nelem, comp);
            v_output.emplace_back(rz);
            dest = util::range<Value*>(rz.end(), dest.end());
            pos_ini += nelem;
            nrange -= nelem;
        }
    }

    // Merge the ranges in the vector v_input using merge_level4. The v_output
    //        vector is used as auxiliary memory in the internal process
    //        The final results is in the range_output range.
    //        All the ranges of v_output are inside the range range_output
    //        All the ranges of v_input are inside the range range_input
    // \param [in] range_input : range including all the ranges of v_input
    // \param [in]range_output : range including all the elements of v_output
    // \param [in] v_input : vector of ranges to merge
    // \param [in] v_output : vector of ranges obtained
    // \param [in] comp : comparison object
    // \return range with all the elements moved
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    util::range<Iter2, Sent2> merge_vector4(
        util::range<Iter1, Sent1> range_input,
        util::range<Iter2, Sent2> range_output,
        std::vector<util::range<Iter1, Sent1>>& v_input,
        std::vector<util::range<Iter2, Sent2>>& v_output, Compare comp)
    {
        using range2_t = util::range<Iter2, Sent2>;
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        v_output.clear();
        if (v_input.size() == 0)
        {
            return range2_t(range_output.begin(), range_output.begin());
        }
        if (v_input.size() == 1)
        {
            return init_move(range_output, v_input[0]);
        }

        bool sw = false;
        std::size_t nrange = v_input.size();
        while (nrange > 1)
        {
            if (sw)
            {
                merge_level4(range_input, v_output, v_input, comp);
                sw = false;
                nrange = v_input.size();
            }
            else
            {
                merge_level4(range_output, v_input, v_output, comp);
                sw = true;
                nrange = v_output.size();
            }
        }
        return sw ? v_output[0] : init_move(range_output, v_input[0]);
    }
}    // namespace hpx::parallel::util
