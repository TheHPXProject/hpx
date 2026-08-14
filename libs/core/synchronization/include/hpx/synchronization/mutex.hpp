//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::mutex, hpx::timed_mutex
/// \headerfile hpx/mutex.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/synchronization/detail/condition_variable.hpp>
#include <hpx/synchronization/spinlock.hpp>

namespace hpx::threads {

    HPX_CXX_CORE_EXPORT using thread_id_ref_type = thread_id_ref;
    HPX_CXX_CORE_EXPORT using thread_self = coroutines::detail::coroutine_self;

    /// The function \a get_self_id returns the HPX thread id of the current
    /// thread (or zero if the current thread is not a HPX thread).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_id get_self_id() noexcept;

    /// The function \a get_self_ptr returns a pointer to the (OS thread
    /// specific) self reference to the current HPX thread.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_self* get_self_ptr() noexcept;
}    // namespace hpx::threads

namespace hpx {

    ///
    /// \brief \a mutex class is a synchronization primitive that can be used
    ///        to protect shared data from being simultaneously accessed by
    ///        multiple threads.
    ///        \a mutex offers exclusive, non-recursive ownership semantics:
    ///
    ///        - A calling thread owns a mutex from the time that it
    ///          successfully calls either \a lock or \a try_lock until it
    ///          calls \a unlock.
    ///        - When a thread owns a \a mutex, all other threads will
    ///          block (for calls to \a lock) or receive a \a false return
    ///          value (for \a try_lock) if they attempt to claim ownership
    ///          of the \a mutex.
    ///        - A calling thread must not own the \a mutex prior to
    ///          calling \a lock or \a try_lock.
    ///
    ///        The behavior of a program is undefined if a \a mutex is
    ///        destroyed while still owned by any threads, or a thread
    ///        terminates while owning a \a mutex. The mutex class satisfies
    ///        all requirements of \namedrequirement{Mutex} and
    ///        \namedrequirement{StandardLayoutType}.
    ///
    ///        \a hpx::mutex is neither copyable nor movable.
    ///
    HPX_CXX_CORE_EXPORT class mutex
    {
    public:
        /// \brief \a hpx::mutex is neither copyable nor movable
        mutex(mutex const&) = delete;
        mutex(mutex&&) = delete;
        mutex& operator=(mutex const&) = delete;
        mutex& operator=(mutex&&) = delete;

    protected:
        /// \cond NOPROTECTED
        using mutex_type = hpx::spinlock;
        /// \endcond NOPROTECTED

    public:
        ///
        /// \brief Constructs the \a mutex. The \a mutex is in unlocked state
        ///        after the constructor completes.
        ///
        /// \note Because the default constructor is \a constexpr, static
        ///       mutexes are initialized as part of static non-local
        ///       initialization, before any dynamic non-local initialization
        ///       begins. This makes it safe to lock a \a mutex in a constructor
        ///       of any static object.
        ///
        /// \param description description of the \a mutex.
        ///
#if defined(HPX_HAVE_TRACING)
        HPX_CORE_EXPORT mutex(char const* const description = "");
#else
        HPX_HOST_DEVICE_CONSTEXPR mutex(char const* const = "") noexcept
          : owner_id_(threads::invalid_thread_id)
        {
        }
#endif

        ///
        /// \brief Destroys the \a mutex.
        ///        The behavior is undefined if the \a mutex is owned by any
        ///        thread or if any thread terminates while holding any
        ///        ownership of the \a mutex.
        ///
        HPX_CORE_EXPORT ~mutex();

        ///
        /// \brief Locks the mutex. If another thread has already locked the
        ///        mutex, a call to lock will block execution until the lock is
        ///        acquired.
        ///        If lock is called by a thread that already owns the mutex,
        ///        the behavior is undefined: for example, the program may
        ///        deadlock.
        ///        \a hpx::mutex can detect the invalid usage and throws
        ///        a \a std::system_error with error condition
        ///        \a resource_deadlock_would_occur instead of deadlocking.
        ///        Prior \a unlock() operations on the same mutex synchronize-
        ///        with (as defined in \a std::memory_order) this operation.
        ///
        /// \note \a lock() is usually not called directly: \a std::unique_lock,
        ///       \a std::scoped_lock, and \a std::lock_guard are used to
        ///       manage exclusive locking.
        ///
        /// \param description Description of the \a mutex
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return void \a lock returns \a void.
        ///
        HPX_CORE_EXPORT
        void lock(char const* description, error_code& ec = throws);

        ///
        /// \brief Locks the mutex. If another thread has already locked the
        ///        mutex, a call to lock will block execution until the lock is
        ///        acquired.
        ///        If lock is called by a thread that already owns the mutex,
        ///        the behavior is undefined: for example, the program may
        ///        deadlock.
        ///        \a hpx::mutex can detect the invalid usage and throws
        ///        a \a std::system_error with error condition
        ///        \a resource_deadlock_would_occur instead of deadlocking.
        ///        Prior \a unlock() operations on the same mutex synchronize -
        ///        with(as defined in \a std::memory_order) this operation.
        ///
        /// \note \a lock() is usually not called directly: \a std::unique_lock,
        ///       \a std::scoped_lock, and \a std::lock_guard are used to
        ///       manage exclusive locking.
        ///       This overload essentially calls
        ///       `void lock(char const* description, error_code& ec =throws);`
        ///       with \a description as `mutex::lock`.
        ///
        /// \param ec Used to hold error code value originated during the
        ///           operation. Defaults to \a throws -- A special 'throw on
        ///           error' \a error_code.
        ///
        /// \return void \a lock returns \a void.
        ///
        void lock(error_code& ec = throws)
        {
            return lock("mutex::lock", ec);
        }

        ///
        /// \brief Tries to lock the \a mutex. Returns immediately. On
        ///        successful lock acquisition returns \a true, otherwise
        ///        returns \a false.
        ///        This function is allowed to fail spuriously and return
        ///        \a false even if the \a mutex is not currently locked by any
        ///        other thread.
        ///        If \a try_lock is called by a thread that already owns the
        ///        \a mutex, the behavior is undefined.
        ///        Prior \a unlock() operation on the same mutex
        ///        synchronizes-with (as defined in \a std::memory_order) this
        ///        operation if it returns \a true. Note that prior \a lock()
        ///        does not synchronize with this operation if it returns
        ///        \a false.
        ///
        /// \param description Description of the \a mutex
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock returns \a true on successful lock
        ///              acquisition, otherwise returns \a false.
        ///
        HPX_CORE_EXPORT bool try_lock(
            char const* description, error_code& ec = throws);

        ///
        /// \brief Tries to lock the \a mutex. Returns immediately. On
        ///        successful lock acquisition returns \a true, otherwise
        ///        returns \a false.
        ///        This function is allowed to fail spuriously and return
        ///        \a false even if the \a mutex is not currently locked by any
        ///        other thread.
        ///        If \a try_lock is called by a thread that already owns the
        ///        \a mutex, the behavior is undefined.
        ///        Prior \a unlock() operation on the same mutex
        ///        synchronizes-with (as defined in \a std::memory_order) this
        ///        operation if it returns \a true. Note that prior \a lock()
        ///        does not synchronize with this operation if it returns
        ///        \a false.
        ///
        /// \note This overload essentially calls
        ///       \code
        ///       void try_lock(char const* description,
        ///                     error_code& ec = throws);
        ///       \endcode
        ///       with \a description as `mutex::try_lock`.
        ///
        /// \param ec Used to hold error code value originated during the
        ///           operation. Defaults to \a throws -- A special
        ///           'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock returns \a true on successful lock
        ///              acquisition, otherwise returns \a false.
        ///
        bool try_lock(error_code& ec = throws)
        {
            return try_lock("mutex::try_lock", ec);
        }

        ///
        /// \brief  Unlocks the \a mutex. The \a mutex must be locked by the
        ///         current thread of execution, otherwise, the behavior is
        ///         undefined.
        ///         This operation \a synchronizes-with (as defined in
        ///         \a std::memory_order) any subsequent \a lock operation that
        ///         obtains ownership of the same \a mutex.
        ///
        /// \param ec Used to hold error code value originated during the
        ///           operation. Defaults to \a throws -- A special
        ///           'throw on error' \a error_code.
        ///
        /// \return \a unlock returns \a void.
        ///
        HPX_CORE_EXPORT void unlock(error_code& ec = throws);

    protected:
        /// \cond NOPROTECTED
        mutable mutex_type mtx_;
        threads::thread_id_type owner_id_;
        hpx::lcos::local::detail::condition_variable cond_;
        HPX_NO_UNIQUE_ADDRESS hpx::tracing::lock_context context_;
        /// \endcond NOPROTECTED
    };

