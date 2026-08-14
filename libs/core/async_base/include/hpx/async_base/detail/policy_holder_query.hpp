//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/async_base/scheduling_properties.hpp>
#include <hpx/modules/coroutines.hpp>

namespace hpx::detail {

    template <typename Derived>
    struct policy_holder;

    template <typename Policy>
    struct policy_holder_query
    {
        [[nodiscard]] constexpr Policy query(
            execution::experimental::with_priority_t,
            threads::thread_priority priority) const noexcept
        {
            Policy policy = static_cast<Policy const&>(*this);
            policy.set_priority(priority);
            return policy;
        }

        [[nodiscard]] constexpr threads::thread_priority query(
            execution::experimental::get_priority_t) const noexcept
        {
            return static_cast<Policy const&>(*this).get_priority();
        }

        [[nodiscard]] constexpr Policy query(
            execution::experimental::with_stacksize_t,
            threads::thread_stacksize stacksize) const noexcept
        {
            Policy policy = static_cast<Policy const&>(*this);
            policy.set_stacksize(stacksize);
            return policy;
        }

        [[nodiscard]] constexpr threads::thread_stacksize query(
            execution::experimental::get_stacksize_t) const noexcept
        {
            return static_cast<Policy const&>(*this).get_stacksize();
        }

        [[nodiscard]] constexpr Policy query(
            execution::experimental::with_hint_t,
            threads::thread_schedule_hint hint) const noexcept
        {
            Policy policy = static_cast<Policy const&>(*this);
            policy.set_hint(hint);
            return policy;
        }

        [[nodiscard]] constexpr threads::thread_schedule_hint query(
            execution::experimental::get_hint_t) const noexcept
        {
            return static_cast<Policy const&>(*this).get_hint();
        }

    private:
        friend Policy;
        template <typename T>
        friend struct policy_holder;
        constexpr policy_holder_query() = default;
    };

}    // namespace hpx::detail
