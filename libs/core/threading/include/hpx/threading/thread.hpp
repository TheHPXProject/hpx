//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file thread.hpp
/// \page hpx::thread, hpx::this_thread::yield, hpx::this_thread::get_id, hpx::this_thread::sleep_for, hpx::this_thread::sleep_until
/// \headerfile hpx/thread.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <iosfwd>
#include <mutex>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using thread_termination_handler_type =
        hpx::function<void(std::exception_ptr const& e)>;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_thread_termination_handler(
        thread_termination_handler_type f);

    /// The class thread represents a single thread of execution. Threads allow
    /// multiple functions to execute concurrently. Threads begin execution
    /// immediately upon construction of the associated thread object (pending
    /// any OS scheduling delays), starting at the top-level function provided
    /// as a constructor argument. The return value of the top-level function is
    /// ignored and if it terminates by throwing an exception, \a hpx::terminate
    /// is called. The top-level function may communicate its return value or an
    /// exception to the caller via \a hpx::promise or by modifying shared
    /// variables (which may require synchronization, see hpx::mutex and
    /// hpx::atomic) hpx::thread objects may also be in the state that does not
    /// represent any thread (after default construction, move from, detach, or
    /// join), and a thread of execution may not be associated with any thread
    /// objects (after detach). No two hpx::thread objects may represent the
    /// same thread of execution; \a hpx::thread is not \a CopyConstructible or
    /// \a CopyAssignable, although it is \a MoveConstructible and \a
    /// MoveAssignable.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT thread
    {
        using mutex_type = hpx::spinlock;

    public:
        class id;
        using native_handle_type = threads::thread_id_type;

        thread() noexcept;

        template <typename F>
            requires(!std::same_as<std::decay_t<F>, thread>)
        explicit thread(F&& f)
        {
            auto const thrd_data = threads::get_self_id_data();
            HPX_ASSERT(thrd_data);
            start_thread(thrd_data->get_scheduler_base()->get_parent_pool(),
                util::deferred_call(HPX_FORWARD(F, f)));
        }

        template <typename F, typename... Ts>
        explicit thread(F&& f, Ts&&... vs)
        {
            auto const thrd_data = threads::get_self_id_data();
            HPX_ASSERT(thrd_data);
            start_thread(thrd_data->get_scheduler_base()->get_parent_pool(),
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...));
        }

        template <typename F>
        thread(threads::thread_pool_base* pool, F&& f)
        {
            start_thread(pool, util::deferred_call(HPX_FORWARD(F, f)));
        }

        template <typename F, typename... Ts>
        thread(threads::thread_pool_base* pool, F&& f, Ts&&... vs)
        {
            start_thread(pool,
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...));
        }

        ~thread();

    public:
        thread(thread&&) noexcept;
        thread& operator=(thread&&) noexcept;

        /// swaps two thread objects
        void swap(thread&) noexcept;

        /// Checks whether the thread is joinable, i.e. potentially running in
        /// parallel context
        bool joinable() const noexcept
        {
            std::scoped_lock<mutex_type> l(mtx_);
            return joinable_locked();
        }

        /// waits for the thread to finish its execution
        void join();

        /// permits the thread to execute independently of the thread handle
        void detach()
        {
            std::scoped_lock<mutex_type> l(mtx_);
            detach_locked();
        }

        /// returns the id of the thread
        id get_id() const noexcept;

        /// returns the underlying implementation-defined thread handle
        native_handle_type native_handle() const    //-V659
        {
            std::scoped_lock<mutex_type> l(mtx_);
            return id_.noref();
        }

        /// returns the number of concurrent threads supported by the
        /// implementation
        [[nodiscard]] static unsigned int hardware_concurrency() noexcept;

        // extensions
        void interrupt(bool flag = true) const;
        bool interruption_requested() const;

        static void interrupt(id const&, bool flag = true);

        hpx::future<void> get_future(error_code& ec = throws) const;

        std::size_t get_thread_data() const;
        std::size_t set_thread_data(std::size_t) const;

#if defined(HPX_HAVE_LIBCDS)
        std::size_t get_libcds_data() const;
        std::size_t set_libcds_data(std::size_t);
        std::size_t get_libcds_hazard_pointer_data() const;
        std::size_t set_libcds_hazard_pointer_data(std::size_t);
        std::size_t get_libcds_dynamic_hazard_pointer_data() const;
        std::size_t set_libcds_dynamic_hazard_pointer_data(std::size_t);
