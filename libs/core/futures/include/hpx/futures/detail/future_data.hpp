//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/futures/future_fwd.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/futures/traits/get_remote_result.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    HPX_CXX_CORE_EXPORT enum class future_status {
        ready,
        timeout,
        deferred,
        uninitialized
    };
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    HPX_CXX_CORE_EXPORT using run_on_completed_error_handler_type =
        hpx::function<void(std::exception_ptr const& e)>;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_run_on_completed_error_handler(
        run_on_completed_error_handler_type f);

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct future_data;

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct future_data_refcnt_base;

    HPX_CXX_CORE_EXPORT void intrusive_ptr_add_ref(
        future_data_refcnt_base* p) noexcept;
    HPX_CXX_CORE_EXPORT void intrusive_ptr_release(
        future_data_refcnt_base* p) noexcept;

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT future_data_refcnt_base
    {
        // future shared states are non-copyable and non-movable
        future_data_refcnt_base(future_data_refcnt_base const&) = delete;
        future_data_refcnt_base(future_data_refcnt_base&&) = delete;
        future_data_refcnt_base& operator=(
            future_data_refcnt_base const&) = delete;
        future_data_refcnt_base& operator=(future_data_refcnt_base&&) = delete;

    public:
        using completed_callback_type = hpx::move_only_function<void()>;
        using completed_callback_vector_type =
            hpx::detail::small_vector<completed_callback_type, 1,
                hpx::util::internal_allocator<completed_callback_type>>;

        virtual ~future_data_refcnt_base();

        virtual void set_on_completed(completed_callback_type&&) = 0;

        HPX_FORCEINLINE bool requires_delete() noexcept
        {
            return 0 == count_.decrement();
        }

        virtual void destroy() noexcept
        {
            delete this;
        }

        // This is a tag type used to convey the information that the caller is
        // _not_ going to addref the future_data instance
        struct init_no_addref
        {
        };

    protected:
        constexpr future_data_refcnt_base() noexcept
          : count_(0)
        {
        }
        constexpr explicit future_data_refcnt_base(init_no_addref) noexcept
          : count_(1)
        {
        }

        // reference counting
        friend void intrusive_ptr_add_ref(future_data_refcnt_base* p) noexcept;
        friend void intrusive_ptr_release(future_data_refcnt_base* p) noexcept;

        util::atomic_count count_;
    };

    // support functions for hpx::intrusive_ptr
    HPX_CXX_CORE_EXPORT inline void intrusive_ptr_add_ref(
        future_data_refcnt_base* p) noexcept
    {
        p->count_.increment();
    }
    HPX_CXX_CORE_EXPORT inline void intrusive_ptr_release(
        future_data_refcnt_base* p) noexcept
    {
        if (p->requires_delete())
        {
            // The thread that decrements the reference count to zero must
            // perform an acquire to ensure that it doesn't start destructing
            // the object until all previous writes have drained.
            std::atomic_thread_fence(std::memory_order_acquire);

            p->destroy();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Result>
    struct future_data_result
    {
        using type = Result;

        template <typename U>
        HPX_FORCEINLINE static constexpr U&& set(U&& u) noexcept
        {
            return HPX_FORWARD(U, u);
        }
    };

    template <typename Result>
    struct future_data_result<Result&>
    {
        using type = Result*;

        HPX_FORCEINLINE static constexpr Result* set(Result* u) noexcept
        {
            return u;
        }

        HPX_FORCEINLINE static constexpr Result* set(Result& u) noexcept
        {
            return &u;
        }
    };

    template <>
    struct future_data_result<void>
    {
        using type = util::unused_type;

        HPX_FORCEINLINE static constexpr util::unused_type set(
            util::unused_type u) noexcept
        {
            return u;
        }
    };

    template <typename Result>
    using future_data_result_t = typename future_data_result<Result>::type;

    ///////////////////////////////////////////////////////////////////////////
    template <typename R>
    struct future_data_storage
    {
        using value_type = future_data_result_t<R>;
        using error_type = std::exception_ptr;

        // determine the required alignment, define aligned storage of proper
        // size
        static constexpr std::size_t max_alignment =
            (std::alignment_of_v<value_type> >
                std::alignment_of_v<error_type>) ?
            std::alignment_of_v<value_type> :
            std::alignment_of_v<error_type>;

        static constexpr std::size_t max_size =
            (sizeof(value_type) > sizeof(error_type)) ? sizeof(value_type) :
                                                        sizeof(error_type);

        using type = hpx::aligned_storage_t<max_size, max_alignment>;
    };

    template <typename Result>
    using future_data_storage_t = typename future_data_storage<Result>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct future_data_base;

    // make sure continuation invocation does not recurse deeper than allowed
    HPX_CORE_EXPORT void handle_on_completed(
        future_data_refcnt_base::completed_callback_type&& on_completed);
    HPX_CORE_EXPORT void handle_on_completed(
        future_data_refcnt_base::completed_callback_vector_type&& on_completed);

    template <>
    struct HPX_CORE_EXPORT future_data_base<traits::detail::future_data_void>
      : future_data_refcnt_base
    {
        using mutex_type = hpx::spinlock;

        future_data_base() noexcept
          : mtx_("future_data_base")
          , state_(empty)
          , runs_child_(threads::invalid_thread_id)
        {
        }

        explicit future_data_base(init_no_addref no_addref) noexcept
          : future_data_refcnt_base(no_addref)
          , mtx_("future_data_base")
          , state_(empty)
          , runs_child_(threads::invalid_thread_id)
        {
        }

        using future_data_refcnt_base::completed_callback_type;
        using future_data_refcnt_base::completed_callback_vector_type;
        using result_type = util::unused_type;
        using init_no_addref = future_data_refcnt_base::init_no_addref;

        future_data_base(future_data_base const&) = delete;
        future_data_base(future_data_base&&) = delete;
        future_data_base& operator=(future_data_base const&) = delete;
        future_data_base& operator=(future_data_base&&) = delete;

        ~future_data_base() override;

        enum state
        {
            empty = 0,
            ready = 1,
            value = 2 | ready,
            exception = 4 | ready
        };

        // Return whether the data is available for this \a future.
        bool is_ready(
            std::memory_order order = std::memory_order_acquire) const noexcept
        {
            return (state_.load(order) & ready) != 0;
        }

        bool has_value() const noexcept
        {
            return state_.load(std::memory_order_acquire) == value;
        }

        bool has_exception() const noexcept
        {
            return state_.load(std::memory_order_acquire) == exception;
        }

        virtual void execute_deferred(error_code& /*ec*/ = throws) {}

        // cancellation is disabled by default
        virtual bool cancelable() const noexcept
        {
            return false;
        }
        virtual void cancel()
        {
            HPX_THROW_EXCEPTION(
                hpx::error::future_does_not_support_cancellation,
                "future_data_base::cancel",
                "this future does not support cancellation");
        }

        result_type* get_result_void(
            void const* storage, error_code& ec = throws);
        virtual result_type* get_result_void(error_code& ec = throws) = 0;

        virtual void set_exception(std::exception_ptr data) = 0;

        // continuation support

        // deferred execution of a given continuation
        static void run_on_completed(
            completed_callback_type&& on_completed) noexcept;
        static void run_on_completed(
            completed_callback_vector_type&& on_completed) noexcept;

        // Set the callback which needs to be invoked when the future becomes
        // ready. If the future is ready the function will be invoked
        // immediately.
        void set_on_completed(completed_callback_type&& data_sink) override;

        virtual state wait(error_code& ec = throws);

        virtual hpx::future_status wait_until(
            std::chrono::steady_clock::time_point const& abs_time,
            error_code& ec = throws);

        virtual std::exception_ptr get_exception_ptr() const = 0;

        void reserve_callbacks(std::size_t capacity)
        {
            on_completed_.reserve(capacity);
        }

    protected:
        // try to perform scoped execution of the associated thread (if any)
        bool execute_thread();

    protected:
        mutable mutex_type mtx_;
        std::atomic<state> state_;    // current state

        completed_callback_vector_type on_completed_;
        local::detail::condition_variable cond_;    // threads waiting in read
        threads::thread_id_ref_type runs_child_;
    };

    HPX_CXX_CORE_EXPORT template <typename Result>
    struct future_data_base : future_data_base<traits::detail::future_data_void>
    {
    private:
        static void construct(void* p)
        {
            hpx::construct_at(static_cast<result_type*>(p));    //-V206
        }

        template <typename T, typename... Ts>
        static void construct(void* p, T&& t, Ts&&... ts)
        {
            hpx::construct_at(static_cast<result_type*>(p),    //-V206
                future_data_result<Result>::set(HPX_FORWARD(T, t)),
                HPX_FORWARD(Ts, ts)...);
        }

    public:
        using result_type = future_data_result_t<Result>;
        using base_type = future_data_base<traits::detail::future_data_void>;
        using init_no_addref = base_type::init_no_addref;
        using completed_callback_type = base_type::completed_callback_type;
        using completed_callback_vector_type =
            base_type::completed_callback_vector_type;

    protected:
        using mutex_type = base_type::mutex_type;

    public:
        // Variable 'hpx::lcos::detail::future_data_base<void>::storage_' is
        // uninitialized.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

        future_data_base() = default;

        explicit future_data_base(init_no_addref no_addref) noexcept
          : base_type(no_addref)
        {
        }

#if defined(HPX_MSVC)
#pragma warning(push)
#endif

        template <typename... Ts>
        future_data_base(init_no_addref no_addref, std::in_place_t, Ts&&... ts)
          : base_type(no_addref)
        {
            auto* value_ptr = reinterpret_cast<result_type*>(&storage_);
            // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
            construct(value_ptr, HPX_FORWARD(Ts, ts)...);
            state_.store(value, std::memory_order_relaxed);
        }

        future_data_base(
            init_no_addref no_addref, std::exception_ptr e) noexcept
          : base_type(no_addref)
        {
            auto* exception_ptr =
                reinterpret_cast<std::exception_ptr*>(&storage_);
            hpx::construct_at(exception_ptr, HPX_MOVE(e));
            state_.store(exception, std::memory_order_relaxed);
        }

        future_data_base(future_data_base const&) = delete;
        future_data_base(future_data_base&&) = delete;
        future_data_base& operator=(future_data_base const&) = delete;
        future_data_base& operator=(future_data_base&&) = delete;

        ~future_data_base() noexcept override
        {
            reset();
        }

        /// Get the result of the requested action. This call blocks (yields
        /// control) if the result is not ready. As soon as the result has been
        /// returned and the waiting thread has been re-scheduled by the thread
        /// manager the function will return.
        ///
        /// \param ec     [in,out] this represents the error status on exit,
        ///               if this is pre-initialized to \a hpx#throws the
        ///               function will throw on error instead. If the operation
        ///               blocks and is aborted because the object went out of
        ///               scope, the code \a hpx#error#yield_aborted is set or
        ///               thrown.
        ///
        /// \note         If there has been an error reported (using the action
        ///               \a base_lco#set_exception), this function will throw
        ///                  an
        ///               exception encapsulating the reported error code and
        ///               error description if <code>&ec == &throws</code>.
        virtual result_type* get_result(error_code& ec = throws)
        {
            if (get_result_void(ec) != nullptr)
            {
                return reinterpret_cast<result_type*>(&storage_);
            }
            return nullptr;
        }

        util::unused_type* get_result_void(error_code& ec = throws) override
        {
            return base_type::get_result_void(&storage_, ec);
        }

        // Set the result of the requested action.
        template <typename... Ts>
        void set_value(Ts&&... ts)
        {
            hpx::intrusive_ptr<future_data_base> this_(this);    // keep alive

            // Note: it is safe to access the data store as no other thread
            //       should access it concurrently. There shouldn't be any
            //       threads attempting to read the value as the state is still
            //       empty. Also, there can be only one thread (this thread)
            //       attempting to set the value by definition.

            // set the data
            auto* value_ptr = reinterpret_cast<result_type*>(&storage_);
            // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
            construct(value_ptr, HPX_FORWARD(Ts, ts)...);

            // At this point the lock needs to be acquired to safely access the
            // registered continuations
            std::unique_lock<mutex_type> l(mtx_);
            [[maybe_unused]] util::ignore_while_checking<decltype(l)> il(&l);

            // handle all threads waiting for the future to become ready
            auto on_completed = HPX_MOVE(on_completed_);
            on_completed_.clear();

            // The value has been set, changing the state to 'value' at this
            // point signals to all other threads that this future is ready.
            state expected = empty;
            if (!state_.compare_exchange_strong(
                    expected, value, std::memory_order_release))
            {
                // this future should be 'empty' still (it can't be made ready
                // more than once).
                l.unlock();
                HPX_THROW_EXCEPTION(hpx::error::promise_already_satisfied,
                    "future_data_base::set_value",
                    "data has already been set for this future");
            }

            // reset runs_child_ thread id to avoid keeping the thread
            // alive as long as the future
            this->base_type::runs_child_.reset();

            // 26111: Caller failing to release lock 'this->mtx_'
            // 26115: Failing to release lock 'this->mtx_'
            // 26800: Use of a moved from object 'l'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26111 26115 26800)
#endif

            // Note: we use notify_one repeatedly instead of notify_all as we
            //       know:
            //
            //       a. that most of the time we have at most one thread
            //          waiting on the future (most futures are not shared), and
            //       b. our implementation of condition_variable::notify_one
            //          relinquishes the lock before resuming the waiting thread
            //          that avoids suspension of this thread when it tries to
            //          re-lock the mutex while exiting from
            //          condition_variable::wait
            while (
                cond_.notify_one(HPX_MOVE(l), threads::thread_priority::boost))
            {
                l = std::unique_lock<mutex_type>(mtx_);
            }

            // Note: cv.notify_one() above 'consumes' the lock 'l' and leaves
            //       it unlocked when returning.
            HPX_ASSERT_DOESNT_OWN_LOCK(l);
            il.reset_owns_registration();

            // invoke the callback (continuation) function
            if (!on_completed.empty())
            {
                handle_on_completed(HPX_MOVE(on_completed));
            }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        }

        void set_exception(std::exception_ptr data) override
        {
            hpx::intrusive_ptr<future_data_base> this_(this);    // keep alive

            // Note: it is safe to access the data store as no other thread
            //       should access it concurrently. There shouldn't be any
            //       threads attempting to read the value as the state is still
            //       empty. Also, there can be only one thread (this thread)
            //       attempting to set the value by definition.

            // set the data
            auto* exception_ptr =
                reinterpret_cast<std::exception_ptr*>(&storage_);
            hpx::construct_at(exception_ptr, HPX_MOVE(data));

            // At this point the lock needs to be acquired to safely access the
            // registered continuations
            std::unique_lock<mutex_type> l(mtx_);
            [[maybe_unused]] util::ignore_while_checking<decltype(l)> il(&l);

            // handle all threads waiting for the future to become ready
            auto on_completed = HPX_MOVE(on_completed_);
            on_completed_.clear();

            // The value has been set, changing the state to 'exception' at this
            // point signals to all other threads that this future is ready.
            state expected = empty;
            if (!state_.compare_exchange_strong(
                    expected, exception, std::memory_order_release))
            {
                // this future should be 'empty' still (it can't be made ready
                // more than once).
                l.unlock();
                HPX_THROW_EXCEPTION(hpx::error::promise_already_satisfied,
                    "future_data_base::set_exception",
                    "data has already been set for this future");
            }

            // reset runs_child_ thread id to avoid keeping the thread
            // alive as long as the future
            this->base_type::runs_child_.reset();

            // 26111: Caller failing to release lock 'this->mtx_'
            // 26115: Failing to release lock 'this->mtx_'
            // 26800: Use of a moved from object 'l'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26111 26115 26800)
#endif

            // Note: we use notify_one repeatedly instead of notify_all as we
            //       know:
            //
            //       a. that most of the time we have at most one thread
            //          waiting on the future (most futures are not shared), and
            //       b. our implementation of condition_variable::notify_one
            //          relinquishes the lock before resuming the waiting thread
            //          that avoids suspension of this thread when it tries to
            //          re-lock the mutex while exiting from
            //          condition_variable::wait
            while (
                cond_.notify_one(HPX_MOVE(l), threads::thread_priority::boost))
            {
                l = std::unique_lock<mutex_type>(mtx_);
            }

            // Note: cv.notify_one() above 'consumes' the lock 'l' and leaves
            //       it unlocked when returning.
            HPX_ASSERT_DOESNT_OWN_LOCK(l);
            il.reset_owns_registration();

            // invoke the callback (continuation) function
            if (!on_completed.empty())
            {
                handle_on_completed(HPX_MOVE(on_completed));
            }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        }

        // helper functions for setting data (if successful) or the error (if
        // non-successful)
        template <typename T>
        void set_data(T&& result)
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() { set_value(HPX_FORWARD(T, result)); },
                [&](std::exception_ptr ep) { set_exception(HPX_MOVE(ep)); });
        }

        // helper functions for setting data (if successful) or the error (if
        // non-successful)
        template <typename T>
        void set_remote_data(T&& result)
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    using naked_type = std::decay_t<T>;

                    using get_remote_result_type =
                        traits::get_remote_result<result_type, naked_type>;

                    // store the value
                    set_value(
                        get_remote_result_type::call(HPX_FORWARD(T, result)));
                },
                [&](std::exception_ptr ep) { set_exception(HPX_MOVE(ep)); });
        }

        // trigger the future with the given error condition
        void set_error(error e, char const* f, char const* msg)
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() { HPX_THROW_EXCEPTION(e, f, msg); },
                [&](std::exception_ptr ep) { set_exception(HPX_MOVE(ep)); });
        }

        // Reset the promise to allow to restart an asynchronous operation.
        // Allows any subsequent set_data operation to succeed.
        void reset(error_code& /*ec*/ = throws)
        {
            // no locking is required as semantics guarantee a single writer
            // and no reader

            // release any stored data and callback functions
            switch (state_.exchange(empty, std::memory_order_relaxed))
            {
            case value:
            {
                auto* value_ptr = reinterpret_cast<result_type*>(&storage_);
                std::destroy_at(value_ptr);
                break;
            }
            case exception:
            {
                auto* exception_ptr =
                    reinterpret_cast<std::exception_ptr*>(&storage_);
                std::destroy_at(exception_ptr);
                break;
            }
            case empty:
                [[fallthrough]];
            case ready:
                break;
            }

            on_completed_.clear();
        }

        std::exception_ptr get_exception_ptr() const override
        {
            HPX_ASSERT(state_.load(std::memory_order_acquire) == exception);
            return *reinterpret_cast<std::exception_ptr const*>(&storage_);
        }

    protected:
        using base_type::mtx_;
        using base_type::on_completed_;
        using base_type::state_;

    private:
        using base_type::cond_;
        future_data_storage_t<Result> storage_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Customization point to have the ability for creating distinct shared
    // states depending on the value type held.
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct future_data : future_data_base<Result>
    {
        using init_no_addref =
            typename future_data_base<Result>::init_no_addref;

        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        future_data() = default;

        explicit future_data(init_no_addref no_addref) noexcept
          : future_data_base<Result>(no_addref)
        {
        }

        template <typename... Ts>
        future_data(
            init_no_addref no_addref, std::in_place_t in_place, Ts&&... ts)
          : future_data_base<Result>(
                no_addref, in_place, HPX_FORWARD(Ts, ts)...)
        {
        }

        future_data(init_no_addref no_addref, std::exception_ptr e) noexcept
          : future_data_base<Result>(no_addref, HPX_MOVE(e))
        {
        }

        future_data(future_data const&) = delete;
        future_data(future_data&&) = delete;
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        future_data& operator=(future_data const&) = delete;
        future_data& operator=(future_data&&) = delete;

        ~future_data() noexcept override = default;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result, typename Allocator,
        typename Derived = void>
    struct future_data_allocator : future_data<Result>
    {
        using init_no_addref = typename future_data<Result>::init_no_addref;

        using allocated_type = std::conditional_t<std::is_void_v<Derived>,
            future_data_allocator, Derived>;

        using other_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<allocated_type>;

        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        explicit future_data_allocator(other_allocator const& alloc) noexcept
          : future_data<Result>()
          , alloc_(alloc)
        {
        }

        future_data_allocator(
            init_no_addref no_addref, other_allocator const& alloc) noexcept
          : future_data<Result>(no_addref)
          , alloc_(alloc)
        {
        }

        template <typename... T>
        future_data_allocator(init_no_addref no_addref,
            other_allocator const& alloc, std::in_place_t in_place, T&&... ts)
          : future_data<Result>(no_addref, in_place, HPX_FORWARD(T, ts)...)
          , alloc_(alloc)
        {
        }

        // clang-format off
        future_data_allocator(init_no_addref no_addref, std::exception_ptr e,
            other_allocator const& alloc) noexcept(noexcept(
                std::is_nothrow_copy_constructible_v<other_allocator>))
          : future_data<Result>(no_addref, HPX_MOVE(e))
          , alloc_(alloc)
        {
        }
        // clang-format on

        // NOLINTEND(bugprone-crtp-constructor-accessibility)

    protected:
        void destroy() noexcept override
        {
            using traits = std::allocator_traits<other_allocator>;

            other_allocator alloc(alloc_);
            traits::destroy(alloc, static_cast<allocated_type*>(this));
            traits::deallocate(alloc, static_cast<allocated_type*>(this), 1);
        }

    private:
        other_allocator alloc_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct timed_future_data : future_data<Result>
    {
    public:
        using base_type = future_data<Result>;
        using result_type = typename base_type::result_type;
        using init_no_addref = typename base_type::init_no_addref;

    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        timed_future_data() = default;

        template <typename Result_>
        timed_future_data(init_no_addref no_addref,
            std::chrono::steady_clock::time_point const& abs_time,
            Result_&& init)
          : base_type(no_addref)
        {
            hpx::intrusive_ptr<timed_future_data> this_(this);

            error_code ec;
            threads::thread_init_data data(
                threads::make_thread_function_nullary(
                    [this_, init = HPX_FORWARD(Result_, init)]() mutable {
                        this_->set_value(HPX_MOVE(init));
                    }),
                "timed_future_data<Result>::timed_future_data",
                threads::thread_priority::boost,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::current,
                threads::thread_schedule_state::suspended, true);
            threads::thread_id_ref_type const id =
                threads::register_thread(data, ec);
            if (ec)
            {
                // thread creation failed, report error to the new future
                this->base_type::set_exception(
                    hpx::detail::access_exception(ec));
                return;
            }

            // start new thread at given point in time
            threads::set_thread_state(id.noref(), abs_time,
                threads::thread_schedule_state::pending,
                threads::thread_restart_state::timeout,
                threads::thread_priority::boost, true, ec);
            if (ec)
            {
                // thread scheduling failed, report error to the new future
                this->base_type::set_exception(
                    hpx::detail::access_exception(ec));
            }
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct task_base : future_data<Result>
    {
    protected:
        using base_type = future_data<Result>;
        using future_base_type = hpx::intrusive_ptr<task_base>;
        using result_type = typename future_data<Result>::result_type;
        using init_no_addref = typename base_type::init_no_addref;

        using mutex_type = typename base_type::mutex_type;
        using base_type::mtx_;

    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        task_base()
          : base_type()
          , started_(false)
        {
        }

        explicit task_base(init_no_addref no_addref) noexcept
          : base_type(no_addref)
          , started_(false)
        {
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        void execute_deferred(error_code& /*ec*/ = throws) override
        {
            if (!started_test_and_set())
            {
                this->do_run();
            }

            // attempt to directly execute thread
            this->execute_thread();
        }

        // retrieving the value
        util::unused_type* get_result_void(error_code& ec = throws) override
        {
            if (!started_test_and_set())
            {
                this->do_run();
            }

            // attempt to directly execute thread
            this->execute_thread();
            return this->base_type::get_result_void(ec);
        }

        // wait support
        typename base_type::state wait(error_code& ec = throws) override
        {
            if (!started_test_and_set())
            {
                this->do_run();
            }

            // attempt to directly execute thread
            this->execute_thread();
            return this->base_type::wait(ec);
        }

        hpx::future_status wait_until(
            std::chrono::steady_clock::time_point const& abs_time,
            error_code& ec = throws) override
        {
            if (!started_test_and_set())
            {
                this->do_run();
            }

            // attempt to directly execute thread
            this->execute_thread();
            return this->base_type::wait_until(abs_time, ec);
        }

    protected:
        bool started_test() const noexcept
        {
            return started_.load(std::memory_order_acquire);
        }

        // returns whether this task was started before
        bool started_test_and_set()
        {
            if (!started_.load(std::memory_order_relaxed))
            {
                return started_.exchange(true, std::memory_order_release);
            }
            return true;
        }

        void check_started()
        {
            if (started_test_and_set())
            {
                HPX_THROW_EXCEPTION(hpx::error::task_already_started,
                    "task_base::check_started",
                    "this task has already been started");
            }
        }

    public:
        // run synchronously
        void run()
        {
            check_started();
            this->do_run();    // always on this thread
        }

        // run in a separate thread
        virtual threads::thread_id_ref_type post(
            threads::thread_pool_base* /*pool*/, char const* /*annotation*/,
            launch /*policy*/, error_code& /*ec*/)
        {
            HPX_ASSERT(false);    // shouldn't ever be called
            return threads::invalid_thread_id;
        }

    protected:
        static void run_impl(future_base_type this_)
        {
            this_->do_run();
        }

    public:
        template <typename T>
        void set_data(T&& result)
        {
            this->future_data<Result>::set_data(HPX_FORWARD(T, result));
        }

        void set_exception(std::exception_ptr e) override
        {
            this->future_data<Result>::set_exception(HPX_MOVE(e));
        }

        virtual void do_run()
        {
            HPX_ASSERT(false);    // shouldn't ever be called
        }

    protected:
        std::atomic<bool> started_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct cancelable_task_base : task_base<Result>
    {
    protected:
        using base_type = task_base<Result>;
        using future_base_type = hpx::intrusive_ptr<cancelable_task_base>;
        using result_type = typename future_data<Result>::result_type;
        using init_no_addref = typename task_base<Result>::init_no_addref;

        using mutex_type = typename base_type::mutex_type;
        using base_type::mtx_;

    protected:
        threads::thread_id_type get_thread_id() const noexcept
        {
            std::lock_guard<mutex_type> l(mtx_);
            return id_;
        }
        void set_thread_id(threads::thread_id_type id) noexcept
        {
            std::lock_guard<mutex_type> l(mtx_);
            id_ = HPX_MOVE(id);
        }

    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        cancelable_task_base() noexcept
          : id_(threads::invalid_thread_id)
        {
        }

        explicit cancelable_task_base(init_no_addref no_addref) noexcept
          : task_base<Result>(no_addref)
          , id_(threads::invalid_thread_id)
        {
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

    private:
        struct reset_id
        {
            explicit reset_id(cancelable_task_base& target)
              : target_(target)
            {
                target.set_thread_id(threads::get_self_id());
            }

            ~reset_id()
            {
                target_.set_thread_id(threads::invalid_thread_id);
            }

            reset_id(reset_id const&) = delete;
            reset_id(reset_id&&) = delete;
            reset_id& operator=(reset_id const&) = delete;
            reset_id& operator=(reset_id&&) = delete;

            cancelable_task_base& target_;
        };

    protected:
        static void run_impl(future_base_type this_)
        {
            reset_id r(*this_);
            this_->do_run();
        }

    public:
        // cancellation support
        bool cancelable() const noexcept override
        {
            return true;
        }

        void cancel() override
        {
            hpx::intrusive_ptr<cancelable_task_base> this_(
                this);    // keep alive

            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if (!this->started_test_and_set())
                    {
                        HPX_THROW_THREAD_INTERRUPTED_EXCEPTION();
                    }

                    if (this->is_ready())
                    {
                        return;    // nothing we can do
                    }

                    std::unique_lock<mutex_type> l(mtx_);
                    if (id_ != threads::invalid_thread_id)
                    {
                        // interrupt the executing thread
                        threads::interrupt_thread(id_);

                        l.unlock();
                        this->set_error(hpx::error::future_cancelled,
                            "task_base<Result>::cancel",
                            "future has been canceled");
                    }
                    else
                    {
                        l.unlock();
                        HPX_THROW_EXCEPTION(
                            hpx::error::future_can_not_be_cancelled,
                            "task_base<Result>::cancel",
                            "future can't be canceled at this time");
                    }
                },
                [&](std::exception_ptr ep) {
                    HPX_ASSERT(this->started_test());
                    this->set_exception(ep);
                    std::rethrow_exception(HPX_MOVE(ep));
                });
        }

    protected:
        threads::thread_id_type id_;
    };
}    // namespace hpx::lcos::detail

template <typename R, typename Allocator>
struct hpx::traits::detail::shared_state_allocator<
    hpx::lcos::detail::future_data<R>, Allocator>
{
    using type = hpx::lcos::detail::future_data_allocator<R, Allocator>;
};

#include <hpx/config/warnings_suffix.hpp>
