// Copyright (c) 2026 Shivansh Singh
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>

#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/executors/parallel_scheduler_backend.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    // Forward declarations
    HPX_CXX_CORE_EXPORT class task_scheduler;

    // P3927R2: Domain for task_scheduler.
    // Intercepts bulk_chunked_t and bulk_unchunked_t senders completing on a
    // task_scheduler and routes them through the backend's virtual dispatch.
    //
    // Step 1: Stub domain that inherits sync_wait_domain functionality.
    // The transform_sender for bulk operations will be added in Step 2.
    HPX_CXX_CORE_EXPORT struct task_scheduler_domain
      : hpx::execution::experimental::detail::sync_wait_domain
    {
    };

    namespace detail {

        // P3927R2: Backend wrapper that adapts an arbitrary scheduler Sch
        // into the parallel_scheduler_backend interface.
        //
        // This is the HPX analog of stdexec's task_scheduler::__backend_for.
        // It wraps a concrete scheduler and implements the three virtual
        // dispatch methods (schedule, schedule_bulk_chunked,
        // schedule_bulk_unchunked) by creating and executing senders from the
        // wrapped scheduler.
        template <typename Sch>
        class task_scheduler_backend_wrapper final
          : public parallel_scheduler_backend
        {
        public:
            explicit task_scheduler_backend_wrapper(Sch sch)
              : scheduler_(HPX_MOVE(sch))
            {
            }

            // Schedule a single unit of work.
            // Creates a sender from the wrapped scheduler and executes the
            // proxy's set_value() on that context.
            void schedule(parallel_scheduler_receiver_proxy& proxy,
                std::span<std::byte> /*storage*/) noexcept override
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        // Use the scheduler's execute() to post work to its
                        // execution context. The proxy must remain alive until
                        // completion (guaranteed by the sender/receiver
                        // contract).
                        scheduler_.execute(
                            [&proxy]() mutable { proxy.set_value(); });
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            // Schedule chunked bulk work.
            // P3927R2 Step 1: Forward to the wrapped scheduler's execute()
            // with a serial loop. The proper parallel dispatch via
            // bulk_chunked senders will be added in Step 2 when
            // task_scheduler_domain::transform_sender is implemented.
            void schedule_bulk_chunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte> /*storage*/) noexcept override
            {
                if (count == 0)
                {
                    proxy.set_value();
                    return;
                }

                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        scheduler_.execute([&proxy, count]() noexcept {
                            hpx::detail::try_catch_exception_ptr(
                                [&]() {
                                    proxy.execute(0, count);
                                    proxy.set_value();
                                },
                                [&](std::exception_ptr ep) {
                                    proxy.set_error(HPX_MOVE(ep));
                                });
                        });
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            // Schedule unchunked bulk work.
            // Same serial-forwarding strategy as schedule_bulk_chunked;
            // proper parallelization comes with the domain transform.
            void schedule_bulk_unchunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte> /*storage*/) noexcept override
            {
                if (count == 0)
                {
                    proxy.set_value();
                    return;
                }

                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        scheduler_.execute([&proxy, count]() noexcept {
                            hpx::detail::try_catch_exception_ptr(
                                [&]() {
                                    for (std::size_t i = 0; i < count; ++i)
                                    {
                                        proxy.execute(i, i + 1);
                                    }
                                    proxy.set_value();
                                },
                                [&](std::exception_ptr ep) {
                                    proxy.set_error(HPX_MOVE(ep));
                                });
                        });
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            // Backend equality: compares the wrapped schedulers.
            bool equal_to(
                parallel_scheduler_backend const& other) const noexcept override
            {
                auto const* p =
                    dynamic_cast<task_scheduler_backend_wrapper const*>(&other);
                if (!p)
                {
                    return false;
                }
                if constexpr (requires { scheduler_ == p->scheduler_; })
                {
                    return p->scheduler_ == scheduler_;
                }
                else
                {
                    return false;
                }
            }

        private:
            Sch scheduler_;
        };

    }    // namespace detail

    // P3927R2: task_scheduler -- a type-erased scheduler backed by
    // parallel_scheduler_backend.
    //
    // The task_scheduler wraps any scheduler conforming to HPX's scheduler
    // concept into a shared_ptr<parallel_scheduler_backend>. This allows
    // coroutines (task<T>) and other type-erased contexts to yield senders
    // that execute on the wrapped scheduler's context, including parallel
    // bulk operations (via task_scheduler_domain, added in Step 2).
    //
    // Construction:
    //   - From any scheduler Sch: wraps it via task_scheduler_backend_wrapper
    //   - From parallel_scheduler: reuses its existing backend shared_ptr
    //   - From task_scheduler: copies/moves the backend shared_ptr
    //
    // Equality: pointer equality on the backend (same as parallel_scheduler).
    HPX_CXX_CORE_EXPORT class task_scheduler
    {
    public:
        // Construct directly from a shared_ptr<parallel_scheduler_backend>.
        explicit task_scheduler(
            std::shared_ptr<parallel_scheduler_backend> backend) noexcept
          : backend_(HPX_MOVE(backend))
        {
        }

        // Construct from any scheduler that models the HPX scheduler concept.
        // Uses task_scheduler_backend_wrapper to adapt it to the backend
        // interface.
        //
        // SFINAE: disabled when Sch is task_scheduler, parallel_scheduler, or
        // convertible to std::shared_ptr<parallel_scheduler_backend>.
        template <typename Sch,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<Sch>, task_scheduler> &&
                !std::is_same_v<std::decay_t<Sch>, parallel_scheduler> &&
                !std::is_convertible_v<std::decay_t<Sch>,
                    std::shared_ptr<parallel_scheduler_backend>>>>
        explicit task_scheduler(Sch&& sch)
          : backend_(std::make_shared<
                detail::task_scheduler_backend_wrapper<std::decay_t<Sch>>>(
                HPX_FORWARD(Sch, sch)))
        {
        }

        // Construct from parallel_scheduler: reuse its existing backend
        // shared_ptr. This avoids wrapping the backend in another layer of
        // indirection and preserves the fast-path optimization for the
        // default HPX backend.
        explicit task_scheduler(parallel_scheduler const& par_sched) noexcept
          : backend_(par_sched.get_backend())
        {
        }

        // Defaulted copy/move for value semantics.
        task_scheduler(task_scheduler const&) noexcept = default;
        task_scheduler(task_scheduler&&) noexcept = default;
        task_scheduler& operator=(task_scheduler const&) noexcept = default;
        task_scheduler& operator=(task_scheduler&&) noexcept = default;

        // P3927R2: equality is backend pointer identity.
        friend bool operator==(
            task_scheduler const& lhs, task_scheduler const& rhs) noexcept
        {
            return lhs.backend_.get() == rhs.backend_.get();
        }

        friend bool operator!=(
            task_scheduler const& lhs, task_scheduler const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        // P3927R2: forward progress guarantee is determined by the backend.
        // For type-erased task_scheduler, we report concurrent to properly
        // represent the weakest bound across the type-erasure boundary.
        forward_progress_guarantee query(
            get_forward_progress_guarantee_t) const noexcept
        {
            return forward_progress_guarantee::concurrent;
        }

        // P3927R2: get_completion_scheduler returns this scheduler.
        task_scheduler const& query(
            get_completion_scheduler_t<set_value_t>) const noexcept
        {
            return *this;
        }

        // P3927R2: domain customization.
        task_scheduler_domain query(get_domain_t) const noexcept
        {
            return {};
        }

        // P3927R2: completion domain for domain resolution.
        task_scheduler_domain query(
            get_completion_domain_t<set_value_t>) const noexcept
        {
            return {};
        }

        // Operation state for task_scheduler::schedule().
        // Mirrors parallel_scheduler::operation_state: adapts a concrete
        // Receiver to the type-erased parallel_scheduler_receiver_proxy
        // interface and delegates to backend->schedule().
        template <typename Receiver>
        struct operation_state
        {
            // Concrete proxy adapting the actual Receiver.
            struct concrete_receiver_proxy final
              : parallel_scheduler_receiver_proxy
            {
                std::decay_t<Receiver>& receiver_;

                explicit concrete_receiver_proxy(
                    std::decay_t<Receiver>& rcvr) noexcept
                  : receiver_(rcvr)
                {
                }

                void set_value() noexcept override
                {
                    hpx::execution::experimental::set_value(
                        HPX_MOVE(receiver_));
                }

                void set_error(std::exception_ptr ep) noexcept override
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                }

                void set_stopped() noexcept override
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(receiver_));
                }

                bool stop_requested() const noexcept override
                {
                    return get_stop_token(get_env(receiver_)).stop_requested();
                }
            };

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
            std::shared_ptr<parallel_scheduler_backend> backend_;
            concrete_receiver_proxy proxy_;

            // Pre-allocated storage for the backend.
            alignas(parallel_scheduler_storage_alignment)
                std::byte storage_[parallel_scheduler_storage_size];

            template <typename Receiver_>
            operation_state(Receiver_&& receiver,
                std::shared_ptr<parallel_scheduler_backend> backend)
              : receiver_(HPX_FORWARD(Receiver_, receiver))
              , backend_(HPX_MOVE(backend))
              , proxy_(receiver_)
            {
            }

            operation_state(operation_state&&) = delete;
            operation_state(operation_state const&) = delete;
            operation_state& operator=(operation_state&&) = delete;
            operation_state& operator=(operation_state const&) = delete;

            void start() noexcept
            {
                // Check stop token before scheduling.
                auto stop_token = get_stop_token(get_env(receiver_));
                if (stop_token.stop_requested())
                {
                    set_stopped(HPX_MOVE(receiver_));
                    return;
                }

                backend_->schedule(proxy_, std::span<std::byte>(storage_));
            }
        };

        // Nested sender type returned by task_scheduler::schedule().
        template <typename Scheduler = task_scheduler>
        struct sender
        {
            Scheduler sched_;

            using sender_concept = sender_t;
            using completion_signatures =
                ::hpx::execution::experimental::completion_signatures<
                    set_value_t(), set_error_t(std::exception_ptr),
                    set_stopped_t()>;

            template <typename Receiver>
            operation_state<std::decay_t<Receiver>> connect(Receiver&& receiver)
                const& noexcept(
                    std::is_nothrow_constructible_v<std::decay_t<Receiver>,
                        Receiver>)
            {
                return {HPX_FORWARD(Receiver, receiver), sched_.get_backend()};
            }

            template <typename Receiver>
            operation_state<std::decay_t<Receiver>>
            connect(Receiver&& receiver) && noexcept(
                std::is_nothrow_constructible_v<std::decay_t<Receiver>,
                    Receiver>)
            {
                return {HPX_FORWARD(Receiver, receiver), sched_.get_backend()};
            }

            struct env
            {
                Scheduler const& sched_;

                auto query(
                    get_completion_scheduler_t<set_value_t>) const noexcept
                {
                    return sched_;
                }

                auto query(
                    get_completion_scheduler_t<set_stopped_t>) const noexcept
                {
                    return sched_;
                }

                task_scheduler_domain query(get_domain_t) const noexcept
                {
                    return {};
                }

                constexpr auto query(get_allocator_t) const noexcept
                {
                    return std::allocator<std::byte>{};
                }
            };

            env get_env() const noexcept
            {
                return {sched_};
            }
        };

        // Direct schedule() member.
        sender<task_scheduler> schedule() const noexcept
        {
            return {*this};
        }

        // Access the backend (for connect and domain transform).
        std::shared_ptr<parallel_scheduler_backend> const& get_backend()
            const noexcept
        {
            return backend_;
        }

    private:
        std::shared_ptr<parallel_scheduler_backend> backend_;
    };

}    // namespace hpx::execution::experimental
