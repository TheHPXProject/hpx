//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_STDEXEC)
#include <hpx/execution_base/stdexec_forward.hpp>
#else

#include <hpx/concepts/concepts.hpp>
#include <hpx/datastructures/tuple.hpp>
#include <hpx/datastructures/variant.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution/algorithms/then.hpp>
#include <hpx/execution_base/completion_scheduler.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/receiver.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/functional/detail/tag_priority_invoke.hpp>
#include <hpx/functional/invoke_result.hpp>
#include <hpx/iterator_support/counting_shape.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Sender, typename Shape,
            typename F>
        struct bulk_sender
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;

            using is_sender = void;

            template <typename Env>
            struct generate_completion_signatures
            {
                template <template <typename...> typename Tuple,
                    template <typename...> typename Variant>
                using value_types =
                    value_types_of_t<Sender, Env, Tuple, Variant>;

                template <template <typename...> typename Variant>
                using error_types = hpx::util::detail::unique_concat_t<
                    error_types_of_t<Sender, Env, Variant>,
                    Variant<std::exception_ptr>>;

                static constexpr bool sends_stopped =
                    sends_stopped_of_v<Sender, Env>;
            };

            // clang-format off
            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, bulk_sender const&, Env) noexcept
                -> generate_completion_signatures<Env>;
            // clang-format on

            // clang-format off
            template <typename CPO,
                HPX_CONCEPT_REQUIRES_(
                    hpx::execution::experimental::detail::is_receiver_cpo_v<CPO> &&
                    hpx::execution::experimental::detail::has_completion_scheduler_v<
                        CPO, std::decay_t<Sender>>
                )>
            // clang-format on
            friend constexpr auto tag_invoke(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>
                    tag,
                bulk_sender const& s)
            {
                return tag(s.sender);
            }

            template <typename Receiver>
            struct bulk_receiver
            {
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Shape> shape;
                HPX_NO_UNIQUE_ADDRESS std::decay_t<F> f;

                template <typename Receiver_, typename Shape_, typename F_>
                bulk_receiver(Receiver_&& receiver, Shape_&& shape, F_&& f)
                  : receiver(HPX_FORWARD(Receiver_, receiver))
                  , shape(HPX_FORWARD(Shape_, shape))
                  , f(HPX_FORWARD(F_, f))
                {
                }

                template <typename Error>
                friend void tag_invoke(
                    set_error_t, bulk_receiver&& r, Error&& error) noexcept
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(r.receiver), HPX_FORWARD(Error, error));
                }

                friend void tag_invoke(
                    set_stopped_t, bulk_receiver&& r) noexcept
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(r.receiver));
                }

                template <typename... Ts>
                void set_value(Ts&&... ts)
                {
                    hpx::detail::try_catch_exception_ptr(
                        [&]() {
                            for (auto const& s : shape)
                            {
                                HPX_INVOKE(f, s, ts...);
                            }
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver), HPX_FORWARD(Ts, ts)...);
                        },
                        [&](std::exception_ptr ep) {
                            hpx::execution::experimental::set_error(
                                HPX_MOVE(receiver), HPX_MOVE(ep));
                        });
                }

                template <typename... Ts>
                friend auto tag_invoke(
                    set_value_t, bulk_receiver&& r, Ts&&... ts) noexcept
                    -> decltype(hpx::execution::experimental::set_value(
                                    std::declval<std::decay_t<Receiver>&&>(),
                                    HPX_FORWARD(Ts, ts)...),
                        void())
                {
                    // set_value is in a member function only because of a
                    // compiler bug in GCC 7. When the body of set_value is
                    // inlined here compilation fails with an internal compiler
                    // error.
                    r.set_value(HPX_FORWARD(Ts, ts)...);
                }
            };

            template <typename Receiver>
            friend auto tag_invoke(
                connect_t, bulk_sender&& s, Receiver&& receiver)
            {
                return hpx::execution::experimental::connect(HPX_MOVE(s.sender),
                    bulk_receiver<Receiver>(HPX_FORWARD(Receiver, receiver),
                        HPX_MOVE(s.shape), HPX_MOVE(s.f)));
            }

            template <typename Receiver>
            friend auto tag_invoke(
                connect_t, bulk_sender& s, Receiver&& receiver)
            {
                return hpx::execution::experimental::connect(s.sender,
                    bulk_receiver<Receiver>(
                        HPX_FORWARD(Receiver, receiver), s.shape, s.f));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //
    // execution::bulk is used to run a task repeatedly for every index in an
    // index space.
    //
    // Returns a sender describing the task of invoking the provided function
    // with every index in the provided shape along with the values sent by the
    // input sender. The returned sender completes once all invocations have
    // completed, or an error has occurred. If it completes by sending values,
    // they are equivalent to those sent by the input sender.
    //
    // No instance of function will begin executing until the returned sender is
    // started. Each invocation of function runs in an execution agent whose
    // forward progress guarantees are determined by the scheduler on which they
    // are run. All agents created by a single use of bulk execute with the same
    // guarantee. This allows, for instance, a scheduler to execute all
    // invocations of the function in parallel.
    //
    // The bulk operation is intended to be used at the point where the number
    // of agents to be created is known and provided to bulk via its shape
    // parameter. For some parallel computations, the number of agents to be
    // created may be a function of the input data or dynamic conditions of the
    // execution environment. In such cases, bulk can be combined with
    // additional operations such as let_value to deliver dynamic shape
    // information to the bulk operation.
    //
    HPX_CXX_CORE_EXPORT inline constexpr struct bulk_t final
      : hpx::functional::detail::tag_priority<bulk_t>
    {
    private:
        // clang-format off
        template <typename Sender, typename Shape, typename F,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                experimental::detail::is_completion_scheduler_tag_invocable_v<
                    hpx::execution::experimental::set_value_t, Sender,
                    bulk_t, Shape, F
                >
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_override_invoke(
            bulk_t, Sender&& sender, Shape const& shape, F&& f)
        {
            auto scheduler =
                hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(sender);

            return hpx::functional::tag_invoke(bulk_t{}, HPX_MOVE(scheduler),
                HPX_FORWARD(Sender, sender), shape, HPX_FORWARD(F, f));
        }

        // clang-format off
        template <typename Sender, typename Shape, typename F,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                std::is_integral_v<Shape>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            bulk_t, Sender&& sender, Shape const& shape, F&& f)
        {
            return detail::bulk_sender<Sender, hpx::util::counting_shape<Shape>,
                F>{HPX_FORWARD(Sender, sender),
                hpx::util::counting_shape(shape), HPX_FORWARD(F, f)};
        }

        // clang-format off
        template <typename Sender, typename Shape, typename F,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                !std::is_integral_v<std::decay_t<Shape>>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            bulk_t, Sender&& sender, Shape&& shape, F&& f)
        {
            return detail::bulk_sender<Sender, Shape, F>{
                HPX_FORWARD(Sender, sender), HPX_FORWARD(Shape, shape),
                HPX_FORWARD(F, f)};
        }

        template <typename Shape, typename F>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            bulk_t, Shape&& shape, F&& f)
        {
            return detail::partial_algorithm<bulk_t, Shape, F>{
                HPX_FORWARD(Shape, shape), HPX_FORWARD(F, f)};
        }
    } bulk{};
}    // namespace hpx::execution::experimental

#endif
