//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#if defined(HPX_HAVE_STDEXEC)
#include <hpx/modules/execution_base.hpp>
#else

#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        // Maps set_value(Ts...) -> set_value(optional<decay_t<T>>)
        //      set_stopped()    -> set_value(nullopt)
        //      set_error(E)     -> set_error(E)
        HPX_CXX_CORE_EXPORT template <typename Receiver, typename T>
        struct stopped_as_optional_receiver
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

            template <typename Error>
            friend void tag_invoke(set_error_t,
                stopped_as_optional_receiver&& r, Error&& error) noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(r.receiver), HPX_FORWARD(Error, error));
            }

            friend void tag_invoke(
                set_stopped_t, stopped_as_optional_receiver&& r) noexcept
            {
                hpx::execution::experimental::set_value(
                    HPX_MOVE(r.receiver), std::optional<T>(std::nullopt));
            }

            friend void tag_invoke(set_value_t,
                stopped_as_optional_receiver&& r, T&& value) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(r.receiver),
                            std::optional<T>(HPX_MOVE(value)));
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(r.receiver), HPX_MOVE(ep));
                    });
            }

            friend void tag_invoke(set_value_t,
                stopped_as_optional_receiver&& r, T const& value) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(r.receiver), std::optional<T>(value));
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(r.receiver), HPX_MOVE(ep));
                    });
            }

            friend auto tag_invoke(
                get_env_t, stopped_as_optional_receiver const& r) noexcept
                -> decltype(hpx::execution::experimental::get_env(r.receiver))
            {
                return hpx::execution::experimental::get_env(r.receiver);
            }
        };

        // Extracts the single value type T from a sender's value_types.
        // stopped_as_optional requires exactly one value set (a single T).
        template <typename Sender, typename Env>
        struct stopped_as_optional_value
        {
            template <typename... Ts>
            struct pack
            {
            };

            // single-value case: set_value(T) -> optional<T>
            template <typename T>
            static T extract(pack<pack<T>>);

            using type =
                decltype(extract(value_types_of_t<Sender, Env, pack, pack>{}));
        };

        template <typename Sender, typename Env>
        using stopped_as_optional_value_t =
            typename stopped_as_optional_value<Sender, Env>::type;

        HPX_CXX_CORE_EXPORT template <typename Sender>
        struct stopped_as_optional_sender
        {
            using is_sender = void;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender;

            template <typename Env>
            struct generate_completion_signatures
            {
                using value_type = stopped_as_optional_value_t<Sender, Env>;

                template <template <typename...> typename Tuple,
                    template <typename...> typename Variant>
                using value_types = Variant<Tuple<std::optional<value_type>>>;

                template <template <typename...> typename Variant>
                using error_types = hpx::util::detail::unique_concat_t<
                    error_types_of_t<Sender, Env, Variant>,
                    Variant<std::exception_ptr>>;

                // stopped is consumed and turned into a value
                static constexpr bool sends_stopped = false;
            };

            template <typename Env>
            friend auto tag_invoke(get_completion_signatures_t,
                stopped_as_optional_sender const&, Env) noexcept
                -> generate_completion_signatures<Env>;

            // clang-format off
            template <typename CPO,
                HPX_CONCEPT_REQUIRES_(
                    meta::value<meta::one_of<
                        std::decay_t<CPO>, set_value_t, set_stopped_t>> &&
                    detail::has_completion_scheduler_v<CPO, Sender>
                )>
            // clang-format on
            friend constexpr auto tag_invoke(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>
                    tag,
                stopped_as_optional_sender const& s)
            {
                return tag(s.sender);
            }

            template <typename Receiver>
            friend auto tag_invoke(
                connect_t, stopped_as_optional_sender&& s, Receiver&& receiver)
            {
                using value_type =
                    stopped_as_optional_value_t<Sender, empty_env>;
                return hpx::execution::experimental::connect(HPX_MOVE(s.sender),
                    stopped_as_optional_receiver<Receiver, value_type>{
                        HPX_FORWARD(Receiver, receiver)});
            }

            template <typename Receiver>
            friend auto tag_invoke(
                connect_t, stopped_as_optional_sender& s, Receiver&& receiver)
            {
                using value_type =
                    stopped_as_optional_value_t<Sender, empty_env>;
                return hpx::execution::experimental::connect(s.sender,
                    stopped_as_optional_receiver<Receiver, value_type>{
                        HPX_FORWARD(Receiver, receiver)});
            }
        };
    }    // namespace detail

    // stopped_as_optional maps a sender's stopped channel into the value
    // channel, wrapping the result in std::optional. If the predecessor
    // completes with set_stopped(), the returned sender completes with
    // set_value(std::nullopt). If the predecessor completes with
    // set_value(T), the returned sender completes with
    // set_value(std::optional<T>(t)). Errors pass through unchanged.
    HPX_CXX_CORE_EXPORT inline constexpr struct stopped_as_optional_t final
      : hpx::functional::detail::tag_priority<stopped_as_optional_t>
    {
    private:
        // clang-format off
        template <typename Sender,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                experimental::detail::is_completion_scheduler_tag_invocable_v<
                    hpx::execution::experimental::set_value_t,
                    Sender, stopped_as_optional_t
                >
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_override_invoke(
            stopped_as_optional_t, Sender&& sender)
        {
            auto scheduler =
                hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(sender);

            return hpx::functional::tag_invoke(stopped_as_optional_t{},
                HPX_MOVE(scheduler), HPX_FORWARD(Sender, sender));
        }

        // clang-format off
        template <typename Sender,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            stopped_as_optional_t, Sender&& sender)
        {
            return detail::stopped_as_optional_sender<Sender>{
                HPX_FORWARD(Sender, sender)};
        }

        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            stopped_as_optional_t)
        {
            return detail::partial_algorithm<stopped_as_optional_t>{};
        }
    } stopped_as_optional{};
}    // namespace hpx::execution::experimental

#endif
