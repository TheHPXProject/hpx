//  Copyright (c) 2015-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/futures/detail/execute_thread.hpp>
#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/futures_factory.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <exception>
#include <mutex>
#include <utility>

namespace hpx::lcos::detail {

    namespace {
        run_on_completed_error_handler_type run_on_completed_error_handler;
    }

    void set_run_on_completed_error_handler(
        run_on_completed_error_handler_type f)
    {
        run_on_completed_error_handler = HPX_MOVE(f);
    }

    future_data_refcnt_base::~future_data_refcnt_base() = default;

    ///////////////////////////////////////////////////////////////////////////
    struct handle_continuation_recursion_count
    {
        handle_continuation_recursion_count() = default;

        std::size_t increment()
        {
            HPX_ASSERT(count_ == nullptr);
            count_ = &threads::get_continuation_recursion_count();
            return ++*count_;
        }

        handle_continuation_recursion_count(
            handle_continuation_recursion_count const&) = delete;
        handle_continuation_recursion_count(
            handle_continuation_recursion_count&&) = delete;
        handle_continuation_recursion_count& operator=(
            handle_continuation_recursion_count const&) = delete;
        handle_continuation_recursion_count& operator=(
            handle_continuation_recursion_count&&) = delete;

        ~handle_continuation_recursion_count()
        {
            if (count_ != nullptr)
            {
                --*count_;
            }
        }

    private:
        std::size_t* count_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Callback>
    void run_on_completed_on_new_thread(Callback&& f)
    {
        lcos::local::futures_factory<void()> p(HPX_FORWARD(Callback, f));

        HPX_ASSERT(nullptr != hpx::threads::get_self_ptr());
        hpx::launch policy = launch::fork;

        policy.set_priority(threads::thread_priority::boost);
        policy.set_stacksize(threads::thread_stacksize::current);

        // launch a new thread executing the given function
        threads::thread_id_ref_type const tid =    //-V821
            p.post("run_on_completed_on_new_thread", policy);

        // make sure this thread is executed last
        this_thread::suspend(
            threads::thread_schedule_state::pending, tid.noref());

        // wait for the task to run
        return p.get_future().get();
    }

    ///////////////////////////////////////////////////////////////////////////
    future_data_base<traits::detail::future_data_void>::~future_data_base()
    {
        if (runs_child_ != threads::invalid_thread_id)
        {
            [[maybe_unused]] auto* thrd = get_thread_id_data(runs_child_);
            LTM_(debug).format(
                "task_object::~task_object({}), description({}): "
                "destroy runs_as_child thread",
                thrd, thrd->get_description());

            runs_child_ = threads::invalid_thread_id;
        }
    }

    // try to performed scoped execution of the associated thread (if any)
    bool future_data_base<traits::detail::future_data_void>::execute_thread()
    {
        // we try to directly execute the thread exactly once
        threads::thread_id_ref_type runs_child = runs_child_;
        if (!runs_child)
        {
            return false;
        }

        auto const state = this->state_.load(std::memory_order_acquire);
        if (state != future_data_base::empty)
        {
            return false;
        }

        // this thread would block on the future
        [[maybe_unused]] auto* thrd = get_thread_id_data(runs_child);

        LTM_(debug).format("task_object::get_result_void: attempting to "
                           "directly execute child({}), description({})",
            thrd, thrd->get_description());

        if (threads::detail::execute_thread(HPX_MOVE(runs_child)))
        {
            // don't try running this twice
            runs_child_.reset();

            LTM_(debug).format("task_object::get_result_void: successfully "
                               "directly executed child({}), description({})",
                thrd, thrd->get_description());

            // thread terminated, mark as being destroyed
            HPX_ASSERT(thrd->get_state().state() ==
                threads::thread_schedule_state::deleted);

            return true;
        }

        LTM_(debug).format("task_object::get_result_void: failed to "
                           "directly execute child({}), description({})",
            thrd, thrd->get_description());

        return false;
    }

    util::unused_type*
    future_data_base<traits::detail::future_data_void>::get_result_void(
        void const* storage, error_code& ec)
    {
        // yields control if needed
        state s = wait(ec);
        if (ec)
        {
            return nullptr;
        }

        // No locking is required. Once a future has been made ready, which is a
        // postcondition of wait, either:
        //
        // - there is only one writer (future), or
        // - there are multiple readers only (shared_future, lock hurts
        //   concurrency)

        // Avoid retrieving state twice. If wait() returns 'empty' then this
        // thread was suspended, in this case we need to load it again.
        if (s == empty)
        {
            s = state_.load(std::memory_order_relaxed);
        }

        if (s == value)
        {
            static util::unused_type unused_;
            return &unused_;
        }

        if (s == empty)
        {
            // the value has already been moved out of this future
            HPX_THROWS_IF(ec, hpx::error::no_state,
                "future_data_base::get_result",
                "this future has no valid shared state");
            return nullptr;
        }

        // the thread has been re-activated by one of the actions supported by
        // this promise (see promise::set_event and promise::set_exception).
        if (s == exception)
        {
            auto const* exception_ptr =
                static_cast<std::exception_ptr const*>(storage);

            // an error has been reported in the meantime, throw or set the
            // error code
            if (&ec == &throws)
            {
                std::rethrow_exception(*exception_ptr);
                // never reached
            }

            ec = make_error_code(*exception_ptr);
        }

        return nullptr;
    }

