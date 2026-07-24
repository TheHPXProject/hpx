//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution/algorithms/future_sender.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {
    // The as_sender CPO can be used to adapt any HPX future as a sender. The
    // value provided by the future will be used to call set_value on the
    // connected receiver once the future has become ready. If the future is
    // exceptional, set_error will be invoked on the connected receiver.
    //
    // The difference to keep_future is that as_future propagates the value
    // stored in the future while keep_future will propagate the future instance
    // itself.
    HPX_CXX_CORE_EXPORT inline constexpr struct as_sender_t final
    {
        // clang-format off
        template <typename Future,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_future_v<std::decay_t<Future>>
            )>
        // clang-format on
        constexpr HPX_FORCEINLINE auto operator()(Future&& future) const
        {
            return detail::future_sender<std::decay_t<Future>>{
                HPX_FORWARD(Future, future)};
        }

        // Scheduler-aware overload: wraps the future into a sender whose
        // environment exposes the given scheduler as completion scheduler.
        template <typename Future, typename Scheduler>
            requires(hpx::traits::is_future_v<std::decay_t<Future>> &&
                hpx::execution::experimental::scheduler<
                    std::decay_t<Scheduler>>)
        constexpr HPX_FORCEINLINE auto operator()(
            Future&& future, Scheduler&& scheduler) const
        {
            return detail::future_sender_with_scheduler<std::decay_t<Future>,
                std::decay_t<Scheduler>>{
                HPX_FORWARD(Future, future), HPX_FORWARD(Scheduler, scheduler)};
        }

        constexpr HPX_FORCEINLINE auto operator()() const
        {
            return detail::partial_algorithm<as_sender_t>{};
        }
    } as_sender{};
}    // namespace hpx::execution::experimental