#endif

    private:
        bool joinable_locked() const noexcept
        {
            return threads::invalid_thread_id != id_;
        }

        void detach_locked()
        {
            id_ = threads::invalid_thread_id;
        }

        void start_thread(threads::thread_pool_base* pool,
            hpx::move_only_function<void()>&& func);

        static threads::thread_result_type thread_function_nullary(
            hpx::move_only_function<void()> const& func);

        mutable mutex_type mtx_;
        threads::thread_id_ref_type id_;
    };

    HPX_CXX_CORE_EXPORT inline void swap(thread& x, thread& y) noexcept
    {
        x.swap(y);
    }

    class thread::id
    {
    private:
        threads::thread_id_type id_;

        friend bool operator==(
            thread::id const& x, thread::id const& y) noexcept;
        friend bool operator!=(
            thread::id const& x, thread::id const& y) noexcept;
        friend bool operator<(
            thread::id const& x, thread::id const& y) noexcept;
        friend bool operator>(
            thread::id const& x, thread::id const& y) noexcept;
        friend bool operator<=(
            thread::id const& x, thread::id const& y) noexcept;
        friend bool operator>=(
            thread::id const& x, thread::id const& y) noexcept;

        template <typename Char, typename Traits>
        friend std::basic_ostream<Char, Traits>& operator<<(
            std::basic_ostream<Char, Traits>&, thread::id const&);

        friend class thread;

    public:
        id() noexcept = default;

        explicit id(threads::thread_id_type const& i) noexcept
          : id_(i)
        {
        }
        explicit id(threads::thread_id_type&& i) noexcept
          : id_(HPX_MOVE(i))
        {
        }

        explicit id(threads::thread_id_ref_type const& i) noexcept
          : id_(i.get().get())
        {
        }
        explicit id(threads::thread_id_ref_type&& i) noexcept
          : id_(HPX_MOVE(i).get().get())
        {
        }

        threads::thread_id_type const& native_handle() const noexcept
        {
            return id_;
        }
    };

    HPX_CXX_CORE_EXPORT inline bool operator==(
        thread::id const& x, thread::id const& y) noexcept
    {
        return x.id_ == y.id_;
    }

    HPX_CXX_CORE_EXPORT inline bool operator!=(
        thread::id const& x, thread::id const& y) noexcept
    {
        return !(x == y);
    }

    HPX_CXX_CORE_EXPORT inline bool operator<(
        thread::id const& x, thread::id const& y) noexcept
    {
        return x.id_ < y.id_;
    }

    HPX_CXX_CORE_EXPORT inline bool operator>(
        thread::id const& x, thread::id const& y) noexcept
    {
        return y < x;
    }

    HPX_CXX_CORE_EXPORT inline bool operator<=(
        thread::id const& x, thread::id const& y) noexcept
    {
        return !(x > y);
    }

    HPX_CXX_CORE_EXPORT inline bool operator>=(
        thread::id const& x, thread::id const& y) noexcept
    {
        return !(x < y);
    }

    HPX_CXX_CORE_EXPORT template <typename Char, typename Traits>
    std::basic_ostream<Char, Traits>& operator<<(
        std::basic_ostream<Char, Traits>& out, thread::id const& id)
    {
        out << id.id_;
        return out;
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace this_thread {

        /// \brief Returns the id of the current thread
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread::id get_id() noexcept;

        /// Provides a hint to the implementation to reschedule the execution of
        /// threads, allowing other threads to run.
        ///
        /// \note  The exact behavior of this function depends on the
        ///        implementation, in particular on the mechanics of the OS
        ///        scheduler in use and the state of the system. For example, a
        ///        first-in-first-out realtime scheduler (SCHED_FIFO in Linux)
        ///        would suspend the current thread and put it on the back of
        ///        the queue of the same-priority threads that are ready to run
        ///        (and if there are no other threads at the same priority,
        ///        yield has no effect).
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void yield() noexcept;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void yield_to(
            thread::id const&) noexcept;

        // extensions
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_priority
        get_priority() noexcept;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ptrdiff_t
        get_stack_size() noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void interruption_point();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool interruption_enabled();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool interruption_requested();

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void interrupt();

        /// Blocks the execution of the current thread until specified
        /// \a abs_time has been reached.
        ///
        /// \details It is recommended to use the clock tied to \a abs_time, in
        ///          which case adjustments of the clock may be taken into
        ///          account. Thus, the duration of the block might be more or
        ///          less than \c abs_time-Clock::now() at the time of the call,
        ///          depending on the direction of the adjustment and whether it
        ///          is honored by the implementation. The function also may
        ///          block until after \a abs_time has been reached due to
        ///          process scheduling or resource contention delays.
        /// \param abs_time absolute time to block until
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void sleep_until(
            hpx::chrono::steady_time_point const& abs_time);

        /// Blocks the execution of the current thread for at least the
        /// specified \a rel_time. This function may block for longer than \a
        /// rel_time due to scheduling or resource contention delays.
        ///
        /// \details It is recommended to use a steady clock to measure the
        ///          duration. If an implementation uses a system clock instead,
        ///          the wait time may also be sensitive to clock adjustments.
        /// \param rel_time time duration to sleep
        HPX_CXX_CORE_EXPORT inline void sleep_for(
            hpx::chrono::steady_duration const& rel_time)
        {
            sleep_until(rel_time.from_now());
        }

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_thread_data();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t set_thread_data(
            std::size_t);

#if defined(HPX_HAVE_LIBCDS)
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_libcds_data();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t set_libcds_data(
            std::size_t);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
        get_libcds_hazard_pointer_data();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
        set_libcds_hazard_pointer_data(std::size_t);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
        get_libcds_dynamic_hazard_pointer_data();
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
        set_libcds_dynamic_hazard_pointer_data(std::size_t);
#endif

        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT disable_interruption
        {
        public:
            disable_interruption(disable_interruption&&) = delete;
            disable_interruption& operator=(disable_interruption&&) = delete;
            disable_interruption(disable_interruption const&) = delete;
            disable_interruption& operator=(
                disable_interruption const&) = delete;

            bool interruption_was_enabled_;
            friend class restore_interruption;

            disable_interruption();
            ~disable_interruption();
        };

        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT restore_interruption
        {
        public:
            restore_interruption(restore_interruption&&) = delete;
            restore_interruption& operator=(restore_interruption&&) = delete;
            restore_interruption(restore_interruption const&) = delete;
            restore_interruption& operator=(
                restore_interruption const&) = delete;

            bool interruption_was_enabled_;

            explicit restore_interruption(disable_interruption const& d);
            ~restore_interruption();
        };
    }    // namespace this_thread
}    // namespace hpx

// specialize std::hash for hpx::thread::id
template <>
struct std::hash<::hpx::thread::id>
{
    std::size_t operator()(::hpx::thread::id const& id) const noexcept
    {
        constexpr std::hash<::hpx::threads::thread_id_ref_type> hasher_;
        return hasher_(id.native_handle());
    }
};

#include <hpx/config/warnings_suffix.hpp>
