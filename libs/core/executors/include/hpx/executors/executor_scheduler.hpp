//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/executor_scheduler.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    // Forward declarations
    template <typename Executor>
    struct executor_scheduler;

    template <typename Executor>
    struct executor_sender;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Executor, typename Receiver>
    struct executor_operation_state
    {
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Executor> exec_;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;

        template <typename Exec, typename Recv>
        executor_operation_state(Exec&& exec, Recv&& recv)
          : exec_(HPX_FORWARD(Exec, exec))
          , receiver_(HPX_FORWARD(Recv, recv))
        {
        }

        executor_operation_state(executor_operation_state&&) = delete;
        executor_operation_state(executor_operation_state const&) = delete;
        executor_operation_state& operator=(
            executor_operation_state&&) = delete;
        executor_operation_state& operator=(
            executor_operation_state const&) = delete;

        ~executor_operation_state() = default;

        friend void tag_invoke(start_t, executor_operation_state& os) noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    hpx::parallel::execution::post(os.exec_,
                        [receiver = HPX_MOVE(os.receiver_)]() mutable {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver));
                        });
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(os.receiver_), HPX_MOVE(ep));
                });
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Executor>
    struct executor_sender
    {
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Executor> exec_;

        using completion_signatures =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_value_t(),
                hpx::execution::experimental::set_error_t(std::exception_ptr)>;

        template <typename Env>
        friend auto tag_invoke(
            hpx::execution::experimental::get_completion_signatures_t,
            executor_sender const&, Env) noexcept -> completion_signatures;

        template <typename Receiver>
        friend executor_operation_state<Executor, Receiver> tag_invoke(
            connect_t, executor_sender&& s, Receiver&& receiver)
        {
            return {HPX_MOVE(s.exec_), HPX_FORWARD(Receiver, receiver)};
        }

        template <typename Receiver>
        friend executor_operation_state<Executor, Receiver> tag_invoke(
            connect_t, executor_sender const& s, Receiver&& receiver)
        {
            return {s.exec_, HPX_FORWARD(Receiver, receiver)};
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Executor>
    struct executor_scheduler
    {
        using executor_type = std::decay_t<Executor>;

        HPX_NO_UNIQUE_ADDRESS executor_type exec_;

        constexpr executor_scheduler() = default;

        template <typename Exec,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Exec>, executor_scheduler>>>
        explicit executor_scheduler(Exec&& exec)
          : exec_(HPX_FORWARD(Exec, exec))
        {
        }

        constexpr bool operator==(executor_scheduler const& rhs) const noexcept
        {
            return exec_ == rhs.exec_;
        }

        constexpr bool operator!=(executor_scheduler const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        friend executor_sender<Executor> tag_invoke(
            schedule_t, executor_scheduler&& sched)
        {
            return {HPX_MOVE(sched.exec_)};
        }

        friend executor_sender<Executor> tag_invoke(
            schedule_t, executor_scheduler const& sched)
        {
            return {sched.exec_};
        }
    };
}    // namespace hpx::execution::experimental
