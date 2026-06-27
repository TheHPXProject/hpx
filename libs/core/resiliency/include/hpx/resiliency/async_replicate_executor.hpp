//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2025 Hartmut Kaiser
//  Copyright (c) 2018-2019 Adrian Serio
//  Copyright (c) 2019 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/resiliency/config.hpp>
#include <hpx/resiliency/async_replicate.hpp>
#include <hpx/resiliency/resiliency_cpos.hpp>

#include <hpx/modules/async_local.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <cstddef>
#include <exception>
#include <utility>
#include <vector>

namespace hpx::resiliency::experimental {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Result>
        struct async_replicate_vote_validate_executor
        {
            template <typename Pred, typename T, typename U>
            static void emplace_result(
                Pred&& pred, std::vector<T>& valid_results, U&& result)
            {
                if (HPX_INVOKE(pred, result))
                {
                    valid_results.emplace_back(HPX_FORWARD(U, result));
                }
            }

            template <typename Pred, typename T, typename U>
            static void emplace_result(Pred&& pred,
                std::vector<T>& valid_results, std::vector<U>&& results)
            {
                for (auto&& result : HPX_MOVE(results))
                {
                    emplace_result(pred, valid_results, HPX_MOVE(result));
                }
            }

            template <typename Executor, typename Vote, typename Pred,
                typename F, typename... Ts>
            static hpx::traits::executor_future_t<Executor, Result> call(
                Executor&& exec, std::size_t n, Vote&& vote, Pred&& pred, F&& f,
                Ts&&... ts)
            {
                using result_type =
                    typename hpx::util::detail::invoke_deferred_result<F,
                        Ts...>::type;

                // launch given function n times
                auto func = [f = HPX_FORWARD(F, f),
                                ... ts = HPX_FORWARD(Ts, ts)](
                                std::size_t) mutable -> result_type {
                    // ignore argument (invocation count of bulk_execute)
                    return HPX_INVOKE(f, ts...);
                };

                auto&& results = hpx::parallel::execution::bulk_async_execute(
                    hpx::execution::to_hierarchical_spawning(
                        HPX_FORWARD(Executor, exec)),
                    HPX_MOVE(func), n);

                // wait for all threads to finish executing and return the first
                // result that passes the predicate, properly handle exceptions
                // do not schedule new thread for the lambda
                return hpx::dataflow(
                    hpx::launch::sync,
                    [n, pred = HPX_FORWARD(Pred, pred),
                        vote = HPX_FORWARD(Vote, vote)](
                        auto&& results) mutable -> result_type {
                        // Store all valid results
                        std::vector<result_type> valid_results;
                        valid_results.reserve(n);

                        std::exception_ptr ex;

                        // clang-format off
                        if constexpr (
                            hpx::traits::is_future_v<decltype(results)>)
                        // clang-format on
                        {
                            if (results.has_exception())
                            {
                                // rethrow abort_replicate_exception, if caught
                                ex =
                                    detail::rethrow_on_abort_replicate(results);
                            }
                            else
                            {
                                emplace_result(HPX_FORWARD(Pred, pred),
                                    valid_results, results.get());
                            }
                        }
                        else
                        {
                            for (auto&& fut :
                                HPX_FORWARD(decltype(results), results))
                            {
                                if (fut.has_exception())
                                {
                                    // rethrow abort_replicate_exception, if
                                    // caught
                                    ex =
                                        detail::rethrow_on_abort_replicate(fut);
                                }
                                else
                                {
                                    auto&& result = fut.get();
                                    if (HPX_INVOKE(pred, result))
                                    {
                                        valid_results.emplace_back(
                                            HPX_MOVE(result));
                                    }
                                }
                            }
                        }

                        if (!valid_results.empty())
                        {
                            return hpx::invoke(HPX_FORWARD(Vote, vote),
                                HPX_MOVE(valid_results));
                        }

                        if (static_cast<bool>(ex))
                        {
                            std::rethrow_exception(ex);
                        }

                        // throw aborting exception no correct results ere
                        // produced
                        throw abort_replicate_exception{};
                    },
                    HPX_MOVE(results));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <>
        struct async_replicate_vote_validate_executor<void>
        {
            template <typename Executor, typename Vote, typename Pred,
                typename F, typename... Ts>
            static typename hpx::traits::executor_future<Executor, void>::type
            call(Executor&& exec, std::size_t n, Vote&&, Pred&&, F&& f,
                Ts&&... ts)
            {
                // launch given function n times
                auto func = [f = HPX_FORWARD(F, f),
                                ... ts = HPX_FORWARD(Ts, ts)](
                                std::size_t) mutable {
                    // ignore argument (invocation count of bulk_execute)
                    HPX_INVOKE(f, ts...);

                    // return non-void result to force executor into providing a
                    // future for each invocation (returning void might optimize
                    // bulk_async_execute to return just a single future)
                    return 0;
                };

                auto&& results = hpx::parallel::execution::bulk_async_execute(
                    hpx::execution::to_hierarchical_spawning(
                        HPX_FORWARD(Executor, exec)),
                    HPX_MOVE(func), n);

                // wait for all threads to finish executing and return the first
                // result that passes the predicate, properly handle exceptions
                // do not schedule new thread for the lambda
                return hpx::dataflow(
                    hpx::launch::sync,
                    [](auto&& results) mutable -> void {
                        std::exception_ptr ex;

                        std::size_t count = 0;
                        // clang-format off
                        if constexpr (hpx::traits::is_future_v<
                                          decltype(results)>)
                        // clang-format on
                        {
                            if (results.has_exception())
                            {
                                // rethrow abort_replicate_exception, if caught
                                ex =
                                    detail::rethrow_on_abort_replicate(results);
                            }
                            else
                            {
                                ++count;
                            }
                        }
                        else
                        {
                            for (auto&& f :
                                HPX_FORWARD(decltype(results), results))
                            {
                                if (f.has_exception())
                                {
                                    // rethrow abort_replicate_exception, if
                                    // caught
                                    ex = detail::rethrow_on_abort_replicate(f);
                                }
                                else
                                {
                                    ++count;
                                }
                            }
                        }

                        if (count != 0)
                        {
                            return;
                        }

                        if (static_cast<bool>(ex))
                        {
                            std::rethrow_exception(ex);
                        }

                        // throw aborting exception if no correct results were
                        // produced
                        throw abort_replicate_exception{};
                    },
                    HPX_MOVE(results));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given function f exactly n times. Verify the result
    // of those invocations using the given predicate pred. Run all the valid
    // results against a user provided voting function. Return the valid output.
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Vote,
        typename Pred, typename F, typename... Ts>
        requires(one_way_executor<Executor> || two_way_executor<Executor>)
    decltype(auto) tag_invoke(async_replicate_vote_validate_t, Executor&& exec,
        std::size_t n, Vote&& vote, Pred&& pred, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        return detail::async_replicate_vote_validate_executor<
            result_type>::call(HPX_FORWARD(Executor, exec), n,
            HPX_FORWARD(Vote, vote), HPX_FORWARD(Pred, pred), HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given function f exactly n times. Verify the result
    // of those invocations using the given predicate pred. Run all the valid
    // results against a user provided voting function. Return the valid output.
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Vote, typename F,
        typename... Ts>
        requires(one_way_executor<Executor> || two_way_executor<Executor>)
    decltype(auto) tag_invoke(async_replicate_vote_t, Executor&& exec,
        std::size_t n, Vote&& vote, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        return detail::async_replicate_vote_validate_executor<
            result_type>::call(HPX_FORWARD(Executor, exec), n,
            HPX_FORWARD(Vote, vote), detail::replicate_validator{},
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given function f exactly n times. Verify the result
    // of those invocations using the given predicate pred. Return the first
    // valid result.
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Pred, typename F,
        typename... Ts>
        requires(one_way_executor<Executor> || two_way_executor<Executor>)
    decltype(auto) tag_invoke(async_replicate_validate_t, Executor&& exec,
        std::size_t n, Pred&& pred, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        return detail::async_replicate_vote_validate_executor<
            result_type>::call(HPX_FORWARD(Executor, exec), n,
            detail::replicate_voter{}, HPX_FORWARD(Pred, pred),
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given function f exactly n times. Verify
    // the result of those invocations by checking for exception.
    // Return the first valid result.
    HPX_CXX_CORE_EXPORT template <typename Executor, typename F, typename... Ts>
        requires(one_way_executor<Executor> || two_way_executor<Executor>)
    decltype(auto) tag_invoke(
        async_replicate_t, Executor&& exec, std::size_t n, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        return detail::async_replicate_vote_validate_executor<
            result_type>::call(HPX_FORWARD(Executor, exec), n,
            detail::replicate_voter{}, detail::replicate_validator{},
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::resiliency::experimental
