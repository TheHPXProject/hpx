//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/forward.hpp>
#include <hpx/config/move.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    // \brief create an object in the memory specified by ptr
    // \tparam Value : typename of the object to create
    // \tparam Args : parameters for the constructor
    // \param [in] ptr : pointer to the memory where to create the object
    // \param [in] args : arguments to the constructor
    HPX_CXX_CORE_EXPORT template <typename Value, typename... Args>
    void construct_object(Value* ptr, Args&&... args) noexcept(
        noexcept(hpx::construct_at(ptr, HPX_FORWARD(Args, args)...)))
    {
        hpx::construct_at(ptr, HPX_FORWARD(Args, args)...);
    }

    // \brief destroy an object in the memory specified by ptr
    // \tparam Value : typename of the object to create
    // \param [in] ptr : pointer to the object to destroy
    HPX_CXX_CORE_EXPORT template <typename Value>
    void destroy_object(Value* ptr) noexcept
    {
        std::destroy_at(ptr);
    }

    // Initialize a range of objects with the object val moving across them
    // \param [in] first : range to initialize
    // \param [in] last : range to initialize
    // \param [in] r : range of elements not initialized
    // \param [in] val : object used for the initialization
    // \returns range initialized
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    void init(Iter first, Sent last, hpx::traits::iter_value_t<Iter>& val)
    {
        if (first == last)
        {
            return;
        }

        construct_object(std::addressof(*first), HPX_MOVE(val));
        Iter it1 = first, it2 = first + 1;
        while (it2 != last)
        {
            // NOLINTNEXTLINE(bugprone-macro-repeated-side-effects)
            construct_object(
                std::addressof(*it2++), std::ranges::iter_move(it1++));
        }

        val = std::ranges::iter_move(last - 1);
    }

    // \brief create an object in the memory specified by ptr
    // \tparam Value : typename of the object to create
    // \tparam Args : parameters for the constructor
    // \param [in] ptr : pointer to the memory where to create the object
    // \param [in] args : arguments to the constructor
    HPX_CXX_CORE_EXPORT template <typename Value, typename... Args>
    void construct(Value* ptr, Args&&... args) noexcept(
        noexcept(hpx::construct_at(ptr, HPX_FORWARD(Args, args)...)))
    {
        hpx::construct_at(ptr, HPX_FORWARD(Args, args)...);
    }

    // \brief Move objects
    // \tparam Iter : iterator to the elements
    // \tparam Value : typename of the object to create
    // \param [in] it_dest : iterator to the final place of the objects
    // \param [in] R : range to move
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2>
    Iter2 init_move(Iter2 it_dest, Iter1 first, Sent1 last)
    {
        while (first != last)
        {
            // NOLINTNEXTLINE(bugprone-macro-repeated-side-effects)
            *it_dest++ = std::ranges::iter_move(first++);
        }
        return it_dest;
    }

    // \brief Move objects to uninitialized memory
    // \tparam Iter : iterator to the elements
    // \tparam Value : typename of the object to construct
    // \param [in] ptr : pointer to the memory where to create the object
    // \param [in] R : range to move
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
        typename Value = hpx::traits::iter_value_t<Iter>>
    Value* uninit_move(Value* ptr, Iter first, Sent last)
    {
        using value_type = hpx::traits::iter_value_t<Iter>;

        static_assert(
            std::is_same_v<Value, value_type>, "Incompatible iterators\n");

        while (first != last)
        {
            // NOLINTNEXTLINE(bugprone-macro-repeated-side-effects)
            hpx::construct_at(ptr++, std::ranges::iter_move(first++));
        }

        return ptr;
    }

    // \brief Move objects to uninitialized memory
    // \tparam Iter : iterator to the elements
    // \tparam Value : typename of the object to construct
    // \param [in] first : range to initialize
    // \param [in] last : range to initialize
    // \param [in] ptr : pointer to the memory where to construct the object
    // \param [in] R : range to move
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    void destroy(Iter first, Sent last) noexcept
    {
        while (first != last)
        {
            std::destroy_at(&*first++);
        }
    }

    // \brief Merge two contiguous buffers pointed by buf1 and buf2 , and put
    //        in the buffer pointed by buf_out
    // \param [in] buf1 : iterator to the first element in the first buffer
    // \param [in] end_buf1 : final iterator of first buffer
    // \param [in] buf2 : iterator to the first iterator to the second buffer
    // \param [in] end_buf2 : final iterator of the second buffer
    // \param [in] buf_out : buffer where move the elements merged
    // \param [in] comp : comparison object
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Compare>
    Iter2 full_merge(Iter1 buf1, Sent1 end_buf1, Iter1 buf2, Sent1 end_buf2,
        Iter2 buf_out, Compare comp)
    {
        using value1_t = hpx::traits::iter_value_t<Iter1>;
        using value2_t = hpx::traits::iter_value_t<Iter2>;

        static_assert(
            std::is_same_v<value1_t, value2_t>, "Incompatible iterators\n");

        while (buf1 != end_buf1 && buf2 != end_buf2)
        {
            *buf_out++ = !comp(*buf2, *buf1) ? std::ranges::iter_move(buf1++) :
                                               std::ranges::iter_move(buf2++);
        }
        return buf1 == end_buf1 ? init_move(buf_out, buf2, end_buf2) :
                                  init_move(buf_out, buf1, end_buf1);
    }

    // \brief Merge two contiguous buffers pointed by first1 and first2 , and put
    //        in the uninitialized buffer pointed by it_out
    // \param [in] first1 : iterator to the first element in the first buffer
    // \param [in] last : last iterator of the first buffer
    // \param [in] first2 : iterator to the first element to the second buffer
    // \param [in] last22 : final iterator of the second buffer
    // \param [in] it_out : uninitialized buffer where move the elements merged
    // \param [in] comp : comparison object
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent, typename Value,
        typename Compare>
    Value* uninit_full_merge(Iter first1, Sent last1, Iter first2, Sent last2,
        Value* it_out, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter>;

        static_assert(std::is_same_v<Value, type1>, "Incompatible iterators\n");

        while (first1 != last1 && first2 != last2)
        {
            construct(it_out++,
                !comp(*first2, *first1) ? std::ranges::iter_move(first1++) :
                                          std::ranges::iter_move(first2++));
        };
        return first1 == last1 ? uninit_move(it_out, first2, last2) :
                                 uninit_move(it_out, first1, last1);
    }

    // \brief : Merge two buffers. The first buffer is in a separate memory.
    //          The second buffer have a empty space before buf2 of the same size
    //          than the (end_buf1 - buf1)
    // \param [in] buf1 : iterator to the first element of the first buffer
    // \param [in] end_buf1 : iterator to the last element of the first buffer
    // \param [in] buf2 : iterator to the first element of the second buffer
    // \param [in] end_buf2 : iterator to the last element of the second buffer
    // \param [in] buf_out : iterator to the first element to the buffer where put
    //                       the result
    // \param [in] comp : object for Compare two elements of the type pointed
    //                    by the Iter1 and Iter2
    // \note The elements pointed by Iter1 and Iter2 must be the same
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Compare>
    Iter2 half_merge(Iter1 buf1, Sent1 end_buf1, Iter2 buf2, Sent2 end_buf2,
        Iter2 buf_out, Compare comp)
    {
        using value1_t = hpx::traits::iter_value_t<Iter1>;
        using value2_t = hpx::traits::iter_value_t<Iter2>;

        static_assert(
            std::is_same_v<value1_t, value2_t>, "Incompatible iterators\n");

        while (buf1 != end_buf1 && buf2 != end_buf2)
        {
            *buf_out++ = !comp(*buf2, *buf1) ? std::ranges::iter_move(buf1++) :
                                               std::ranges::iter_move(buf2++);
        }
        return buf2 == end_buf2 ? init_move(buf_out, buf1, end_buf1) : end_buf2;
    }

    // Merge two non contiguous buffers, placing the results in the buffers
    //          for to do this use an auxiliary buffer pointed by aux
    // \param [in] src1 : iterator to the first element of the first buffer
    // \param [in] end_src1 : last iterator  of the first buffer
    // \param [in] src2 : iterator to the first element of the second buffer
    // \param [in] end_src2 : last iterator  of the second buffer
    // \param [in] aux  : iterator to the first element of the auxiliary buffer
    // \param [in] comp : object for to Compare elements
    // \exception
    // \returns true : not changes done
    //         false : changes in the buffers
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Iter3, typename Compare>
    bool in_place_merge_uncontiguous(Iter1 src1, Sent1 end_src1, Iter2 src2,
        Sent2 end_src2, Iter3 aux, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;
        using type3 = hpx::traits::iter_value_t<Iter3>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");
        static_assert(std::is_same_v<type3, type2>, "Incompatible iterators\n");

        if (src1 == end_src1 || src2 == end_src2 ||
            !comp(*src2, *(end_src1 - 1)))
        {
            return true;
        }

        while (src1 != end_src1 && !comp(*src2, *src1))
        {
            ++src1;
        }

        Iter3 const end_aux = aux + (end_src1 - src1);
        Iter2 src2_first = src2;
        init_move(aux, src1, end_src1);

        while (src1 != end_src1 && src2 != end_src2)
        {
            *src1++ = std::move(!comp(*src2, *aux) ? *aux++ : *src2++);
        }

        if (src2 == end_src2)
        {
            while (src1 != end_src1)
            {
                *src1++ = std::ranges::iter_move(aux++);
            }
            init_move(src2_first, aux, end_aux);
        }
        else
        {
            half_merge(aux, end_aux, src2, end_src2, src2_first, comp);
        }
        return false;
    }

    // \brief : merge two contiguous buffers,using an auxiliary buffer pointed
    //          by buf
    //
    // \param [in] src1: iterator to the first position of the first buffer
    // \param [in] src2: final iterator of the first buffer and first iterator
    //                   of the second buffer
    // \param [in] end_src2 : final iterator of the second buffer
    // \param [in] buf  : iterator to buffer used as auxiliary memory
    // \param [in] comp : object for to Compare elements
    // \exception
    // \returns true : not changes done
    //         false : changes in the buffers
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
        typename Iter2, typename Compare>
    bool in_place_merge(
        Iter1 src1, Iter1 src2, Sent1 end_src2, Iter2 buf, Compare comp)
    {
        using type1 = hpx::traits::iter_value_t<Iter1>;
        using type2 = hpx::traits::iter_value_t<Iter2>;

        static_assert(std::is_same_v<type1, type2>, "Incompatible iterators\n");

        if (src1 == src2 || src2 == end_src2 || !comp(*src2, *(src2 - 1)))
        {
            return true;
        }

        Iter1 end_src1 = src2;
        while (src1 != end_src1 && !comp(*src2, *src1))
        {
            ++src1;
        }

        if (src1 == end_src1)
        {
            return false;
        }

        size_t nx = end_src1 - src1;
        init_move(buf, src1, end_src1);
        half_merge(buf, buf + nx, src2, end_src2, src1, comp);
        return false;
    }
}    // namespace hpx::parallel::util
