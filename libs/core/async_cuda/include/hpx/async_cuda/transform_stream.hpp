//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_cuda/custom_gpu_api.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::cuda::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename R, typename... Ts>
        void set_value_event_callback_helper(
            cudaError_t const status, R&& r, Ts&&... ts)
        {
            static_assert(sizeof...(Ts) <= 1, "Expecting at most one value");

            HPX_ASSERT(status != cudaErrorNotReady);

            if (status == cudaSuccess)
            {
                hpx::execution::experimental::set_value(
                    HPX_FORWARD(R, r), HPX_FORWARD(Ts, ts)...);
            }
            else
            {
                hpx::execution::experimental::set_error(HPX_FORWARD(R, r),
                    std::make_exception_ptr(cuda_exception(
                        std::string("Getting event after CUDA stream transform "
                                    "failed with status ") +
                            cudaGetErrorString(status),
                        status)));
            }
        }

        HPX_CXX_CORE_EXPORT template <typename... Ts>
        void extend_argument_lifetimes(cudaStream_t stream, Ts&&... ts)
        {
            if constexpr (sizeof...(Ts) > 0)
            {
                detail::add_event_callback(
                    [... keep_alive = HPX_FORWARD(Ts, ts)](
                        cudaError_t const status) {
                        (..., (void) keep_alive);
                        HPX_ASSERT(status != cudaErrorNotReady);
                        HPX_UNUSED(status);
                    },
                    stream);
            }
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename... Ts>
        void set_value_immediate_void(cudaStream_t stream, R&& r, Ts&&... ts)
        {
            hpx::execution::experimental::set_value(HPX_FORWARD(R, r));

            // Even though we call set_value immediately, we still extend the
            // life-time of the arguments by capturing them in a callback that
            // is triggered when the event is ready.
            extend_argument_lifetimes(stream, HPX_FORWARD(Ts, ts)...);
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename... Ts>
        void set_value_event_callback_void(
            cudaStream_t stream, R&& r, Ts&&... ts)
        {
            detail::add_event_callback(
                [r = HPX_FORWARD(R, r), ... keep_alive = HPX_FORWARD(Ts, ts)](
                    cudaError_t status) mutable {
                    (..., (void) keep_alive);
                    set_value_event_callback_helper(status, HPX_MOVE(r));
                },
                stream);
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename T, typename... Ts>
        void set_value_immediate_non_void(
            cudaStream_t stream, R&& r, T&& t, Ts&&... ts)
        {
            hpx::execution::experimental::set_value(
                HPX_FORWARD(R, r), HPX_FORWARD(T, t));

            // Even though we call set_value immediately, we still extend the
            // life-time of the arguments by capturing them in a callback that
            // is triggered when the event is ready.
            extend_argument_lifetimes(stream, HPX_FORWARD(Ts, ts)...);
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename T, typename... Ts>
        void set_value_event_callback_non_void(
            cudaStream_t stream, R&& r, T&& t, Ts&&... ts)
        {
            detail::add_event_callback(
                [t = HPX_FORWARD(T, t), r = HPX_FORWARD(R, r),
                    ... keep_alive = HPX_FORWARD(Ts, ts)](
                    cudaError_t status) mutable {
                    (..., (void) keep_alive);
                    set_value_event_callback_helper(
                        status, HPX_MOVE(r), HPX_MOVE(t));
                },
                stream);
        }

        HPX_CXX_CORE_EXPORT template <typename R, typename F>
        struct transform_stream_receiver;

        HPX_CXX_CORE_EXPORT template <typename R>
        struct is_transform_stream_receiver : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename R, typename F>
        struct is_transform_stream_receiver<transform_stream_receiver<R, F>>
          : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename R, typename F>
        struct transform_stream_receiver
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;
            std::decay_t<R> r;
            std::decay_t<F> f;
            cudaStream_t stream;

            template <typename R_, typename F_>
            transform_stream_receiver(R_&& r, F_&& f, cudaStream_t const stream)
              : r(HPX_FORWARD(R_, r))
              , f(HPX_FORWARD(F_, f))
              , stream(stream)
            {
            }

            template <typename E>
            void set_error(E&& e) noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(r), HPX_FORWARD(E, e));
            }

            void set_stopped() noexcept
            {
                hpx::execution::experimental::set_stopped(HPX_MOVE(r));
            }

            template <typename... Ts>
            void set_value(Ts&&... ts) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (std::is_void_v<
                                          typename hpx::util::invoke_result<F,
                                              Ts..., cudaStream_t>::type>)
                        {
                            // When the return type is void, there is no value to
                            // forward to the receiver
                            HPX_INVOKE(f, ts..., stream);

                            if constexpr (is_transform_stream_receiver<
                                              std::decay_t<R>>::value)
                            {
                                if (r.stream == stream)
                                {
                                    // When the next receiver is also a
                                    // transform_stream_receiver, we can
                                    // immediately call set_value, with the
                                    // knowledge that a later receiver will
                                    // synchronize the stream when a
                                    // non-transform_stream receiver is
                                    // connected.
                                    set_value_immediate_void(stream,
                                        HPX_MOVE(r), HPX_FORWARD(Ts, ts)...);
                                }
                                else
                                {
                                    // When the streams are different, we add a
                                    // callback which will call set_value on the
                                    // receiver.
                                    set_value_event_callback_void(stream,
                                        HPX_MOVE(r), HPX_FORWARD(Ts, ts)...);
                                }
                            }
                            else
                            {
                                // When the next receiver is not a
                                // transform_stream_receiver, we add a callback
                                // which will call set_value on the receiver.
                                set_value_event_callback_void(stream,
                                    HPX_MOVE(r), HPX_FORWARD(Ts, ts)...);
                            }
                        }
                        else
                        {
                            // When the return type is non-void, we have to
                            // forward the value to the receiver
                            auto t =
                                HPX_INVOKE(f, HPX_FORWARD(Ts, ts)..., stream);

                            if constexpr (is_transform_stream_receiver<
                                              std::decay_t<R>>::value)
                            {
                                if (r.stream == stream)
                                {
                                    // When the next receiver is also a
                                    // transform_stream_receiver, we can
                                    // immediately call set_value, with the
                                    // knowledge that a later receiver will
                                    // synchronize the stream when a
                                    // non-transform_stream receiver is
                                    // connected.
                                    set_value_immediate_non_void(stream,
                                        HPX_MOVE(r), HPX_MOVE(t),
                                        HPX_FORWARD(Ts, ts)...);
                                }
                                else
                                {
                                    // When the streams are different, we add a
                                    // callback which will call set_value on the
                                    // receiver.
                                    set_value_event_callback_non_void(stream,
                                        HPX_MOVE(r), HPX_MOVE(t),
                                        HPX_FORWARD(Ts, ts)...);
                                }
                            }
                            else
                            {
                                // When the next receiver is not a
                                // transform_stream_receiver, we add a callback
                                // which will call set_value on the receiver.
                                set_value_event_callback_non_void(stream,
                                    HPX_MOVE(r), HPX_MOVE(t),
                                    HPX_FORWARD(Ts, ts)...);
                            }
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(r), HPX_MOVE(ep));
                    });
            }
        };

        template <typename S, typename F>
        struct transform_stream_sender
        {
            std::decay_t<S> s;
            std::decay_t<F> f;
            cudaStream_t stream{};

            using sender_concept = hpx::execution::experimental::sender_t;

            template <typename... Args>
            struct invoke_function_transformation_helper
            {
                template <bool IsVoid, typename T>
                struct set_value_void_checked
                {
                    using type = hpx::execution::experimental::set_value_t(T);
                };

                template <typename T>
                struct set_value_void_checked<true, T>
                {
                    using type = hpx::execution::experimental::set_value_t();
                };

                static_assert(hpx::is_invocable_v<F, Args..., cudaStream_t>,
                    "F not invocable with the value_types specified.");

                using result_type =
                    hpx::util::invoke_result_t<F, Args..., cudaStream_t>;
                using set_value_result_type =
                    typename set_value_void_checked<std::is_void_v<result_type>,
                        result_type>::type;
                using type =
                    hpx::execution::experimental::completion_signatures<
                        set_value_result_type>;
            };

            template <typename... Args>
            using invoke_function_transformation =
                invoke_function_transformation_helper<Args...>::type;

            template <typename Self, typename Env>
            static consteval auto get_completion_signatures()
                -> hpx::execution::experimental::
                    transform_completion_signatures_of<std::decay_t<S>, Env,
                        hpx::execution::experimental::completion_signatures<
                            hpx::execution::experimental::set_error_t(
                                std::exception_ptr)>,
                        invoke_function_transformation>
            {
                return {};
            }

            constexpr auto get_env() const noexcept
            {
                return hpx::execution::experimental::get_env(s);
            }

            template <typename R>
            auto connect(R&& r) &&
            {
                return hpx::execution::experimental::connect(HPX_MOVE(s),
                    transform_stream_receiver<R, F>{
                        HPX_FORWARD(R, r), HPX_MOVE(f), stream});
            }

            template <typename R>
            auto connect(R&& r) &
            {
                return hpx::execution::experimental::connect(s,
                    transform_stream_receiver<R, F>{
                        HPX_FORWARD(R, r), f, stream});
            }
        };
    }    // namespace detail

    // NOTE: This is not a customization of hpx::execution::experimental::then.
    // It has different semantics:
    // - a cudaStream_t is inserted as an additional argument into the call to f
    // - values from the predecessor sender are not forwarded, only passed by
    //   reference, to the call to f to keep them alive until the event is ready
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_stream_t final
      : hpx::functional::detail::tag_fallback<transform_stream_t>
    {
    private:
        template <typename S, typename F,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<F>, cudaStream_t>>>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            transform_stream_t, S&& s, F&& f, cudaStream_t stream = {})
        {
            return detail::transform_stream_sender<S, F>{
                HPX_FORWARD(S, s), HPX_FORWARD(F, f), stream};
        }

        template <typename F>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            transform_stream_t, F&& f, cudaStream_t stream = {})
        {
            return hpx::execution::experimental::detail::partial_algorithm<
                transform_stream_t, F, cudaStream_t>{HPX_FORWARD(F, f), stream};
        }
    } transform_stream{};
}    // namespace hpx::cuda::experimental
