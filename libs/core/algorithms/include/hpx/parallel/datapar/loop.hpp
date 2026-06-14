//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Helper class to repeatedly call a function starting from a given
        // iterator position.
        HPX_CXX_CORE_EXPORT template <typename Iterator, bool IsConst = false>
        struct datapar_loop
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<Begin>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Begin>;

                if constexpr (is_contiguous && datapar_compatible)
                {
                    while (first != last && !is_data_aligned(first))
                    {
                        datapar_loop_step<Begin, IsConst>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    while (last - first > static_cast<std::ptrdiff_t>(size + 1))
                    {
                        datapar_loop_step<Begin, IsConst>::callv(f, first);
                    }

                    while (first != last)
                    {
                        datapar_loop_step<Begin, IsConst>::call1(f, first);
                    }
                    return first;
                }
                else if constexpr (datapar_compatible)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    End const lastV = last - size + 1;
                    while (first < lastV)
                    {
                        datapar_loop_step<Begin, IsConst>::calls(f, first);
                    }

                    while (first != last)
                    {
                        datapar_loop_step<Begin, IsConst>::call1(f, first);
                    }
                    return first;
                }
                else
                {
                    while (first != last)
                    {
                        datapar_loop_step<Begin, IsConst>::call1(f, first);
                    }
                    return first;
                }
            }

            template <typename Begin, typename End, typename CancelToken,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return first;

                return call(first, last, HPX_FORWARD(F, f));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // Helper class to repeatedly call a function starting from a given
        // iterator position till the predicate returns true.
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_loop_pred
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename Begin, typename End, typename Pred>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, Pred&& pred)
            {
                while (first != last && !is_data_aligned(first))
                {
                    if (datapar_loop_pred_step<Begin>::call1(pred, first) != -1)
                        return first;
                    ++first;
                }

                constexpr std::size_t size = traits::vector_pack_size_v<V>;

                while (last - first > static_cast<std::ptrdiff_t>(size + 1))
                {
                    int offset =
                        datapar_loop_pred_step<Begin>::callv(pred, first);
                    if (offset != -1)
                    {
                        std::advance(first, offset);
                        return first;
                    }
                    std::advance(first, size);
                }

                while (first != last)
                {
                    if (datapar_loop_pred_step<Begin>::call1(pred, first) != -1)
                        return first;
                    ++first;
                }

                return first;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_loop_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<Begin>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Begin>;

                if constexpr (is_contiguous && datapar_compatible)
                {
                    while (first != last && !is_data_aligned(first))
                    {
                        datapar_loop_step_ind<Begin>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    while (last - first > static_cast<std::ptrdiff_t>(size + 1))
                    {
                        datapar_loop_step_ind<Begin>::callv(f, first);
                    }

                    while (first != last)
                    {
                        datapar_loop_step_ind<Begin>::call1(f, first);
                    }

                    return first;
                }
                else if constexpr (datapar_compatible)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    End const lastV = last - size + 1;
                    while (first < lastV)
                    {
                        datapar_loop_step_ind<Begin>::calls(f, first);
                    }

                    while (first != last)
                    {
                        datapar_loop_step_ind<Begin>::call1(f, first);
                    }

                    return first;
                }
                else
                {
                    while (first != last)
                    {
                        datapar_loop_step_ind<Begin>::call1(f, first);
                    }
                    return first;
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct datapar_loop2
        {
            template <typename InIter1, typename InIter2, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr std::enable_if_t<
                iterators_datapar_compatible_v<InIter1, InIter2> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2>,
                std::pair<InIter1, InIter2>>
            call(InIter1 it1, InIter1 last1, InIter2 it2, F&& f)
            {
                using iterator_type = std::decay_t<InIter1>;
                using value_type =
                    std::iterator_traits<iterator_type>::value_type;

                using V = traits::vector_pack_type_t<value_type>;

                while (it1 != last1 &&
                    (!is_data_aligned(it1) || !is_data_aligned(it2)))
                {
                    datapar_loop_step2_ind<InIter1, InIter2>::call1(
                        f, it1, it2);
                }

                constexpr std::size_t size = traits::vector_pack_size_v<V>;

                while (last1 - it1 > static_cast<std::ptrdiff_t>(size + 1))
                {
                    datapar_loop_step2_ind<InIter1, InIter2>::callv(
                        f, it1, it2);
                }

                while (it1 != last1)
                {
                    datapar_loop_step2_ind<InIter1, InIter2>::call1(
                        f, it1, it2);
                }

                return std::make_pair(HPX_MOVE(it1), HPX_MOVE(it2));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, bool IsConst = false,
            typename Enable = void>
        struct datapar_loop_n;

        template <typename Iterator, bool IsConst>
        struct datapar_loop_n<Iterator, IsConst,
            std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr InIter call(
                InIter first, std::size_t count, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<InIter>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<InIter>;

                if constexpr (is_contiguous && datapar_compatible)
                {
                    std::size_t len = count;

                    // clang-format off
                    for (/* */; len != 0 && !detail::is_data_aligned(first);
                        --len)
                    {
                        datapar_loop_step<InIter, IsConst>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    while (len > size + 1)
                    {
                        datapar_loop_step<InIter, IsConst>::callv(f, first);
                        len -= size;
                    }
                    // clang-format on

                    for (/* */; len != 0; --len)
                    {
                        datapar_loop_step<InIter, IsConst>::call1(f, first);
                    }

                    return first;
                }
                else if constexpr (datapar_compatible)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    // clang-format off
                    for (auto len_v =
                             static_cast<std::int64_t>(count - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), count -= size)
                    {
                        datapar_loop_step<InIter, IsConst>::calls(f, first);
                    }
                    // clang-format on

                    for (/* */; count != 0; --count)
                    {
                        datapar_loop_step<InIter, IsConst>::call1(f, first);
                    }

                    return first;
                }
                else
                {
                    for (/* */; count != 0; --count)
                    {
                        datapar_loop_step<InIter, IsConst>::call1(f, first);
                    }
                    return first;
                }
            }

            template <typename InIter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr InIter call(
                InIter first, std::size_t count, CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return first;

                return call(first, count, HPX_FORWARD(F, f));
            }
        };

        template <typename I, bool IsConst>
        struct datapar_loop_n<I, IsConst,
            std::enable_if_t<std::is_integral_v<I>>>
        {
            using V = traits::vector_pack_type_t<I>;

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter first, std::size_t const count, F&& f)
            {
                std::size_t len = count;
                constexpr std::size_t size = traits::vector_pack_size_v<V>;

                // clang-format off
                while (len > size + 1)
                {
                    datapar_loop_step<I, true>::callv(f, first);
                    len -= size;
                }
                // clang-format on

                for (/* */; len != 0; --len)
                {
                    datapar_loop_step<I, true>::call1(f, first);
                }
                return first;
            }

            template <typename Iter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter first, std::size_t count, CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return first;

                return call(first, count, HPX_FORWARD(F, f));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_loop_n_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr InIter call(
                InIter first, std::size_t count, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<InIter>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<InIter>;

                if constexpr (is_contiguous && datapar_compatible)
                {
                    std::size_t len = count;

                    // clang-format off
                    for (/* */; len != 0 && !detail::is_data_aligned(first);
                        --len)
                    {
                        datapar_loop_step_ind<InIter>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    while (len > size + 1)
                    {
                        datapar_loop_step_ind<InIter>::callv(f, first);
                        len -= size;
                    }
                    // clang-format on

                    for (/* */; len != 0; --len)
                    {
                        datapar_loop_step_ind<InIter>::call1(f, first);
                    }
                    return first;
                }
                else if constexpr (datapar_compatible)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    // clang-format off
                    for (auto len_v =
                             static_cast<std::int64_t>(count - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), count -= size)
                    {
                        datapar_loop_step_ind<InIter>::calls(f, first);
                    }
                    // clang-format on

                    for (/* */; count != 0; --count)
                    {
                        datapar_loop_step_ind<InIter>::call1(f, first);
                    }
                    return first;
                }
                else
                {
                    for (/* */; count != 0; --count)
                    {
                        datapar_loop_step_ind<InIter>::call1(f, first);
                    }
                    return first;
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_loop_idx_n
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                std::size_t base_idx, Iter it, std::size_t const count, F&& f)
            {
                std::size_t len = count;

                for (/* */; len != 0 && !detail::is_data_aligned(it); --len)
                {
                    datapar_loop_idx_step<Iter>::call1(f, it, base_idx);
                    ++it;
                    ++base_idx;
                }

                constexpr std::size_t size = traits::vector_pack_size_v<V>;

                // clang-format off
                while (len > size + 1)
                {
                    datapar_loop_idx_step<Iter>::callv(f, it, base_idx);
                    std::advance(it, size);
                    base_idx += size;
                    len -= size;
                }
                // clang-format on

                for (/* */; len != 0; --len)
                {
                    datapar_loop_idx_step<Iter>::call1(f, it, base_idx);
                    ++it;
                    ++base_idx;
                }
                return it;
            }

            template <typename Iter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                std::size_t base_idx, Iter it, std::size_t count,
                CancelToken& tok, F&& f)
            {
                if (tok.was_cancelled(base_idx))
                    return it;

                return call(base_idx, it, count, HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin tag_invoke(
        hpx::parallel::util::loop_t, ExPolicy&&, Begin begin, End end, F&& f)
    {
        return detail::datapar_loop<Begin>::call(begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin tag_invoke(
        hpx::parallel::util::loop_t, ExPolicy&&, Begin begin, End end,
        CancelToken& tok, F&& f)
    {
        return detail::datapar_loop<Begin>::call(
            begin, end, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin tag_invoke(
        hpx::parallel::util::const_loop_t, ExPolicy&&, Begin begin, End end,
        F&& f)
    {
        return detail::datapar_loop<Begin, true>::call(
            begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin tag_invoke(
        hpx::parallel::util::const_loop_t, ExPolicy&&, Begin begin, End end,
        CancelToken& tok, F&& f)
    {
        return detail::datapar_loop<Begin, true>::call(
            begin, end, tok, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename Pred>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin tag_invoke(
        hpx::parallel::util::loop_pred_t<ExPolicy>, Begin first, End end,
        Pred&& pred)
    {
        constexpr bool datapar_compatible =
            detail::iterator_datapar_compatible_v<Begin>;

        if constexpr (datapar_compatible)
        {
            return hpx::parallel::util::detail::datapar_loop_pred<Begin>::call(
                first, end, HPX_FORWARD(Pred, pred));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));

            return loop_pred<base_policy_type>(
                first, end, HPX_FORWARD(Pred, pred));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_ind_t<ExPolicy>, Begin begin, End end, F&& f)
    {
        return detail::datapar_loop_ind<Begin>::call(
            begin, end, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Iter2, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE std::pair<Iter1, Iter2> tag_invoke(
        hpx::parallel::util::loop2_t<ExPolicy>, Iter1 first1, Iter1 last1,
        Iter2 first2, F&& f)
    {
        if constexpr (detail::iterator_datapar_compatible_v<Iter1> &&
            detail::iterator_datapar_compatible_v<Iter2>)
        {
            return detail::datapar_loop2::call(
                first1, last1, first2, HPX_FORWARD(F, f));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));

            return loop2<base_policy_type>(
                first1, last1, first2, HPX_FORWARD(F, f));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter it, std::size_t count,
        F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_n<Iter>::call(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter it, std::size_t count,
        CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_n<Iter>::call(
            it, count, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_n<Iter, true>::call(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_n<Iter, true>::call(
            it, count, tok, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_ind_t<ExPolicy>, Iter it, std::size_t count,
        F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_n_ind<Iter>::call(
            it, count, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_idx_n<Iter>::call(
            base_idx, it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter it, std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::detail::datapar_loop_idx_n<Iter>::call(
            base_idx, it, count, tok, HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util

#endif
