//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/receiver.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/functional/invoke_fused.hpp>
#include <hpx/functional/tag_invoke.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        enum class ensure_started_state_enum
        {
            empty,
            started,
            completed
        };

        template <typename Receiver>
        struct ensure_started_error_visitor
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver>& receiver;

            template <typename Error>
            void operator()(Error const& error) noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(receiver), error);
            }
        };

        template <typename Receiver>
        struct ensure_started_value_visitor
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver>& receiver;

            template <typename Ts>
            void operator()(Ts const& ts) noexcept
            {
                hpx::invoke_fused(
                    hpx::bind_front(hpx::execution::experimental::set_value,
                        HPX_MOVE(receiver)),
                    ts);
            }
        };

        template <typename Sender, typename Allocator>
        struct ensure_started_sender
        {
            using is_sender = void;

            template <typename Tuple>
            struct value_types_helper
            {
                using const_type =
                    hpx::util::detail::transform_t<Tuple, std::add_const>;
                using type = hpx::util::detail::transform_t<const_type,
                    std::add_lvalue_reference>;
            };

            template <typename Env>
            struct generate_completion_signatures
            {
                template <template <typename...> typename Tuple,
                    template <typename...> typename Variant>
                using value_types = hpx::util::detail::transform_t<
                    value_types_of_t<Sender, Env, Tuple, Variant>,
                    value_types_helper>;

                template <template <typename...> typename Variant>
                using error_types =
                    hpx::util::detail::unique_t<hpx::util::detail::prepend_t<
                        error_types_of_t<Sender, Env, Variant>,
                        std::exception_ptr>>;

                static constexpr bool sends_stopped = true;
            };

            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, ensure_started_sender const&, Env)
                -> generate_completion_signatures<Env>;

            friend auto tag_invoke(hpx::execution::experimental::get_env_t,
                ensure_started_sender const& sender) noexcept
            {
                return hpx::execution::experimental::get_env(
                    sender.state->sender_);
            }

            struct shared_state
            {
                struct ensure_started_receiver;

                using allocator_type = typename std::allocator_traits<
                    Allocator>::template rebind_alloc<shared_state>;
                HPX_NO_UNIQUE_ADDRESS allocator_type alloc;

                hpx::spinlock mtx;
                hpx::util::atomic_count reference_count{0};
                std::atomic<ensure_started_state_enum> state_enum{
                    ensure_started_state_enum::empty};

                using operation_state_type = std::decay_t<
                    connect_result_t<Sender, ensure_started_receiver>>;
                operation_state_type os;

                using signatures = generate_completion_signatures<empty_env>;

                struct stopped_type
                {
                };
                using value_type = value_types_of_t<Sender, empty_env,
                    decayed_tuple, hpx::variant>;
                using error_type = detail::error_types_from<signatures,
                    meta::func<hpx::variant>>;

                hpx::variant<hpx::monostate, stopped_type, error_type,
                    value_type>
                    v;

                using continuation_type = hpx::move_only_function<void()>;
                hpx::detail::small_vector<continuation_type, 1> continuations;

                // Store the predecessor sender to be able to extract its environment
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender_;

                struct ensure_started_receiver
                {
                    hpx::intrusive_ptr<shared_state> state;

                    template <typename Error>
                    friend void tag_invoke(set_error_t,
                        ensure_started_receiver&& r, Error&& error) noexcept
                    {
                        HPX_MOVE(r).set_error(HPX_FORWARD(Error, error));
                    }

                    template <typename Error>
                    void set_error(Error&& error) && noexcept
                    {
                        try
                        {
                            state->v.template emplace<error_type>(
                                error_type(HPX_FORWARD(Error, error)));
                        }
                        catch (...)
                        {
                            std::terminate();
                        }
                        state->set_completed();
                        state.reset();
                    }

                    friend void tag_invoke(
                        set_stopped_t, ensure_started_receiver&& r) noexcept
                    {
                        if (r.state)
                        {
                            r.state->v.template emplace<stopped_type>();
                            r.state->set_completed();
                            r.state.reset();
                        }
                    };

                    using value_type = value_types_of_t<Sender, empty_env,
                        decayed_tuple, hpx::variant>;

                    template <typename... Ts>
                    friend auto tag_invoke(set_value_t,
                        ensure_started_receiver&& r, Ts&&... ts) noexcept
                        -> decltype(std::declval<hpx::variant<hpx::monostate,
                                        value_type>>()
                                        .template emplace<value_type>(
                                            hpx::tuple<std::decay_t<Ts>...>(
                                                HPX_FORWARD(Ts, ts)...)),
                            void())
                    {
                        hpx::detail::try_catch_exception_ptr(
                            [&]() {
                                r.state->v.template emplace<value_type>(
                                    hpx::make_tuple(HPX_FORWARD(Ts, ts)...));
                                r.state->set_completed();
                                r.state.reset();
                            },
                            [&](std::exception_ptr ep) {
                                HPX_MOVE(r).set_error(HPX_MOVE(ep));
                            });
                    }
                };

                template <typename Sender_,
                    HPX_CONCEPT_REQUIRES_(meta::value<
                        meta::none_of<shared_state, std::decay_t<Sender_>>>)>
                shared_state(Sender_&& sender, allocator_type const& alloc)
                  : alloc(alloc)
                  , os(hpx::execution::experimental::connect(
                        HPX_FORWARD(Sender_, sender),
                        ensure_started_receiver{this}))
                  , sender_(HPX_FORWARD(Sender_, sender))
                {
                }

                virtual ~shared_state()
                {
                    HPX_ASSERT_MSG(
                        state_enum.load() != ensure_started_state_enum::empty,
                        "start was never called on the operation state of "
                        "ensure_started.");
                }

                template <typename Receiver>
                struct done_error_value_visitor
                {
                    HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

                    [[noreturn]] void operator()(hpx::monostate) const
                    {
                        HPX_UNREACHABLE;
                    }

                    void operator()(stopped_type)
                    {
                        hpx::execution::experimental::set_stopped(
                            HPX_MOVE(receiver));
                    }

                    void operator()(error_type const& error)
                    {
                        hpx::visit(
                            ensure_started_error_visitor<Receiver>{receiver},
                            error);
                    }

                    void operator()(value_type const& ts)
                    {
                        hpx::visit(
                            ensure_started_value_visitor<Receiver>{receiver},
                            ts);
                    }
                };

                void set_completed()
                {
                    state_enum.store(ensure_started_state_enum::completed);

                    {
                        std::unique_lock l{mtx};
                    }

                    if (!continuations.empty())
                    {
                        for (auto const& continuation : continuations)
                        {
                            continuation();
                        }
                        continuations.clear();
                    }
                }

                template <typename Receiver>
                void add_continuation(Receiver& receiver) = delete;

                template <typename Receiver>
                void add_continuation(Receiver&& receiver)
                {
                    if (state_enum.load() ==
                        ensure_started_state_enum::completed)
                    {
                        hpx::visit(
                            done_error_value_visitor<Receiver>{
                                HPX_FORWARD(Receiver, receiver)},
                            v);
                    }
                    else
                    {
                        std::unique_lock l{mtx};

                        if (state_enum.load() ==
                            ensure_started_state_enum::completed)
                        {
                            l.unlock();
                            hpx::visit(
                                done_error_value_visitor<Receiver>{
                                    HPX_FORWARD(Receiver, receiver)},
                                v);
                        }
                        else
                        {
                            continuations.emplace_back(
                                [this,
                                    receiver = HPX_FORWARD(
                                        Receiver, receiver)]() mutable {
                                    hpx::visit(
                                        done_error_value_visitor<Receiver>{
                                            HPX_MOVE(receiver)},
                                        v);
                                });
                        }
                    }
                }

                void start() & noexcept
                {
                    ensure_started_state_enum expected =
                        ensure_started_state_enum::empty;
                    if (state_enum.compare_exchange_strong(
                            expected, ensure_started_state_enum::started))
                    {
                        hpx::execution::experimental::start(os);
                    }
                }

                friend void intrusive_ptr_add_ref(shared_state* p) noexcept
                {
                    p->reference_count.increment();
                }

                friend void intrusive_ptr_release(shared_state* p) noexcept
                {
                    if (p->reference_count.decrement() == 0)
                    {
                        std::atomic_thread_fence(std::memory_order_acquire);

                        allocator_type other_alloc(p->alloc);
                        std::allocator_traits<allocator_type>::destroy(
                            other_alloc, p);
                        std::allocator_traits<allocator_type>::deallocate(
                            other_alloc, p, 1);
                    }
                }
            };

            hpx::intrusive_ptr<shared_state> state;

            template <typename Sender_>
            ensure_started_sender(Sender_&& sender, Allocator const& allocator)
            {
                using allocator_type = Allocator;
                using other_allocator = typename std::allocator_traits<
                    allocator_type>::template rebind_alloc<shared_state>;
                using allocator_traits = std::allocator_traits<other_allocator>;
                using unique_ptr = std::unique_ptr<shared_state,
                    util::allocator_deleter<other_allocator>>;

                other_allocator alloc(allocator);
                unique_ptr p(allocator_traits::allocate(alloc, 1),
                    hpx::util::allocator_deleter<other_allocator>{alloc});

                allocator_traits::construct(
                    alloc, p.get(), HPX_FORWARD(Sender_, sender), allocator);
                state = p.release();

                state->start();
            }

            ensure_started_sender(ensure_started_sender const&) = default;
            ensure_started_sender& operator=(
                ensure_started_sender const&) = default;
            ensure_started_sender(ensure_started_sender&&) = default;
            ensure_started_sender& operator=(ensure_started_sender&&) = default;

            template <typename Receiver>
            struct operation_state
            {
                HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
                hpx::intrusive_ptr<shared_state> state;

                template <typename Receiver_>
                operation_state(Receiver_&& receiver,
                    hpx::intrusive_ptr<shared_state> state)
                  : receiver(HPX_FORWARD(Receiver_, receiver))
                  , state(HPX_MOVE(state))
                {
                }

                operation_state(operation_state&&) = delete;
                operation_state& operator=(operation_state&&) = delete;
                operation_state(operation_state const&) = delete;
                operation_state& operator=(operation_state const&) = delete;

                friend void tag_invoke(start_t, operation_state& os) noexcept
                {
                    os.state->add_continuation(HPX_MOVE(os.receiver));
                }
            };

            template <typename Receiver>
            friend operation_state<Receiver> tag_invoke(
                connect_t, ensure_started_sender&& s, Receiver&& receiver)
            {
                return {HPX_FORWARD(Receiver, receiver), HPX_MOVE(s.state)};
            }

            template <typename Receiver>
            friend operation_state<Receiver> tag_invoke(
                connect_t, ensure_started_sender const& s, Receiver&& receiver)
            {
                return {HPX_FORWARD(Receiver, receiver), s.state};
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT inline constexpr struct ensure_started_t final
      : hpx::functional::detail::tag_priority<ensure_started_t>
    {
    private:
        template <typename Sender,
            typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(is_sender_v<Sender>&&
                    hpx::traits::is_allocator_v<Allocator>&& experimental::
                        detail::is_completion_scheduler_tag_invocable_v<
                            hpx::execution::experimental::set_value_t, Sender,
                            ensure_started_t, Allocator>)>
        friend constexpr HPX_FORCEINLINE auto tag_override_invoke(
            ensure_started_t, Sender&& sender, Allocator const& allocator = {})
        {
            auto scheduler =
                hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(sender);

            return hpx::functional::tag_invoke(ensure_started_t{},
                HPX_MOVE(scheduler), HPX_FORWARD(Sender, sender), allocator);
        }

        template <typename Sender,
            typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender>&& hpx::traits::is_allocator_v<Allocator>)>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            ensure_started_t, Sender&& sender, Allocator const& allocator = {})
        {
            return detail::ensure_started_sender<Sender, Allocator>{
                HPX_FORWARD(Sender, sender), allocator};
        }

        template <typename Sender, typename Allocator>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            ensure_started_t,
            detail::ensure_started_sender<Sender, Allocator> sender,
            Allocator const& = {})
        {
            return sender;
        }

        template <typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(hpx::traits::is_allocator_v<Allocator>)>
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            ensure_started_t, Allocator const& allocator = {})
        {
            return detail::partial_algorithm<ensure_started_t, Allocator>{
                allocator};
        }
    } ensure_started{};
}    // namespace hpx::execution::experimental
