//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//  Copyright (c) 2025 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/execution_base/stdexec_forward.hpp>

#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/detail/single_result.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::when_all_vector_detail {

    // callback object to request cancellation
    HPX_CXX_CORE_EXPORT struct on_stop_requested
    {
        hpx::experimental::in_place_stop_source& stop_source_;
        void operator()() noexcept
        {
            stop_source_.request_stop();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sender>
    struct when_all_vector_sender_impl
    {
        struct when_all_vector_sender_type;
    };

    HPX_CXX_CORE_EXPORT template <typename Sender>
    using when_all_vector_sender = typename when_all_vector_sender_impl<
        Sender>::when_all_vector_sender_type;

    template <typename Sender>
    struct when_all_vector_sender_impl<Sender>::when_all_vector_sender_type
    {
        using sender_concept = hpx::execution::experimental::sender_t;
        using senders_type = std::vector<Sender>;
        senders_type senders;

        explicit constexpr when_all_vector_sender_type(senders_type&& senders)
          : senders(HPX_MOVE(senders))
        {
        }

        explicit constexpr when_all_vector_sender_type(
            senders_type const& senders)
          : senders(senders)
        {
        }

        // We expect a single value type or nothing from the predecessor
        // sender type
        using element_value_type =
            std::decay_t<hpx::execution::experimental::detail::single_result_t<
                hpx::execution::experimental::value_types_of_t<Sender,
                    hpx::execution::experimental::empty_env, meta::pack,
                    meta::pack>>>;

        static constexpr bool is_void_value_type =
            std::is_void_v<element_value_type>;

        // This is a helper empty type for the case that nothing is sent
        // from the predecessors
        struct void_value_type
        {
        };

        // Dummy parameter introduced to please GCC11 which enforces
        // explicit specialization in non-namespace scope as an error.
        // Reference: https://cplusplus.com/forum/general/58906/#msg318049
        template <typename T, typename Dummy = void>
        struct set_value_completion_helper
        {
            using type = hpx::execution::experimental::set_value_t(
                std::vector<T>);
        };

        template <typename Dummy>
        struct set_value_completion_helper<void, Dummy>
        {
            using type = hpx::execution::experimental::set_value_t();
        };

        using set_value_transform_to_vector =
            typename set_value_completion_helper<element_value_type>::type;

        template <typename...>
        using transformed_comp_sigs_identity =
            hpx::execution::experimental::completion_signatures<
                set_value_transform_to_vector>;

        template <typename Err>
        using decay_set_error =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_error_t(std::decay_t<Err>)>;

        template <typename Env>
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        friend auto tag_invoke(
            hpx::execution::experimental::get_completion_signatures_t,
            when_all_vector_sender_type const&, Env const&) noexcept
            -> hpx::execution::experimental::transform_completion_signatures<
                hpx::execution::experimental::completion_signatures_of_t<Sender,
                    Env>,
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>,
                transformed_comp_sigs_identity, decay_set_error>;
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif

        template <typename Receiver>
        struct operation_state
        {
            using receiver_type = std::decay_t<Receiver>;
            using operation_state_concept =
                hpx::execution::experimental::operation_state_t;

            struct when_all_vector_receiver
            {
                using receiver_concept =
                    hpx::execution::experimental::receiver_t;
                operation_state& op_state;
                std::size_t const i;

                template <typename Error>
                friend void tag_invoke(
                    hpx::execution::experimental::set_error_t,
                    when_all_vector_receiver&& r, Error&& error) noexcept
                {
                    if (!r.op_state.set_stopped_error_called.exchange(true))
                    {
                        r.op_state.stop_source_.request_stop();
                        try
                        {
                            r.op_state.error = HPX_FORWARD(Error, error);
                        }
                        catch (...)
                        {
                            // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
                            r.op_state.error = std::current_exception();
                        }
                    }

                    r.op_state.finish();
                }

                friend void tag_invoke(
                    hpx::execution::experimental::set_stopped_t,
                    when_all_vector_receiver&& r) noexcept
                {
                    // request stop only if we're not in error state
                    if (!r.op_state.set_stopped_error_called.exchange(true))
                    {
                        r.op_state.stop_source_.request_stop();
                    }
                    r.op_state.finish();
                };

                template <typename... Ts>
                friend void tag_invoke(
                    hpx::execution::experimental::set_value_t,
                    when_all_vector_receiver&& r, Ts&&... ts) noexcept
                {
                    if (!r.op_state.set_stopped_error_called)
                    {
                        try
                        {
                            // We only have something to store if the
                            // predecessor sends the single value that it should
                            // send. We have nothing to store for predecessor
                            // senders that send nothing.
                            if constexpr (sizeof...(Ts) == 1)
                            {
                                r.op_state.ts[r.i].emplace(
                                    HPX_FORWARD(Ts, ts)...);
                            }
                        }
                        catch (...)
                        {
                            if (!r.op_state.set_stopped_error_called.exchange(
                                    true))
                            {
                                // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
                                r.op_state.error = std::current_exception();
                            }
                        }
                    }

                    r.op_state.finish();
                }

                // clang-format off
                // TODO: Make this a method
                friend auto tag_invoke(hpx::execution::experimental::get_env_t,
                    when_all_vector_receiver const& r)
                    noexcept
                    -> hpx::execution::experimental::env<
                        hpx::execution::experimental::env_of_t<receiver_type>,
                        hpx::execution::experimental::prop<
                            hpx::execution::experimental::get_stop_token_t,
                            hpx::experimental::in_place_stop_token>>
                {
                    /* The new calling convention is:
                     * env(old_env, prop(tag, val))*/

                    // Due to the bug described in the get_env.cpp tests,
                    // returning an env constructed directly with the
                    // temporaries returned by the functions causes wrong
                    // behaviour.
                    auto e = hpx::execution::experimental::get_env(
                        r.op_state.receiver);
                    auto p = hpx::execution::experimental::prop(
                        hpx::execution::experimental::get_stop_token,
                        r.op_state.stop_source_.get_token());
                    return hpx::execution::experimental::env(
                        std::move(e), std::move(p));
                }
                // clang-format on
            };

            std::size_t const num_predecessors;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

            hpx::experimental::in_place_stop_source stop_source_{};

            using stop_token_t = hpx::execution::experimental::stop_token_of_t<
                hpx::execution::experimental::env_of_t<receiver_type>&>;
            hpx::optional<typename stop_token_t::template callback_type<
                on_stop_requested>>
                on_stop_{};

            // Number of predecessor senders that have not yet called any of
            // the set signals.
            std::atomic<std::size_t> predecessors_remaining{num_predecessors};

            // The values sent by the predecessor senders are stored in a
            // vector of optional or the dummy type void_value_type if the
            // predecessor senders send nothing
            using value_types_storage_type =
                std::conditional_t<is_void_value_type, void_value_type,
                    std::vector<std::optional<element_value_type>>>;
            value_types_storage_type ts;

            // The first error sent by any predecessor sender is stored in a
            // optional of a variant of the error_types
            using error_types =
                typename hpx::execution::experimental::error_types_of_t<
                    when_all_vector_sender_impl<
                        Sender>::when_all_vector_sender_type,
                    hpx::execution::experimental::empty_env, hpx::variant>;
            std::optional<error_types> error;

            // Set to true when set_stopped or set_error has been called
            std::atomic<bool> set_stopped_error_called{false};

            // The operation states are stored in an array of optionals of
            // the operation states to handle the non-movability and
            // non-copyability of them
            using operation_state_type =
                hpx::execution::experimental::connect_result_t<Sender,
                    when_all_vector_receiver>;
            using operation_states_storage_type =
                std::unique_ptr<std::optional<operation_state_type>[]>;
            operation_states_storage_type op_states = nullptr;

            template <typename Receiver_>
            operation_state(Receiver_&& receiver, std::vector<Sender>&& senders)
              : num_predecessors(senders.size())
              , receiver(HPX_FORWARD(Receiver_, receiver))
            {
                op_states =
                    std::make_unique<std::optional<operation_state_type>[]>(
                        num_predecessors);
                std::size_t i = 0;
                for (auto&& sender : senders)
                {
#if defined(HPX_HAVE_CXX17_COPY_ELISION)
#if defined(__NVCC__)
                    op_states[i].emplace(
                        hpx::util::detail::with_result_of([&]() {
                            return hpx::execution::experimental::connect(
                                std::move(sender),
                                when_all_vector_receiver{*this, i});
                        }));
#else
                    op_states[i].emplace(
                        hpx::util::detail::with_result_of([&]() {
                            return hpx::execution::experimental::connect(
                                HPX_MOVE(sender),
                                when_all_vector_receiver{*this, i});
                        }));
#endif
#else
                    // MSVC doesn't get copy elision quite right, the operation
                    // state must be constructed explicitly directly in place
                    op_states[i].template emplace_f<operation_state_type>(
                        hpx::execution::experimental::connect, HPX_MOVE(sender),
                        when_all_vector_receiver{*this, i});
#endif
                    ++i;
                }

                if constexpr (!is_void_value_type)
                {
                    ts.resize(num_predecessors);
                }
            }

            operation_state(operation_state&&) = delete;
            operation_state& operator=(operation_state&&) = delete;
            operation_state(operation_state const&) = delete;
            operation_state& operator=(operation_state const&) = delete;

            void finish() noexcept
            {
                if (--predecessors_remaining == 0)
                {
                    if (!set_stopped_error_called)
                    {
                        if constexpr (is_void_value_type)
                        {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver));
                        }
                        else
                        {
                            std::vector<element_value_type> values;
                            values.reserve(num_predecessors);
                            for (auto&& t : ts)
                            {
#if defined(__NVCC__)
                                values.push_back(std::move(t.value()));
#else
                                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                                values.push_back(HPX_MOVE(t.value()));
#endif
                            }
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver), HPX_MOVE(values));
                        }
                    }
                    else if (error)
                    {
                        hpx::visit(
                            [this](auto&& error) {
                                hpx::execution::experimental::set_error(
                                    HPX_MOVE(receiver),
                                    HPX_FORWARD(decltype(error), error));
                            },
                            HPX_MOVE(error.value()));
                    }
                    else
                    {
                        if constexpr (hpx::execution::experimental::
                                          sends_stopped<Sender>)
                        {
                            hpx::execution::experimental::set_stopped(
                                HPX_MOVE(receiver));
                        }
                        else
                        {
                            HPX_UNREACHABLE;
                        }
                    }
                }
            }

            friend void tag_invoke(hpx::execution::experimental::start_t,
                operation_state& os) noexcept
            {
                // register stop callback
                os.on_stop_.emplace(
                    hpx::execution::experimental::get_stop_token(
                        hpx::execution::experimental::get_env(os.receiver)),
                    on_stop_requested{os.stop_source_});

                // If a stop has already been requested. Don't bother starting
                // the child operations.
                if (os.stop_source_.stop_requested())
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_FORWARD(Receiver, os.receiver));
                    return;
                }

                // If there are no predecessors we can signal the
                // continuation as soon as start is called.
                if (os.num_predecessors == 0)
                {
                    // If the predecessor sender type sends nothing, we also
                    // send nothing to the continuation.
                    if constexpr (is_void_value_type)
                    {
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(os.receiver));
                    }
                    // If the predecessor sender type sends something we
                    // send an empty vector of that type to the continuation.
                    else
                    {
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(os.receiver),
                            std::vector<element_value_type>{});
                    }
                }
                // Otherwise we start all the operation states and wait for
                // the predecessors to signal completion.
                else
                {
                    for (std::size_t i = 0; i < os.num_predecessors; ++i)
                    {
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                        hpx::execution::experimental::start(
                            os.op_states.get()[i].value());
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
                    }
                }
            }
        };

        template <typename Receiver>
        friend auto tag_invoke(hpx::execution::experimental::connect_t,
            when_all_vector_sender_type&& s, Receiver&& receiver)
        {
            return operation_state<Receiver>(
                HPX_FORWARD(Receiver, receiver), HPX_MOVE(s.senders));
        }

        template <typename Receiver>
        friend auto tag_invoke(hpx::execution::experimental::connect_t,
            when_all_vector_sender_type& s, Receiver&& receiver)
        {
            return operation_state<Receiver>(receiver, s.senders);
        }
    };    // namespace hpx::when_all_vector_detail
}    // namespace hpx::when_all_vector_detail

