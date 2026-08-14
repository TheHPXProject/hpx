//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2024 Hartmut Kaiser
//  Copyright (c) 2018-2019 Adrian Serio
//  Copyright (c) 2019 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/resiliency/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/resiliency/resiliency_cpos.hpp>
#include <hpx/resiliency/util.hpp>

#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>

#include <cstddef>
#include <exception>
#include <utility>
#include <vector>

namespace hpx::resiliency::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Vote, typename Pred, typename F,
            typename... Ts>
        hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
        async_replicate_vote_validate(
            std::size_t n, Vote&& vote, Pred&& pred, F&& f, Ts&&... ts)
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

            // launch given function n times
            std::vector<hpx::future<result_type>> results;
            results.reserve(n);

            for (std::size_t i = 0; i != n; ++i)
            {
                results.emplace_back(hpx::async(f, ts...));
            }

            // wait for all threads to finish executing and return the first
            // result that passes the predicate, properly handle exceptions
            return hpx::dataflow(
                // do not schedule new thread for the lambda
                hpx::launch::sync,
                [pred = HPX_FORWARD(Pred, pred), vote = HPX_FORWARD(Vote, vote),
                    n](std::vector<hpx::future<result_type>>&& results) mutable
                    -> result_type {
                    // Store all valid results
                    std::vector<result_type> valid_results;
                    valid_results.reserve(n);

                    std::exception_ptr ex;

                    for (auto& f : results)
                    {
                        HPX_ASSERT(f.is_ready());
                        if (f.has_exception())
                        {
                            // rethrow abort_replicate_exception, if caught
                            ex = detail::rethrow_on_abort_replicate(f);
                        }
                        else
                        {
                            auto&& result = f.get();
                            if (HPX_INVOKE(pred, result))
                            {
                                valid_results.emplace_back(HPX_MOVE(result));
                            }
                        }
                    }

                    if (!valid_results.empty())
                    {
                        return HPX_INVOKE(
                            HPX_FORWARD(Vote, vote), HPX_MOVE(valid_results));
                    }

                    if (bool(ex))
                        std::rethrow_exception(ex);

                    // throw aborting exception no correct results ere produced
                    throw abort_replicate_exception{};
                },
                HPX_MOVE(results));
        }
    }    // namespace detail

    /// \brief ADL hook for async_replicate_vote_validate CPO.
    ///
    /// Asynchronously launches \a f exactly \a n times, validates results
    /// with \a pred, then runs valid results through \a vote.
    ///
    /// \param tag   CPO tag (async_replicate_vote_validate_t).
    /// \param n     Number of replicated invocations.
    /// \param vote  Voting function selecting among valid results.
    /// \param pred  Predicate validating each invocation result.
    /// \param f     Callable to invoke asynchronously.
    /// \param ts    Arguments forwarded to \a f.
    ///
    /// \returns future with the voted-upon result.
    HPX_CXX_CORE_EXPORT template <typename Vote, typename Pred, typename F,
        typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(async_replicate_vote_validate_t, std::size_t n, Vote&& vote,
        Pred&& pred, F&& f, Ts&&... ts)
    {
        return detail::async_replicate_vote_validate(n, HPX_FORWARD(Vote, vote),
            HPX_FORWARD(Pred, pred), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }

    /// \brief ADL hook for async_replicate_vote CPO.
    ///
    /// Asynchronously launches \a f exactly \a n times, runs valid
    /// results through \a vote. Validation uses the default exception
    /// check.
    ///
    /// \param tag   CPO tag (async_replicate_vote_t).
    /// \param n     Number of replicated invocations.
    /// \param vote  Voting function selecting among valid results.
    /// \param f     Callable to invoke asynchronously.
    /// \param ts    Arguments forwarded to \a f.
    ///
    /// \returns future with the voted-upon result.
    HPX_CXX_CORE_EXPORT template <typename Vote, typename F, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(
        async_replicate_vote_t, std::size_t n, Vote&& vote, F&& f, Ts&&... ts)
    {
        return detail::async_replicate_vote_validate(n, HPX_FORWARD(Vote, vote),
            detail::replicate_validator{}, HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...);
    }

    /// \brief ADL hook for async_replicate_validate CPO.
    ///
    /// Asynchronously launches \a f exactly \a n times, validates results
    /// with \a pred. Returns the first valid result.
    ///
    /// \param tag   CPO tag (async_replicate_validate_t).
    /// \param n     Number of replicated invocations.
    /// \param pred  Predicate validating each invocation result.
    /// \param f     Callable to invoke asynchronously.
    /// \param ts    Arguments forwarded to \a f.
    ///
    /// \returns future with the first valid result.
    HPX_CXX_CORE_EXPORT template <typename Pred, typename F, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(async_replicate_validate_t, std::size_t n, Pred&& pred, F&& f,
        Ts&&... ts)
    {
        return detail::async_replicate_vote_validate(n,
            detail::replicate_voter{}, HPX_FORWARD(Pred, pred),
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }

    /// \brief ADL hook for async_replicate CPO.
    ///
    /// Asynchronously launches \a f exactly \a n times. Validates by
    /// checking for exceptions. Returns the first valid result.
    ///
    /// \param tag  CPO tag (async_replicate_t).
    /// \param n    Number of replicated invocations.
    /// \param f    Callable to invoke asynchronously.
    /// \param ts   Arguments forwarded to \a f.
    ///
    /// \returns future with the first non-throwing result.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(async_replicate_t, std::size_t n, F&& f, Ts&&... ts)
    {
        return detail::async_replicate_vote_validate(n,
            detail::replicate_voter{}, detail::replicate_validator{},
            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::resiliency::experimental
