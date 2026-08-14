//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <mutex>
#include <type_traits>

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT struct make_future_t;

    namespace detail {

        HPX_CXX_CORE_EXPORT struct sync_wait_domain;

        HPX_CXX_CORE_EXPORT struct run_loop_data;

        HPX_CORE_EXPORT void intrusive_ptr_add_ref(run_loop_data* p) noexcept;
        HPX_CORE_EXPORT void intrusive_ptr_release(run_loop_data* p) noexcept;

        HPX_CXX_CORE_EXPORT struct run_loop_data
        {
            using mutex_type = hpx::spinlock;

            run_loop_data() noexcept
              : count_(1)
            {
            }

            run_loop_data(run_loop_data const&) = delete;
            run_loop_data(run_loop_data&&) = delete;
            run_loop_data& operator=(run_loop_data const&) = delete;
            run_loop_data& operator=(run_loop_data&&) = delete;

            ~run_loop_data() = default;

            mutable mutex_type mtx_;

        private:
            friend HPX_CORE_EXPORT void intrusive_ptr_add_ref(
                run_loop_data*) noexcept;
            friend HPX_CORE_EXPORT void intrusive_ptr_release(
                run_loop_data*) noexcept;

            hpx::util::atomic_count count_;
        };
    }    // namespace detail

    // A run_loop is an execution context on which work can be scheduled. It
    // maintains a simple, thread-safe first-in-first-out queue of work. Its
    // run() member function removes elements from the queue and executes them
    // in a loop on whatever thread of execution calls run().
    //
    // A run_loop instance has an associated count that corresponds to the
    // number of work items that are in its queue. Additionally, a run_loop has
    // an associated state that can be one of starting, running, or finishing.
    //
    // Concurrent invocations of the member functions of run_loop, other than
    // run and its destructor, do not introduce data races. The member functions
    // pop_front, push_back, and finish execute atomically.
    //
    // [Note: Implementations are encouraged to use an intrusive queue of
    // operation states to hold the work units to make scheduling
    // allocation-free. -- end note]
    //
    HPX_CXX_CORE_EXPORT class run_loop
    {
        struct run_loop_opstate_base
        {
            explicit run_loop_opstate_base(run_loop_opstate_base* tail) noexcept
              : next(this)
              , tail(tail)
            {
            }

            run_loop_opstate_base(run_loop_opstate_base* tail,
                void (*execute)(run_loop_opstate_base*) noexcept) noexcept
              : next(tail)
              , execute_(execute)
            {
            }

            run_loop_opstate_base(run_loop_opstate_base const&) = delete;
            run_loop_opstate_base(run_loop_opstate_base&&) = delete;
            run_loop_opstate_base& operator=(
                run_loop_opstate_base const&) = delete;
            run_loop_opstate_base& operator=(run_loop_opstate_base&&) = delete;

            ~run_loop_opstate_base() = default;

            run_loop_opstate_base* next;
            union
            {
                void (*execute_)(run_loop_opstate_base*) noexcept;
                run_loop_opstate_base* tail;
            };

            void execute() noexcept
            {
                (*execute_)(this);
            }
        };

        template <typename Receiver>
        struct run_loop_opstate : run_loop_opstate_base
        {
            run_loop* loop;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

            template <typename Receiver_>
            run_loop_opstate(run_loop_opstate_base* tail, run_loop* loop,
                Receiver_&& receiver) noexcept(noexcept(std::
                    is_nothrow_constructible_v<std::decay_t<Receiver>,
                        Receiver_>))
              : run_loop_opstate_base(tail)
              , loop(loop)
              , receiver(HPX_FORWARD(Receiver_, receiver))
            {
            }

            static void execute(run_loop_opstate_base* p) noexcept
            {
                auto& receiver = static_cast<run_loop_opstate*>(p)->receiver;
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if (get_stop_token(get_env(receiver)).stop_requested())
                        {
                            hpx::execution::experimental::set_stopped(
                                HPX_MOVE(receiver));
                        }
                        else
                        {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver));
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver), HPX_MOVE(ep));
                    });
            }

            run_loop_opstate(
                run_loop_opstate_base* next, run_loop* loop, Receiver r)
              : run_loop_opstate_base(next, &execute)
              , loop(loop)
              , receiver(HPX_MOVE(r))
            {
            }

            void start() & noexcept;
        };

        struct env_t
        {
            [[nodiscard]]
            auto query(get_completion_scheduler_t<set_value_t>) const noexcept;

            [[nodiscard]]
            auto query(
                get_completion_scheduler_t<set_stopped_t>) const noexcept;

            // P3826R5: advertise the HPX-aware sync_wait domain on this
            // env so that stdexec::sync_wait routes through
            // detail::sync_wait_domain instead of default_domain. The body
            // is defined in detail/sync_wait_domain.hpp after sync_wait_domain
            // is complete; this header only forward-declares it (see top of
            // file). Templating on CPO defers instantiation until the
            // domain type is fully visible to the caller.
            template <typename CPO>
            [[nodiscard]]
            static auto query(get_completion_domain_t<CPO>) noexcept
                -> detail::sync_wait_domain;

            run_loop* loop;
        };

    public:
        class run_loop_scheduler : public env_t
        {
        public:
            struct run_loop_sender
            {
                using sender_concept = hpx::execution::experimental::sender_t;

                template <typename Receiver>
                run_loop_opstate<Receiver> connect(
                    Receiver&& receiver) const noexcept
                {
                    return run_loop_opstate<Receiver>(
                        &loop->head, loop, HPX_FORWARD(Receiver, receiver));
                }

                template <typename ReceiverEnv>
                static consteval auto get_completion_signatures() noexcept
                {
                    return completion_signatures<set_value_t(),
                        set_error_t(std::exception_ptr), set_stopped_t()>{};
                }

                [[nodiscard]] constexpr env_t get_env() const noexcept
                {
                    return env_t{loop};
                }

            private:
                friend run_loop_scheduler;

                constexpr explicit run_loop_sender(run_loop* loop) noexcept
                  : loop(loop)
                {
                }

                run_loop* loop;
            };

            friend run_loop;

        public:
            explicit run_loop_scheduler(run_loop* loop) noexcept
              : env_t(loop)
            {
            }

            run_loop& get_run_loop() const noexcept
            {
                return *loop;
            }

            run_loop_sender schedule() const noexcept
            {
                return run_loop_sender(loop);
            }

            using env_t::query;

            [[nodiscard]] static constexpr auto query(
                get_forward_progress_guarantee_t) noexcept
            {
                return forward_progress_guarantee::parallel;
            }

            template <typename Sender,
                typename Allocator = hpx::util::internal_allocator<>>
            [[nodiscard]] auto query(make_future_t, Sender&& sender,
                Allocator const& allocator = Allocator{}) const;

            [[nodiscard]] friend constexpr bool operator==(
                run_loop_scheduler const& lhs,
                run_loop_scheduler const& rhs) noexcept
            {
                return lhs.loop == rhs.loop;
            }
            [[nodiscard]] friend constexpr bool operator!=(
                run_loop_scheduler const& lhs,
                run_loop_scheduler const& rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };

    private:
        friend struct run_loop_scheduler::run_loop_sender;

        hpx::intrusive_ptr<detail::run_loop_data> mtx;
        hpx::lcos::local::detail::condition_variable cond_var;

        // MSVC and gcc don't properly handle the friend declaration above
#if defined(HPX_MSVC) || defined(HPX_GCC_VERSION)
    public:
#endif
        run_loop_opstate_base head;

    private:
        bool stop = false;

        void push_back(run_loop_opstate_base* t)
        {
            auto const local_mtx = mtx;    // keep alive
            std::unique_lock l(local_mtx->mtx_);

            stop = false;
            t->next = &head;
            head.tail = head.tail->next = t;
            cond_var.notify_one(HPX_MOVE(l));
        }

        run_loop_opstate_base* pop_front()
        {
            auto const local_mtx = mtx;    // keep alive
            std::unique_lock l(local_mtx->mtx_);

            while (head.next == &head && !stop)
            {
                cond_var.wait(l);
            }

            if (head.tail == head.next)
            {
                head.tail = &head;
            }

            // std::exchange(head.next, head.next->next);
            auto const old_val = HPX_MOVE(head.next);
            head.next = HPX_MOVE(head.next->next);
            return old_val;
        }

    public:
        // [exec.run_loop.ctor] construct/copy/destroy
        run_loop() noexcept
          // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
          : mtx(new detail::run_loop_data(), false)
          , head(&head)    //-V546
        {
        }

        run_loop(run_loop const&) = delete;
        run_loop(run_loop&&) = delete;
        run_loop& operator=(run_loop const&) = delete;
        run_loop& operator=(run_loop&&) = delete;

        // If count is not 0 or if state is running, invokes terminate().
        // Otherwise, has no effects.
        ~run_loop()
        {
            if (head.next != &head || !stop)
            {
                std::terminate();
            }
        }

        // [exec.run_loop.members] Member functions:
        run_loop_scheduler get_scheduler()
        {
            return run_loop_scheduler(this);
        }

        void run()
        {
            // Precondition: state is starting.
            //HPX_ASSERT(head.next != &head || !stop);
            for (run_loop_opstate_base* t; (t = pop_front()) != &head; /**/)
            {
                t->execute();
            }
            HPX_ASSERT(stop);    // Postcondition: state is finishing.
        }

        void finish()
        {
            auto const local_mtx = mtx;    // keep alive
            std::unique_lock l(local_mtx->mtx_);

            stop = true;
            cond_var.notify_all(HPX_MOVE(l));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    inline auto run_loop::env_t::query(
        get_completion_scheduler_t<set_value_t>) const noexcept
    {
        return run_loop_scheduler(loop);
    }

    inline auto run_loop::env_t::query(
        get_completion_scheduler_t<set_stopped_t>) const noexcept
    {
        return query(get_completion_scheduler<set_value_t>);
    }

    // run_loop::env_t::query(get_completion_domain_t<CPO>) is defined in
    // detail/sync_wait_domain.hpp once sync_wait_domain is complete.

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using run_loop_scheduler = run_loop::run_loop_scheduler;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Receiver>
    inline void run_loop::run_loop_opstate<Receiver>::start() & noexcept
    try
    {
        loop->push_back(this);
    }
    catch (...)
    {
        set_error(HPX_MOVE(receiver), std::current_exception());
    }
}    // namespace hpx::execution::experimental
