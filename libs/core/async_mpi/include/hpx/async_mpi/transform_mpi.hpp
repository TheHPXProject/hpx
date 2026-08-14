//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/async_mpi/transform_mpi.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_mpi/mpi_future.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/mpi_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::mpi::experimental {

    namespace detail {

        // Detects whether an ADL customization point `hpx_invoke(tag, args...)`
        // has been defined for the given CPO `Tag`. This is the replacement
        // hook for external customizations of `transform_mpi_t`, used in
        // place of the legacy tag-based dispatch.
        template <typename Tag, typename... Args>
        concept has_hpx_invoke = requires(Tag const& tag, Args&&... args) {
            hpx_invoke(tag, HPX_FORWARD(Args, args)...);
        };

        template <typename R, typename... Ts>
        void set_value_request_callback_helper(
            int mpi_status, R&& r, Ts&&... ts)
        {
            static_assert(sizeof...(Ts) <= 1, "Expecting at most one value");
            if (mpi_status == MPI_SUCCESS)
            {
                hpx::execution::experimental::set_value(
                    HPX_FORWARD(R, r), HPX_FORWARD(Ts, ts)...);
            }
            else
            {
                hpx::execution::experimental::set_error(HPX_FORWARD(R, r),
                    std::make_exception_ptr(mpi_exception(mpi_status)));
            }
        }

        // Register a completion callback for an MPI request whose backing
        // function returns void. The receiver `r` and the upstream
        // arguments `ts...` are captured into a lambda whose lifetime is
        // owned by the polling driver until `MPI_Test*` reports completion.
        //
        // Contract:
        //   * `Ts...` are captured by the callback to keep the upstream
        //     sender values alive until the MPI request completes.
        //   * The lambda fires on the polling thread (inside `poll()` in
        //     `mpi_future.cpp`), not on the receiver's preferred completion
        //     scheduler. Receivers must be safe to invoke on that thread.
        //
        // The `static_assert` below makes the receiver-shape requirement
        // explicit at the helper boundary so misuse fails here with a clear
        // message rather than deep inside the captured lambda's body.
        template <typename R, typename... Ts>
        void set_value_request_callback_void(
            MPI_Request request, R&& r, Ts&&... ts)
        {
            static_assert(
                std::is_invocable_v<hpx::execution::experimental::set_value_t,
                    std::decay_t<R>&&>,
                "set_value_request_callback_void: the receiver R must be "
                "invocable as `set_value(receiver)` (no value args) for the "
                "void MPI-return path. Did you mean to use the "
                "_non_void variant?");

            detail::add_request_callback(
                [r = HPX_FORWARD(R, r), ... keep_alive = HPX_FORWARD(Ts, ts)](
                    int status) mutable {
                    (..., (void) keep_alive);
                    set_value_request_callback_helper(status, HPX_MOVE(r));
                },
                request);
        }

        // Register a completion callback for an MPI request whose backing
        // function returns a value `res` to forward to the receiver.
        // Same contract as the void overload; additionally, `res` is
        // captured separately so it can be forwarded to the receiver's
        // `set_value` on completion.
        template <typename R, typename InvokeResult, typename... Ts>
        void set_value_request_callback_non_void(
            MPI_Request request, R&& r, InvokeResult&& res, Ts&&... ts)
        {
            static_assert(!std::is_void_v<std::decay_t<InvokeResult>>,
                "set_value_request_callback_non_void: InvokeResult must be "
                "non-void; for void-returning MPI functions use the _void "
                "variant.");
            static_assert(
                std::is_invocable_v<hpx::execution::experimental::set_value_t,
                    std::decay_t<R>&&, std::decay_t<InvokeResult>&&>,
                "set_value_request_callback_non_void: the receiver R must "
                "be invocable as `set_value(receiver, result)` for the "
                "non-void MPI-return path.");

            detail::add_request_callback(
                [r = HPX_FORWARD(R, r), res = HPX_FORWARD(InvokeResult, res),
                    ... keep_alive = HPX_FORWARD(Ts, ts)](int status) mutable {
                    (..., (void) keep_alive);
                    set_value_request_callback_helper(
                        status, HPX_MOVE(r), HPX_MOVE(res));
                },
                request);
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename F>
        struct transform_mpi_receiver
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<R> r;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;

            template <typename R_, typename F_>
            transform_mpi_receiver(R_&& r, F_&& f)
              : r(HPX_FORWARD(R_, r))
              , f(HPX_FORWARD(F_, f))
            {
            }

            template <typename E>
            void set_error(E&& e) && noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(r), HPX_FORWARD(E, e));
            }

            void set_stopped() && noexcept
            {
                hpx::execution::experimental::set_stopped(HPX_MOVE(r));
            }

            template <typename... Ts>
                requires(hpx::is_invocable_v<F &&, Ts && ..., MPI_Request*>)
            void set_value(Ts&&... ts) && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (std::is_void_v<util::invoke_result_t<F&&,
                                          Ts&&..., MPI_Request*>>)
                        {
                            MPI_Request request;
                            HPX_INVOKE(
                                HPX_MOVE(f), HPX_FORWARD(Ts, ts)..., &request);
                            // When the return type is void, there is no value
                            // to forward to the receiver
                            set_value_request_callback_void(
                                request, HPX_MOVE(r), HPX_FORWARD(Ts, ts)...);
                        }
                        else
                        {
                            MPI_Request request;
                            // When the return type is non-void, we have to
                            // forward the value to the receiver.
                            auto&& result = HPX_INVOKE(
                                HPX_MOVE(f), HPX_FORWARD(Ts, ts)..., &request);
                            set_value_request_callback_non_void(request,
                                HPX_MOVE(r), HPX_MOVE(result),
                                HPX_FORWARD(Ts, ts)...);
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(r), HPX_MOVE(ep));
                    });
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Sender, typename F>
        struct transform_mpi_sender
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> s;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;

            using is_sender = void;
            using sender_concept = hpx::execution::experimental::sender_t;

            struct invoke_function_transformation_fn
            {
                template <typename... Args>
                consteval auto operator()() const noexcept
                {
                    static_assert(
                        hpx::is_invocable_v<F&&, Args&&..., MPI_Request*>,
                        "F not invocable with the value_types specified.");

                    using result_type = hpx::util::invoke_result_t<F&&,
                        Args&&..., MPI_Request*>;

                    if constexpr (std::is_void_v<result_type>)
                    {
                        return hpx::execution::experimental::
                            completion_signatures<
                                hpx::execution::experimental::set_value_t()>{};
                    }
                    else
                    {
                        return hpx::execution::experimental::
                            completion_signatures<
                                hpx::execution::experimental::set_value_t(
                                    result_type)>{};
                    }
                }
            };

            struct default_set_error_fn
            {
                template <typename Err>
                consteval auto operator()() const noexcept
                {
                    return hpx::execution::experimental::completion_signatures<
                        hpx::execution::experimental::set_error_t(Err)>{};
                }
            };

            // clang-format off
            template <typename Self, typename Env>
            static consteval auto get_completion_signatures() noexcept
            ->  decltype(hpx::execution::experimental::transform_completion_signatures(
                    hpx::execution::experimental::completion_signatures_of_t<
                        Sender, Env>{},
                    invoke_function_transformation_fn{},
                    default_set_error_fn{},
                    hpx::execution::experimental::ignore_completion{},
                    hpx::execution::experimental::completion_signatures<
                        hpx::execution::experimental::set_stopped_t()>{}))
            {
                return hpx::execution::experimental::transform_completion_signatures(
                    hpx::execution::experimental::completion_signatures_of_t<
                        Sender, Env>{},
                    invoke_function_transformation_fn{},
                    default_set_error_fn{},
                    hpx::execution::experimental::ignore_completion{},
                    hpx::execution::experimental::completion_signatures<
                        hpx::execution::experimental::set_stopped_t()>{});
            }
            // clang-format on

            constexpr decltype(auto) get_env() const
                noexcept(noexcept(hpx::execution::experimental::get_env(s)))
            {
                return hpx::execution::experimental::get_env(s);
            }

            template <typename R>
            constexpr auto connect(R&& r) &
            {
                return hpx::execution::experimental::connect(
                    s, transform_mpi_receiver<R, F>(HPX_FORWARD(R, r), f));
            }

            template <typename R>
            constexpr auto connect(R&& r) &&
            {
                return hpx::execution::experimental::connect(HPX_MOVE(s),
                    transform_mpi_receiver<R, F>(
                        HPX_FORWARD(R, r), HPX_MOVE(f)));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT inline constexpr struct transform_mpi_t final
    {
        // Dispatch order (mirrors the previous tag_fallback semantics):
        //   1. hpx_invoke(tag, args...)  external customization (new hook)
        //   2. the default bodies below
        template <typename... Args>
            requires(detail::has_hpx_invoke<transform_mpi_t, Args...>)
        constexpr HPX_FORCEINLINE decltype(auto) operator()(
            Args&&... args) const
        {
            return hpx_invoke(*this, HPX_FORWARD(Args, args)...);
        }

        template <typename Sender, typename F>
            requires(hpx::execution::experimental::is_sender_v<Sender> &&
                !detail::has_hpx_invoke<transform_mpi_t, Sender, F>)
        constexpr HPX_FORCEINLINE auto operator()(Sender&& s, F&& f) const
        {
            return detail::transform_mpi_sender<Sender, F>{
                HPX_FORWARD(Sender, s), HPX_FORWARD(F, f)};
        }

        template <typename F>
            requires(!detail::has_hpx_invoke<transform_mpi_t, F>)
        constexpr HPX_FORCEINLINE auto operator()(F&& f) const
        {
            return ::hpx::execution::experimental::detail::partial_algorithm<
                transform_mpi_t, F>{HPX_FORWARD(F, f)};
        }
    } transform_mpi{};
}    // namespace hpx::mpi::experimental
