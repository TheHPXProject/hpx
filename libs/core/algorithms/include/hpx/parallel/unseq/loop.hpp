//  Copyright (c) 2022 A Kishore Kumar
//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_loop_n
        {
            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static InIter call(
                InIter HPX_RESTRICT it, std::size_t const num, F&& f)
            {
                // clang-format off
                HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                for (std::size_t i = 0; i != num; ++i)
                {
                    HPX_INVOKE(f, it);
                    ++it;
                }
                // clang-format on

                return it;
            }

            template <typename InIter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static InIter call(
                InIter HPX_RESTRICT it, std::size_t count, CancelToken& tok,
                F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(it, count, HPX_FORWARD(F, f));
            }
        };

        HPX_CXX_CORE_EXPORT struct unseq_loop_n_ind
        {
            template <typename InIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static InIter call(
                InIter HPX_RESTRICT it, std::size_t const num, F&& f)
            {
                // clang-format off
                HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                for (std::size_t i = 0; i != num; ++i)
                {
                    HPX_INVOKE(f, *it);
                    ++it;
                }
                // clang-format on

                return it;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_loop
        {
            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin HPX_RESTRICT it, End HPX_RESTRICT end, F&& f)
            {
                if constexpr (std::random_access_iterator<Begin>)
                {
                    return unseq_loop_n::call(it,
                        hpx::parallel::detail::distance(it, end),
                        HPX_FORWARD(F, f));
                }
                else
                {
                    for (/* */; it != end; ++it)
                    {
                        HPX_INVOKE(f, it);
                    }
                    return it;
                }
            }

            template <typename Begin, typename End, typename CancelToken,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Begin call(
                Begin HPX_RESTRICT it, End HPX_RESTRICT end, CancelToken& tok,
                F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(it, end, HPX_FORWARD(F, f));
            }
        };

        HPX_CXX_CORE_EXPORT struct unseq_loop_ind
        {
            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin HPX_RESTRICT it, End HPX_RESTRICT end, F&& f)
            {
                if constexpr (std::random_access_iterator<Begin>)
                {
                    return unseq_loop_n_ind::call(it,
                        hpx::parallel::detail::distance(it, end),
                        HPX_FORWARD(F, f));
                }
                else
                {
                    for (/* */; it != end; ++it)
                    {
                        HPX_INVOKE(f, *it);
                    }
                    return it;
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_loop2
        {
            template <typename InIter1, typename InIter2, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::pair<InIter1, InIter2>
            call(InIter1 HPX_RESTRICT it1, InIter1 HPX_RESTRICT last1,
                InIter2 HPX_RESTRICT it2, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2>;

                if constexpr (iterators_are_random_access)
                {
                    std::size_t const num =
                        hpx::parallel::detail::distance(it1, last1);

                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i != num; ++i)
                    {
                        HPX_INVOKE(f, it1, it2);
                        ++it1, ++it2;
                    }
                    // clang-format on
                }
                else
                {
                    for (/* */; it1 != last1; (void) ++it1, ++it2)
                    {
                        HPX_INVOKE(f, it1, it2);
                    }
                }
                return std::make_pair(HPX_MOVE(it1), HPX_MOVE(it2));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_loop_idx_n
        {
            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                std::size_t base_idx, Iter HPX_RESTRICT it,
                std::size_t const num, F&& f)
            {
                // clang-format off
                HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                for (std::size_t i = 0; i != num; ++i)
                {
                    HPX_INVOKE(f, *it, base_idx);
                    ++it, ++base_idx;
                }
                // clang-format on

                return it;
            }

            template <typename Iter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                std::size_t base_idx, Iter HPX_RESTRICT it, std::size_t count,
                CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled(base_idx))
                    return it;

                return call(base_idx, it, count, HPX_FORWARD(F, f));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename IterCat>
        struct unseq_loop_with_cleanup_n
        {
            ///////////////////////////////////////////////////////////////////
            template <typename FwdIter, typename F, typename Cleanup>
            static FwdIter call(
                FwdIter it, std::size_t const num, F&& f, Cleanup&& cleanup)
            {
                FwdIter base = it;
                try
                {
                    std::size_t count(
                        num & static_cast<std::size_t>(-4));    // -V112

                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i < count; i += 4)    // -V112
                    {
                        HPX_INVOKE(f, it++);
                        HPX_INVOKE(f, it++);
                        HPX_INVOKE(f, it++);
                        HPX_INVOKE(f, it++);
                    }

                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = count; i < num; ++i)
                    {
                        HPX_INVOKE(f, it++);
                    }
                    // clang-format on

                    return it;
                }
                catch (...)
                {
                    for (/**/; base != it; ++base)
                    {
                        HPX_INVOKE(cleanup, base);
                    }
                    throw;
                }
            }

            template <typename Iter, typename FwdIter, typename F,
                typename Cleanup>
            static FwdIter call(Iter it, std::size_t const num, FwdIter dest,
                F&& f, Cleanup&& cleanup)
            {
                FwdIter base = dest;
                try
                {
                    std::size_t count(
                        num & static_cast<std::size_t>(-4));    // -V112

                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i < count; i += 4)    // -V112
                    {
                        HPX_INVOKE(f, it++, dest++);
                        HPX_INVOKE(f, it++, dest++);
                        HPX_INVOKE(f, it++, dest++);
                        HPX_INVOKE(f, it++, dest++);
                    }

                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = count; i < num; ++i)
                    {
                        HPX_INVOKE(f, it++, dest++);
                    }
                    // clang-format on

                    return dest;
                }
                catch (...)
                {
                    for (/**/; base != dest; ++base)
                    {
                        HPX_INVOKE(cleanup, base);
                    }
                    throw;
                }
            }

            template <typename FwdIter, typename CancelToken, typename F,
                typename Cleanup>
            HPX_HOST_DEVICE HPX_FORCEINLINE static FwdIter call_with_token(
                FwdIter it, std::size_t num, CancelToken& tok, F&& f,
                Cleanup&& cleanup)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(
                    it, num, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename Iter, typename FwdIter, typename CancelToken,
                typename F, typename Cleanup>
            static FwdIter call_with_token(Iter it, std::size_t num,
                FwdIter dest, CancelToken& tok, F&& f, Cleanup&& cleanup)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return dest;

                return call(it, num, dest, HPX_FORWARD(F, f),
                    HPX_FORWARD(Cleanup, cleanup));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Begin, typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_t, hpx::execution::unsequenced_policy,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop::call(begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_t, hpx::execution::unsequenced_task_policy,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop::call(begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::const_loop_t, hpx::execution::unsequenced_policy,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop::call(begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::const_loop_t,
        hpx::execution::unsequenced_task_policy, Begin HPX_RESTRICT begin,
        End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop::call(begin, end, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Begin, typename End,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_t, hpx::execution::unsequenced_policy,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, CancelToken& tok, F&& f)
    {
        return detail::unseq_loop::call(begin, end, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename End,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_t, hpx::execution::unsequenced_task_policy,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, CancelToken& tok, F&& f)
    {
        return detail::unseq_loop::call(begin, end, tok, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Begin, typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_ind_t<hpx::execution::unsequenced_policy>,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop_ind::call(begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::loop_ind_t<
            hpx::execution::unsequenced_task_policy>,
        Begin HPX_RESTRICT begin, End HPX_RESTRICT end, F&& f)
    {
        return detail::unseq_loop_ind::call(begin, end, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Iter2, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::pair<Iter1, Iter2>
    tag_invoke(hpx::parallel::util::loop2_t<ExPolicy>,
        Iter1 HPX_RESTRICT first1, Iter1 HPX_RESTRICT last1,
        Iter2 HPX_RESTRICT first2, F&& f)
    {
        return detail::unseq_loop2::call(
            first1, last1, first2, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter HPX_RESTRICT it,
        std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_n::call(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_t<ExPolicy>, Iter HPX_RESTRICT it,
        std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_n::call(
            it, count, tok, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter HPX_RESTRICT it,
        std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_n::call(
            it, count, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_n_ind_t<ExPolicy>, Iter HPX_RESTRICT it,
        std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_n_ind::call(
            it, count, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter HPX_RESTRICT it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_idx_n::call(
            base_idx, it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
        requires(hpx::is_unsequenced_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter tag_invoke(
        hpx::parallel::util::loop_idx_n_t<ExPolicy>, std::size_t base_idx,
        Iter HPX_RESTRICT it, std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::detail::unseq_loop_idx_n::call(
            base_idx, it, count, tok, HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util
