//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <list>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct algorithm_result_helper
    {
        template <typename T_>
        HPX_FORCEINLINE static constexpr T_ call(T_&& val)
        {
            return HPX_FORWARD(T_, val);
        }
    };

    template <>
    struct algorithm_result_helper<future<void>>
    {
        HPX_FORCEINLINE static future<void> call(future<void>&& f)
        {
            return HPX_MOVE(f);
        }
    };

    template <typename Iterator>
    struct algorithm_result_helper<Iterator,
        std::enable_if_t<hpx::traits::is_segmented_local_iterator_v<Iterator>>>
    {
        using traits = hpx::traits::segmented_local_iterator_traits<Iterator>;

        HPX_FORCEINLINE static Iterator call(
            typename traits::local_raw_iterator&& it)
        {
            return traits::remote(HPX_MOVE(it));
        }
    };

    template <typename Iterator1, typename Iterator2>
    struct algorithm_result_helper<util::in_out_result<Iterator1, Iterator2>,
        std::enable_if_t<
            hpx::traits::is_segmented_local_iterator_v<Iterator1> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator2>>>
    {
        using traits1 = hpx::traits::segmented_local_iterator_traits<Iterator1>;
        using traits2 = hpx::traits::segmented_local_iterator_traits<Iterator2>;

        HPX_FORCEINLINE static util::in_out_result<
            typename traits1::local_iterator, typename traits2::local_iterator>
        call(util::in_out_result<typename traits1::local_raw_iterator,
            typename traits2::local_raw_iterator>&& p)
        {
            return util::in_out_result<typename traits1::local_iterator,
                typename traits2::local_iterator>{
                traits1::remote(HPX_MOVE(p.in)),
                traits2::remote(HPX_MOVE(p.out))};
        }
    };

    template <typename Iterator>
    struct algorithm_result_helper<util::min_max_result<Iterator>,
        std::enable_if_t<hpx::traits::is_segmented_local_iterator_v<Iterator>>>
    {
        using traits1 = hpx::traits::segmented_local_iterator_traits<Iterator>;

        static HPX_FORCEINLINE
            util::min_max_result<typename traits1::local_iterator>
            call(util::min_max_result<typename traits1::local_raw_iterator>&& p)
        {
            return util::min_max_result<typename traits1::local_iterator>{
                traits1::remote(HPX_MOVE(p.min)),
                traits1::remote(HPX_MOVE(p.max))};
        }
    };

    template <typename Iterator1, typename Iterator2, typename Iterator3>
    struct algorithm_result_helper<
        util::in_in_out_result<Iterator1, Iterator2, Iterator3>,
        std::enable_if_t<
            hpx::traits::is_segmented_local_iterator_v<Iterator1> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator2> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator3>>>
    {
        using traits1 = hpx::traits::segmented_local_iterator_traits<Iterator1>;
        using traits2 = hpx::traits::segmented_local_iterator_traits<Iterator2>;
        using traits3 = hpx::traits::segmented_local_iterator_traits<Iterator3>;

        HPX_FORCEINLINE static util::in_in_out_result<
            typename traits1::local_iterator, typename traits2::local_iterator,
            typename traits3::local_iterator>
        call(util::in_in_out_result<typename traits1::local_raw_iterator,
            typename traits2::local_raw_iterator,
            typename traits3::local_raw_iterator>&& p)
        {
            return util::in_in_out_result<typename traits1::local_iterator,
                typename traits2::local_iterator,
                typename traits3::local_iterator>{
                traits1::remote(HPX_MOVE(p.in1)),
                traits2::remote(HPX_MOVE(p.in2)),
                traits3::remote(HPX_MOVE(p.out))};
        }
    };

    template <typename Iterator>
    struct algorithm_result_helper<future<Iterator>,
        std::enable_if_t<hpx::traits::is_segmented_local_iterator_v<Iterator>>>
    {
        using traits = hpx::traits::segmented_local_iterator_traits<Iterator>;

        HPX_FORCEINLINE static future<Iterator> call(
            future<typename traits::local_raw_iterator>&& f)
        {
            return hpx::make_future<Iterator>(HPX_MOVE(f),
                [](typename traits::local_raw_iterator&& val) -> Iterator {
                    return traits::remote(HPX_MOVE(val));
                });
        }
    };

    template <typename Iterator1, typename Iterator2>
    struct algorithm_result_helper<
        future<util::in_out_result<Iterator1, Iterator2>>,
        std::enable_if_t<
            hpx::traits::is_segmented_local_iterator_v<Iterator1> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator2>>>
    {
        using traits1 = hpx::traits::segmented_local_iterator_traits<Iterator1>;
        using traits2 = hpx::traits::segmented_local_iterator_traits<Iterator2>;

        using arg_type =
            util::in_out_result<typename traits1::local_raw_iterator,
                typename traits2::local_raw_iterator>;
        using result_type =
            util::in_out_result<typename traits1::local_iterator,
                typename traits2::local_iterator>;

        HPX_FORCEINLINE static future<result_type> call(future<arg_type>&& f)
        {
            return hpx::make_future<result_type>(
                HPX_MOVE(f), [](arg_type&& p) -> result_type {
                    return {traits1::remote(p.in), traits2::remote(p.out)};
                });
        }
    };

    template <typename Iterator>
    struct algorithm_result_helper<future<util::min_max_result<Iterator>>,
        std::enable_if_t<hpx::traits::is_segmented_local_iterator_v<Iterator>>>
    {
        using traits = hpx::traits::segmented_local_iterator_traits<Iterator>;

        using arg_type =
            util::min_max_result<typename traits::local_raw_iterator>;
        using result_type =
            util::min_max_result<typename traits::local_iterator>;

        HPX_FORCEINLINE static future<result_type> call(future<arg_type>&& f)
        {
            return hpx::make_future<result_type>(
                HPX_MOVE(f), [](arg_type&& p) -> result_type {
                    return {traits::remote(p.min), traits::remote(p.max)};
                });
        }
    };

    template <typename Iterator1, typename Iterator2, typename Iterator3>
    struct algorithm_result_helper<
        future<util::in_in_out_result<Iterator1, Iterator2, Iterator3>>,
        std::enable_if_t<
            hpx::traits::is_segmented_local_iterator_v<Iterator1> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator2> ||
            hpx::traits::is_segmented_local_iterator_v<Iterator3>>>
    {
        using traits1 = hpx::traits::segmented_local_iterator_traits<Iterator1>;
        using traits2 = hpx::traits::segmented_local_iterator_traits<Iterator2>;
        using traits3 = hpx::traits::segmented_local_iterator_traits<Iterator3>;

        using arg_type =
            util::in_in_out_result<typename traits1::local_raw_iterator,
                typename traits2::local_raw_iterator,
                typename traits3::local_raw_iterator>;
        using result_type =
            util::in_in_out_result<typename traits1::local_iterator,
                typename traits2::local_iterator,
                typename traits3::local_iterator>;

        HPX_FORCEINLINE static future<result_type> call(future<arg_type>&& f)
        {
            return hpx::make_future<result_type>(
                HPX_MOVE(f), [](arg_type&& p) -> result_type {
                    return {traits1::remote(HPX_MOVE(p.in1)),
                        traits2::remote(HPX_MOVE(p.in2)),
                        traits3::remote(HPX_MOVE(p.out))};
                });
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Algo, typename ExPolicy, typename... Args>
    struct dispatcher
    {
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            typename std::decay_t<Algo>::result_type>;

        HPX_FORCEINLINE static result_type sequential(
            Algo const& algo, ExPolicy policy, Args... args)
        {
            using hpx::traits::segmented_local_iterator_traits;
            if constexpr (std::is_void_v<result_type>)
            {
                return algo.call2(HPX_FORWARD(ExPolicy, policy),
                    std::true_type(),
                    segmented_local_iterator_traits<std::decay_t<Args>>::local(
                        HPX_FORWARD(Args, args))...);
            }
            else
            {
                return detail::algorithm_result_helper<
                    result_type>::call(algo.call2(HPX_FORWARD(ExPolicy, policy),
                    std::true_type(),
                    segmented_local_iterator_traits<std::decay_t<Args>>::local(
                        HPX_FORWARD(Args, args))...));
            }
        }

        HPX_FORCEINLINE static result_type parallel(
            Algo const& algo, ExPolicy policy, Args... args)
        {
            using hpx::traits::segmented_local_iterator_traits;
            if constexpr (std::is_void_v<result_type>)
            {
                return algo.call2(HPX_FORWARD(ExPolicy, policy),
                    std::false_type(),
                    segmented_local_iterator_traits<std::decay_t<Args>>::local(
                        HPX_FORWARD(Args, args))...);
            }
            else
            {
                return detail::algorithm_result_helper<
                    result_type>::call(algo.call2(HPX_FORWARD(ExPolicy, policy),
                    std::false_type(),
                    segmented_local_iterator_traits<std::decay_t<Args>>::local(
                        HPX_FORWARD(Args, args))...));
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Algo, typename ExPolicy, typename IsSeq, typename F>
    struct algorithm_invoker_action;

    // sequential
    template <typename Algo, typename ExPolicy, typename R, typename... Args>
    struct algorithm_invoker_action<Algo, ExPolicy, std::true_type, R(Args...)>
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy, Args...),
            &dispatcher<Algo, ExPolicy, Args...>::sequential,
            algorithm_invoker_action<Algo, ExPolicy, std::true_type,
                R(Args...)>>::type
    {
    };

    // parallel
    template <typename Algo, typename ExPolicy, typename R, typename... Args>
    struct algorithm_invoker_action<Algo, ExPolicy, std::false_type, R(Args...)>
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy, Args...),
            &dispatcher<Algo, ExPolicy, Args...>::parallel,
            algorithm_invoker_action<Algo, ExPolicy, std::false_type,
                R(Args...)>>::type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Algo, typename ExPolicy, typename IsSeq,
        typename... Args>
    HPX_FORCEINLINE future<typename std::decay_t<Algo>::result_type>
    dispatch_async(
        id_type const& id, Algo&& algo, ExPolicy policy, IsSeq, Args&&... args)
    {
        using algo_type = std::decay_t<Algo>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            typename algo_type::result_type>;

        algorithm_invoker_action<algo_type, ExPolicy, typename IsSeq::type,
            result_type(hpx::util::decay_unwrap_t<Args>...)>
            act;

        return hpx::async(act, hpx::colocated(id), HPX_FORWARD(Algo, algo),
            HPX_MOVE(policy), HPX_FORWARD(Args, args)...);
    }

    HPX_CXX_EXPORT template <typename Algo, typename ExPolicy, typename IsSeq,
        typename... Args>
    HPX_FORCEINLINE typename std::decay_t<Algo>::result_type dispatch(
        id_type const& id, Algo&& algo, ExPolicy policy, IsSeq is_seq,
        Args&&... args)
    {
        // synchronously invoke remote operation
        future<typename std::decay_t<Algo>::result_type> f =
            dispatch_async(id, HPX_FORWARD(Algo, algo), HPX_MOVE(policy),
                is_seq, HPX_FORWARD(Args, args)...);
        f.wait();

        // handle any remote exceptions
        if (f.has_exception())
        {
            std::list<std::exception_ptr> errors;
            parallel::util::detail::handle_remote_exceptions<ExPolicy>::call(
                f.get_exception_ptr(),
                errors);    // NOLINT(bugprone-use-after-move)

            // NOLINTNEXTLINE(bugprone-use-after-move)
            HPX_ASSERT(errors.empty());
            throw exception_list(HPX_MOVE(errors));
        }
        return f.get();
    }
}    // namespace hpx::parallel::detail
