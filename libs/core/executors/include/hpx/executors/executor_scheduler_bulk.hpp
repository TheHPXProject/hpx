//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/execution_base/stdexec_forward.hpp>

#include <hpx/execution/algorithms/bulk.hpp>
#include <hpx/execution_base/completion_scheduler.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/execution.hpp>
#include <hpx/execution_base/receiver.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/executors/executor_scheduler.hpp>
#include <hpx/modules/errors.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {
        template <typename Executor, typename Receiver, typename Shape,
            typename F>
        struct executor_bulk_receiver
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Executor> exec_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f_;

            template <typename Error>
            friend void tag_invoke(
                set_error_t, executor_bulk_receiver&& r, Error&& error) noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(r.receiver_), HPX_FORWARD(Error, error));
            }

            friend void tag_invoke(
                set_stopped_t, executor_bulk_receiver&& r) noexcept
            {
                hpx::execution::experimental::set_stopped(
                    HPX_MOVE(r.receiver_));
            }

            template <typename... Ts>
            friend void tag_invoke(
                set_value_t, executor_bulk_receiver&& r, Ts&&... ts) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        hpx::parallel::execution::bulk_sync_execute(
                            r.exec_, r.f_, r.shape_, ts...);

                        hpx::execution::experimental::set_value(
                            HPX_MOVE(r.receiver_), HPX_FORWARD(Ts, ts)...);
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(r.receiver_), HPX_MOVE(ep));
                    });
            }
        };

        template <typename Executor, typename Sender, typename Shape,
            typename F>
        struct executor_bulk_sender
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Executor> exec_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f_;

            using sender_concept = hpx::execution::experimental::sender_t;

            template <typename Env>
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
            friend auto tag_invoke(
                hpx::execution::experimental::get_completion_signatures_t,
                executor_bulk_sender const&, Env const&) -> hpx::execution::
                experimental::transform_completion_signatures<
                    hpx::execution::experimental::completion_signatures_of_t<
                        Sender, Env>,
                    hpx::execution::experimental::completion_signatures<
                        hpx::execution::experimental::set_error_t(
                            std::exception_ptr)>>;
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif

            struct env
            {
                std::decay_t<Sender> const& pred_snd;
                std::decay_t<Executor> const& exec;

                template <typename CPO>
                    requires(
                        meta::value<meta::one_of<CPO,
                            hpx::execution::experimental::set_error_t,
                            hpx::execution::experimental::set_stopped_t>> &&
                        hpx::execution::experimental::detail::
                            has_completion_scheduler_v<CPO,
                                std::decay_t<Sender>>)
                friend constexpr auto tag_invoke(
                    hpx::execution::experimental::get_completion_scheduler_t<
                        CPO>
                        tag,
                    env const& e) noexcept
                {
                    return tag(
                        hpx::execution::experimental::get_env(e.pred_snd));
                }

                friend constexpr auto tag_invoke(
                    hpx::execution::experimental::get_completion_scheduler_t<
                        hpx::execution::experimental::set_value_t>,
                    env const& e) noexcept
                {
                    return hpx::execution::experimental::executor_scheduler<
                        Executor>{e.exec};
                }
            };

            friend constexpr auto tag_invoke(
                hpx::execution::experimental::get_env_t,
                executor_bulk_sender const& s) noexcept
            {
                return env{s.sender_, s.exec_};
            }

            template <typename Receiver>
            friend auto tag_invoke(connect_t,
                executor_bulk_sender<Executor, Sender, Shape, F>&& s,
                Receiver&& receiver)
            {
                return hpx::execution::experimental::connect(
                    HPX_MOVE(s.sender_),
                    executor_bulk_receiver<Executor, std::decay_t<Receiver>,
                        Shape, F>{HPX_MOVE(s.exec_),
                        HPX_FORWARD(Receiver, receiver), HPX_MOVE(s.shape_),
                        HPX_MOVE(s.f_)});
            }

            template <typename Receiver>
            friend auto tag_invoke(connect_t,
                executor_bulk_sender<Executor, Sender, Shape, F>& s,
                Receiver&& receiver)
            {
                return hpx::execution::experimental::connect(s.sender_,
                    executor_bulk_receiver<Executor, std::decay_t<Receiver>,
                        Shape, F>{s.exec_, HPX_FORWARD(Receiver, receiver),
                        s.shape_, s.f_});
            }

            template <typename Receiver>
            friend auto tag_invoke(connect_t,
                executor_bulk_sender<Executor, Sender, Shape, F> const& s,
                Receiver&& receiver)
            {
                return hpx::execution::experimental::connect(s.sender_,
                    executor_bulk_receiver<Executor, std::decay_t<Receiver>,
                        Shape, F>{s.exec_, HPX_FORWARD(Receiver, receiver),
                        s.shape_, s.f_});
            }
        };
    }    // namespace detail

    template <typename Executor, typename Sender, typename Shape, typename F>
    auto tag_invoke(bulk_t, executor_scheduler<Executor> const& sched,
        Sender&& sender, Shape const& shape, F&& f)
    {
        return detail::executor_bulk_sender<Executor, std::decay_t<Sender>,
            Shape, std::decay_t<F>>{
            sched.exec_, HPX_FORWARD(Sender, sender), shape, HPX_FORWARD(F, f)};
    }
}    // namespace hpx::execution::experimental
