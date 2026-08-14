//  Copyright (c)      2025 Aditya Sapra
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/thrust/detail/algorithm_map.hpp>
#include <hpx/thrust/policy.hpp>

#include <type_traits>
#include <utility>

namespace hpx::thrust {

    /// \brief ADL hook dispatching HPX algorithm CPOs to Thrust backends.
    ///
    /// When a Thrust execution policy is passed to an HPX algorithm CPO,
    /// this hook maps the call to the corresponding Thrust
    /// implementation. Async policies wrap the result in a future.
    ///
    /// \tparam Tag           The algorithm CPO tag type.
    /// \tparam ThrustPolicy  A Thrust execution policy satisfying
    ///                       is_thrust_execution_policy.
    /// \tparam Args          Arguments forwarded to the Thrust algorithm.
    ///
    /// \param tag     The algorithm CPO tag instance.
    /// \param policy  Thrust execution policy controlling dispatch.
    /// \param args    Arguments forwarded to the mapped algorithm.
    ///
    /// \returns The algorithm result, or a future thereof for async
    ///   policies.
    template <typename Tag, typename ThrustPolicy, typename... Args>
        requires(is_thrust_execution_policy_v<std::decay_t<ThrustPolicy>> &&
            detail::is_algorithm_mapped<Tag, ThrustPolicy, Args...>)
    decltype(auto) hpx_invoke(Tag tag, ThrustPolicy&& policy, Args&&... args)
    {
        if constexpr (hpx::is_async_execution_policy_v<
                          std::decay_t<ThrustPolicy>>)
        {
            using result_type = decltype(detail::algorithm_map<Tag>::invoke(
                std::declval<ThrustPolicy>(), std::declval<Args>()...));
            if constexpr (std::is_void_v<result_type>)
            {
                detail::algorithm_map<Tag>::invoke(
                    HPX_FORWARD(ThrustPolicy, policy),
                    HPX_FORWARD(Args, args)...);
                return policy.get_future();    // future<void>
            }
            else
            {
                auto result = detail::algorithm_map<Tag>::invoke(
                    HPX_FORWARD(ThrustPolicy, policy),
                    HPX_FORWARD(Args, args)...);
                return policy.get_future().then(
                    [result = HPX_MOVE(result)](auto&& f) mutable {
                        f.get();
                        return HPX_MOVE(result);
                    });    // future<T>
            }
        }
        else
        {
            return detail::algorithm_map<Tag>::invoke(
                HPX_FORWARD(ThrustPolicy, policy), HPX_FORWARD(Args, args)...);
        }
    }
}    // namespace hpx::thrust
