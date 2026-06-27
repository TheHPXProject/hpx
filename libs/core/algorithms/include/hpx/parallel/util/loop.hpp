//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function starting from a given
        // iterator position.
        HPX_CXX_CORE_EXPORT template <typename Iterator, bool IsConst = false>
        struct loop
        {
            ///////////////////////////////////////////////////////////////////
            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin it, End end, F&& f)
            {
                return call(hpx::execution::seq, it, end, HPX_FORWARD(F, f));
            }

            template <typename Begin, typename End, typename CancelToken,
                typename F>
                requires(hpx::traits::is_iterator_v<End> &&
                    std::forward_iterator<Begin>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static Begin call(
                Begin it, End end, CancelToken& tok, F&& f)
            {
                return call(
                    hpx::execution::seq, it, end, tok, HPX_FORWARD(F, f));
            }

            template <typename ExPolicy, typename Begin, typename End,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                ExPolicy&&, Begin it, End last, F&& f)
            {
                for (/**/; it != last; ++it)
                {
                    HPX_INVOKE(f, it);
                }
                return it;
            }

            template <typename ExPolicy, typename Begin, typename End,
                typename CancelToken, typename F>
                requires(hpx::is_execution_policy_v<ExPolicy>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static Begin call(
                ExPolicy&& policy, Begin it, End end, CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(
                    HPX_FORWARD(ExPolicy, policy), it, end, HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct loop_t final
      : hpx::functional::detail::tag_fallback<loop_t>
    {
    private:
        template <typename ExPolicy, typename Begin, typename End, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::loop_t, ExPolicy&& policy,
            Begin begin, End end, F&& f)
        {
            return detail::loop<Begin>::call(
                HPX_FORWARD(ExPolicy, policy), begin, end, HPX_FORWARD(F, f));
        }

        template <typename ExPolicy, typename Begin, typename End,
            typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::loop_t, ExPolicy&& policy,
            Begin begin, End end, CancelToken& tok, F&& f)
        {
            return detail::loop<Begin>::call(HPX_FORWARD(ExPolicy, policy),
                begin, end, tok, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr loop_t loop = loop_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop(
        ExPolicy&& policy, Begin begin, End end, F&& f)
    {
        return hpx::parallel::util::loop_t{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop(
        ExPolicy&& policy, Begin begin, End end, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::loop_t{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, tok, HPX_FORWARD(F, f));
    }
#endif

    HPX_CXX_CORE_EXPORT struct const_loop_t final
      : hpx::functional::detail::tag_fallback<const_loop_t>
    {
    private:
        template <typename ExPolicy, typename Begin, typename End, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::const_loop_t,
            ExPolicy&& policy, Begin begin, End end, F&& f)
        {
            return detail::loop<Begin, true>::call(
                HPX_FORWARD(ExPolicy, policy), begin, end, HPX_FORWARD(F, f));
        }

        template <typename ExPolicy, typename Begin, typename End,
            typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::const_loop_t,
            ExPolicy&& policy, Begin begin, End end, CancelToken& tok, F&& f)
        {
            return detail::loop<Begin, true>::call(
                HPX_FORWARD(ExPolicy, policy), begin, end, tok,
                HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr const_loop_t const_loop =
        const_loop_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) const_loop(
        ExPolicy&& policy, Begin begin, End end, F&& f)
    {
        return hpx::parallel::util::const_loop_t{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) const_loop(
        ExPolicy&& policy, Begin begin, End end, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::const_loop_t{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, tok, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function starting from a given
        // iterator position till the predicate returns true.
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct loop_pred
        {
            ///////////////////////////////////////////////////////////////////
            template <typename Begin, typename End, typename Pred>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin it, End end, Pred&& pred)
            {
                for (/**/; it != end; ++it)
                {
                    if (HPX_INVOKE(pred, it))
                        return it;
                }
                return it;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop_pred_t final
      : hpx::functional::detail::tag_fallback<loop_pred_t<ExPolicy>>
    {
    private:
        template <typename Begin, typename End, typename Pred>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::loop_pred_t<ExPolicy>,
            Begin begin, End end, Pred&& pred)
        {
            return detail::loop_pred<Begin>::call(
                begin, end, HPX_FORWARD(Pred, pred));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop_pred_t<ExPolicy> loop_pred = loop_pred_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename Pred>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_pred(
        Begin begin, End end, Pred&& pred)
    {
        return hpx::parallel::util::loop_pred_t<ExPolicy>{}(
            begin, end, HPX_FORWARD(Pred, pred));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function starting from a given
        // iterator position.
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct loop_ind
        {
            ///////////////////////////////////////////////////////////////////
            template <typename Begin, typename End, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Begin call(
                Begin start, End sent, F&& f)
            {
                if constexpr (std::random_access_iterator<Begin>)
                {
                    auto end = start;
                    auto const len =
                        parallel::detail::advance_and_get_distance(end, sent);

                    if (len != 0)
                    {
                        auto it = hpx::util::get_unwrapped(start);
                        for (auto count = len; count != 0; (void) ++it, --count)
                        {
                            HPX_INVOKE(f, *it);
                        }
                    }
                    return std::next(start, len);
                }
                else
                {
                    for (/**/; start != sent; ++start)
                    {
                        HPX_INVOKE(f, *start);
                    }
                    return start;
                }
            }

            template <typename Begin, typename End, typename CancelToken,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Begin call(
                Begin it, End end, CancelToken& tok, F&& f)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(it, end, HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop_ind_t final
      : hpx::functional::detail::tag_fallback<loop_ind_t<ExPolicy>>
    {
    private:
        template <typename Begin, typename End, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::loop_ind_t<ExPolicy>,
            Begin begin, End end, F&& f)
        {
            return detail::loop_ind<std::decay_t<Begin>>::call(
                begin, end, HPX_FORWARD(F, f));
        }

        template <typename Begin, typename End, typename CancelToken,
            typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin
        tag_fallback_invoke(hpx::parallel::util::loop_ind_t<ExPolicy>,
            Begin begin, End end, CancelToken& tok, F&& f)
        {
            return detail::loop_ind<std::decay_t<Begin>>::call(
                begin, end, tok, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop_ind_t<ExPolicy> loop_ind = loop_ind_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_ind(
        Begin begin, End end, F&& f)
    {
        return hpx::parallel::util::loop_ind_t<ExPolicy>{}(
            begin, end, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename End, typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Begin loop_ind(
        Begin begin, End end, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::loop_ind_t<ExPolicy>{}(
            begin, end, tok, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function starting from a given
        // iterator position.
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct loop2
        {
            ///////////////////////////////////////////////////////////////////
            template <typename Begin1, typename End1, typename Begin2,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<Begin1, Begin2>
                call(Begin1 it1, End1 last1, Begin2 it2, F&& f)
            {
                for (/**/; it1 != last1; (void) ++it1, ++it2)
                {
                    HPX_INVOKE(f, it1, it2);
                }
                return std::make_pair(it1, it2);
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop2_t final
      : hpx::functional::detail::tag_fallback<loop2_t<ExPolicy>>
    {
    private:
        template <typename Begin1, typename End1, typename Begin2, typename F>
        friend HPX_HOST_DEVICE
            HPX_FORCEINLINE constexpr std::pair<Begin1, Begin2>
            tag_fallback_invoke(hpx::parallel::util::loop2_t<ExPolicy>,
                Begin1 begin1, End1 end1, Begin2 begin2, F&& f)
        {
            return detail::loop2<Begin1, Begin2>::call(
                begin1, end1, begin2, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop2_t<ExPolicy> loop2 = loop2_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin1,
        typename End1, typename Begin2, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop2(
        Begin1 begin1, End1 end1, Begin2 begin2, F&& f)
    {
        return hpx::parallel::util::loop2_t<ExPolicy>{}(
            begin1, end1, begin2, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position.
        HPX_CXX_CORE_EXPORT template <bool IsConst = false>
        struct loop_n_helper
        {
            ///////////////////////////////////////////////////////////////////
            // handle sequences of non-futures
            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter it, std::size_t num, F&& f, std::false_type)
            {
                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112
                for (std::size_t i = 0; i < count;
                    (void) ++it, i += 4)    // -V112
                {
                    HPX_INVOKE(f, it);
                    HPX_INVOKE(f, ++it);
                    HPX_INVOKE(f, ++it);
                    HPX_INVOKE(f, ++it);
                }
                for (/**/; count < num; (void) ++count, ++it)
                {
                    HPX_INVOKE(f, it);
                }

                return it;
            }

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter it, std::size_t num, F&& f, std::true_type)
            {
                while (num >= 4)    //-V112
                {
                    HPX_INVOKE(f, it);
                    HPX_INVOKE(f, it + 1);
                    HPX_INVOKE(f, it + 2);
                    HPX_INVOKE(f, it + 3);

                    it += 4;     //-V112
                    num -= 4;    //-V112
                }

                switch (num)
                {
                case 3:
                    HPX_INVOKE(f, it);
                    HPX_INVOKE(f, it + 1);
                    HPX_INVOKE(f, it + 2);
                    it += 3;
                    break;

                case 2:
                    HPX_INVOKE(f, it);
                    HPX_INVOKE(f, it + 1);
                    it += 2;
                    break;

                case 1:
                    HPX_INVOKE(f, it);
                    ++it;
                    break;

                default:
                    break;
                }

                return it;
            }

            template <typename Iter, typename CancelToken, typename F,
                typename Tag>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                Iter it, std::size_t num, CancelToken& tok, F&& f, Tag tag)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(it, num, HPX_FORWARD(F, f), tag);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop_n_t final
      : hpx::functional::detail::tag_fallback<loop_n_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::loop_n_t<ExPolicy>, Iter it,
            std::size_t count, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_helper<false>::call(
                it, count, HPX_FORWARD(F, f), pred());
        }

        template <typename Iter, typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_fallback_invoke(
            hpx::parallel::util::loop_n_t<ExPolicy>, Iter it, std::size_t count,
            CancelToken& tok, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_helper<false>::call(
                it, count, tok, HPX_FORWARD(F, f), pred());
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop_n_t<ExPolicy> loop_n = loop_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_n(
        Iter it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::loop_n_t<ExPolicy>{}(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_n(
        Iter it, std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::loop_n_t<ExPolicy>{}(
            it, count, tok, HPX_FORWARD(F, f));
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct const_loop_n_t final
      : hpx::functional::detail::tag_fallback<const_loop_n_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::const_loop_n_t<ExPolicy>,
            Iter it, std::size_t count, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_helper<true>::call(
                it, count, HPX_FORWARD(F, f), pred());
        }

        template <typename Iter, typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_fallback_invoke(
            hpx::parallel::util::const_loop_n_t<ExPolicy>, Iter it,
            std::size_t count, CancelToken& tok, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_helper<true>::call(
                it, count, tok, HPX_FORWARD(F, f), pred());
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr const_loop_n_t<ExPolicy> const_loop_n =
        const_loop_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) const_loop_n(
        Iter it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::const_loop_n_t<ExPolicy>{}(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) const_loop_n(
        Iter it, std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::const_loop_n_t<ExPolicy>{}(
            it, count, tok, HPX_FORWARD(F, f));
    }
#endif

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position.
        HPX_CXX_CORE_EXPORT struct loop_n_ind_helper
        {
            ///////////////////////////////////////////////////////////////////
            // handle sequences of non-futures
            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter
            call_iteration(Iter it, std::size_t num, F&& f, std::false_type)
            {
                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                for (std::size_t i = 0; i < count;
                    (void) ++it, i += 4)    // -V112
                {
                    HPX_INVOKE(f, *it);
                    HPX_INVOKE(f, *++it);
                    HPX_INVOKE(f, *++it);
                    HPX_INVOKE(f, *++it);
                }

                for (/**/; count < num; (void) ++count, ++it)
                {
                    HPX_INVOKE(f, *it);
                }

                return it;
            }

            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter
            call_iteration(Iter it, std::size_t num, F&& f, std::true_type)
            {
                while (num >= 4)    //-V112
                {
                    HPX_INVOKE(f, *it);
                    HPX_INVOKE(f, *(it + 1));
                    HPX_INVOKE(f, *(it + 2));
                    HPX_INVOKE(f, *(it + 3));

                    it += 4;     //-V112
                    num -= 4;    //-V112
                }

                switch (num)
                {
                case 3:
                    HPX_INVOKE(f, *it);
                    HPX_INVOKE(f, *(it + 1));
                    HPX_INVOKE(f, *(it + 2));
                    it += 3;
                    break;

                case 2:
                    HPX_INVOKE(f, *it);
                    HPX_INVOKE(f, *(it + 1));
                    it += 2;
                    break;

                case 1:
                    HPX_INVOKE(f, *it);
                    ++it;
                    break;

                default:
                    break;
                }

                return it;
            }

            template <typename Iter, typename F, typename Tag>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                Iter start, std::size_t num, F&& f, Tag tag)
            {
                if (num == 0)
                {
                    return start;
                }

                if constexpr (std::random_access_iterator<Iter>)
                {
                    auto it = hpx::util::get_unwrapped(start);

                    call_iteration(it, num, HPX_FORWARD(F, f), tag);

                    return parallel::detail::next(start, num);
                }
                else
                {
                    return call_iteration(start, num, HPX_FORWARD(F, f), tag);
                }
            }

            template <typename Iter, typename CancelToken, typename F,
                typename Tag>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                Iter it, std::size_t num, CancelToken& tok, F&& f, Tag tag)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(it, num, HPX_FORWARD(F, f), tag);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop_n_ind_t final
      : hpx::functional::detail::tag_fallback<loop_n_ind_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::loop_n_ind_t<ExPolicy>,
            Iter it, std::size_t count, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_ind_helper::call(
                it, count, HPX_FORWARD(F, f), pred());
        }

        template <typename Iter, typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::loop_n_ind_t<ExPolicy>,
            Iter it, std::size_t count, CancelToken& tok, F&& f)
        {
            using pred = std::integral_constant<bool,
                std::random_access_iterator<Iter> || std::is_integral_v<Iter>>;

            return detail::loop_n_ind_helper::call(
                it, count, tok, HPX_FORWARD(F, f), pred());
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop_n_ind_t<ExPolicy> loop_n_ind =
        loop_n_ind_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_n_ind(
        Iter it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::loop_n_ind_t<ExPolicy>{}(
            it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_n_ind(
        Iter it, std::size_t count, CancelToken& tok, F&& f)
    {
        return hpx::parallel::util::loop_n_ind_t<ExPolicy>{}(
            it, count, tok, HPX_FORWARD(F, f));
    }
#endif

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position. If an exception is thrown,
        // the given cleanup function will be called.
        HPX_CXX_CORE_EXPORT struct loop_with_cleanup final
          : hpx::functional::detail::tag_fallback<loop_with_cleanup>
        {
            ///////////////////////////////////////////////////////////////////
            template <typename ExPolicy, typename FwdIter, typename F,
                typename Sent, typename Cleanup>
            static FwdIter call(
                ExPolicy&&, FwdIter first, Sent last, F&& f, Cleanup&& cleanup)
            {
                auto it = first;
                try
                {
                    for (/**/; it != last; (void) ++it)
                    {
                        HPX_INVOKE(f, it);
                    }
                    return it;
                }
                catch (...)
                {
                    for (/**/; first != it; ++first)
                    {
                        HPX_INVOKE(cleanup, first);
                    }
                    throw;
                }
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename FwdIter, typename F, typename Cleanup>
            static FwdIter call(ExPolicy&&, Iter it, Sent last, FwdIter dest,
                F&& f, Cleanup&& cleanup)
            {
                FwdIter base = dest;
                try
                {
                    for (/**/; it != last; (void) ++it, ++dest)
                    {
                        HPX_INVOKE(f, it, dest);
                    }
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

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Sent, typename Cleanup>
            friend HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter tag_fallback_invoke(
                hpx::parallel::util::detail::loop_with_cleanup,
                ExPolicy&& policy, FwdIter first, Sent last, F&& f,
                Cleanup&& cleanup)
            {
                return call(HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename FwdIter, typename F, typename Cleanup>
            friend HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter tag_fallback_invoke(
                hpx::parallel::util::detail::loop_with_cleanup,
                ExPolicy&& policy, Iter it, Sent last, FwdIter dest, F&& f,
                Cleanup&& cleanup)
            {
                return call(HPX_FORWARD(ExPolicy, policy), it, last, dest,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Sent, typename Cleanup>
            static FwdIter unseq_call(
                ExPolicy&&, FwdIter first, Sent last, F&& f, Cleanup&& cleanup)
            {
                auto it = first;
                try
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (FwdIter& iter = it; iter != last; ++iter)
                    {
                        HPX_INVOKE(f, iter);
                    }
                    // clang-format on

                    return it;
                }
                catch (...)
                {
                    for (/**/; first != it; ++first)
                    {
                        HPX_INVOKE(cleanup, first);
                    }
                    throw;
                }
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename FwdIter, typename F, typename Cleanup>
            static FwdIter unseq_call(ExPolicy&&, Iter it, Sent last,
                FwdIter dest, F&& f, Cleanup&& cleanup)
            {
                FwdIter base = dest;
                try
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (Iter& iter = it; iter != last; ++iter)
                    {
                        HPX_INVOKE(f, iter, dest++);
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
        };
    }    // namespace detail

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr detail::loop_with_cleanup
        loop_with_cleanup = detail::loop_with_cleanup{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename Sent, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_with_cleanup(
        ExPolicy&& policy, Begin begin, Sent end, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename Sent, typename Begin2, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_with_cleanup(
        ExPolicy&& policy, Begin begin, Sent end, Begin2 dest, F&& f,
        Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup{}(
            HPX_FORWARD(ExPolicy, policy), begin, end, dest, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Sent, typename F,
        typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup,
        hpx::execution::unsequenced_policy, Begin HPX_RESTRICT begin, Sent end,
        F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup::unseq_call(
            begin, end, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Sent, typename F,
        typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup,
        hpx::execution::unsequenced_task_policy, Begin HPX_RESTRICT begin,
        Sent end, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup::unseq_call(
            begin, end, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Sent,
        typename Begin2, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup,
        hpx::execution::unsequenced_policy, Begin HPX_RESTRICT begin, Sent end,
        Begin2 dest, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup::unseq_call(
            begin, end, dest, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Sent,
        typename Begin2, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup,
        hpx::execution::unsequenced_task_policy, Begin HPX_RESTRICT begin,
        Sent end, Begin2 dest, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup::unseq_call(
            begin, end, dest, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    namespace detail {

        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position.
        HPX_CXX_CORE_EXPORT struct loop_with_cleanup_n final
          : hpx::functional::detail::tag_fallback<loop_with_cleanup_n>
        {
            ///////////////////////////////////////////////////////////////////
            template <typename ExPolicy, typename Iter, typename FwdIter,
                typename F, typename Cleanup>
            static FwdIter call(ExPolicy&&, Iter it, std::size_t num,
                FwdIter dest, F&& f, Cleanup&& cleanup)
            {
                FwdIter base = dest;
                try
                {
                    std::size_t count(
                        num & static_cast<std::size_t>(-4));    // -V112
                    for (std::size_t i = 0; i < count;
                        (void) ++it, ++dest, i += 4)    // -V112
                    {
                        HPX_INVOKE(f, it, dest);
                        HPX_INVOKE(f, ++it, ++dest);
                        HPX_INVOKE(f, ++it, ++dest);
                        HPX_INVOKE(f, ++it, ++dest);
                    }
                    for (/**/; count < num; (void) ++count, ++it, ++dest)
                    {
                        HPX_INVOKE(f, it, dest);
                    }

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

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Cleanup>
            static FwdIter call(ExPolicy&&, FwdIter it, std::size_t num, F&& f,
                Cleanup&& cleanup)
            {
                FwdIter base = it;
                try
                {
                    std::size_t count(
                        num & static_cast<std::size_t>(-4));    // -V112
                    for (std::size_t i = 0; i < count;
                        (void) ++it, i += 4)    // -V112
                    {
                        HPX_INVOKE(f, it);
                        HPX_INVOKE(f, ++it);
                        HPX_INVOKE(f, ++it);
                        HPX_INVOKE(f, ++it);
                    }
                    for (/**/; count < num; (void) ++count, ++it)
                    {
                        HPX_INVOKE(f, it);
                    }
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

            template <typename ExPolicy, typename Iter, typename FwdIter,
                typename F, typename Cleanup>
            friend HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter tag_fallback_invoke(
                loop_with_cleanup_n, ExPolicy&& policy, Iter it,
                std::size_t num, FwdIter dest, F&& f, Cleanup&& cleanup)
            {
                return call(HPX_FORWARD(ExPolicy, policy), it, num, dest,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Cleanup>
            friend HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter tag_fallback_invoke(
                loop_with_cleanup_n, ExPolicy&& policy, FwdIter it,
                std::size_t num, F&& f, Cleanup&& cleanup)
            {
                return call(HPX_FORWARD(ExPolicy, policy), it, num,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename ExPolicy, typename Iter, typename FwdIter,
                typename F, typename Cleanup>
            FwdIter unseq_call(Iter it, std::size_t num, FwdIter dest, F&& f,
                Cleanup&& cleanup)
            {
                FwdIter base = dest;
                try
                {
                    std::size_t const count(
                        num & static_cast<std::size_t>(-4));    // -V112

                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i < count; ++i)
                    {
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

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Cleanup>
            static FwdIter unseq_call(
                ExPolicy, FwdIter it, std::size_t num, F&& f, Cleanup&& cleanup)
            {
                FwdIter base = it;
                try
                {
                    std::size_t const count(
                        num & static_cast<std::size_t>(-4));    // -V112

                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i < count; ++i)
                    {
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

            template <typename ExPolicy, typename FwdIter, typename CancelToken,
                typename F, typename Cleanup>
            HPX_HOST_DEVICE HPX_FORCEINLINE static FwdIter call_with_token(
                ExPolicy&& policy, FwdIter it, std::size_t num,
                CancelToken& tok, F&& f, Cleanup&& cleanup)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return it;

                return call(HPX_FORWARD(ExPolicy, policy), it, num,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }

            template <typename ExPolicy, typename Iter, typename FwdIter,
                typename CancelToken, typename F, typename Cleanup>
            static FwdIter call_with_token(ExPolicy&& policy, Iter it,
                std::size_t num, FwdIter dest, CancelToken& tok, F&& f,
                Cleanup&& cleanup)
            {
                // check at the start of a partition only
                if (tok.was_cancelled())
                    return dest;

                return call(HPX_FORWARD(ExPolicy, policy), it, num, dest,
                    HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
            }
        };
    }    // namespace detail

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr detail::loop_with_cleanup_n
        loop_with_cleanup_n = detail::loop_with_cleanup_n{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin, typename F,
        typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto)
    loop_with_cleanup_n(
        ExPolicy&& policy, Begin begin, std::size_t n, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n{}(
            HPX_FORWARD(ExPolicy, policy), begin, n, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Begin,
        typename Begin2, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto)
    loop_with_cleanup_n(ExPolicy&& policy, Begin begin, std::size_t n,
        Begin2 dest, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n{}(
            HPX_FORWARD(ExPolicy, policy), begin, n, dest, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename Begin, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup_n,
        hpx::execution::unsequenced_policy, Begin HPX_RESTRICT begin,
        std::size_t n, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n::unseq_call(
            begin, n, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename F, typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup_n,
        hpx::execution::unsequenced_task_policy, Begin HPX_RESTRICT begin,
        std::size_t n, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n::unseq_call(
            begin, n, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Begin2, typename F,
        typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup_n,
        hpx::execution::unsequenced_policy, Begin HPX_RESTRICT begin,
        std::size_t n, Begin2 dest, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n::unseq_call(
            begin, n, dest, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Begin, typename Begin2, typename F,
        typename Cleanup>
    HPX_HOST_DEVICE HPX_FORCEINLINE Begin tag_invoke(
        hpx::parallel::util::detail::loop_with_cleanup_n,
        hpx::execution::unsequenced_task_policy, Begin HPX_RESTRICT begin,
        std::size_t n, Begin2 dest, F&& f, Cleanup&& cleanup)
    {
        return hpx::parallel::util::detail::loop_with_cleanup_n::unseq_call(
            begin, n, dest, HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Iter, typename CancelToken,
        typename F, typename Cleanup>
        requires(hpx::traits::is_iterator_v<Iter>)
    HPX_FORCEINLINE constexpr Iter loop_with_cleanup_n_with_token(
        Iter it, std::size_t count, CancelToken& tok, F&& f, Cleanup&& cleanup)
    {
        return detail::loop_with_cleanup_n::call_with_token(
            ::hpx::execution::seq, it, count, tok, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename Iter, typename FwdIter,
        typename CancelToken, typename F, typename Cleanup>
        requires(hpx::traits::is_iterator_v<Iter> &&
            hpx::traits::is_iterator_v<FwdIter>)
    HPX_FORCEINLINE constexpr FwdIter loop_with_cleanup_n_with_token(Iter it,
        std::size_t count, FwdIter dest, CancelToken& tok, F&& f,
        Cleanup&& cleanup)
    {
        return detail::loop_with_cleanup_n::call_with_token(
            ::hpx::execution::seq, it, count, dest, tok, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F, typename Cleanup>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<Iter>)
    HPX_FORCEINLINE constexpr Iter loop_with_cleanup_n_with_token(
        ExPolicy&& policy, Iter it, std::size_t count, CancelToken& tok, F&& f,
        Cleanup&& cleanup)
    {
        return detail::loop_with_cleanup_n::call_with_token(
            HPX_FORWARD(ExPolicy, policy), it, count, tok, HPX_FORWARD(F, f),
            HPX_FORWARD(Cleanup, cleanup));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename FwdIter, typename CancelToken, typename F, typename Cleanup>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<Iter> &&
            hpx::traits::is_iterator_v<FwdIter>)
    HPX_FORCEINLINE constexpr FwdIter loop_with_cleanup_n_with_token(
        ExPolicy&& policy, Iter it, std::size_t count, FwdIter dest,
        CancelToken& tok, F&& f, Cleanup&& cleanup)
    {
        return detail::loop_with_cleanup_n::call_with_token(
            HPX_FORWARD(ExPolicy, policy), it, count, dest, tok,
            HPX_FORWARD(F, f), HPX_FORWARD(Cleanup, cleanup));
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position.
        HPX_CXX_CORE_EXPORT template <typename IterCat>
        struct loop_idx_n
        {
            ///////////////////////////////////////////////////////////////////
            // handle sequences of non-futures
            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                std::size_t base_idx, Iter start, std::size_t num, F&& f)
            {
                if (num == 0)
                {
                    return start;
                }

                std::size_t count(
                    num & static_cast<std::size_t>(-4));    // -V112

                auto it = hpx::util::get_unwrapped(start);

                for (std::size_t i = 0; i < count;
                    (void) ++it, i += 4)    // -V112
                {
                    HPX_INVOKE(f, *it, base_idx++);
                    HPX_INVOKE(f, *++it, base_idx++);
                    HPX_INVOKE(f, *++it, base_idx++);
                    HPX_INVOKE(f, *++it, base_idx++);
                }

                for (/**/; count < num; (void) ++count, ++it, ++base_idx)
                {
                    HPX_INVOKE(f, *it, base_idx);
                }

                return it;
            }

            template <typename Iter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                std::size_t base_idx, Iter it, std::size_t count,
                CancelToken& tok, F&& f)
            {
                if (tok.was_cancelled(base_idx))
                    return it;

                return call(base_idx, it, count, HPX_FORWARD(F, f));
            }
        };

        template <>
        struct loop_idx_n<std::random_access_iterator_tag>
        {
            ///////////////////////////////////////////////////////////////////
            // handle sequences of non-futures
            template <typename Iter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr Iter call(
                std::size_t base_idx, Iter start, std::size_t num, F&& f)
            {
                if (num == 0)
                {
                    return start;
                }

                auto it = hpx::util::get_unwrapped(start);

                while (num >= 4)    //-V112
                {
                    HPX_INVOKE(f, *it, base_idx++);
                    HPX_INVOKE(f, *(it + 1), base_idx++);
                    HPX_INVOKE(f, *(it + 2), base_idx++);
                    HPX_INVOKE(f, *(it + 3), base_idx++);

                    it += 4;     //-V112
                    num -= 4;    //-V112
                }

                switch (num)
                {
                case 3:
                    HPX_INVOKE(f, *it, base_idx++);
                    HPX_INVOKE(f, *(it + 1), base_idx++);
                    HPX_INVOKE(f, *(it + 2), base_idx);
                    it += 3;
                    break;

                case 2:
                    HPX_INVOKE(f, *it, base_idx++);
                    HPX_INVOKE(f, *(it + 1), base_idx);
                    it += 2;
                    break;

                case 1:
                    HPX_INVOKE(f, *it, base_idx);
                    ++it;
                    break;

                default:
                    break;
                }

                return std::next(start, num);
            }

            template <typename Iter, typename CancelToken, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
                std::size_t base_idx, Iter it, std::size_t num,
                CancelToken& tok, F&& f)
            {
                if (tok.was_cancelled(base_idx))
                    return it;

                return call(base_idx, it, num, HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct loop_idx_n_t final
      : hpx::functional::detail::tag_fallback<loop_idx_n_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::loop_idx_n_t<ExPolicy>,
            std::size_t base_idx, Iter it, std::size_t count, F&& f)
        {
            using cat = std::iterator_traits<Iter>::iterator_category;
            return detail::loop_idx_n<cat>::call(
                base_idx, it, count, HPX_FORWARD(F, f));
        }

        template <typename Iter, typename CancelToken, typename F>
        friend HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter
        tag_fallback_invoke(hpx::parallel::util::loop_idx_n_t<ExPolicy>,
            std::size_t base_idx, Iter it, std::size_t count, CancelToken& tok,
            F&& f)
        {
            using cat = std::iterator_traits<Iter>::iterator_category;
            return detail::loop_idx_n<cat>::call(
                base_idx, it, count, tok, HPX_FORWARD(F, f));
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr loop_idx_n_t<ExPolicy> loop_idx_n =
        loop_idx_n_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_idx_n(
        std::size_t base_idx, Iter it, std::size_t count, F&& f)
    {
        return hpx::parallel::util::loop_idx_n_t<ExPolicy>{}(
            base_idx, it, count, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename CancelToken, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) loop_idx_n(
        std::size_t base_idx, Iter it, std::size_t count, CancelToken& tok,
        F&& f)
    {
        return hpx::parallel::util::loop_idx_n_t<ExPolicy>{}(
            base_idx, it, count, tok, HPX_FORWARD(F, f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helper class to repeatedly call a function a given number of times
        // starting from a given iterator position.
        HPX_CXX_CORE_EXPORT template <typename IterCat>
        struct accumulate_n
        {
            template <typename Iter, typename T, typename Pred>
            static constexpr T call(
                Iter it, std::size_t count, T init, Pred&& f)
            {
                for (/**/; count != 0; (void) --count, ++it)
                {
                    init = HPX_INVOKE(f, init, *it);
                }
                return init;
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename T, typename Pred>
    HPX_FORCEINLINE constexpr T accumulate_n(
        Iter it, std::size_t count, T init, Pred&& f)
    {
        using cat = std::iterator_traits<Iter>::iterator_category;
        return detail::accumulate_n<cat>::call(
            it, count, HPX_MOVE(init), HPX_FORWARD(Pred, f));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Iter, typename Sent,
        typename Reduce, typename Conv = hpx::identity>
    HPX_FORCEINLINE constexpr T accumulate(
        Iter first, Sent last, Reduce&& r, Conv&& conv = Conv())
    {
        T val = HPX_INVOKE(conv, *first);
        ++first;
        while (first != last)
        {
            val = HPX_INVOKE(r, val, HPX_INVOKE(conv, *first));
            ++first;
        }
        return val;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Iter1, typename Sent1,
        typename Iter2, typename Reduce, typename Conv>
    HPX_FORCEINLINE constexpr T accumulate(
        Iter1 first1, Sent1 last1, Iter2 first2, Reduce&& r, Conv&& conv)
    {
        T val = HPX_INVOKE(conv, *first1, *first2);
        ++first1;
        ++first2;
        while (first1 != last1)
        {
            val = HPX_INVOKE(r, val, HPX_INVOKE(conv, *first1, *first2));
            ++first1;
            ++first2;
        }
        return val;
    }
}    // namespace hpx::parallel::util
