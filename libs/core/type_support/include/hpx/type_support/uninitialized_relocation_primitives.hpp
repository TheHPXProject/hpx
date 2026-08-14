//  Copyright (c) 2023 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/type_support/is_contiguous_iterator.hpp>
#include <hpx/type_support/is_relocatable.hpp>
#include <hpx/type_support/is_trivially_relocatable.hpp>
#include <hpx/type_support/relocate_at.hpp>

#include <cstring>    // for memmove
#include <iterator>
#include <tuple>
#include <type_traits>

#if defined(HPX_HAVE_P1144_RELOCATE_AT)
#include <memory>
#endif

namespace hpx::experimental::util {

    namespace detail {    // Utility metafunctions

        HPX_CXX_CORE_EXPORT struct buffer_memcpy_tag
        {
        };

        HPX_CXX_CORE_EXPORT struct for_loop_nothrow_tag
        {
        };

        HPX_CXX_CORE_EXPORT struct for_loop_try_catch_tag
        {
        };

        HPX_CXX_CORE_EXPORT template <typename InIter, typename OutIter,
            bool iterators_are_contiguous_v = false>
        struct relocation_traits
        {
            using in_type = typename std::iterator_traits<InIter>::value_type;
            using out_type = typename std::iterator_traits<OutIter>::value_type;

            constexpr static bool valid_relocation =
                is_relocatable_from_v<out_type, in_type>;
            //     ^^^^ Cannot relocate between unrelated types

            constexpr static bool is_memcpyable =
                is_trivially_relocatable_v<in_type> &&
                // ^^^^ The important check
                !std::is_volatile_v<in_type> && !std::is_volatile_v<out_type>;
            //     ^^^^ Cannot memcpy volatile types

            constexpr static bool is_buffer_memcpyable =
                is_memcpyable && iterators_are_contiguous_v;

            constexpr static bool can_move_construct_nothrow =
                std::is_nothrow_constructible_v<in_type, out_type>;
            // This is to skip the try-catch block
            // type_dst is treated as an rvalue by is_nothrow_constructible

            constexpr static bool is_noexcept_relocatable_v =
                can_move_construct_nothrow || is_memcpyable;
            // If memcpy is not possible, we need to check if the move
            // constructor is noexcept

