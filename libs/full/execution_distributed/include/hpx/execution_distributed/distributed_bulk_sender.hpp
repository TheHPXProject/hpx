//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file distributed_bulk_sender.hpp
/// \brief P2300-compliant distributed bulk sender adaptor.
///
/// Provides a bulk sender that executes data-parallel work on a remote
/// HPX locality via the distributed_scheduler. The sender intercepts
/// ex::bulk() when the upstream sender's completion scheduler is a
/// distributed_scheduler, and dispatches a shape-indexed invocation
/// across the parcelport.
///
/// Architecture:
///   upstream_sender
///       |
///       v
///   ex::bulk(shape, f)
///       |  (completion scheduler == distributed_scheduler)
///       v
///   distributed_bulk_sender<Sender, Shape, F>
///       |
///       v  (connect + start)
///   local_receiver wraps downstream_receiver
///       |  set_value(Ts...)
///       |  => for each index in shape: invoke f(index, ts...)
///       |  => forward set_value(ts...) to downstream
///       v
///   downstream_receiver
///
/// Current Status:
///   Local fallback loop over the shape. Remote parcelport dispatch
///   will be added once the component-based receiver marshalling
///   infrastructure (nvexec-style) is complete.

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)

#include <hpx/execution_distributed/distributed_scheduler.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::distributed::experimental::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Distributed bulk sender: wraps an upstream sender with shape + function
    // for data-parallel execution on a remote locality.
    template <typename Sender, typename Shape, typename F>
    struct distributed_bulk_sender
    {
        using sender_concept = hpx::execution::experimental::sender_t;

        HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender_;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape_;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f_;
        distributed_scheduler scheduler_;

        // Completion signatures: same value types as upstream, plus
        // exception_ptr for errors thrown by f.
        template <typename... Args>
        using default_set_value =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_value_t(Args...)>;

        template <typename Arg>
        using default_set_error =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_error_t(Arg)>;

        struct default_set_value_fn
        {
            template <class... Args>
            consteval auto operator()() const noexcept
            {
                return hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(Args...)>{};
            }
        };

        struct default_set_error_fn
        {
            template <class Err>
            consteval auto operator()() const noexcept
            {
                return hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_error_t(
                        std::decay_t<Err>)>{};
            }
        };

        template <typename Self, typename Env>
        static consteval auto get_completion_signatures() noexcept
            -> decltype(hpx::execution::experimental::
                    transform_completion_signatures(
                        hpx::execution::experimental::
                            completion_signatures_of_t<Sender, Env>{},
                        default_set_value_fn{}, default_set_error_fn{},
                        hpx::execution::experimental::keep_completion<
                            hpx::execution::experimental::set_stopped_t>{},
                        hpx::execution::experimental::completion_signatures<
                            hpx::execution::experimental::set_error_t(
                                std::exception_ptr)>{}))
        {
            return {};
        }

        // Environment: advertise the distributed_scheduler as the
        // completion scheduler for set_value_t.
        struct env
        {
            distributed_scheduler scheduler;

            auto query(
                hpx::execution::experimental::get_domain_t) const noexcept
            {
                return distributed_domain{};
            }

            template <typename CPO>
                requires meta::value<
                    meta::one_of<CPO, hpx::execution::experimental::set_value_t,
                        hpx::execution::experimental::set_stopped_t>>
            auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>)
                const noexcept
            {
                return scheduler;
            }
        };

        constexpr auto get_env() const noexcept
        {
            return env{scheduler_};
        }

        ///////////////////////////////////////////////////////////////////////
        // Receiver that wraps the downstream receiver, intercepts
        // set_value to execute the bulk function, then forwards.
        template <typename Receiver>
        struct bulk_receiver
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape_;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f_;

            template <typename Receiver_, typename Shape_, typename F_>
            bulk_receiver(Receiver_&& receiver, Shape_&& shape, F_&& f)
              : receiver_(HPX_FORWARD(Receiver_, receiver))
              , shape_(HPX_FORWARD(Shape_, shape))
              , f_(HPX_FORWARD(F_, f))
            {
            }

            template <typename Error>
            void set_error(Error&& error) && noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(receiver_), HPX_FORWARD(Error, error));
            }

            void set_stopped() && noexcept
            {
                hpx::execution::experimental::set_stopped(HPX_MOVE(receiver_));
            }

            template <typename... Ts>
            void set_value(Ts&&... ts) && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        // TODO: Implement remote parcelport dispatch.
                        //
                        // In the final implementation, the shape, function,
                        // and values will be serialized and dispatched to
                        // the target locality via a component action. The
                        // remote locality will execute the bulk loop and
                        // signal completion back through the
                        // distributed_receiver_component.
                        //
                        // For now, execute the bulk loop locally as a
                        // functional fallback / stub.
                        for (auto const& s : shape_)
                        {
                            HPX_INVOKE(f_, s, ts...);
                        }
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(receiver_), HPX_FORWARD(Ts, ts)...);
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver_), HPX_MOVE(ep));
                    });
            }

            auto get_env() const noexcept
            {
                return hpx::execution::experimental::get_env(receiver_);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // connect: wire the bulk_receiver into the upstream sender.
        template <typename Receiver>
        auto connect(Receiver&& receiver) &&
        {
            return hpx::execution::experimental::connect(HPX_MOVE(sender_),
                bulk_receiver<Receiver>(HPX_FORWARD(Receiver, receiver),
                    HPX_MOVE(shape_), HPX_MOVE(f_)));
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) &
        {
            return hpx::execution::experimental::connect(sender_,
                bulk_receiver<Receiver>(
                    HPX_FORWARD(Receiver, receiver), shape_, f_));
        }
    };

}    // namespace hpx::distributed::experimental::detail

#endif    // HPX_HAVE_NETWORKING
