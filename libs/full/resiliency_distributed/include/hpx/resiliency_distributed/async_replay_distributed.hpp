//  Copyright (c) 2019-2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::resiliency::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, typename Pred, typename Action,
            typename Tuple>
        struct distributed_async_replay_helper
          : std::enable_shared_from_this<
                distributed_async_replay_helper<Result, Pred, Action, Tuple>>
        {
            template <typename Pred_, typename Action_, typename Tuple_>
            distributed_async_replay_helper(
                Pred_&& pred, Action_&& action, Tuple_&& tuple)
              : pred_(HPX_FORWARD(Pred_, pred))
              , action_(HPX_FORWARD(Action_, action))
              , t_(HPX_FORWARD(Tuple_, tuple))
            {
            }

            template <std::size_t... Is>
            hpx::future<Result> invoke_distributed(
                hpx::id_type id, hpx::util::index_pack<Is...>)
            {
                return hpx::async(action_, id, std::get<Is>(t_)...);
            }

            hpx::future<Result> call(
                std::vector<hpx::id_type> const& ids, std::size_t iteration = 0)
            {
                hpx::future<Result> f = invoke_distributed(ids.at(iteration),
                    hpx::util::make_index_pack<
                        std::tuple_size<Tuple>::value>{});

                // attach a continuation that will relaunch the task, if
                // necessary
                auto this_ = this->shared_from_this();
                return f.then(hpx::launch::sync,
                    [this_ = HPX_MOVE(this_), ids, iteration](
                        hpx::future<Result>&& f) {
                        if (f.has_exception())
                        {
                            // rethrow abort_replay_exception, if caught
                            auto ex = rethrow_on_abort_replay(f);

                            // execute the task again if an error occurred and
                            // this was not the last attempt
                            if (iteration != ids.size() - 1)
                            {
                                return this_->call(ids, iteration + 1);
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
                            if (iteration != ids.size() - 1)
                            {
                                return this_->call(ids, iteration + 1);
                            }

                            // throw aborting exception as attempts were
                            // exhausted
                            throw abort_replay_exception();
                        }

                        if (iteration != ids.size())
                        {
                            // return result
                            return hpx::make_ready_future(HPX_MOVE(result));
                        }

                        // throw aborting exception as attempts were
                        // exhausted
                        throw abort_replay_exception();
                    });
            }

            Pred pred_;
            Action action_;
            Tuple t_;
        };

        template <typename Result, typename Pred, typename Action,
            typename... Ts>
        std::shared_ptr<distributed_async_replay_helper<Result,
            typename std::decay<Pred>::type, typename std::decay<Action>::type,
            std::tuple<typename std::decay<Ts>::type...>>>
        make_distributed_async_replay_helper(
            Pred&& pred, Action&& action, Ts&&... ts)
        {
            using return_type = distributed_async_replay_helper<Result,
                typename std::decay<Pred>::type,
                typename std::decay<Action>::type,
                std::tuple<typename std::decay<Ts>::type...>>;

            return std::make_shared<return_type>(HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Action, action),
                std::make_tuple(HPX_FORWARD(Ts, ts)...));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given Action \a action on locality \a id.
    // Repeat launching on error exactly \a n times (except if
    // abort_replay_exception is thrown).
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<Action,
        hpx::id_type, Ts...>>
    tag_invoke(async_replay_t, std::vector<hpx::id_type> const& ids,
        Action&& action, Ts&&... ts)
    {
        HPX_ASSERT(ids.size() > 0);

        using result_type = hpx::util::detail::invoke_deferred_result_t<Action,
            hpx::id_type, Ts...>;

        auto helper = detail::make_distributed_async_replay_helper<result_type>(
            detail::replay_validator{}, HPX_FORWARD(Action, action),
            HPX_FORWARD(Ts, ts)...);

        return helper->call(ids);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Asynchronously launch given Action \a action on locality \a id.
    // Repeat launching on error exactly \a n times (except if
    // abort_replay_exception is thrown).
    HPX_CXX_EXPORT template <typename Pred, typename Action, typename... Ts>
    hpx::future<hpx::util::detail::invoke_deferred_result_t<Action,
        hpx::id_type, Ts...>>
    tag_invoke(async_replay_validate_t, std::vector<hpx::id_type> const& ids,
        Pred&& pred, Action&& action, Ts&&... ts)
    {
        HPX_ASSERT(ids.size() > 0);

        using result_type = hpx::util::detail::invoke_deferred_result_t<Action,
            hpx::id_type, Ts...>;

        auto helper = detail::make_distributed_async_replay_helper<result_type>(
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Action, action),
            HPX_FORWARD(Ts, ts)...);

        return helper->call(ids);
    }
}    // namespace hpx::resiliency::experimental

#endif
