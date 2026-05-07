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
#include <hpx/modules/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::mpi::experimental {

    namespace detail {

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

        template <typename R, typename... Ts>
        void set_value_request_callback_void(
            MPI_Request request, R&& r, Ts&&... ts)
        {
            detail::add_request_callback(
                [r = HPX_FORWARD(R, r), ... keep_alive = HPX_FORWARD(Ts, ts)](
                    int status) mutable {
                    (..., (void) keep_alive);
                    set_value_request_callback_helper(status, HPX_MOVE(r));
                },
                request);
        }

        template <typename R, typename InvokeResult, typename... Ts>
        void set_value_request_callback_non_void(
            MPI_Request request, R&& r, InvokeResult&& res, Ts&&... ts)
        {
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

            template <typename... Ts,
                typename = std::enable_if_t<
                    hpx::is_invocable_v<F, Ts..., MPI_Request*>>>
            void set_value(Ts&&... ts) && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (std::is_void_v<util::invoke_result_t<F,
                                          Ts..., MPI_Request*>>)
                        {
                            MPI_Request request;
                            HPX_INVOKE(f, ts..., &request);
                            // When the return type is void, there is no value
                            // to forward to the receiver
                            set_value_request_callback_void(
                                request, HPX_MOVE(r), HPX_FORWARD(Ts, ts)...);
                        }
                        else
                        {
                            MPI_Request request;
                            // When the return type is non-void, we have to
                            // forward the value to the receiver. Pass `ts...`
                            // unforwarded into `f` so the same arguments can
                            // be moved once into the keep-alive callback
                            // below; this matches the void branch above and
                            // avoids a double-move when `Ts...` are rvalues.
                            auto&& result = HPX_INVOKE(f, ts..., &request);
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
                    static_assert(hpx::is_invocable_v<F, Args..., MPI_Request*>,
                        "F not invocable with the value_types specified.");

                    using result_type =
                        hpx::util::invoke_result_t<F, Args..., MPI_Request*>;

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
<<<<<<< HEAD
            template <typename Self, typename Env>
            static consteval auto get_completion_signatures() noexcept
            ->  decltype(hpx::execution::experimental::transform_completion_signatures(
                    hpx::execution::experimental::completion_signatures_of_t<
                        Sender, Env>{},
                    invoke_function_transformation_fn{},
                    default_set_error_fn{},
                    hpx::execution::experimental::ignore_completion{}))
            {
                return hpx::execution::experimental::transform_completion_signatures(
                    hpx::execution::experimental::completion_signatures_of_t<
                        Sender, Env>{},
                    invoke_function_transformation_fn{},
                    default_set_error_fn{},
                    hpx::execution::experimental::ignore_completion{});
            }
=======
            template <typename Env>
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
            friend auto tag_invoke(
                hpx::execution::experimental::get_completion_signatures_t,
                transform_mpi_sender const&, Env const&)
            ->  hpx::execution::experimental::transform_completion_signatures<
                    hpx::execution::experimental::completion_signatures_of_t<Sender, Env>,
                    hpx::execution::experimental::completion_signatures<
                        hpx::execution::experimental::set_error_t(std::exception_ptr)
                    >,
                    invoke_function_transformation,
                    default_set_error,
                    no_set_stopped_signature
                >;
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
>>>>>>> 13e2e82d54 (Migrate from deprecated transform_completion_signatures_of to transform_completion_signatures)
            // clang-format on

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
      : hpx::functional::detail::tag_fallback<transform_mpi_t>
    {
    private:
        template <typename Sender, typename F>
            requires(hpx::execution::experimental::is_sender_v<Sender>)
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            transform_mpi_t, Sender&& s, F&& f)
        {
            return detail::transform_mpi_sender<Sender, F>{
                HPX_FORWARD(Sender, s), HPX_FORWARD(F, f)};
        }

        template <typename F>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            transform_mpi_t, F&& f)
        {
            return ::hpx::execution::experimental::detail::partial_algorithm<
                transform_mpi_t, F>{HPX_FORWARD(F, f)};
        }
    } transform_mpi{};
}    // namespace hpx::mpi::experimental
