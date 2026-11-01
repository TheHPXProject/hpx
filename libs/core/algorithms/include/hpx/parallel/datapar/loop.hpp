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
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
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
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N,
            bool IsConst = false>
        struct datapar_loop
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<Begin>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Begin>;

                if constexpr (is_contiguous && datapar_compatible && N != 1)
                {
                    while (first != last && !is_pack_aligned<V>(first))
                    {
                        datapar_loop_step<Begin, N, IsConst>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    End lastV = first;

                    // at least `size` elements remain
                    if (static_cast<std::size_t>(last - first) >= size)
                    {
                        lastV = last - (size - 1);
                    }

                    while (first < lastV)
                    {
                        datapar_loop_step<Begin, N, IsConst>::callv(f, first);
                    }
                }
                else if constexpr (datapar_compatible && N != 1)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    End lastV = first;

                    // at least `size` elements remain
                    if (static_cast<std::size_t>(last - first) >= size)
                    {
                        lastV = last - (size - 1);
                    }

                    while (first < lastV)
                    {
                        datapar_loop_step<Begin, N, IsConst>::calls(f, first);
                    }
                }

                while (first != last)
                {
                    datapar_loop_step<Begin, N, IsConst>::call1(f, first);
                }

                return first;
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
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_loop_pred
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename Begin, typename End, typename Pred>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, Pred&& pred)
            {
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Begin>;

                if constexpr (datapar_compatible && N != 1)
                {
                    while (first != last && !is_pack_aligned<V>(first))
                    {
                        if (datapar_loop_pred_step<Begin, N>::call1(
                                pred, first) != -1)
                        {
                            return first;
                        }
                        ++first;
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    End lastV = first;

                    // at least `size` elements remain
                    if (static_cast<std::size_t>(last - first) >= size)
                    {
                        lastV = last - (size - 1);
                    }

                    while (first < lastV)
                    {
                        int offset = datapar_loop_pred_step<Begin, N>::callv(
                            pred, first);
                        if (offset != -1)
                        {
                            std::advance(first, offset);
                            return first;
                        }
                        std::advance(first, size);
                    }
                }

                while (first != last)
                {
                    if (datapar_loop_pred_step<Begin, N>::call1(pred, first) !=
                        -1)
                        return first;
                    ++first;
                }

                return first;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_loop_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin first, End last, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<Begin>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Begin>;

                if constexpr (is_contiguous && datapar_compatible && N != 1)
                {
                    while (first != last && !is_pack_aligned<V>(first))
                    {
                        datapar_loop_step_ind<Begin, N>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    End lastV = first;

                    // at least `size` elements remain
                    if (static_cast<std::size_t>(last - first) >= size)
                    {
                        lastV = last - (size - 1);
                    }

                    while (first < lastV)
                    {
                        datapar_loop_step_ind<Begin, N>::callv(f, first);
                    }
                }
                else if constexpr (datapar_compatible && N != 1)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    End lastV = first;

                    // at least `size` elements remain
                    if (static_cast<std::size_t>(last - first) >= size)
                    {
                        lastV = last - (size - 1);
                    }

                    while (first < lastV)
                    {
                        datapar_loop_step_ind<Begin, N>::calls(f, first);
                    }
                }

                while (first != last)
                {
                    datapar_loop_step_ind<Begin, N>::call1(f, first);
                }

                return first;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <std::size_t N>
        struct datapar_loop2
        {
            template <typename InIter1, typename InIter2, typename F>
                requires(iterators_datapar_compatible_v<InIter1, InIter2> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2>)
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter1, InIter2>
                call(InIter1 it1, InIter1 last1, InIter2 it2, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2>;

                if constexpr (datapar_compatible && N != 1)
                {
                    using iterator_type = std::decay_t<InIter1>;
                    using value_type =
                        std::iterator_traits<iterator_type>::value_type;

                    using V = traits::vector_pack_type_t<value_type, N>;
                    using value_type2 =
                        std::iterator_traits<std::decay_t<InIter2>>::value_type;
                    using V2 = traits::vector_pack_type_t<value_type2, N>;

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    static_assert(size == traits::vector_pack_size_v<V2>,
                        "both input packs must have the same lane count");

                    while (it1 != last1 && !is_pack_aligned<V>(it1))
                    {
                        datapar_loop_step2_ind<InIter1, InIter2, N>::call1(
                            f, it1, it2);
                    }

                    // one-time check: vectorize only if it2 is also aligned now
                    if (is_pack_aligned<V2>(it2))
                    {
                        // empty vector range by default
                        InIter1 last1V = it1;
                        if (static_cast<std::size_t>(last1 - it1) >= size)
                            last1V = last1 - (size - 1);

                        while (it1 < last1V)
                        {
                            datapar_loop_step2_ind<InIter1, InIter2, N>::callv(
                                f, it1, it2);
                        }
                    }
                }

                while (it1 != last1)
                {
                    datapar_loop_step2_ind<InIter1, InIter2, N>::call1(
                        f, it1, it2);
                }

                return std::make_pair(HPX_MOVE(it1), HPX_MOVE(it2));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N,
            bool IsConst = false, typename Enable = void>
        struct datapar_loop_n;

        template <typename Iterator, std::size_t N, bool IsConst>
        struct datapar_loop_n<Iterator, N, IsConst,
            std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr InIter call(
                InIter first, std::size_t count, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<InIter>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<InIter>;

                std::size_t len = count;
                if constexpr (is_contiguous && datapar_compatible && N != 1)
                {
                    for (/* */; len != 0 && !detail::is_pack_aligned<V>(first);
                        --len)
                    {
                        datapar_loop_step<InIter, N, IsConst>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_step<InIter, N, IsConst>::callv(f, first);
                    }
                }
                else if constexpr (datapar_compatible && N != 1)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_step<InIter, N, IsConst>::calls(f, first);
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_loop_step<InIter, N, IsConst>::call1(f, first);
                }

                return first;
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

        template <typename I, std::size_t N, bool IsConst>
        struct datapar_loop_n<I, N, IsConst,
            std::enable_if_t<std::is_integral_v<I>>>
        {
            using V = traits::vector_pack_type_t<I, N>;

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter first, std::size_t const count, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Iter>;

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_step<I, N, true>::callv(f, first);
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_loop_step<I, N, true>::call1(f, first);
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
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_loop_n_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr InIter call(
                InIter first, std::size_t count, F&& f)
            {
                constexpr bool is_contiguous =
                    hpx::traits::is_contiguous_iterator_v<InIter>;
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<InIter>;

                std::size_t len = count;
                if constexpr (is_contiguous && datapar_compatible && N != 1)
                {
                    for (/* */; len != 0 && !detail::is_pack_aligned<V>(first);
                        --len)
                    {
                        datapar_loop_step_ind<InIter, N>::call1(f, first);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_step_ind<InIter, N>::callv(f, first);
                    }
                }
                else if constexpr (datapar_compatible && N != 1)
                {
                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_step_ind<InIter, N>::calls(f, first);
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_loop_step_ind<InIter, N>::call1(f, first);
                }

                return first;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_loop_idx_n
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                std::size_t base_idx, Iter it, std::size_t const count, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterator_datapar_compatible_v<Iter>;

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    for (/* */; len != 0 && !detail::is_pack_aligned<V>(it);
                        --len)
                    {
                        datapar_loop_idx_step<Iter, N>::call1(f, it, base_idx);
                        ++it;
                        ++base_idx;
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    for (/* */; len >= size; len -= size)
                    {
                        datapar_loop_idx_step<Iter, N>::callv(f, it, base_idx);
                        std::advance(it, size);
                        base_idx += size;
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_loop_idx_step<Iter, N>::call1(f, it, base_idx);
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
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin hpx_invoke(
        hpx::parallel::util::loop_t, ExPolicy&&, Begin begin, End end, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_loop<Begin, num_lanes>::call(
            begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin hpx_invoke(
        hpx::parallel::util::loop_t, ExPolicy&&, Begin begin, End end,
        CancelToken& tok, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_loop<Begin, num_lanes>::call(
            begin, end, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin hpx_invoke(
        hpx::parallel::util::const_loop_t, ExPolicy&&, Begin begin, End end,
        F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_loop<Begin, num_lanes, true>::call(
            begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin hpx_invoke(
        hpx::parallel::util::const_loop_t, ExPolicy&&, Begin begin, End end,
        CancelToken& tok, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_loop<Begin, num_lanes, true>::call(
            begin, end, tok, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename Pred>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin hpx_invoke(
        hpx::parallel::util::loop_pred_t<ExPolicy>, Begin first, End end,
        Pred&& pred)
    {
        constexpr bool datapar_compatible =
            detail::iterator_datapar_compatible_v<Begin>;

        if constexpr (datapar_compatible)
        {
            constexpr std::size_t num_lanes = hpx::execution::policy_traits<
                std::decay_t<ExPolicy>>::num_lanes;
            return hpx::parallel::util::detail::datapar_loop_pred<Begin,
                num_lanes>::call(first, end, HPX_FORWARD(Pred, pred));
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
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin hpx_invoke(
        hpx::parallel::util::loop_ind_t<ExPolicy>, Begin begin, End end, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_loop_ind<Begin, num_lanes>::call(
            begin, end, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Iter2, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE std::pair<Iter1, Iter2> hpx_invoke(
        hpx::parallel::util::loop2_t<ExPolicy>, Iter1 first1, Iter1 last1,
        Iter2 first2, F&& f)
    {
        if constexpr (detail::iterator_datapar_compatible_v<Iter1> &&
            detail::iterator_datapar_compatible_v<Iter2>)
        {
            constexpr std::size_t num_lanes = hpx::execution::policy_traits<
                std::decay_t<ExPolicy>>::num_lanes;
            return detail::datapar_loop2<num_lanes>::call(
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
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter it, std::size_t count,
        F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_n<Iter,
            num_lanes>::call(it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter it, std::size_t count,
        CancelToken& tok, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_n<Iter,
            num_lanes>::call(it, count, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_n<Iter, num_lanes,
            true>::call(it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, CancelToken& tok, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_n<Iter, num_lanes,
            true>::call(it, count, tok, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::loop_n_ind_t<ExPolicy>, Iter it, std::size_t count,
        F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_n_ind<Iter,
            num_lanes>::call(it, count, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter it, std::size_t count, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_idx_n<Iter,
            num_lanes>::call(base_idx, it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter hpx_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter it, std::size_t count, CancelToken& tok, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return hpx::parallel::util::detail::datapar_loop_idx_n<Iter,
            num_lanes>::call(base_idx, it, count, tok, HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util

#endif