            // Using a tag to distinguish implementations
            // clang-format off
            using implementation_tag = std::conditional_t<
                is_buffer_memcpyable,
                    buffer_memcpy_tag,
                    std::conditional_t<is_noexcept_relocatable_v,
                        for_loop_nothrow_tag,
                        for_loop_try_catch_tag
                    >
                >;
            // clang-format on
        };
    }    // namespace detail

#if defined(HPX_HAVE_P1144_RELOCATE_AT)
    //////////////////////////////
    // uninitialized_relocate_n //
    //////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename InIter, typename FwdIter,
        typename Size,
        typename Dummy>    // Dummy is used retain the same signature
                           // as the implementation before P1144
    // clang-format off
    std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive(InIter first, Size n,
        FwdIter dst, Dummy) noexcept(
            detail::relocation_traits<InIter, FwdIter>::is_noexcept_relocatable_v)
    // clang-format on
    {
        static_assert(
            detail::relocation_traits<InIter, FwdIter>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        return std::uninitialized_relocate_n(first, n, dst);
    }

    HPX_CXX_CORE_EXPORT template <typename InIter, typename Size,
        typename FwdIter>
    std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive(InIter first,
        Size n, FwdIter dst) noexcept(detail::relocation_traits<InIter,
        FwdIter>::is_noexcept_relocatable_v)
    {
        return uninitialized_relocate_n_primitive(first, n, dst, bool{});
    }

    ////////////////////////////
    // uninitialized_relocate //
    ////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
        typename FwdIter,
        typename Dummy>    // Dummy is used retain the same signature
                           // as the implementation before P1144
    // clang-format off
    std::tuple<InIter, FwdIter> uninitialized_relocate_primitive(InIter first, Sent last,
        FwdIter dst, Dummy) noexcept(
            detail::relocation_traits<InIter, FwdIter>::is_noexcept_relocatable_v)
    // clang-format on
    {
        // TODO CHECK SENT
        static_assert(
            detail::relocation_traits<InIter, FwdIter>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        return std::uninitialized_relocate(first, last, dst);
    }

    HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
        typename FwdIter>
    std::tuple<InIter, FwdIter> uninitialized_relocate_primitive(InIter first,
        Sent last, FwdIter dst) noexcept(detail::relocation_traits<InIter,
        FwdIter>::is_noexcept_relocatable_v)
    {
        return uninitialized_relocate_primitive(first, last, dst, bool{});
    }

    /////////////////////////////////////
    // uninitialized_relocate_backward //
    /////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2,
        typename Dummy>    // Dummy is used retain the same signature
                           // as the implementation before P1144
    // clang-format off
    std::tuple<BiIter1, BiIter2> uninitialized_relocate_backward_primitive(
        BiIter1 first, BiIter1 last,BiIter2 dst_last, Dummy) noexcept(
            detail::relocation_traits<BiIter1, BiIter2>::is_noexcept_relocatable_v)
    // clang-format on
    {
        // TODO CHECK SENT
        static_assert(
            detail::relocation_traits<BiIter1, BiIter2>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        return std::uninitialized_relocate_backward(first, last, dst_last);
    }

    HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2>
    std::tuple<BiIter1, BiIter2> uninitialized_relocate_backward_primitive(
        BiIter1 first, BiIter1 last,
        BiIter2 dst_last) noexcept(detail::relocation_traits<BiIter1,
        BiIter2>::is_noexcept_relocatable_v)
    {
        return uninitialized_relocate_backward_primitive(
            first, last, dst_last, bool{});
    }
#else

    namespace detail {

        //////////////////////////////
        // uninitialized_relocate_n //
        //////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Size,
            typename FwdIter>
        std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive_helper(
            InIter first, Size n, FwdIter dst, buffer_memcpy_tag) noexcept
        {
            if (n != 0)
            {
                std::byte const* first_byte =
                    reinterpret_cast<std::byte const*>(std::addressof(*first));

                std::byte* dst_byte = const_cast<std::byte*>(
                    reinterpret_cast<std::byte const*>(std::addressof(*dst)));

                Size n_bytes = n * sizeof(*first);

                // Ideally we would want to convey to the compiler
                // That the new buffer actually contains objects
                // within their lifetime. But this is not possible
                // with current language features.
                std::memmove(dst_byte, first_byte, n_bytes);

                first += n;
                dst += n;
            }

            return {first, dst};
        }

        HPX_CXX_CORE_EXPORT template <typename InIter, typename Size,
            typename FwdIter>
        // Either the buffer is not contiguous or the types are no-throw
        // move constructible but not trivially relocatable
        std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive_helper(
            InIter first, Size n, FwdIter dst, for_loop_nothrow_tag) noexcept
        {
            for (Size i = 0; i < n; ++first, ++dst, ++i)
            {
                // if the type is trivially relocatable this will be a memcpy
                // otherwise it will be a move + destroy
                hpx::experimental::detail::relocate_at_helper(
                    std::addressof(*first), std::addressof(*dst));
            }

            return {first, dst};
        }

        HPX_CXX_CORE_EXPORT template <typename InIter, typename Size,
            typename FwdIter>
        std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive_helper(
            InIter first, Size n, FwdIter dst, for_loop_try_catch_tag)
        {
            FwdIter original_dst = dst;

            for (Size i = 0; i < n; ++first, ++dst, ++i)
            {
                try
                {
                    // the move + destroy version will be used
                    hpx::experimental::detail::relocate_at_helper(
                        std::addressof(*first), std::addressof(*dst));
                }
                catch (...)
                {
                    // destroy all objects other that the one
                    // that caused the exception
                    // (relocate_at already destroyed that one)

                    // destroy all objects constructed so far
                    std::destroy(original_dst, dst);
                    // destroy all the objects not relocated yet
                    std::destroy_n(++first, n - i - 1);
                    // Note:
                    // using destroy_n instead of destroy + advance
                    // to avoid calculating the distance

                    throw;
                }
            }

            return {first, dst};
        }

        ////////////////////////////
        // uninitialized_relocate //
        ////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename FwdIter>
        std::tuple<InIter, FwdIter> uninitialized_relocate_primitive_helper(
            InIter first, Sent last, FwdIter dst, buffer_memcpy_tag) noexcept
        {
            return uninitialized_relocate_n_primitive_helper(
                first, std::distance(first, last), dst, buffer_memcpy_tag{});
        }

        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename FwdIter>
        // Either the buffer is not contiguous or the types are no-throw
        // move constructible but not trivially relocatable
        std::tuple<InIter, FwdIter> uninitialized_relocate_primitive_helper(
            InIter first, Sent last, FwdIter dst, for_loop_nothrow_tag) noexcept
        {
            for (; first != last; ++first, ++dst)
            {
                // if the type is trivially relocatable this will be a memcpy
                // otherwise it will be a move + destroy
                hpx::experimental::detail::relocate_at_helper(
                    std::addressof(*first), std::addressof(*dst));
            }

            return {first, dst};
        }

        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename FwdIter>
        std::tuple<InIter, FwdIter> uninitialized_relocate_primitive_helper(
            InIter first, Sent last, FwdIter dst, for_loop_try_catch_tag)
        {
            FwdIter original_dst = dst;

            for (; first != last; ++first, ++dst)
            {
                try
                {
                    // the move + destroy version will be used
                    hpx::experimental::detail::relocate_at_helper(
                        std::addressof(*first), std::addressof(*dst));
                }
                catch (...)
                {
                    // destroy all objects other that the one
                    // that caused the exception
                    // (relocate_at already destroyed that one)

                    // destroy all objects constructed so far
                    std::destroy(original_dst, dst);
                    // destroy all the objects not relocated yet
                    std::destroy(++first, last);

                    throw;
                }
            }

            return {first, dst};
        }

        /////////////////////////////////////
        // uninitialized_relocate_backward //
        /////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2>
        std::tuple<BiIter1, BiIter2>
        uninitialized_relocate_backward_primitive_helper(BiIter1 first,
            BiIter1 last, BiIter2 dst_last, buffer_memcpy_tag) noexcept
        {
            // Here we know the iterators are contiguous
            // So calculating the distance and the previous
            // iterator is O(1)
            auto n_objects = std::distance(first, last);
            BiIter2 dst_first = std::prev(dst_last, n_objects);

            uninitialized_relocate_n_primitive_helper(
                first, n_objects, dst_first, buffer_memcpy_tag{});

            return {first, dst_first};
        }

        HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2>
        // Either the buffer is not contiguous or the types are no-throw
        // move constructible but not trivially relocatable
        // dst_last is one past the last element of the destination
        std::tuple<BiIter1, BiIter2>
        uninitialized_relocate_backward_primitive_helper(BiIter1 first,
            BiIter1 last, BiIter2 dst_last, for_loop_nothrow_tag) noexcept
        {
            while (first != last)
            {
                // if the type is trivially relocatable this will be a memcpy
                // otherwise it will be a move + destroy
                std::advance(last, -1);
                std::advance(dst_last, -1);
                hpx::experimental::detail::relocate_at_helper(
                    std::addressof(*last), std::addressof(*dst_last));
            }

            return {last, dst_last};
        }

        HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2>
        std::tuple<BiIter1, BiIter2>
        uninitialized_relocate_backward_primitive_helper(BiIter1 first,
            BiIter1 last, BiIter2 dst_last, for_loop_try_catch_tag)
        {
            BiIter2 original_dst_last = dst_last;

            while (first != last)
            {
                try
                {
                    std::advance(last, -1);
                    std::advance(dst_last, -1);
                    // the move + destroy version will be used
                    hpx::experimental::detail::relocate_at_helper(
                        std::addressof(*last), std::addressof(*dst_last));
                }
                catch (...)
                {
                    // destroy all objects other that the one
                    // that caused the exception
                    // (relocate_at already destroyed that one)

                    // destroy all objects constructed so far
                    std::destroy(++dst_last, original_dst_last);
                    // destroy all the objects not relocated yet
                    std::destroy(first, last);

                    throw;
                }
            }

            return {last, dst_last};
        }
    }    // namespace detail

    //////////////////////////////
    // uninitialized_relocate_n //
    //////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename InIter, typename FwdIter,
        typename Size, typename iterators_are_contiguous_t>
    // clang-format off
    std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive(InIter first, Size n,
        FwdIter dst, iterators_are_contiguous_t) noexcept(
            detail::relocation_traits<InIter, FwdIter>::is_noexcept_relocatable_v)
    // clang-format on
    {
        static_assert(
            detail::relocation_traits<InIter, FwdIter>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        using implementation_tag = typename detail::relocation_traits<InIter,
            FwdIter, iterators_are_contiguous_t::value>::implementation_tag;

        return detail::uninitialized_relocate_n_primitive_helper(
            first, n, dst, implementation_tag{});
    }

    HPX_CXX_CORE_EXPORT template <typename InIter, typename Size,
        typename FwdIter>
    std::tuple<InIter, FwdIter> uninitialized_relocate_n_primitive(InIter first,
        Size n, FwdIter dst) noexcept(detail::relocation_traits<InIter,
        FwdIter>::is_noexcept_relocatable_v)
    {
        using iterators_are_contiguous_default_t =
            std::bool_constant<hpx::traits::is_contiguous_iterator_v<InIter> &&
                hpx::traits::is_contiguous_iterator_v<FwdIter>>;

        return uninitialized_relocate_n_primitive(
            first, n, dst, iterators_are_contiguous_default_t{});
    }

    ////////////////////////////
    // uninitialized_relocate //
    ////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
        typename FwdIter, typename iterators_are_contiguous_t>
    // clang-format off
    std::tuple<InIter, FwdIter> uninitialized_relocate_primitive(InIter first, Sent last,
        FwdIter dst, iterators_are_contiguous_t) noexcept(
            detail::relocation_traits<InIter, FwdIter>::is_noexcept_relocatable_v)
    // clang-format on
    {
        // TODO CHECK SENT
        static_assert(
            detail::relocation_traits<InIter, FwdIter>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        using implementation_tag = typename detail::relocation_traits<InIter,
            FwdIter, iterators_are_contiguous_t::value>::implementation_tag;

        return detail::uninitialized_relocate_primitive_helper(
            first, last, dst, implementation_tag{});
    }

    HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
        typename FwdIter>
    std::tuple<InIter, FwdIter> uninitialized_relocate_primitive(InIter first,
        Sent last, FwdIter dst) noexcept(detail::relocation_traits<InIter,
        FwdIter>::is_noexcept_relocatable_v)
    {
        using iterators_are_contiguous_default_t =
            std::bool_constant<hpx::traits::is_contiguous_iterator_v<InIter> &&
                hpx::traits::is_contiguous_iterator_v<FwdIter>>;

        return uninitialized_relocate_primitive(
            first, last, dst, iterators_are_contiguous_default_t{});
    }

    /////////////////////////////////////
    // uninitialized_relocate_backward //
    /////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2,
        typename iterators_are_contiguous_t>
    std::tuple<BiIter1, BiIter2> uninitialized_relocate_backward_primitive(
        BiIter1 first, BiIter1 last, BiIter2 dst_last,
        iterators_are_contiguous_t) noexcept(detail::relocation_traits<BiIter1,
        BiIter2>::is_noexcept_relocatable_v)
    {
        // TODO CHECK SENT
        static_assert(
            detail::relocation_traits<BiIter1, BiIter2>::valid_relocation,
            "uninitialized_move(first, last, dst) must be well-formed");

        using implementation_tag = typename detail::relocation_traits<BiIter1,
            BiIter2, iterators_are_contiguous_t::value>::implementation_tag;

        return detail::uninitialized_relocate_backward_primitive_helper(
            first, last, dst_last, implementation_tag{});
    }

    HPX_CXX_CORE_EXPORT template <typename BiIter1, typename BiIter2>
    std::tuple<BiIter1, BiIter2> uninitialized_relocate_backward_primitive(
        BiIter1 first, BiIter1 last,
        BiIter2 dst_last) noexcept(detail::relocation_traits<BiIter1,
        BiIter2>::is_noexcept_relocatable_v)
    {
        using iterators_are_contiguous_default_t =
            std::bool_constant<hpx::traits::is_contiguous_iterator_v<BiIter1> &&
                hpx::traits::is_contiguous_iterator_v<BiIter2>>;

        return uninitialized_relocate_backward_primitive(
            first, last, dst_last, iterators_are_contiguous_default_t{});
    }

#endif    // defined(HPX_HAVE_P1144_RELOCATE_AT)

}    // namespace hpx::experimental::util
