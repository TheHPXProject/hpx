//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/low_level.hpp>

#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::util {

    // \struct range
    // \brief this represent a range between two iterators
    // \tparam Iter type of parameters of the range
    HPX_CXX_CORE_EXPORT template <typename Iterator,
        typename Sentinel = Iterator>
    using range = hpx::util::iterator_range<Iterator, Sentinel>;

    // \brief concatenate two contiguous ranges
    // \param [in] it1 : first range
    // \param [in] it2 : second range
    // \returns  range resulting of the concatenation
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    range<Iter, Sent> concat(
        range<Iter, Sent> const& it1, range<Iter, Sent> const& it2)
    {
        return range<Iter, Sent>(it1.begin(), it2.end());
    }

    /// \brief Move objects from the range src to dest
    /// \param [in] dest : range where move the objects
    /// \param [in] src : range from where move the objects
    /// \return range with the objects moved and the size adjusted
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2>
    range<Iter2, Iter2> init_move(
        range<Iter2, Sent2> const& dest, range<Iter1, Sent1> const& src)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        if (src.size() == 0)
        {
            return range<Iter2, Iter2>(dest.begin(), dest.begin());
        }

        init_move(dest.begin(), src.begin(), src.end());
        return range<Iter2, Iter2>(
            dest.begin(), std::next(dest.begin(), src.size()));
    }

    //-----------------------------------------------------------------------------
    //  function : uninit_move
    /// \brief Move objects from the range src creating them in  dest
    /// \param [in] dest : range where move and create the objects
    /// \param [in] src : range from where move the objects
    /// \return range with the objects moved and the size adjusted
    //-----------------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2>
    range<Iter2, Sent2> uninit_move(
        range<Iter2, Sent2> const& dest, range<Iter1, Sent1> const& src)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        if (src.size() == 0)
        {
            return range<Iter2, Iter2>(dest.begin(), dest.begin());
        }

        uninit_move(dest.begin(), src.begin(), src.end());
        return range<Iter2, Iter2>(
            dest.begin(), std::next(dest.begin(), src.size()));
    }

    //  function : destroy
    /// \brief destroy a range of objects
    /// \param [in] r : range to destroy
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    void destroy_range(range<Iter, Sent> r)
    {
        destroy(r.begin(), r.end());
    }

    /// \brief initialize a range of objects with the object val moving across them
    /// \param [in] r : range of elements not initialized
    /// \param [in] val : object used for the initialization
    /// \return range initialized
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    range<Iter, Sent> init(range<Iter, Sent> const& r,
        typename std::iterator_traits<Iter>::value_type& val)
    {
        init(r.begin(), r.end(), val);
        return r;
    }

    /// \brief : indicate if two ranges have a possible merge
    /// \param [in] src1 : first range
    /// \param [in] src2 : second range
    /// \param [in] comp : object for to compare elements
    /// \return true : they can be merged
    ///         false : they can't be merged
    /// \remarks
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    bool is_mergeable(range<Iter1, Sent1> const& src1,
        range<Iter2, Sent2> const& src2, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        return comp(*src2.front(), *src1.back());
    }

    /// \brief Merge two contiguous ranges src1 and src2 , and put the result in
    ///        the range dest, returning the range merged
    /// \param [in] dest : range where locate the elements merged. the size of dest
    ///                    must be  greater or equal than the sum of the sizes of
    ///                    src1 and src2
    /// \param [in] src1 : first range to merge
    /// \param [in] src2 : second range to merge
    /// \param [in] comp : comparison object
    /// \return range with the elements merged and the size adjusted
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Iter3, typename Sent3,
        typename Compare>
    range<Iter3, Sent3> full_merge(range<Iter3, Sent3> const& dest,
        range<Iter1, Sent1> const& src1, range<Iter2, Sent2> const& src2,
        Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;
        using type3 = hpx::traits::iter_value_t<Iter3>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");
        static_assert(std::is_same_v<type3, type2>, "Incompatible iterators\n");

        return range<Iter3, Sent3>(dest.begin(),
            full_merge(src1.begin(), src1.end(), src2.begin(), src2.end(),
                dest.begin(), comp));
    }

    /// \brief Merge two contiguous ranges src1 and src2 , and create and move the
    ///        result in the uninitialized range dest, returning the range merged
    /// \param [in] dest : range where locate the elements merged. the size of dest
    ///                    must be  greater or equal than the sum of the sizes of
    ///                    src1 and src2. Initially is un-initialize memory
    /// \param [in] src1 : first range to merge
    /// \param [in] src2 : second range to merge
    /// \param [in] comp : comparison object
    /// \return range with the elements merged and the size adjusted
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Value, typename Compare>
    range<Value*> uninit_full_merge(range<Value*> const& dest,
        range<Iter1, Sent1> const& src1, range<Iter2, Sent2> const& src2,
        Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");
        static_assert(std::is_same_v<Value, type2>, "Incompatible iterators\n");

        return range<Value*>(dest.begin(),
            uninit_full_merge(src1.begin(), src1.end(), src2.begin(),
                src2.end(), dest.begin(), comp));
    }

    /// \brief : Merge two buffers. The first buffer is in a separate memory
    /// \param [in] dest : range where finish the two buffers merged
    /// \param [in] src1 : first range to merge in a separate memory
    /// \param [in] src2 : second range to merge, in the final part of the
    ///                    range where deposit the final results
    /// \param [in] comp : object for compare two elements of the type pointed
    ///                    by the Iter1 and Iter2
    /// \return : range with the two buffers merged
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    range<Iter2, Sent2> half_merge(range<Iter2, Sent2> const& dest,
        range<Iter1, Sent1> const& src1, range<Iter2, Sent2> const& src2,
        Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        return range<Iter2, Sent2>(dest.begin(),
            half_merge(src1.begin(), src1.end(), src2.begin(), src2.end(),
                dest.begin(), comp));
    }

    /// \brief : merge two non contiguous buffers src1 , src2, using the range
    ///          aux as auxiliary memory
    /// \param [in] src1 : first range to merge
    /// \param [in] src2 : second range to merge
    /// \param [in] aux : auxiliary range used in the merge
    /// \param [in] comp : object for to compare elements
    /// \return true : not changes done
    ///         false : changes in the buffers
    /// \remarks
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Iter3, typename Sent3,
        typename Compare>
    bool in_place_merge_uncontiguous(range<Iter1, Sent1> const& src1,
        range<Iter2, Sent2> const& src2, range<Iter3, Sent3>& aux, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;
        using type3 = hpx::traits::iter_value_t<Iter3>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");
        static_assert(std::is_same_v<type3, type2>, "Incompatible iterators\n");

        return in_place_merge_uncontiguous(src1.begin(), src1.end(),
            src2.begin(), src2.end(), aux.begin(), comp);
    }

    /// \brief : merge two contiguous buffers ( src1, src2) using buf as
    ///          auxiliary memory
    /// \param [in] src1 : first range to merge
    /// \param [in] src2 : second range to merge
    /// \param [in] buf : auxiliary memory used in the merge
    /// \param [in] comp : object for to compare elements
    /// \return true : not changes done
    ///         false : changes in the buffers
    /// \remarks
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    range<Iter1, Sent1> in_place_merge(range<Iter1, Sent1> const& src1,
        range<Iter1, Sent1> const& src2, range<Iter2, Sent2>& buf, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        in_place_merge(src1.begin(), src1.end(), src2.end(), buf.begin(), comp);
        return concat(src1, src2);
    }

    // \brief : merge two contiguous buffers
    // \tparam Iter1 : iterator to the elements
    // \tparam Compare : object for to compare two elements pointed by Iter
    //                   iterators
    // \param [in] first : iterator to the first element
    // \param [in] last : iterator to the element after the last in the range
    // \param [in] comp : object for to compare elements
    // \returns true : not changes done
    //         false : changes in the buffers
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    void merge_flow(range<Iter1, Sent1> rng1, range<Iter2, Sent2> rbuf,
        range<Iter1, Sent1> rng2, Compare cmp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        range<Iter2, Sent2> rbx(rbuf);
        range<Iter1, Sent1> rx1(rng1), rx2(rng2);

        while (rx1.begin() != rx1.end())
        {
            if (cmp(*rbx.begin(), *rx2.begin()))
            {
                *rx1.begin() = std::ranges::iter_move(rbx.begin());
                rbx = range<Iter2, Sent2>(rbx.begin() + 1, rbx.end());
            }
            else
            {
                *rx1.begin() = std::ranges::iter_move(rx2.begin());
                rx2 = range<Iter2, Sent2>(rx2.begin() + 1, rx2.end());
            }
            rx1 = range<Iter1, Sent1>(rx1.begin() + 1, rx1.end());
        }

        if (rx2.begin() == rx2.end())
        {
            return;
        }

        if (rbx.begin() == rbx.end())
        {
            util::init_move(rbuf, rng2);
        }
        else
        {
            util::half_merge(rbuf, rx2, rbx, cmp);
        }
    }
}    // namespace hpx::parallel::util
