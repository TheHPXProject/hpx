//  Copyright (c) 2007-2025 Hartmut Kaiser
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

    // Forward declaration
    template <typename Executor>
    struct executor_scheduler;

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
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

            void start() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (
                            std::is_same_v<
                                hpx::traits::executor_execution_category_t<
                                    std::decay_t<Executor>>,
                                hpx::execution::sequenced_execution_tag>)
                        {
                            // For sequenced executors, invoke inline
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver_));
                        }
                        else
                        {
                            hpx::parallel::execution::post(exec_,
                                [receiver = HPX_MOVE(receiver_)]() mutable {
                                    hpx::execution::experimental::set_value(
                                        HPX_MOVE(receiver));
                                });
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver_), HPX_MOVE(ep));
                    });
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Executor>
        struct executor_sender
        {
            using sender_concept = hpx::execution::experimental::sender_t;

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Executor> exec_;

            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>;

            template <typename Self, typename... Env>
            static consteval auto get_completion_signatures() noexcept
                -> completion_signatures
            {
                return {};
            }

            constexpr auto get_env() const noexcept
            {
                struct env
                {
                    std::decay_t<Executor> const& exec_;

                    constexpr auto query(hpx::execution::experimental::
                            get_completion_scheduler_t<
                                hpx::execution::experimental::set_value_t>)
                        const noexcept
                    {
                        return executor_scheduler<Executor>{exec_};
                    }
                };
                return env{exec_};
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) &&
            {
                return executor_operation_state<Executor,
                    std::decay_t<Receiver>>{
                    HPX_MOVE(exec_), HPX_FORWARD(Receiver, receiver)};
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) const&
            {
                return executor_operation_state<Executor,
                    std::decay_t<Receiver>>{
                    exec_, HPX_FORWARD(Receiver, receiver)};
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Executor>
    struct executor_scheduler
    {
        using executor_type = std::decay_t<Executor>;

        HPX_NO_UNIQUE_ADDRESS executor_type exec_;

        constexpr executor_scheduler() = default;

        template <typename Exec>
            requires(!std::is_same_v<std::decay_t<Exec>, executor_scheduler>)
        constexpr explicit executor_scheduler(Exec&& exec) noexcept(
            std::is_nothrow_constructible_v<executor_type, Exec>)
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

        constexpr detail::executor_sender<Executor> schedule() const&
            noexcept(std::is_nothrow_copy_constructible_v<executor_type>)
        {
            return {exec_};
        }

        constexpr detail::executor_sender<Executor> schedule() && noexcept(
            std::is_nothrow_move_constructible_v<executor_type>)
        {
            return {HPX_MOVE(exec_)};
        }
    };
}    // namespace hpx::execution::experimental