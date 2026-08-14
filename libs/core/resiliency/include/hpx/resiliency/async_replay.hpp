//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2018-2026 Hartmut Kaiser
//  Copyright (c) 2018-2019 Adrian Serio
//  Copyright (c) 2019 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/resiliency/config.hpp>
#include <hpx/resiliency/resiliency_cpos.hpp>
#include <hpx/resiliency/util.hpp>

#include <hpx/functional/invoke.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::resiliency::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Result, typename Pred,
            typename F, typename Tuple>
        struct async_replay_helper
          : std::enable_shared_from_this<
                async_replay_helper<Result, Pred, F, Tuple>>
        {
            template <typename Pred_, typename F_, typename Tuple_>
            async_replay_helper(Pred_&& pred, F_&& f, Tuple_&& tuple)
              : pred_(HPX_FORWARD(Pred_, pred))
              , f_(HPX_FORWARD(F_, f))
              , t_(HPX_FORWARD(Tuple_, tuple))
            {
            }

            template <std::size_t... Is>
            hpx::future<Result> invoke(hpx::util::index_pack<Is...>)
            {
                return hpx::async(f_, std::get<Is>(t_)...);
            }

            hpx::future<Result> call(std::size_t n)
            {
                // launch given function asynchronously
                hpx::future<Result> f = invoke(
                    hpx::util::make_index_pack<std::tuple_size_v<Tuple>>{});

                // attach a continuation that will relaunch the task, if
                // necessary
                auto this_ = this->shared_from_this();
                return f.then(hpx::launch::sync,
                    [this_ = HPX_MOVE(this_), n](hpx::future<Result>&& f) {
                        if (f.has_exception())
                        {
                            // rethrow abort_replay_exception, if caught
                            auto ex = rethrow_on_abort_replay(f);

                            // execute the task again if an error occurred and
                            // this was not the last attempt
                            if (n != 0)
                            {
                                return this_->call(n - 1);
                            }

                            // rethrow exception if the number of replays has
                            // been exhausted
                            std::rethrow_exception(ex);
                        }

                        auto&& result = f.get();

                        if (!HPX_INVOKE(this_->pred_, result))
                        {
                            // execute the task again if an error occurred and
                            // this was not the last attempt
                            if (n != 0)
                            {
                                return this_->call(n - 1);
                            }

                            // throw aborting exception as attempts were
                            // exhausted
                            throw abort_replay_exception();
                        }

                        return hpx::make_ready_future(HPX_MOVE(result));
                    });
            }

            Pred pred_;
            F f_;
            Tuple t_;
        };

        HPX_CXX_CORE_EXPORT template <typename Result, typename Pred,
            typename F, typename... Ts>
        std::shared_ptr<async_replay_helper<Result, std::decay_t<Pred>,
            std::decay_t<F>, std::tuple<std::decay_t<Ts>...>>>
        make_async_replay_helper(Pred&& pred, F&& f, Ts&&... ts)
        {
            using return_type = async_replay_helper<Result, std::decay_t<Pred>,
                std::decay_t<F>, std::tuple<std::decay_t<Ts>...>>;

            return std::make_shared<return_type>(HPX_FORWARD(Pred, pred),
                HPX_FORWARD(F, f), std::make_tuple(HPX_FORWARD(Ts, ts)...));
        }
    }    // namespace detail

    /// \brief ADL hook for async_replay_validate CPO.
    ///
    /// Asynchronously launches \a f, validating results with \a pred.
    /// Repeats on error up to \a n times (aborts on
    /// abort_replay_exception).
    ///
    /// \param tag    CPO tag (async_replay_validate_t).
    /// \param n      Maximum number of retry attempts.
    /// \param pred   Predicate validating each invocation result.
    /// \param f      Callable to invoke asynchronously.
    /// \param ts     Arguments forwarded to \a f.
    ///
    /// \returns future with the first valid result of \a f.
    HPX_CXX_CORE_EXPORT template <typename Pred, typename F, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(
        async_replay_validate_t, std::size_t n, Pred&& pred, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        auto helper = detail::make_async_replay_helper<result_type>(
            HPX_FORWARD(Pred, pred), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);

        return helper->call(n);
    }

    /// \brief ADL hook for async_replay CPO.
    ///
    /// Asynchronously launches \a f, repeating on error up to \a n times
    /// (aborts on abort_replay_exception).
    ///
    /// \param tag  CPO tag (async_replay_t).
    /// \param n    Maximum number of retry attempts.
    /// \param f    Callable to invoke asynchronously.
    /// \param ts   Arguments forwarded to \a f.
    ///
    /// \returns future with the first successful result of \a f.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
    hpx_invoke(async_replay_t, std::size_t n, F&& f, Ts&&... ts)
    {
        using result_type =
            hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

        auto helper = detail::make_async_replay_helper<result_type>(
            detail::replay_validator{}, HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...);

        return helper->call(n);
    }
}    // namespace hpx::resiliency::experimental