    // deferred execution of a given continuation
    void future_data_base<traits::detail::future_data_void>::run_on_completed(
        completed_callback_type&& on_completed) noexcept
    {
        hpx::detail::try_catch_exception_ptr(
            [&]() {
                hpx::scoped_annotation annotate(on_completed);
                HPX_MOVE(on_completed)();
            },
            [&](std::exception_ptr const& ep) {
                // If the completion handler throws an exception, there's
                // nothing we can do, report the exception and terminate.
                if (run_on_completed_error_handler)
                {
                    run_on_completed_error_handler(ep);
                }
                else
                {
                    std::terminate();
                }
            });
    }

    void future_data_base<traits::detail::future_data_void>::run_on_completed(
        completed_callback_vector_type&& on_completed) noexcept
    {
        for (auto&& func : HPX_MOVE(on_completed))
        {
            run_on_completed(HPX_MOVE(func));
        }
    }

    // make sure continuation invocation does not recurse deeper than allowed
    template <typename Callback>
    void handle_on_completed_impl(Callback&& on_completed)
    {
        HPX_TRACING_MARK_EVENT("future::handle_on_completed");

        // We need to run the completion on a new thread if we are on a non HPX
        // thread.
        bool const is_hpx_thread = nullptr != hpx::threads::get_self_ptr();
        bool recurse_asynchronously = false;

        handle_continuation_recursion_count cnt;
        if (is_hpx_thread)
        {
#if defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
            recurse_asynchronously = !this_thread::has_sufficient_stack_space();
#else
            recurse_asynchronously =
                cnt.increment() > HPX_CONTINUATION_MAX_RECURSION_DEPTH;
#endif
        }

        using future_data_base =
            future_data_base<traits::detail::future_data_void>;

        if (!is_hpx_thread || !recurse_asynchronously)
        {
            // directly execute continuation on this thread
            future_data_base::run_on_completed(
                HPX_FORWARD(Callback, on_completed));
        }
        else
        {
            // re-spawn continuation on a new thread
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    // clang-format off
                    constexpr void (*p)(std::decay_t<Callback>&&) noexcept =
                        &future_data_base::run_on_completed;
                    // clang-format on
                    run_on_completed_on_new_thread(util::deferred_call(
                        p, HPX_FORWARD(Callback, on_completed)));
                },
                [&](std::exception_ptr const& ep) {
                    // If an exception while creating the new task or inside the
                    // completion handler is thrown, there is nothing we can do...
                    // ... but terminate and report the error
                    if (run_on_completed_error_handler)
                    {
                        run_on_completed_error_handler(ep);
                    }
                    else
                    {
                        std::rethrow_exception(ep);
                    }
                });
        }
    }

    void handle_on_completed(
        future_data_refcnt_base::completed_callback_type&& on_completed)
    {
        handle_on_completed_impl(HPX_MOVE(on_completed));
    }

    void handle_on_completed(
        future_data_refcnt_base::completed_callback_vector_type&& on_completed)
    {
        handle_on_completed_impl(HPX_MOVE(on_completed));
    }

    // Set the callback which needs to be invoked when the future becomes ready.
    // If the future is ready the function will be invoked immediately.
    void future_data_base<traits::detail::future_data_void>::set_on_completed(
        completed_callback_type&& data_sink)
    {
        if (!data_sink)
            return;

        hpx::intrusive_ptr<future_data_base> this_(this);    // keep alive
        if (is_ready(std::memory_order_relaxed))
        {
            // invoke the callback (continuation) function right away
            handle_on_completed_impl(HPX_MOVE(data_sink));
        }
        else
        {
            std::unique_lock l(mtx_);
            if (is_ready())
            {
                l.unlock();

                // invoke the callback (continuation) function
                handle_on_completed_impl(HPX_MOVE(data_sink));
            }
            else
            {
                on_completed_.push_back(HPX_MOVE(data_sink));
            }
        }
    }

    future_data_base<traits::detail::future_data_void>::state
    future_data_base<traits::detail::future_data_void>::wait(error_code& ec)
    {
        // block if this entry is empty
        state s = state_.load(std::memory_order_acquire);
        if (s == empty)
        {
            hpx::intrusive_ptr<future_data_base> this_(this);    // keep alive

            std::unique_lock l(mtx_);
            s = state_.load(std::memory_order_relaxed);
            if (s == empty)
            {
                cond_.wait(l, "future_data_base::wait", ec);
                if (ec)
                {
                    return s;
                }

                // reload the state, it's not empty anymore
                s = state_.load(std::memory_order_relaxed);
            }
        }

        if (&ec != &throws)
        {
            ec = make_success_code();
        }
        return s;
    }

    hpx::future_status
    future_data_base<traits::detail::future_data_void>::wait_until(
        std::chrono::steady_clock::time_point const& abs_time, error_code& ec)
    {
        // block if this entry is empty
        if (state_.load(std::memory_order_acquire) == empty)
        {
            hpx::intrusive_ptr<future_data_base> this_(this);    // keep alive

            std::unique_lock l(mtx_);
            if (state_.load(std::memory_order_relaxed) == empty)
            {
                // stop waiting if the future becomes ready
                hpx::move_only_function<bool()> wait_cond = [this]() {
                    return state_.load(std::memory_order_acquire) != empty;
                };

                threads::thread_restart_state const reason =
                    cond_.wait_until(l, abs_time, HPX_MOVE(wait_cond),
                        "future_data_base::wait_until", ec);
                if (ec)
                {
                    return hpx::future_status::uninitialized;
                }

                if (reason == threads::thread_restart_state::timeout &&
                    state_.load(std::memory_order_acquire) == empty)
                {
                    return hpx::future_status::timeout;
                }
            }
        }

        if (&ec != &throws)
        {
            ec = make_success_code();
        }
        return hpx::future_status::ready;    //-V110
    }
}    // namespace hpx::lcos::detail