    ///
    /// \brief The \a timed_mutex class is a synchronization primitive that can
    ///        be used to protect shared data from being simultaneously
    ///        accessed by multiple threads.
    ///        In a manner similar to \a mutex, \a timed_mutex offers exclusive,
    ///        non-recursive ownership semantics. In addition, \a timed_mutex
    ///        provides the ability to attempt to claim ownership of a
    ///        \a timed_mutex with a timeout via the member functions
    ///        \a try_lock_for() and \a try_lock_until().
    ///        The \a timed_mutex class satisfies all requirements of
    ///        \namedrequirement{TimedMutex} and
    ///        \namedrequirement{StandardLayoutType}.
    ///
    ///        \a hpx::timed_mutex is neither copyable nor movable.
    ///
    HPX_CXX_CORE_EXPORT class timed_mutex : private mutex
    {
    public:
        /// \brief \a hpx::timed_mutex is neither copyable nor movable
        timed_mutex(timed_mutex const&) = delete;
        timed_mutex(timed_mutex&&) = delete;
        timed_mutex& operator=(timed_mutex const&) = delete;
        timed_mutex& operator=(timed_mutex&&) = delete;

    public:
        ///
        /// \brief Constructs a \a timed_mutex. The mutex is in unlocked state
        ///        after the call.
        ///
        /// \param description Description of the \a timed_mutex.
        ///
        HPX_CORE_EXPORT timed_mutex(char const* const description = "");

        /// \brief Destroys the \a timed_mutex. The behavior is undefined if
        ///        the mutex is owned by any thread or if any thread terminates
        ///        while holding any ownership of the mutex.
        HPX_CORE_EXPORT ~timed_mutex();

        using mutex::lock;
        using mutex::try_lock;
        using mutex::unlock;

        ///
        /// \brief Tries to lock the mutex. Blocks until specified \a abs_time
        ///        has been reached or the lock is acquired, whichever comes
        ///        first. On successful lock acquisition returns \a true,
        ///        otherwise returns \a false.
        ///        If \a abs_time has already passed, this function behaves
        ///        like \a try_lock().
        ///        As with \a try_lock(), this function is allowed to fail
        ///        spuriously and return \a false even if the mutex was not
        ///        locked by any other thread at some point before
        ///        \a abs_time.
        ///        Prior \a unlock() operation on the same mutex
        ///        \a synchronizes-with (as defined in \a std::memory_order)
        ///        this operation if it returns \a true.
        ///        If \a try_lock_until is called by a thread that already owns
        ///        the mutex, the behavior is undefined.
        ///
        /// \param abs_time    time point to block until
        /// \param description Description of the \a timed_mutex
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock_until returns \a true if the lock was
        ///              acquired successfully, otherwise \a false.
        ///
        HPX_CORE_EXPORT bool try_lock_until(
            hpx::chrono::steady_time_point const& abs_time,
            char const* description, error_code& ec = throws);

        ///
        /// \brief Tries to lock the mutex. Blocks until specified \a abs_time
        ///        has been reached or the lock is acquired, whichever comes
        ///        first. On successful lock acquisition returns \a true,
        ///        otherwise returns \a false.
        ///        If \a abs_time has already passed, this function behaves
        ///        like \a try_lock().
        ///        As with \a try_lock(), this function is allowed to fail
        ///        spuriously and return \a false even if the mutex was not
        ///        locked by any other thread at some point before
        ///        \a abs_time.
        ///        Prior \a unlock() operation on the same mutex
        ///        \a synchronizes-with (as defined in \a std::memory_order)
        ///        this operation if it returns \a true.
        ///        If \a try_lock_until is called by a thread that already owns
        ///        the mutex, the behavior is undefined.
        ///
        /// \note This overload essentially calls
        ///       \code
        ///         bool try_lock_until(
        ///           hpx::chrono::steady_time_point const& abs_time,
        ///           char const* description, error_code& ec = throws);
        ///       \endcode
        ///       with \a description as `mutex::try_lock_until`.
        ///
        /// \param abs_time    time point to block until
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock_until returns \a true if the lock was
        ///              acquired successfully, otherwise \a false.
        ///
        bool try_lock_until(hpx::chrono::steady_time_point const& abs_time,
            error_code& ec = throws)
        {
            return try_lock_until(abs_time, "mutex::try_lock_until", ec);
        }

        ///
        /// \brief Tries to lock the mutex. Blocks until specified \a rel_time
        ///        has elapsed or the lock is acquired, whichever comes first.
        ///        On successful lock acquisition returns \a true, otherwise
        ///        returns \a false.
        ///        If \a rel_time is less or equal \a rel_time.zero(), the
        ///        function behaves like \a try_lock().
        ///        This function may block for longer than \a rel_time due to
        ///        scheduling or resource contention delays.
        ///        As with \a try_lock(), this function is allowed to fail
        ///        spuriously and return \a false even if the mutex was not
        ///        locked by any other thread at some point during \a rel_time.
        ///        Prior \a unlock() operation on the same mutex
        ///        \a synchronizes-with (as defined in \a std::memory_order)
        ///        this operation if it returns \a true.
        ///        If \a try_lock_for is called by a thread that already owns
        ///        the mutex, the behavior is undefined.
        ///
        /// \param rel_time    minimum duration to block for
        /// \param description Description of the \a timed_mutex
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock_for returns \a true if the lock was
        ///              acquired successfully, otherwise \a false.
        ///
        bool try_lock_for(hpx::chrono::steady_duration const& rel_time,
            char const* description, error_code& ec = throws)
        {
            return try_lock_until(rel_time.from_now(), description, ec);
        }

        ///
        /// \brief Tries to lock the mutex. Blocks until specified \a rel_time
        ///        has elapsed or the lock is acquired, whichever comes first.
        ///        On successful lock acquisition returns \a true, otherwise
        ///        returns \a false.
        ///        If \a rel_time is less or equal \a rel_time.zero(), the
        ///        function behaves like \a try_lock().
        ///        This function may block for longer than \a rel_time due to
        ///        scheduling or resource contention delays.
        ///        As with \a try_lock(), this function is allowed to fail
        ///        spuriously and return \a false even if the mutex was not
        ///        locked by any other thread at some point during \a rel_time.
        ///        Prior \a unlock() operation on the same mutex
        ///        \a synchronizes-with (as defined in \a std::memory_order)
        ///        this operation if it returns \a true.
        ///        If \a try_lock_for is called by a thread that already owns
        ///        the mutex, the behavior is undefined.
        /// \note This overload essentially calls
        ///       \code
        ///         bool try_lock_for(
        ///             hpx::chrono::steady_duration const& rel_time,
        ///             char const* description, error_code& ec = throws)
        ///       \endcode
        ///       with \a description as `mutex::try_lock_for`.
        ///
        /// \param rel_time    minimum duration to block for
        /// \param ec          Used to hold error code value originated during
        ///                    the operation. Defaults to \a throws -- A
        ///                    special 'throw on error' \a error_code.
        ///
        /// \return bool \a try_lock_for returns \a true if the lock was
        ///              acquired successfully, otherwise \a false.
        ///
        bool try_lock_for(hpx::chrono::steady_duration const& rel_time,
            error_code& ec = throws)
        {
            return try_lock_for(rel_time, "mutex::try_lock_for", ec);
        }
    };
}    // namespace hpx
