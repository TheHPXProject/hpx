//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_STDEXEC)
#include <hpx/modules/execution_base.hpp>
#else

#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Sender, typename Scheduler>
        struct schedule_from_sender
        {
            using is_sender = void;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> predecessor_sender;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler;

            template <typename Env>
            struct generate_completion_signatures
            {
                template <template <typename...> typename Tuple,
                    template <typename...> typename Variant>
                using value_types =
                    value_types_of_t<Sender, Env, Tuple, Variant>;

                template <template <typename...> typename Variant>
                using predecessor_sender_error_types =
                    error_types_of_t<Sender, Env, Variant>;

                using scheduler_sender_type = hpx::util::invoke_result_t<
                    hpx::execution::experimental::schedule_t, Scheduler>;

                template <template <typename...> typename Variant>
                using scheduler_sender_error_types =
                    error_types_of_t<scheduler_sender_type, Env, Variant>;

                template <template <typename...> typename Variant>
                using error_types = hpx::util::detail::unique_concat_t<
                    predecessor_sender_error_types<Variant>,
                    scheduler_sender_error_types<Variant>>;

                static constexpr bool sends_stopped =
                    sends_stopped_of_v<Sender, Env> ||
                    sends_stopped_of_v<scheduler_sender_type, Env>;
            };

            // clang-format off
            template <typename Env>
            friend auto tag_invoke(get_completion_signatures_t,
                schedule_from_sender const&,
                Env) -> generate_completion_signatures<Env>;

            template <typename CPO,
                HPX_CONCEPT_REQUIRES_(
                    meta::value<meta::one_of<
                        std::decay_t<CPO>, set_value_t, set_stopped_t>>
                )>
            // clang-format on
            friend constexpr auto tag_invoke(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>,
                schedule_from_sender const& sender)
            {
                return sender.scheduler;
            }

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
            template <typename Receiver>
            struct operation_state
            {
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler;
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

                struct predecessor_sender_receiver;
                struct scheduler_sender_receiver;

                using value_type = hpx::util::detail::prepend_t<
                    value_types_of_t<Sender, empty_env, decayed_tuple,
                        hpx::variant>,
                    hpx::monostate>;
                value_type ts;

                using sender_operation_state_type =
                    connect_result_t<Sender, predecessor_sender_receiver>;
                sender_operation_state_type sender_os;

                using scheduler_operation_state_type = connect_result_t<
                    hpx::util::invoke_result_t<schedule_t, Scheduler>,
                    scheduler_sender_receiver>;
                hpx::optional<scheduler_operation_state_type>
                    scheduler_op_state;

                template <typename Sender_, typename Scheduler_,
                    typename Receiver_>
                operation_state(Sender_&& predecessor_sender,
                    Scheduler_&& scheduler, Receiver_&& receiver)
                  : scheduler(HPX_FORWARD(Scheduler, scheduler))
                  , receiver(HPX_FORWARD(Receiver_, receiver))
                  , sender_os(hpx::execution::experimental::connect(
                        HPX_FORWARD(Sender_, predecessor_sender),
                        predecessor_sender_receiver{*this}))
                {
                }

                operation_state(operation_state&&) = delete;
                operation_state(operation_state const&) = delete;
                operation_state& operator=(operation_state&&) = delete;
                operation_state& operator=(operation_state const&) = delete;

                struct predecessor_sender_receiver
                {
                    operation_state& op_state;

                    template <typename Error>
                    friend void tag_invoke(set_error_t,
                        predecessor_sender_receiver&& r, Error&& error) noexcept
                    {
                        r.op_state.set_error_predecessor_sender(
                            HPX_FORWARD(Error, error));
                    }

                    friend void tag_invoke(
                        set_stopped_t, predecessor_sender_receiver&& r) noexcept
                    {
                        r.op_state.set_stopped_predecessor_sender();
                    }

                    // This typedef is duplicated from the parent struct. The
                    // parent typedef is not instantiated early enough for use
                    // here.
                    using value_type = hpx::util::detail::prepend_t<
                        value_types_of_t<Sender, empty_env, decayed_tuple,
                            hpx::variant>,
                        hpx::monostate>;

                    template <typename... Ts>
                    friend auto tag_invoke(set_value_t,
                        predecessor_sender_receiver&& r, Ts&&... ts) noexcept
                        -> decltype(std::declval<value_type>()
                                        .template emplace<hpx::tuple<Ts...>>(
                                            HPX_FORWARD(Ts, ts)...),
                            void())
                    {
                        r.op_state.set_value_predecessor_sender(
                            HPX_FORWARD(Ts, ts)...);
                    }

                    // Pass through the get_env receiver query
                    friend auto tag_invoke(
                        get_env_t, predecessor_sender_receiver const& r)
                        -> env_of_t<std::decay_t<Receiver>>
                    {
                        return hpx::execution::experimental::get_env(
                            r.op_state.receiver);
                    }
                };

                template <typename Error>
                void set_error_predecessor_sender(Error&& error) noexcept
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver), HPX_FORWARD(Error, error));
                }

                void set_stopped_predecessor_sender() noexcept
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(receiver));
                }

                template <typename... Us>
                void set_value_predecessor_sender(Us&&... us) noexcept
                {
                    ts.template emplace<hpx::tuple<Us...>>(
                        HPX_FORWARD(Us, us)...);
#if defined(HPX_HAVE_CXX17_COPY_ELISION) &&                                    \
    defined(HPX_HAVE_CXX17_OPTIONAL_COPY_ELISION)
                    // with_result_of is used to emplace the operation
                    // state returned from connect without any
                    // intermediate copy construction (the operation
                    // state is not required to be copyable nor movable).
                    scheduler_op_state.emplace(
                        hpx::util::detail::with_result_of([&]() {
                            return hpx::execution::experimental::connect(
                                hpx::execution::experimental::schedule(
                                    HPX_MOVE(scheduler)),
                                scheduler_sender_receiver{*this});
                        }));
#else
                    // earlier versions of MSVC don't get copy elision quite
                    // right, the operation state must be constructed explicitly
                    // directly in place
                    scheduler_op_state.emplace_f(
                        hpx::execution::experimental::connect,
                        hpx::execution::experimental::schedule(
                            HPX_MOVE(scheduler)),
                        scheduler_sender_receiver{*this});
#endif
                    hpx::execution::experimental::start(*scheduler_op_state);
                }

                struct scheduler_sender_receiver
                {
                    operation_state& op_state;

                    template <typename Error>
                    friend void tag_invoke(set_error_t,
                        scheduler_sender_receiver&& r, Error&& error) noexcept
                    {
                        r.op_state.set_error_scheduler_sender(
                            HPX_FORWARD(Error, error));
                    }

                    friend void tag_invoke(
                        set_stopped_t, scheduler_sender_receiver&& r) noexcept
                    {
                        r.op_state.set_stopped_scheduler_sender();
                    }

                    friend void tag_invoke(
                        set_value_t, scheduler_sender_receiver&& r) noexcept
                    {
                        r.op_state.set_value_scheduler_sender();
                    }

                    // Pass through the get_env receiver query
                    friend auto tag_invoke(
                        get_env_t, scheduler_sender_receiver const& r)
                        -> env_of_t<std::decay_t<Receiver>>
                    {
                        return get_env(r.op_state.receiver);
                    }
                };

                struct scheduler_sender_value_visitor
                {
                    HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

                    [[noreturn]] void operator()(hpx::monostate) const
                    {
                        HPX_UNREACHABLE;
                    }

                    template <typename Ts,
                        typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<Ts>, hpx::monostate>>>
                    void operator()(Ts&& ts)
                    {
                        hpx::invoke_fused(
                            hpx::bind_front(
                                hpx::execution::experimental::set_value,
                                HPX_MOVE(receiver)),
                            HPX_FORWARD(Ts, ts));
                    }
                };

                template <typename Error>
                void set_error_scheduler_sender(Error&& error) noexcept
                {
                    scheduler_op_state.reset();
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver), HPX_FORWARD(Error, error));
                }

                void set_stopped_scheduler_sender() noexcept
                {
                    scheduler_op_state.reset();
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(receiver));
                }

                void set_value_scheduler_sender() noexcept
                {
                    scheduler_op_state.reset();
                    hpx::visit(
                        scheduler_sender_value_visitor{HPX_MOVE(receiver)},
                        HPX_MOVE(ts));
                }

                friend void tag_invoke(start_t, operation_state& os) noexcept
                {
                    hpx::execution::experimental::start(os.sender_os);
                }
            };
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

            template <typename Receiver>
            friend operation_state<Receiver> tag_invoke(
                connect_t, schedule_from_sender&& s, Receiver&& receiver)
            {
                return {HPX_MOVE(s.predecessor_sender), HPX_MOVE(s.scheduler),
                    HPX_FORWARD(Receiver, receiver)};
            }

            template <typename Receiver>
            friend operation_state<Receiver> tag_invoke(
                connect_t, schedule_from_sender& s, Receiver&& receiver)
            {
                return {s.predecessor_sender, s.scheduler,
                    HPX_FORWARD(Receiver, receiver)};
            }
        };
    }    // namespace detail

    // execution::schedule_from is used to schedule work dependent on the
    // completion of a sender onto a scheduler's associated execution context.
    //
    // [Note: schedule_from is not meant to be used in user code; it is used in
    // the implementation of transfer. -end note]
    //
    // Senders returned from execution::schedule_from shall not propagate the
    // sender queries get_completion_scheduler<CPO> to an input sender. They
    // will implement get_completion_scheduler<CPO>, where CPO is one of
    // set_value_t and set_stopped_t; this query returns a scheduler equivalent
    // to the sch argument from those queries. The
    // get_completion_scheduler<set_error_t> is not implemented, as the
    // scheduler cannot be guaranteed in case an error is thrown while trying to
    // schedule work on the given scheduler object.
    HPX_CXX_CORE_EXPORT
    HPX_HOST_DEVICE_INLINE_CONSTEXPR_VARIABLE struct schedule_from_t final
      : hpx::functional::detail::tag_fallback<schedule_from_t>
    {
    private:
        // clang-format off
        template <typename Scheduler, typename Sender,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            schedule_from_t, Scheduler&& scheduler, Sender&& predecessor_sender)
        {
            return detail::schedule_from_sender<Sender, Scheduler>{
                HPX_FORWARD(Sender, predecessor_sender),
                HPX_FORWARD(Scheduler, scheduler)};
        }
    } schedule_from{};
}    // namespace hpx::execution::experimental

#endif
