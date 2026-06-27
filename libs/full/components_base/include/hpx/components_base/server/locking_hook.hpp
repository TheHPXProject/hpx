//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/get_lva.hpp>
#include <hpx/components_base/traits/action_decorate_function.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx::components {

    namespace detail {

        template <typename Mutex>
        struct refcounted_mutex
        {
            friend void intrusive_ptr_add_ref(refcounted_mutex* p) noexcept
            {
                p->reference_count.increment();
            }

            friend void intrusive_ptr_release(refcounted_mutex* p) noexcept
            {
                if (p->reference_count.decrement() == 0)
                {
                    // The thread that decrements the reference count to zero must
                    // perform an acquire to ensure that it doesn't start destructing
                    // the object until all previous writes have drained.
                    std::atomic_thread_fence(std::memory_order_acquire);

                    delete p;
                }
            }

            hpx::util::atomic_count reference_count{1};
            Mutex mtx_;
        };
    }    // namespace detail

    // This hook can be inserted into the derivation chain of any component
    // allowing to automatically lock all action invocations for any instance
    // of the given component.
    HPX_CXX_EXPORT template <typename BaseComponent,
        typename Mutex = hpx::spinlock>
    struct locking_hook : BaseComponent
    {
    private:
        using mutex_type = detail::refcounted_mutex<Mutex>;
        using base_type = BaseComponent;
        using this_component_type = typename base_type::this_component_type;

    public:
        locking_hook()
          : mtx_(new mutex_type(), false)
        {
        }

        template <typename T, typename... Ts,
            typename Enable = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, locking_hook>>>
        explicit locking_hook(T&& t, Ts&&... ts)
          : base_type(HPX_FORWARD(T, t), HPX_FORWARD(Ts, ts)...)
          , mtx_(new mutex_type(), false)
        {
        }

        locking_hook(locking_hook const& rhs) = default;
        locking_hook(locking_hook&& rhs) = default;

        locking_hook& operator=(locking_hook const& rhs) = default;
        locking_hook& operator=(locking_hook&& rhs) = default;

        ~locking_hook() = default;

        using decorates_action = void;

        // This is the hook implementation for decorate_action which locks the
        // component ensuring that only one action is executed at a time for
        // this component instance.
        template <typename F>
        static threads::thread_function_type decorate_action(
            naming::address_type lva, F&& f)
        {
            return util::one_shot(
                hpx::bind_front(&locking_hook::thread_function,
                    get_lva<this_component_type>::call(lva),
                    traits::component_decorate_function<base_type>::call(
                        lva, HPX_FORWARD(F, f))));
        }

    protected:
        using yield_decorator_type = hpx::function<threads::thread_arg_type(
            threads::thread_result_type)>;

        struct decorate_wrapper
        {
            template <typename F,
                typename Enable = std::enable_if_t<
                    !std::is_same_v<std::decay_t<F>, decorate_wrapper>>>
            // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
            explicit decorate_wrapper(F&& f)
            {
                threads::get_self().decorate_yield(HPX_FORWARD(F, f));
            }

            decorate_wrapper(decorate_wrapper const&) = delete;
            decorate_wrapper(decorate_wrapper&&) = delete;
            decorate_wrapper& operator=(decorate_wrapper const&) = delete;
            decorate_wrapper& operator=(decorate_wrapper&&) = delete;

            ~decorate_wrapper()
            {
                threads::get_self().undecorate_yield();
            }
        };

        // Execute the wrapped action. This locks the mutex ensuring a thread
        // safe action invocation.
        threads::thread_result_type thread_function(
            threads::thread_function_type const& f,
            threads::thread_arg_type state)
        {
            threads::thread_result_type result;

            // now lock the mutex and execute the action
            auto mtx(mtx_);    // keep alive
            std::unique_lock l(mtx->mtx_);

            // We can safely ignore this lock while checking as it is guaranteed
            // to be unlocked before the thread is suspended.
            //
            // If this lock is not ignored it will cause false positives as the
            // check for held locks is performed before this lock is unlocked.
            util::ignore_while_checking il(&l);
            HPX_UNUSED(il);

            {
                // register our yield decorator
                decorate_wrapper const yield_decorator(
                    hpx::bind_front(&locking_hook::yield_function, this));

                result = f(state);

                (void) yield_decorator;    // silence gcc warnings
            }

            return result;
        }

        struct undecorate_wrapper
        {
            undecorate_wrapper()
              : yield_decorator_(threads::get_self().undecorate_yield())
            {
            }

            undecorate_wrapper(undecorate_wrapper const&) = delete;
            undecorate_wrapper(undecorate_wrapper&&) = delete;
            undecorate_wrapper& operator=(undecorate_wrapper const&) = delete;
            undecorate_wrapper& operator=(undecorate_wrapper&&) = delete;

            ~undecorate_wrapper()
            {
                threads::get_self().decorate_yield(HPX_MOVE(yield_decorator_));
            }

            yield_decorator_type yield_decorator_;
        };

        // The yield decorator unlocks the mutex and calls the system yield
        // which gives up control back to the thread manager.
        threads::thread_arg_type yield_function(
            threads::thread_result_type const& state)
        {
            // We undecorate the yield function as the lock handling may
            // suspend, which causes an infinite recursion otherwise.
            undecorate_wrapper const yield_decorator;
            threads::thread_arg_type result;

            {
                unlock_guard ul(mtx_->mtx_);
                result = threads::get_self().yield_impl(state);
            }

            // Re-enable ignoring the lock on the mutex above (this information
            // is lost in the lock tracking tables once a mutex is unlocked).
            util::ignore_lock(mtx_.get());

            return result;
        }

    private:
        hpx::intrusive_ptr<mutex_type> mtx_;
    };
}    // namespace hpx::components
