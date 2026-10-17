// Copyright (c) 2026 Shivansh Singh
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>

#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/executors/parallel_scheduler_backend.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace hpx::execution::experimental {

    // Forward declarations
    HPX_CXX_CORE_EXPORT class task_scheduler;

    namespace detail {

        // P3927R2: Bulk sender returned by task_scheduler_domain::transform_sender.
        // Adapts bulk_chunked_t and bulk_unchunked_t algorithms to route through
        // the backend's virtual dispatch methods.
        HPX_CXX_CORE_EXPORT template <typename ChildSender, typename F,
            bool IsChunked, bool IsParallel>
        struct task_bulk_sender
        {
            using sender_concept = sender_t;

            std::shared_ptr<parallel_scheduler_backend> backend_;
            std::size_t count_;
            std::size_t actual_shape_;
            F f_;
            ChildSender child_;

            template <typename Self, typename Env>
            static consteval auto get_completion_signatures() noexcept
                -> decltype(hpx::execution::experimental::
                        transform_completion_signatures(
                            hpx::execution::experimental::
                                completion_signatures_of_t<ChildSender, Env>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_value_t>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_error_t>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_stopped_t>{},
                            hpx::execution::experimental::completion_signatures<
                                hpx::execution::experimental::set_error_t(
                                    std::exception_ptr)>{}))
            {
                return {};
            }

            template <typename Receiver>
            struct operation_state
            {
                std::unique_ptr<base_parallel_bulk_op> impl_;

                explicit operation_state(
                    std::unique_ptr<base_parallel_bulk_op> p)
                  : impl_(HPX_MOVE(p))
                {
                }

                operation_state(operation_state&&) = delete;
                operation_state(operation_state const&) = delete;
                operation_state& operator=(operation_state&&) = delete;
                operation_state& operator=(operation_state const&) = delete;

                void start() noexcept
                {
                    impl_->start();
                }
            };

            template <typename Receiver>
            operation_state<std::decay_t<Receiver>> connect(Receiver&& rcvr) &&
            {
                return operation_state<std::decay_t<Receiver>>{
                    std::make_unique<virtual_parallel_bulk_op<F, IsChunked,
                        IsParallel, ChildSender, std::decay_t<Receiver>>>(
                        HPX_MOVE(backend_), count_, actual_shape_, HPX_MOVE(f_),
                        HPX_MOVE(child_), HPX_FORWARD(Receiver, rcvr))};
            }

            template <typename Receiver>
            operation_state<std::decay_t<Receiver>> connect(
                Receiver&& rcvr) const&
            {
                auto child_copy = child_;
                return operation_state<std::decay_t<Receiver>>{
                    std::make_unique<virtual_parallel_bulk_op<F, IsChunked,
                        IsParallel, ChildSender, std::decay_t<Receiver>>>(
                        backend_, count_, actual_shape_, f_,
                        HPX_MOVE(child_copy), HPX_FORWARD(Receiver, rcvr))};
            }
        };

        // P3927R2: Backend wrapper that adapts an arbitrary scheduler Sch
        // into the parallel_scheduler_backend interface.
        //
        // This is the HPX analog of stdexec's task_scheduler::__backend_for.
        // It wraps a concrete scheduler and implements the three virtual
        // dispatch methods (schedule, schedule_bulk_chunked,
        // schedule_bulk_unchunked) by creating and executing senders from the
        // wrapped scheduler.
        HPX_CXX_CORE_EXPORT template <typename Sch>
        class task_scheduler_backend_wrapper final
          : public parallel_scheduler_backend
        {
        private:
            struct custom_receiver_env
            {
                Sch const* sched_ = nullptr;

                constexpr Sch const& query(
                    hpx::execution::experimental::get_scheduler_t)
                    const noexcept
                {
                    return *sched_;
                }

                constexpr Sch const&
                query(hpx::execution::experimental::get_completion_scheduler_t<
                    hpx::execution::experimental::set_value_t>) const noexcept
                {
                    return *sched_;
                }
            };

            struct custom_receiver
            {
                using receiver_concept =
                    hpx::execution::experimental::receiver_t;
                parallel_scheduler_receiver_proxy* proxy_;
                void (*destroy_)(void*) = nullptr;
                void* storage_ = nullptr;
                Sch const* sched_ = nullptr;

                void set_value() noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    proxy->set_value();
                }

                template <typename Error = std::exception_ptr>
                void set_error(Error ep) noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    if constexpr (std::is_same_v<std::decay_t<Error>,
                                      std::exception_ptr>)
                    {
                        proxy->set_error(HPX_MOVE(ep));
                    }
                    else
                    {
                        proxy->set_error(std::make_exception_ptr(HPX_MOVE(ep)));
                    }
                }

                void set_stopped() noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    proxy->set_stopped();
                }

                constexpr custom_receiver_env get_env() const noexcept
                {
                    return {sched_};
                }
            };

            struct custom_bulk_receiver
            {
                using receiver_concept =
                    hpx::execution::experimental::receiver_t;
                parallel_scheduler_bulk_item_receiver_proxy* proxy_;
                void (*destroy_)(void*) = nullptr;
                void* storage_ = nullptr;
                Sch const* sched_ = nullptr;

                void set_value() noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    proxy->set_value();
                }

                template <typename Error = std::exception_ptr>
                void set_error(Error ep) noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    if constexpr (std::is_same_v<std::decay_t<Error>,
                                      std::exception_ptr>)
                    {
                        proxy->set_error(HPX_MOVE(ep));
                    }
                    else
                    {
                        proxy->set_error(std::make_exception_ptr(HPX_MOVE(ep)));
                    }
                }

                void set_stopped() noexcept
                {
                    auto* proxy = proxy_;
                    auto destroy = destroy_;
                    auto* storage = storage_;
                    if (destroy && storage)
                        destroy(storage);
                    proxy->set_stopped();
                }

                constexpr custom_receiver_env get_env() const noexcept
                {
                    return {sched_};
                }
            };

        public:
            explicit task_scheduler_backend_wrapper(Sch sch)
              : scheduler_(HPX_MOVE(sch))
            {
            }

            // Schedule a single unit of work.
            // Creates a sender from the wrapped scheduler and executes the
            // proxy's set_value() on that context.
            void schedule(parallel_scheduler_receiver_proxy& proxy,
                std::span<std::byte> storage) noexcept override
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        auto snd =
                            hpx::execution::experimental::schedule(scheduler_);
                        using op_state_t =
                            decltype(hpx::execution::experimental::connect(
                                HPX_MOVE(snd),
                                custom_receiver{
                                    &proxy, nullptr, nullptr, &scheduler_}));

                        if constexpr (sizeof(op_state_t) <=
                                parallel_scheduler_storage_size &&
                            alignof(op_state_t) <=
                                parallel_scheduler_storage_alignment)
                        {
                            HPX_ASSERT(storage.size() >= sizeof(op_state_t));

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                            };

                            auto* op =
                                ::new (static_cast<void*>(storage.data()))
                                    op_state_t(
                                        hpx::execution::experimental::connect(
                                            HPX_MOVE(snd),
                                            custom_receiver{&proxy, destroy_fn,
                                                storage.data(), &scheduler_}));
                            hpx::execution::experimental::start(*op);
                        }
                        else
                        {
                            void* heap_storage =
                                ::operator new(sizeof(op_state_t),
                                    std::align_val_t{alignof(op_state_t)});

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                                ::operator delete(
                                    ptr, std::align_val_t{alignof(op_state_t)});
                            };

                            op_state_t* op = nullptr;
                            try
                            {
                                op = ::new (heap_storage) op_state_t(
                                    hpx::execution::experimental::connect(
                                        HPX_MOVE(snd),
                                        custom_receiver{&proxy, destroy_fn,
                                            heap_storage, &scheduler_}));
                            }
                            catch (...)
                            {
                                ::operator delete(heap_storage,
                                    std::align_val_t{alignof(op_state_t)});
                                throw;
                            }
                            hpx::execution::experimental::start(*op);
                        }
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            // Schedule chunked bulk work.
            void schedule_bulk_chunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte> storage) noexcept override
            {
                if (count == 0)
                {
                    proxy.set_value();
                    return;
                }

                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        auto snd =
                            hpx::execution::experimental::schedule(scheduler_) |
                            hpx::execution::experimental::bulk_chunked(
                                hpx::execution::experimental::par, count,
                                [&proxy](std::size_t begin, std::size_t end) {
                                    proxy.execute(begin, end);
                                });
                        using op_state_t =
                            decltype(hpx::execution::experimental::connect(
                                HPX_MOVE(snd),
                                custom_bulk_receiver{
                                    &proxy, nullptr, nullptr, &scheduler_}));

                        if constexpr (sizeof(op_state_t) <=
                                parallel_scheduler_storage_size &&
                            alignof(op_state_t) <=
                                parallel_scheduler_storage_alignment)
                        {
                            HPX_ASSERT(storage.size() >= sizeof(op_state_t));

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                            };

                            auto* op = ::new (
                                static_cast<void*>(storage.data()))
                                op_state_t(
                                    hpx::execution::experimental::connect(
                                        HPX_MOVE(snd),
                                        custom_bulk_receiver{&proxy, destroy_fn,
                                            storage.data(), &scheduler_}));
                            hpx::execution::experimental::start(*op);
                        }
                        else
                        {
                            void* heap_storage =
                                ::operator new(sizeof(op_state_t),
                                    std::align_val_t{alignof(op_state_t)});

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                                ::operator delete(
                                    ptr, std::align_val_t{alignof(op_state_t)});
                            };

                            op_state_t* op = nullptr;
                            try
                            {
                                op = ::new (heap_storage) op_state_t(
                                    hpx::execution::experimental::connect(
                                        HPX_MOVE(snd),
                                        custom_bulk_receiver{&proxy, destroy_fn,
                                            heap_storage, &scheduler_}));
                            }
                            catch (...)
                            {
                                ::operator delete(heap_storage,
                                    std::align_val_t{alignof(op_state_t)});
                                throw;
                            }
                            hpx::execution::experimental::start(*op);
                        }
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            // Schedule unchunked bulk work.
            void schedule_bulk_unchunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte> storage) noexcept override
            {
                if (count == 0)
                {
                    proxy.set_value();
                    return;
                }

                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        auto snd =
                            hpx::execution::experimental::schedule(scheduler_) |
                            hpx::execution::experimental::bulk_unchunked(
                                hpx::execution::experimental::par, count,
                                [&proxy](std::size_t i) {
                                    proxy.execute(i, i + 1);
                                });
                        using op_state_t =
                            decltype(hpx::execution::experimental::connect(
                                HPX_MOVE(snd),
                                custom_bulk_receiver{
                                    &proxy, nullptr, nullptr, &scheduler_}));

                        if constexpr (sizeof(op_state_t) <=
                                parallel_scheduler_storage_size &&
                            alignof(op_state_t) <=
                                parallel_scheduler_storage_alignment)
                        {
                            HPX_ASSERT(storage.size() >= sizeof(op_state_t));

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                            };

                            auto* op = ::new (
                                static_cast<void*>(storage.data()))
                                op_state_t(
                                    hpx::execution::experimental::connect(
                                        HPX_MOVE(snd),
                                        custom_bulk_receiver{&proxy, destroy_fn,
                                            storage.data(), &scheduler_}));
                            hpx::execution::experimental::start(*op);
                        }
                        else
                        {
                            void* heap_storage =
                                ::operator new(sizeof(op_state_t),
                                    std::align_val_t{alignof(op_state_t)});

                            auto destroy_fn = [](void* ptr) {
                                static_cast<op_state_t*>(ptr)->~op_state_t();
                                ::operator delete(
                                    ptr, std::align_val_t{alignof(op_state_t)});
                            };

                            op_state_t* op = nullptr;
                            try
                            {
                                op = ::new (heap_storage) op_state_t(
                                    hpx::execution::experimental::connect(
                                        HPX_MOVE(snd),
                                        custom_bulk_receiver{&proxy, destroy_fn,
                                            heap_storage, &scheduler_}));
                            }
                            catch (...)
                            {
                                ::operator delete(heap_storage,
                                    std::align_val_t{alignof(op_state_t)});
                                throw;
                            }
                            hpx::execution::experimental::start(*op);
                        }
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

            // P3927R2: Query the forward progress guarantee of the wrapped scheduler.
            hpx::execution::experimental::forward_progress_guarantee
            get_forward_progress_guarantee() const noexcept override
            {
                return hpx::execution::experimental::
                    get_forward_progress_guarantee(scheduler_);
            }

        private:
            Sch scheduler_;
        };

    }    // namespace detail

    // P3927R2: Domain for task_scheduler.
    // Intercepts bulk_chunked_t and bulk_unchunked_t senders completing on a
    // task_scheduler and routes them through the backend's virtual dispatch.
    HPX_CXX_CORE_EXPORT struct task_scheduler_domain
      : hpx::execution::experimental::detail::sync_wait_domain
    {
        template <bulk_chunked_or_unchunked_sender Sender, typename Env>
        auto transform_sender(hpx::execution::experimental::set_value_t,
            Sender&& sndr, Env const& /*env*/) const
        {
            if constexpr (hpx::execution::experimental::stdexec_internal::
                              __completes_on<Sender, task_scheduler, Env>)
            {
                // Extract bulk parameters using structured binding
                auto&& [tag, data, child] = HPX_FORWARD(Sender, sndr);
                auto&& [pol, shape, f] = data;

                // Get the task_scheduler from the bulk sender's env.
                auto task_sched =
                    hpx::execution::experimental::get_completion_scheduler<
                        hpx::execution::experimental::set_value_t>(
                        hpx::execution::experimental::get_env(sndr));

                constexpr bool is_chunked =
                    sender_invokes_algorithm_v<Sender, bulk_chunked_t>;

                constexpr bool is_parallel =
                    !is_sequenced_policy_v<std::decay_t<decltype(pol.__get())>>;

                if constexpr (!std::is_lvalue_reference_v<Sender>)
                {
                    return detail::task_bulk_sender<
                        std::decay_t<decltype(child)>,
                        std::decay_t<decltype(f)>, is_chunked, is_parallel>{
                        task_sched.get_backend(),
                        static_cast<std::size_t>(is_parallel ? shape : 1),
                        static_cast<std::size_t>(shape), HPX_MOVE(f),
                        HPX_MOVE(child)};
                }
                else
                {
                    return detail::task_bulk_sender<
                        std::decay_t<decltype(child)>,
                        std::decay_t<decltype(f)>, is_chunked, is_parallel>{
                        task_sched.get_backend(),
                        static_cast<std::size_t>(is_parallel ? shape : 1),
                        static_cast<std::size_t>(shape), f, child};
                }
            }
            else
            {
                static_assert(hpx::execution::experimental::stdexec_internal::
                                  __completes_on<Sender, task_scheduler, Env>,
                    "Cannot dispatch bulk algorithm to the task_scheduler: "
                    "no task_scheduler found in the environment. "
                    "Add a continues_on transition to the task_scheduler "
                    "before the bulk algorithm.");
            }
        }
    };

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
            HPX_ASSERT(backend_);
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
        forward_progress_guarantee query(
            get_forward_progress_guarantee_t) const noexcept
        {
            return backend_->get_forward_progress_guarantee();
        }

        // P3927R2: get_completion_scheduler returns this scheduler.
        task_scheduler query(
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

            protected:
                void query_env(std::type_info const& query_type,
                    std::type_info const& result_type,
                    void* result) const noexcept override
                {
                    using token_type = std::decay_t<
                        decltype(hpx::execution::experimental::get_stop_token(
                            hpx::execution::experimental::get_env(receiver_)))>;
                    if (query_type ==
                            typeid(hpx::execution::experimental::
                                    get_stop_token_t) &&
                        result_type == typeid(token_type))
                    {
                        static_cast<std::optional<token_type>*>(result)
                            ->emplace(
                                hpx::execution::experimental::get_stop_token(
                                    hpx::execution::experimental::get_env(
                                        receiver_)));
                    }
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