namespace hpx::execution::experimental {

    // execution::when_all_vector is an extension over P2300 (wg21.link/p2300)
    //
    // execution::when_all_vector is used to join an arbitrary number of sender
    // chains and create a sender whose execution is dependent on all the
    // input senders that only send a single set of values.
    // execution::when_all_vector_with_variant is used to join multiple sender
    // chains and create a sender whose execution is dependent on all the
    // input senders, each of which may have one or more sets of sent values.
    //
    // when_all_vector returns a sender that completes once all the input
    // senders have completed. It is constrained to only accept senders that can
    // complete with a single set of values (_i.e._, it only calls one overload
    // of set_value on its receiver). The values sent by this sender are the
    // values sent by each of the input senders, in order of the arguments
    // passed to when_all_vector. It completes inline on the execution context
    // on which the last input sender completes, unless stop is requested before
    // when_all is started, in which case it completes inline within the call to
    // start.
    //
    // The returned sender has no completion schedulers.
    HPX_CXX_CORE_EXPORT inline constexpr struct when_all_vector_t final
      : hpx::functional::detail::tag_fallback<when_all_vector_t>
    {
    private:
        // clang-format off
        template <typename Sender,
            HPX_CONCEPT_REQUIRES_(
                hpx::execution::experimental::is_sender_v<Sender>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            when_all_vector_t, std::vector<Sender>&& senders)
        {
            return when_all_vector_detail::when_all_vector_sender<Sender>{
                HPX_MOVE(senders)};
        }

        // clang-format off
        template <typename Sender,
            HPX_CONCEPT_REQUIRES_(
                hpx::execution::experimental::is_sender_v<Sender>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            when_all_vector_t, std::vector<Sender> const& senders)
        {
            return when_all_vector_detail::when_all_vector_sender<Sender>{
                senders};
        }
    } when_all_vector{};
}    // namespace hpx::execution::experimental
