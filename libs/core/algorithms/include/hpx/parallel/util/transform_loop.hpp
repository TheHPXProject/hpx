//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct transform_loop
        {
            template <typename InIterB, typename InIterE, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_out_result<InIterB,
                    OutIter>
                call(InIterB start, InIterE sent, OutIter out, F&& f)
            {
                if constexpr (std::random_access_iterator<InIterB>)
                {
                    auto end = start;
                    auto const len =
                        parallel::detail::advance_and_get_distance(end, sent);

                    if (len == 0)
                    {
                        return {start, out};
                    }

                    auto first = hpx::util::get_unwrapped(start);
                    auto dest = hpx::util::get_unwrapped(out);

                    for (auto count = len; count != 0;
                        (void) ++first, ++dest, --count)
                    {
                        *dest = HPX_INVOKE(f, first);
                    }

                    if constexpr (!std::input_iterator<OutIter>)
                    {
                        return {end, dest};
                    }
                    else
                    {
                        return {end, std::next(out, len)};
                    }
                }
                else
                {
                    for (/* */; start != sent; (void) ++start, ++sent)
                    {
                        *out = HPX_INVOKE(f, start);
                    }
                    return {start, out};
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct transform_loop_t final
      : hpx::detail::tag_dispatch<transform_loop_t, hpx::detail::no_base>
    {
        template <typename ExPolicy, typename IterB, typename IterE,
            typename OutIter, typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr util::in_out_result<IterB, OutIter>
            invoke_default(ExPolicy&&, IterB it, IterE end, OutIter dest, F&& f)
        {
            return detail::transform_loop<IterB>::call(
                it, end, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr transform_loop_t transform_loop =
        transform_loop_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename IterB,
        typename IterE, typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        transform_loop(
            ExPolicy&& policy, IterB it, IterE end, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_loop_t{}(
            HPX_FORWARD(ExPolicy, policy), it, end, dest, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct transform_loop_ind
        {
            template <typename InIterB, typename InIterE, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_out_result<InIterB,
                    OutIter>
                call(InIterB start, InIterE sent, OutIter out, F&& f)
            {
                if constexpr (std::random_access_iterator<InIterB>)
                {
                    auto end = start;
                    auto const len =
                        parallel::detail::advance_and_get_distance(end, sent);

                    if (len == 0)
                    {
                        return {start, out};
                    }

                    auto first = hpx::util::get_unwrapped(start);
                    auto dest = hpx::util::get_unwrapped(out);

                    for (auto count = len; count != 0;
                        (void) ++first, ++dest, --count)
                    {
                        *dest = HPX_INVOKE(f, *first);
                    }

                    if constexpr (!std::input_iterator<OutIter>)
                    {
                        return {end, dest};
                    }
                    else
                    {
                        return {end, std::next(out, len)};
                    }
                }
                else
                {
                    for (/* */; start != sent; (void) ++start, ++out)
                    {
                        *out = HPX_INVOKE(f, *start);
                    }
                    return {start, out};
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct transform_loop_ind_t final
      : hpx::detail::tag_dispatch<transform_loop_ind_t, hpx::detail::no_base>
    {
        template <typename ExPolicy, typename IterB, typename IterE,
            typename OutIter, typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr util::in_out_result<IterB, OutIter>
            invoke_default(ExPolicy&&, IterB it, IterE end, OutIter dest, F&& f)
        {
            return detail::transform_loop_ind<IterB>::call(
                it, end, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr transform_loop_ind_t
        transform_loop_ind = transform_loop_ind_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename IterB,
        typename IterE, typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        transform_loop_ind(
            ExPolicy&& policy, IterB it, IterE end, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_loop_ind_t{}(
            HPX_FORWARD(ExPolicy, policy), it, end, dest, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct transform_binary_loop
        {
            template <typename InIter1B, typename InIter1E, typename InIter2,
                typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::
                in_in_out_result<InIter1B, InIter2, OutIter>
                call(InIter1B first1, InIter1E last1, InIter2 first2,
                    OutIter dest, F&& f)
            {
                for (/* */; first1 != last1; (void) ++first1, ++first2, ++dest)
                {
                    *dest = HPX_INVOKE(f, first1, first2);
                }
                return {first1, first2, dest};
            }

            template <typename InIter1B, typename InIter1E, typename InIter2,
                typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::
                in_in_out_result<InIter1B, InIter2, OutIter>
                call(InIter1B first1, InIter1E last1, InIter2 first2,
                    InIter2 last2, OutIter dest, F&& f)
            {
                for (/* */; first1 != last1 && first2 != last2;
                    (void) ++first1, ++first2, ++dest)
                {
                    *dest = HPX_INVOKE(f, first1, first2);
                }
                return {first1, first2, dest};
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_binary_loop_t final
      : hpx::detail::tag_dispatch<transform_binary_loop_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1B, typename InIter1E, typename InIter2,
            typename OutIter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::in_in_out_result<
            InIter1B, InIter2, OutIter>
        invoke_default(InIter1B first1, InIter1E last1, InIter2 first2,
            OutIter dest, F&& f)
        {
            return detail::transform_binary_loop<InIter1B, InIter2>::call(
                first1, last1, first2, dest, HPX_FORWARD(F, f));
        }

        template <typename InIter1B, typename InIter1E, typename InIter2B,
            typename InIter2E, typename OutIter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::in_in_out_result<
            InIter1B, InIter2B, OutIter>
        invoke_default(InIter1B first1, InIter1E last1, InIter2B first2,
            InIter2E last2, OutIter dest, F&& f)
        {
            return detail::transform_binary_loop<InIter1B, InIter2B>::call(
                first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_binary_loop_t<ExPolicy> transform_binary_loop =
        transform_binary_loop_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1B,
        typename InIter1E, typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1B,
        InIter2, OutIter>
    transform_binary_loop(
        InIter1B first1, InIter1E last1, InIter2 first2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_t<ExPolicy>{}(
            first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1B,
        typename InIter1E, typename InIter2B, typename InIter2E,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1B,
        InIter2B, OutIter>
    transform_binary_loop(InIter1B first1, InIter1E last1, InIter2B first2,
        InIter2E last2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_t<ExPolicy>{}(
            first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct transform_binary_loop_ind
        {
            template <typename InIter1B, typename InIter1E, typename InIter2,
                typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::
                in_in_out_result<InIter1B, InIter2, OutIter>
                call(InIter1B start1, InIter1E sent1, InIter2 start2,
                    OutIter out, F&& f)
            {
                if constexpr (std::random_access_iterator<InIter1B>)
                {
                    auto end1 = start1;
                    auto const len =
                        parallel::detail::advance_and_get_distance(end1, sent1);

                    if (len == 0)
                    {
                        return {start1, start2, out};
                    }

                    auto first1 = hpx::util::get_unwrapped(start1);
                    auto first2 = hpx::util::get_unwrapped(start2);
                    auto dest = hpx::util::get_unwrapped(out);

                    for (auto count = len; count != 0;
                        (void) ++first1, ++first2, ++dest, --count)
                    {
                        *dest = HPX_INVOKE(f, *first1, *first2);
                    }

                    if constexpr (!std::input_iterator<OutIter>)
                    {
                        return {end1, std::next(start2, len), dest};
                    }
                    else
                    {
                        return {
                            end1, std::next(start2, len), std::next(out, len)};
                    }
                }
                else
                {
                    for (/* */; start1 != sent1;
                        (void) ++start1, ++start2, ++out)
                    {
                        *out = HPX_INVOKE(f, *start1, *start2);
                    }

                    return {start1, start2, out};
                }
            }

            template <typename InIter1B, typename InIter1E, typename InIter2,
                typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::
                in_in_out_result<InIter1B, InIter2, OutIter>
                call(InIter1B start1, InIter1E sent1, InIter2 start2,
                    InIter2 sent2, OutIter out, F&& f)
            {
                if constexpr (std::random_access_iterator<InIter1B>)
                {
                    auto end1 = start1;
                    auto const len1 =
                        parallel::detail::advance_and_get_distance(end1, sent1);

                    auto end2 = start2;
                    auto const len2 =
                        parallel::detail::advance_and_get_distance(end2, sent2);

                    auto len = (std::min) (len1, len2);
                    if (len == 0)
                    {
                        return {start1, start2, out};
                    }

                    auto first1 = hpx::util::get_unwrapped(start1);
                    auto first2 = hpx::util::get_unwrapped(start2);
                    auto dest = hpx::util::get_unwrapped(out);

                    for (auto count = len; count != 0;
                        (void) ++first1, ++first2, ++dest, --count)
                    {
                        *dest = HPX_INVOKE(f, *first1, *first2);
                    }

                    if constexpr (!std::input_iterator<OutIter>)
                    {
                        return {end1, end2, dest};
                    }
                    else
                    {
                        return {end1, end2, std::next(out, len)};
                    }
                }
                else
                {
                    for (/* */; start1 != sent1 && start2 != sent2;
                        (void) ++start1, ++start2, ++out)
                    {
                        *out = HPX_INVOKE(f, *start1, *start2);
                    }
                    return {start1, start2, out};
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_binary_loop_ind_t final
      : hpx::detail::tag_dispatch<transform_binary_loop_ind_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1B, typename InIter1E, typename InIter2,
            typename OutIter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::in_in_out_result<
            InIter1B, InIter2, OutIter>
        invoke_default(InIter1B first1, InIter1E last1, InIter2 first2,
            OutIter dest, F&& f)
        {
            return detail::transform_binary_loop_ind<InIter1B, InIter2>::call(
                first1, last1, first2, dest, HPX_FORWARD(F, f));
        }

        template <typename InIter1B, typename InIter1E, typename InIter2B,
            typename InIter2E, typename OutIter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr util::in_in_out_result<
            InIter1B, InIter2B, OutIter>
        invoke_default(InIter1B first1, InIter1E last1, InIter2B first2,
            InIter2E last2, OutIter dest, F&& f)
        {
            return detail::transform_binary_loop_ind<InIter1B, InIter2B>::call(
                first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_binary_loop_ind_t<ExPolicy>
        transform_binary_loop_ind = transform_binary_loop_ind_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1B,
        typename InIter1E, typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1B,
        InIter2, OutIter>
    transform_binary_loop_ind(
        InIter1B first1, InIter1E last1, InIter2 first2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>{}(
            first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1B,
        typename InIter1E, typename InIter2B, typename InIter2E,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1B,
        InIter2B, OutIter>
    transform_binary_loop_ind(InIter1B first1, InIter1E last1, InIter2B first2,
        InIter2E last2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>{}(
            first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
    }
#endif

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct transform_loop_n
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter it, std::size_t num, OutIter dest, F&& f,
                    std::false_type)
            {
                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                for (std::size_t i = 0; i < count;
                    (void) ++it, i += 4)    // -V112
                {
                    *dest++ = HPX_INVOKE(f, it);
                    *dest++ = HPX_INVOKE(f, ++it);
                    *dest++ = HPX_INVOKE(f, ++it);
                    *dest++ = HPX_INVOKE(f, ++it);
                }

                for (/**/; count < num; (void) ++count, ++it, ++dest)
                {
                    *dest = HPX_INVOKE(f, it);
                }

                return std::make_pair(it, dest);
            }

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter it, std::size_t num, OutIter dest, F&& f,
                    std::true_type)
            {
                while (num >= 4)
                {
                    *dest++ = HPX_INVOKE(f, it);
                    *dest++ = HPX_INVOKE(f, it + 1);
                    *dest++ = HPX_INVOKE(f, it + 2);
                    *dest++ = HPX_INVOKE(f, it + 3);

                    it += 4;
                    num -= 4;
                }

                switch (num)
                {
                case 3:
                    *dest++ = HPX_INVOKE(f, it);
                    *dest++ = HPX_INVOKE(f, it + 1);
                    *dest++ = HPX_INVOKE(f, it + 2);
                    it += 3;
                    break;

                case 2:
                    *dest++ = HPX_INVOKE(f, it);
                    *dest++ = HPX_INVOKE(f, it + 1);
                    it += 2;
                    break;

                case 1:
                    *dest++ = HPX_INVOKE(f, it);
                    ++it;
                    break;

                default:
                    break;
                }

                return std::make_pair(it, dest);
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_loop_n_t final
      : hpx::detail::tag_dispatch<transform_loop_n_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename Iter, typename OutIter, typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr std::pair<Iter, OutIter>
            invoke_default(Iter it, std::size_t count, OutIter dest, F&& f)
        {
            using pred = std::bool_constant<std::random_access_iterator<Iter>>;

            return detail::transform_loop_n<Iter>::call(
                it, count, dest, HPX_FORWARD(F, f), pred());
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_loop_n_t<ExPolicy> transform_loop_n =
        transform_loop_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::pair<Iter, OutIter>
    transform_loop_n(Iter it, std::size_t count, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_loop_n_t<ExPolicy>{}(
            it, count, dest, HPX_FORWARD(F, f));
    }
#endif

    namespace detail {

        template <typename Iter>
        struct transform_loop_n_ind
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter it, std::size_t num, OutIter dest, F&& f,
                    std::false_type)
            {
                if (num == 0)
                {
                    return {it, dest};
                }

                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                for (std::size_t i = 0; i < count;
                    (void) ++it, i += 4)    // -V112
                {
                    *dest++ = HPX_INVOKE(f, *it);
                    *dest++ = HPX_INVOKE(f, *++it);
                    *dest++ = HPX_INVOKE(f, *++it);
                    *dest++ = HPX_INVOKE(f, *++it);
                }

                for (/**/; count < num; (void) ++count, ++it, ++dest)
                {
                    *dest = HPX_INVOKE(f, *it);
                }

                return {it, dest};
            }

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter start, std::size_t num, OutIter out, F&& f,
                    std::true_type)
            {
                if (num == 0)
                {
                    return {start, out};
                }

                auto it = hpx::util::get_unwrapped(start);
                auto dest = hpx::util::get_unwrapped(out);

                while (num >= 4)
                {
                    *dest++ = HPX_INVOKE(f, *it);
                    *dest++ = HPX_INVOKE(f, *(it + 1));
                    *dest++ = HPX_INVOKE(f, *(it + 2));
                    *dest++ = HPX_INVOKE(f, *(it + 3));

                    it += 4;
                    num -= 4;
                }

                switch (num)
                {
                case 3:
                    *dest++ = HPX_INVOKE(f, *it);
                    *dest++ = HPX_INVOKE(f, *(it + 1));
                    *dest++ = HPX_INVOKE(f, *(it + 2));
                    break;

                case 2:
                    *dest++ = HPX_INVOKE(f, *it);
                    *dest++ = HPX_INVOKE(f, *(it + 1));
                    break;

                case 1:
                    *dest++ = HPX_INVOKE(f, *it);
                    break;

                default:
                    break;
                }

                if constexpr (!std::input_iterator<OutIter>)
                {
                    return {std::next(start, num), dest};
                }
                else
                {
                    return {std::next(start, num), std::next(out, num)};
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_loop_n_ind_t final
      : hpx::detail::tag_dispatch<transform_loop_n_ind_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename Iter, typename OutIter, typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr std::pair<Iter, OutIter>
            invoke_default(Iter it, std::size_t count, OutIter dest, F&& f)
        {
            using pred = std::bool_constant<std::random_access_iterator<Iter>>;

            return detail::transform_loop_n_ind<Iter>::call(
                it, count, dest, HPX_FORWARD(F, f), pred());
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_loop_n_ind_t<ExPolicy> transform_loop_n_ind =
        transform_loop_n_ind_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::pair<Iter, OutIter>
    transform_loop_n_ind(Iter it, std::size_t count, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_loop_n_ind_t<ExPolicy>{}(
            it, count, dest, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Inter2>
        struct transform_binary_loop_n
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
                InIter2, OutIter>
            call(InIter1 first1, std::size_t num, InIter2 first2, OutIter dest,
                F&& f)
            {
                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                for (std::size_t i = 0; i < count;
                    (void) ++first1, ++first2, i += 4)    // -V112
                {
                    *dest++ = HPX_INVOKE(f, first1, first2);
                    *dest++ = HPX_INVOKE(f, ++first1, ++first2);
                    *dest++ = HPX_INVOKE(f, ++first1, ++first2);
                    *dest++ = HPX_INVOKE(f, ++first1, ++first2);
                }

                for (/**/; count < num;
                    (void) ++count, ++first1, ++first2, ++dest)
                {
                    *dest = HPX_INVOKE(f, first1, first2);
                }

                return hpx::make_tuple(first1, first2, dest);
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_binary_loop_n_t final
      : hpx::detail::tag_dispatch<transform_binary_loop_n_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1, typename InIter2, typename OutIter,
            typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
            InIter2, OutIter>
        invoke_default(InIter1 first1, std::size_t count, InIter2 first2,
            OutIter dest, F&& f)
        {
            return detail::transform_binary_loop_n<InIter1, InIter2>::call(
                first1, count, first2, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_binary_loop_n_t<ExPolicy>
        transform_binary_loop_n = transform_binary_loop_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr hpx::tuple<InIter1, InIter2, OutIter>
        transform_binary_loop_n(InIter1 first1, std::size_t count,
            InIter2 first2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_n_t<ExPolicy>{}(
            first1, count, first2, dest, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Inter2>
        struct transform_binary_loop_ind_n
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
                InIter2, OutIter>
            call_iteration(InIter1 first1, std::size_t num, InIter2 first2,
                OutIter dest, F&& f)
            {
                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                for (std::size_t i = 0; i < count;
                    (void) ++first1, ++first2, i += 4)    // -V112
                {
                    *dest++ = HPX_INVOKE(f, *first1, *first2);
                    *dest++ = HPX_INVOKE(f, *++first1, *++first2);
                    *dest++ = HPX_INVOKE(f, *++first1, *++first2);
                    *dest++ = HPX_INVOKE(f, *++first1, *++first2);
                }

                for (/**/; count < num;
                    (void) ++count, ++first1, ++first2, ++dest)
                {
                    *dest = HPX_INVOKE(f, *first1, *first2);
                }

                return hpx::make_tuple(first1, first2, dest);
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
                InIter2, OutIter>
            call(InIter1 start1, std::size_t num, InIter2 start2, OutIter out,
                F&& f)
            {
                if (num == 0)
                {
                    return hpx::make_tuple(start1, start2, out);
                }

                if constexpr (std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2>)
                {
                    auto first1 = hpx::util::get_unwrapped(start1);
                    auto first2 = hpx::util::get_unwrapped(start2);
                    auto dest = hpx::util::get_unwrapped(out);

                    call_iteration(
                        first1, num, first2, dest, HPX_FORWARD(F, f));

                    if constexpr (!std::input_iterator<OutIter>)
                    {
                        return hpx::make_tuple(std::next(start1, num),
                            std::next(start2, num), dest);
                    }
                    else
                    {
                        return hpx::make_tuple(std::next(start1, num),
                            std::next(start2, num), std::next(out, num));
                    }
                }
                else
                {
                    return call_iteration(
                        start1, num, start2, out, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct transform_binary_loop_ind_n_t final
      : hpx::detail::tag_dispatch<transform_binary_loop_ind_n_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1, typename InIter2, typename OutIter,
            typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
            InIter2, OutIter>
        invoke_default(InIter1 first1, std::size_t count, InIter2 first2,
            OutIter dest, F&& f)
        {
            return detail::transform_binary_loop_ind_n<InIter1, InIter2>::call(
                first1, count, first2, dest, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr transform_binary_loop_ind_n_t<ExPolicy>
        transform_binary_loop_ind_n = transform_binary_loop_ind_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr hpx::tuple<InIter1, InIter2, OutIter>
        transform_binary_loop_ind_n(InIter1 first1, std::size_t count,
            InIter2 first2, OutIter dest, F&& f)
    {
        return hpx::parallel::util::transform_binary_loop_ind_n_t<ExPolicy>{}(
            first1, count, first2, dest, HPX_FORWARD(F, f));
    }
#endif
}    // namespace hpx::parallel::util
