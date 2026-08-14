//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2023-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>

#include <cstddef>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads {

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT execution_context final
      : hpx::execution_base::context_base
    {
        hpx::execution_base::resource_base const& resource()
            const noexcept override
        {
            return resource_;
        }

        hpx::execution_base::resource_base resource_;
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT execution_agent
      : hpx::execution_base::agent_base
    {
        explicit execution_agent(
            coroutines::detail::coroutine_impl* coroutine) noexcept;

        std::string description() const override;

        execution_context const& context() const noexcept override
        {
            return context_;
        }

        void yield(char const* desc) override;
        bool yield_k(std::size_t k, char const* desc) override;
        void suspend(char const* desc) override;
        void resume(
            hpx::threads::thread_priority priority, char const* desc) override;
        void abort(char const* desc) override;
        threads::thread_restart_state sleep_for(
            hpx::chrono::steady_duration const& sleep_duration,
            hpx::move_only_function<bool()>&& wait_cond,
            char const* desc) override;
        threads::thread_restart_state sleep_until(
            hpx::chrono::steady_time_point const& sleep_time,
            hpx::move_only_function<bool()>&& wait_cond,
            char const* desc) override;

    private:
        coroutines::detail::coroutine_stackful_self self_;

        hpx::threads::thread_restart_state do_yield(
            char const* desc, threads::thread_schedule_state state);

        void do_resume(hpx::threads::thread_priority priority, char const* desc,
            hpx::threads::thread_restart_state statex) const;

        execution_context context_;
    };
}    // namespace hpx::threads

#include <hpx/config/warnings_suffix.hpp>
