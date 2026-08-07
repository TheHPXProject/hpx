//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file distributed_bulk_sender.hpp
/// \brief P2300-compliant distributed bulk sender adaptor.
///
/// \note **Current status: local-only stub.** The bulk loop executes
///       sequentially on the calling locality. Remote parcelport dispatch
///       will be added once the component-based receiver marshalling
///       infrastructure (nvexec-style) is complete.
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

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)

#include <hpx/execution_distributed/distributed_scheduler.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/runtime_distributed/find_here.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::distributed::experimental::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Extracted bulk loop for remote execution.
    //
    // This function encapsulates the shape-indexed invocation logic so
    // that it can be dispatched as an HPX action on a remote locality.
    // It handles both integral shapes (index-based loop) and range-based
    // shapes (iterator-based loop).
    //
    // Parameters:
    //   shape - The iteration space (integral count or iterable range)
    //   f     - The user function invoked for each index
    //   ts... - The upstream values forwarded from the predecessor sender
    template <typename Shape, typename F, typename... Ts>
    void remote_bulk_execute(Shape const& shape, F& f, Ts&... ts)
    {
        if constexpr (std::is_integral_v<std::decay_t<Shape>>)
        {
            for (std::decay_t<Shape> i = 0; i < shape; ++i)
            {
                HPX_INVOKE(f, i, ts...);
            }
        }
        else
        {
            for (auto const& s : shape)
            {
                HPX_INVOKE(f, s, ts...);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Tuple-packed variant for HPX action dispatch.
    //
    // HPX_DEFINE_PLAIN_ACTION requires a concrete function pointer
    // (decltype(&func)), which is incompatible with function templates.
    // We solve this by packing the variadic upstream values into an
    // hpx::tuple<Ts...>, yielding a fixed-arity function signature
    // per <Shape, F, ArgsTuple> instantiation.
    //
    // The action type is defined manually below using
    // hpx::actions::make_action_t for each concrete instantiation.
    template <typename Shape, typename F, typename ArgsTuple>
    void remote_bulk_execute_tuple(Shape shape, F f, ArgsTuple args_tuple)
    {
        hpx::invoke_fused(
            [&](auto&... args) { remote_bulk_execute(shape, f, args...); },
            args_tuple);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Action type for remote_bulk_execute_tuple.
    //
    // This manually defines the action struct that
    // HPX_DEFINE_PLAIN_ACTION would generate, but parameterized on
    // <Shape, F, ArgsTuple>. Each instantiation produces a concrete
    // action type that can be dispatched via hpx::async.
    //
    // Usage:
    //   hpx::async<remote_bulk_execute_action<Shape, F, ArgsTuple>>(
    //       target_id, shape, f, args_tuple);
    template <typename Shape, typename F, typename ArgsTuple>
    struct remote_bulk_execute_action
      : hpx::actions::make_action_t<
            decltype(&remote_bulk_execute_tuple<Shape, F, ArgsTuple>),
            &remote_bulk_execute_tuple<Shape, F, ArgsTuple>,
            remote_bulk_execute_action<Shape, F, ArgsTuple>>
    {
    };

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
            hpx::id_type target_locality_;

            template <typename Receiver_, typename Shape_, typename F_>
            bulk_receiver(Receiver_&& receiver, Shape_&& shape, F_&& f,
                hpx::id_type target)
              : receiver_(HPX_FORWARD(Receiver_, receiver))
              , shape_(HPX_FORWARD(Shape_, shape))
              , f_(HPX_FORWARD(F_, f))
              , target_locality_(HPX_MOVE(target))
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
                // Fail early if the function is not invocable with the
                // shape element type and the upstream value types.
                if constexpr (std::is_integral_v<std::decay_t<Shape>>)
                {
                    static_assert(hpx::is_invocable_v<std::decay_t<F>,
                                      std::decay_t<Shape>, Ts...>,
                        "distributed_bulk_sender: F must be invocable "
                        "with (Shape, Ts...)");
                }
                else
                {
                    using element_type = decltype(*std::begin(
                        std::declval<std::decay_t<Shape> const&>()));
                    static_assert(hpx::is_invocable_v<std::decay_t<F>,
                                      element_type, Ts...>,
                        "distributed_bulk_sender: F must be invocable "
                        "with (element_type, Ts...)");
                }

                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        // Pack the upstream values into a tuple for
                        // action dispatch.
                        auto args_tuple =
                            hpx::make_tuple(HPX_FORWARD(Ts, ts)...);
                        using args_tuple_type = decltype(args_tuple);

                        if (target_locality_ == hpx::find_here())
                        {
                            // Local execution: call directly without
                            // going through the parcelport.
                            // Pass args_tuple by lvalue so it retains
                            // its values for the downstream set_value.
                            hpx::invoke_fused(
                                [&](auto&... args) {
                                    remote_bulk_execute(shape_, f_, args...);
                                },
                                args_tuple);
                        }
                        else
                        {
                            // Remote execution: dispatch over the
                            // network via the action. The action
                            // consumes a copy; the local tuple is
                            // preserved for set_value forwarding.
                            using action_type =
                                remote_bulk_execute_action<std::decay_t<Shape>,
                                    std::decay_t<F>, args_tuple_type>;
                            hpx::async<action_type>(
                                target_locality_, shape_, f_, args_tuple)
                                .get();
                        }

                        // Forward the (locally retained) values to the
                        // downstream receiver.
                        hpx::invoke_fused(
                            [&](auto&&... unpacked) {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(receiver_),
                                    HPX_FORWARD(
                                        decltype(unpacked), unpacked)...);
                            },
                            HPX_MOVE(args_tuple));
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver_), HPX_MOVE(ep));
                    });
            }

            using env_type = decltype(hpx::execution::experimental::get_env(
                std::declval<std::decay_t<Receiver> const&>()));

            env_type get_env() const noexcept;
        };

        ///////////////////////////////////////////////////////////////////////
        // connect: wire the bulk_receiver into the upstream sender.
        template <typename Receiver>
        auto connect(Receiver&& receiver) &&
        {
            return hpx::execution::experimental::connect(HPX_MOVE(sender_),
                bulk_receiver<Receiver>(HPX_FORWARD(Receiver, receiver),
                    HPX_MOVE(shape_), HPX_MOVE(f_), scheduler_.target()));
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) &
        {
            return hpx::execution::experimental::connect(sender_,
                bulk_receiver<Receiver>(HPX_FORWARD(Receiver, receiver), shape_,
                    f_, scheduler_.target()));
        }
    };

    template <typename Sender, typename Shape, typename F>
    template <typename Receiver>
    inline auto distributed_bulk_sender<Sender, Shape,
        F>::bulk_receiver<Receiver>::get_env() const noexcept -> env_type
    {
        return hpx::execution::experimental::get_env(receiver_);
    }

}    // namespace hpx::distributed::experimental::detail

#endif    // HPX_HAVE_NETWORKING
