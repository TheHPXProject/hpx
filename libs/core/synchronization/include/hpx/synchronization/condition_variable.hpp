//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file condition_variable.hpp
/// \page hpx::condition_variable, hpx::condition_variable_any, hpx::cv_status
/// \headerfile hpx/condition_variable.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/synchronization/detail/condition_variable.hpp>
#include <hpx/synchronization/mutex.hpp>
#include <hpx/synchronization/spinlock.hpp>
#include <hpx/synchronization/stop_token.hpp>

#include <mutex>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///
    /// \brief The scoped enumeration \a hpx::cv_status describes whether a
    /// timed wait returned because of timeout or not.
    /// \a hpx::cv_status is used by the \a wait_for and \a wait_until member
    /// functions of \a hpx::condition_variable and
    /// \a hpx::condition_variable_any.
    ///
    HPX_CXX_CORE_EXPORT enum class cv_status {
        /// The condition variable was awakened with \a notify_all,
        /// \a notify_one, or spuriously
        no_timeout,

        /// the condition variable was awakened by timeout expiration
        timeout,

        /// there was an error
        error
    };

    ///
    /// \brief The \a condition_variable class is a synchronization primitive that
    /// can be used to block a thread, or multiple threads at the same time,
    /// until another thread both modifies a shared variable (the condition),
    /// and notifies the \a condition_variable.
    ///
    /// The thread that intends to modify the shared variable has to
    ///      1. acquire a \a hpx::mutex (typically via \a std::lock_guard)
    ///      2. perform the modification while the lock is held
    ///      3. execute \a notify_one or \a notify_all on the
    ///         condition_variable (the lock does not need to be held for
    ///         notification)
    ///
    /// Even if the shared variable is atomic, it must be modified under the mutex
    /// in order to correctly publish the modification to the waiting thread.
    /// Any thread that intends to wait on \a condition_variable has to
    ///      1. acquire a \a std::unique_lock<hpx::mutex>, on the same mutex as
    ///         used to protect the shared variable
    ///      2. either
    ///          1. check the condition, in case it was already updated and
    ///             notified
    ///          2. execute \a wait, \a await_for, or \a wait_until. The wait
    ///             operations atomically release the mutex and suspend the
    ///             execution
    ///             of the thread.
    ///          3. When the condition variable is notified, a timeout expires, or
    ///             a spurious wakeup occurs, the thread is awakened, and the
    ///             mutex is atomically reacquired. The thread should then check
    ///             the condition and resume waiting if the wake-up was spurious.
    ///          or
    ///          1. use the predicated overload of \a wait, \a wait_for, and
    ///          \a wait_until, which takes care of the three steps above.
    ///
    /// \a hpx::condition_variable works only with \a std::unique_lock<hpx::mutex>.
    /// This restriction allows for maximal efficiency on some platforms.
    /// \a hpx::condition_variable_any provides a condition variable that works
    /// with any \namedrequirement{BasicLockable} object, such as
    /// \a std::shared_lock.
    ///
    /// Condition variables permit concurrent invocation of the \a wait,
    /// \a wait_for, \a wait_until, \a notify_one and \a notify_all member
    /// functions.
    ///
    /// The class \a hpx::condition_variable is a
    /// \namedrequirement{StandardLayoutType}.
    /// It is not \namedrequirement{CopyConstructible},
    /// \namedrequirement{MoveConstructible},
    /// \namedrequirement{CopyAssignable}, or
    /// \namedrequirement{MoveAssignable}.
    ///
    HPX_CXX_CORE_EXPORT class condition_variable
    {
    private:
        using mutex_type =
            lcos::local::detail::condition_variable_data::mutex_type;
        using data_type =
            hpx::intrusive_ptr<lcos::local::detail::condition_variable_data>;

    public:
        /// \brief Construct an object of type \a hpx::condition_variable.
        condition_variable()
          : data_(data_type(
                new lcos::local::detail::condition_variable_data, false))
        {
        }

        ///
        /// \brief Destroys the object of type \a hpx::condition_variable.
        ///
        /// \note Preconditions: There is no thread blocked on *this. [Note:
        ///                      That is, all threads have been notified; they
        ///                      could subsequently block on the lock specified
        ///                      in the wait.This relaxes the usual rules,
        ///                      which would have required all wait calls to
        ///                      happen before destruction. Only the
        ///                      notification to unblock the wait needs to
        ///                      happen before destruction.The user should take
        ///                      care to ensure that no threads wait on *this
        ///                      once the destructor has been started,
        ///                      especially when the waiting threads are calling
        ///                      the \a wait functions in a loop or using the
        ///                      overloads of \a wait, \a wait_for, or
        ///                      \a wait_until that take a predicate. end note]
        ///
        /// IOW, \a ~condition_variable() can execute before a signaled thread
        /// returns from a wait. If this happens with \a condition_variable,
        /// that waiting thread will attempt to lock the destructed mutex.
        /// To fix this, there must be shared ownership of the data members
        /// between the \a condition_variable object and the member
        /// functions \a wait (\a wait_for, etc.).
        ///
        ~condition_variable() = default;

        condition_variable(condition_variable const&) = delete;
        condition_variable(condition_variable&&) = delete;
        condition_variable& operator=(condition_variable const&) = delete;
        condition_variable& operator=(condition_variable&&) = delete;

        ///
        /// \brief If any threads are waiting on \a *this, calling \a notify_one
        /// unblocks one of the waiting threads.
        ///
        /// \param ec Used to hold error code value originated during the
        /// operation. Defaults to \a throws -- A special 'throw on error'
        /// \a error_code.
        ///
        /// \returns \a notify_one returns \a void.
        ///
        void notify_one(error_code& ec = throws) const
        {
            std::unique_lock<mutex_type> l(data_->mtx_);
            data_->cond_.notify_one(HPX_MOVE(l), ec);
        }

        ///
        /// \brief Unblocks all threads currently waiting for \a *this.
        ///
        /// \param ec Used to hold error code value originated during the
        /// operation. Defaults to \a throws -- A special 'throw on error'
        /// \a error_code.
        ///
        /// \returns \a notify_all returns \a void.
        ///
        void notify_all(error_code& ec = throws) const
        {
            std::unique_lock<mutex_type> l(data_->mtx_);
            data_->cond_.notify_all(HPX_MOVE(l), ec);
        }

        ///
        /// \brief \a wait causes the current thread to block until the
        /// condition variable is notified or a spurious wakeup occurs,
        /// optionally looping until some predicate is satisfied
        /// \a (bool(pred())==true).
        ///
        /// Atomically unlocks lock, blocks the current executing thread, and
        /// adds it to the list of threads waiting on \a *this. The thread will
        /// be unblocked when \a notify_all() or \a notify_one() is executed.
        /// It may also be unblocked spuriously. When unblocked, regardless of
        /// the reason, lock is reacquired and wait exits.
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///
        /// \tparam Mutex Type of mutex to wait on.
        ///
        /// \param lock \a unique_lock that must be locked by the current thread
        /// \param ec   Used to hold error code value originated during the
        ///             operation. Defaults to \a throws -- A special 'throw on
        ///             error' \a error_code.
        ///
        /// \returns \a wait returns \a void.
        ///
        template <typename Mutex>
        void wait(std::unique_lock<Mutex>& lock, error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            auto const data = data_;    // keep data alive

            [[maybe_unused]] util::ignore_all_while_checking const ignore_lock;

            std::unique_lock<mutex_type> l(data->mtx_);
            unlock_guard<std::unique_lock<Mutex>> unlock(lock);

            // The following ensures that the inner lock will be unlocked
            // before the outer to avoid deadlock (fixes issue #3608)
            std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                l, std::adopt_lock);

            data->cond_.wait(l, ec);
        }

        ///
        /// \brief \a wait causes the current thread to block until the
        /// condition variable is notified or a spurious wakeup occurs,
        /// optionally looping until some predicate is satisfied
        /// \a (bool(pred())==true).
        ///
        /// Equivalent to
        /// \code
        ///      while (!pred()) {
        ///          wait(lock);
        ///      }
        /// \endcode
        ///
        /// This overload may be used to ignore spurious awakenings while
        /// waiting for a specific condition to become true. Note that lock must
        /// be acquired before entering this method, and it is reacquired after
        /// \a wait(lock) exits, which means that lock can be used to guard
        /// access to \a pred().
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///
        /// \tparam Mutex     Type of mutex to wait on.
        /// \tparam Predicate Type of predicate \a pred function.
        ///
        /// \param lock \a unique_lock that must be locked by the current
        ///             thread
        /// \param pred Predicate which returns \a false if the waiting should
        ///             be continued \a (bool(pred())==false). The
        ///             signature of the predicate function should be equivalent
        ///             to the following: `bool pred();`
        ///
        /// \returns \a wait returns \a void.
        ///
        template <typename Mutex, typename Predicate>
        void wait(std::unique_lock<Mutex>& lock, Predicate pred,
            error_code& /*ec*/ = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            while (!pred())
            {
                wait(lock);
            }
        }

        ///
        /// \brief \a wait_until causes the current thread to block until the
        /// condition variable is notified, a specific time is reached, or a
        /// spurious wakeup occurs, optionally looping until some predicate is
        /// satisfied \a (bool(pred())==true).
        ///
        /// Atomically releases lock, blocks the current executing thread, and
        /// adds it to the list of threads waiting on \a *this. The thread will
        /// be unblocked when \a notify_all() or \a notify_one() is executed, or
        /// when the absolute time point \a abs_time is reached. It may also
        /// be unblocked spuriously. When unblocked, regardless of the reason,
        /// lock is reacquired and \a wait_until exits.
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///
        /// \tparam Mutex Type of mutex to wait on.
        ///
        /// \param lock      \a unique_lock that must be locked by the current
        ///                  thread
        /// \param abs_time  Represents the time when waiting should be stopped
        /// \param ec        Used to hold error code value originated during the
        ///                  operation. Defaults to \a throws -- A special
        ///                  'throw on error' \a error_code.
        ///
        /// \returns cv_status \a wait_until returns \a hpx::cv_status::timeout
        ///                    if the absolute timeout specified by
        ///                    \a abs_time was reached and
        ///                    \a hpx::cv_status::no_timeout otherwise.
        ///
        template <typename Mutex>
        cv_status wait_until(std::unique_lock<Mutex>& lock,
            hpx::chrono::steady_time_point const& abs_time,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            auto const data = data_;    // keep data alive

            [[maybe_unused]] util::ignore_all_while_checking const ignore_lock;

            std::unique_lock<mutex_type> l(data->mtx_);
            unlock_guard<std::unique_lock<Mutex>> unlock(lock);

            // The following ensures that the inner lock will be unlocked
            // before the outer to avoid deadlock (fixes issue #3608)
            std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                l, std::adopt_lock);

            threads::thread_restart_state const reason =
                data->cond_.wait_until(l, abs_time, ec);

            if (ec)
                return cv_status::error;

            // if the timer has hit, the waiting period timed out
            return (reason ==
                       threads::thread_restart_state::timeout) ?    //-V110
                cv_status::timeout :
                cv_status::no_timeout;
        }

    protected:
        template <typename Mutex, typename Predicate>
        cv_status wait_until_pred(std::unique_lock<Mutex>& lock,
            hpx::chrono::steady_time_point const& abs_time, Predicate wait_cond,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            auto const data = data_;    // keep data alive

            [[maybe_unused]] util::ignore_all_while_checking const ignore_lock;

            std::unique_lock<mutex_type> l(data->mtx_);
            unlock_guard<std::unique_lock<Mutex>> unlock(lock);

            // The following ensures that the inner lock will be unlocked
            // before the outer to avoid deadlock (fixes issue #3608)
            std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                l, std::adopt_lock);

            threads::thread_restart_state const reason = data->cond_.wait_until(
                l, abs_time,
                [&, pred = HPX_MOVE(wait_cond)]() {
                    // re-acquire the outer lock to make sure that the
                    // user-supplied predicate is protected
                    relock_guard<std::unique_lock<Mutex>> relock(unlock);
                    return pred();
                },
                "condition_variable::wait_until_pred", ec);

            if (ec)
                return cv_status::error;

            // if the timer has hit, the waiting period timed out
            return (reason ==
                       threads::thread_restart_state::timeout) ?    //-V110
                cv_status::timeout :
                cv_status::no_timeout;
        }

    public:
        ///
        /// \brief \a wait_until causes the current thread to block until the
        /// condition variable is notified, a specific time is reached, or a
        /// spurious wakeup occurs, optionally looping until some predicate is
        /// satisfied \a (bool(pred())==true).
        ///
        /// Equivalent to
        /// \code
        ///      while (!pred()) {
        ///          if (wait_until(lock, abs_time) == hpx::cv_status::timeout) {
        ///              return pred();
        ///          }
        ///      }
        ///      return true;
        /// \endcode
        /// This overload may be used to ignore spurious wakeups.
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///
        /// \tparam Mutex     Type of mutex to wait on.
        /// \tparam Predicate Type of predicate \a pred function.
        ///
        /// \param lock      \a unique_lock that must be locked by the current
        ///                  thread
        /// \param abs_time  Represents the time when waiting should be stopped
        /// \param pred      Predicate which returns \a false if the waiting
        ///                  should be continued
        ///                  \a (bool(pred())==false). The signature of
        ///                  the predicate function should be equivalent to the
        ///                  following: `bool pred();`
        /// \param ec        Used to hold error code value originated during the
        ///                  operation. Defaults to \a throws -- A special
        ///                  'throw on error' \a error_code.
        ///
        /// \returns bool \a wait_until returns \a false if the predicate
        ///               \a pred still evaluates to false after
        ///               the \a abs_time timeout has expired, otherwise
        ///               \a true. If the timeout had already expired,
        ///               evaluates and returns the result of
        ///               \a pred.
        ///
        template <typename Mutex, typename Predicate>
        bool wait_until(std::unique_lock<Mutex>& lock,
            hpx::chrono::steady_time_point const& abs_time, Predicate pred,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            while (!pred())
            {
                if (wait_until_pred(lock, abs_time, pred, ec) ==
                    cv_status::timeout)
                {
                    return pred();
                }
            }
            return true;
        }

        ///
        /// \brief Atomically releases lock, blocks the current executing
        /// thread, and adds it to the list of threads waiting on \a *this. The
        /// thread will be unblocked when \a notify_all() or \a notify_one() is
        /// executed, or when the relative timeout \a rel_time expires. It may
        /// also be unblocked spuriously. When unblocked, regardless of the
        /// reason, lock is reacquired and \a wait_for() exits.
        ///
        /// The standard recommends that a steady clock be used to measure the
        /// duration. This function may block for longer than \a rel_time due
        /// to scheduling or resource contention delays.
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///       3. Even if notified under lock, this overload makes no
        ///          guarantees about the state of the associated predicate when
        ///          returning due to timeout.
        ///
        /// \tparam Mutex Type of mutex to wait on.
        ///
        /// \param lock     \a unique_lock that must be locked by the current
        ///                 thread
        /// \param rel_time represents the maximum time to spend waiting. Note
        ///                 that \a rel_time must be small enough not to
        ///                 overflow when added to
        ///                 \a hpx::chrono::steady_clock::now().
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special
        ///                 'throw on error' \a error_code.
        ///
        /// \return cv_status \a hpx::cv_status::timeout if the relative timeout
        ///                   specified by \a rel_time expired,
        ///                   \a hpx::cv_status::no_timeout otherwise.
        ///
        template <typename Mutex>
        cv_status wait_for(std::unique_lock<Mutex>& lock,
            hpx::chrono::steady_duration const& rel_time,
            error_code& ec = throws)
        {
            return wait_until(lock, rel_time.from_now(), ec);
        }

        ///
        /// \brief Equivalent to
        /// \code
        ///      return wait_until(lock,
        ///                 hpx::chrono::steady_clock::now() + rel_time,
        ///                 hpx::move(pred));
        /// \endcode
        /// This overload may be used to ignore spurious awakenings by looping
        /// until some predicate is satisfied \a (bool(pred())==true).
        ///
        /// The standard recommends that a steady clock be used to measure the
        /// duration. This function may block for longer than \a rel_time due
        /// to scheduling or resource contention delays.
        ///
        /// \note 1. Calling this function if \a lock.mutex() is not locked by
        ///          the current thread is undefined behavior.
        ///       2. Calling this function if \a lock.mutex() is not the same
        ///          mutex as the one used by all other threads that are
        ///          currently waiting on the same condition variable is
        ///          undefined behavior.
        ///
        /// \tparam Mutex     Type of mutex to wait on.
        /// \tparam Predicate Type of predicate \a pred function.
        ///
        /// \param lock     \a unique_lock that must be locked by the current
        ///                 thread
        /// \param rel_time represents the maximum time to spend waiting. Note
        ///                 that \a rel_time must be small enough not to
        ///                 overflow when added to
        ///                 \a hpx::chrono::steady_clock::now().
        /// \param pred     Predicate which returns \a false if the waiting
        ///                 should be continued
        ///                 \a (bool(pred())==false). The signature of
        ///                 the predicate function should be equivalent to the
        ///                 following: `bool pred();`
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special
        ///                 'throw on error' \a error_code.
        ///
        /// \return bool \a wait_for returns \a false if the predicate pred
        ///              still evaluates to \a false after the \a rel_time
        ///              timeout expired, otherwise \a true.
        ///
        template <typename Mutex, typename Predicate>
        bool wait_for(std::unique_lock<Mutex>& lock,
            hpx::chrono::steady_duration const& rel_time, Predicate pred,
            error_code& ec = throws)
        {
            return wait_until(lock, rel_time.from_now(), HPX_MOVE(pred), ec);
        }

    private:
        hpx::util::cache_aligned_data_derived<data_type> data_;
    };

    ///
    /// \brief The \a condition_variable_any class is a generalization of
    /// \a hpx::condition_variable. Whereas \a hpx::condition_variable works
    /// only on \a std::unique_lock<std::mutex>, \a a condition_variable_any can
    /// operate on any lock that meets the \namedrequirement{BasicLockable}
    /// requirements.
    ///
    /// See \a hpx::condition_variable for the description of the semantics of
    /// condition variables.
    /// It is not \namedrequirement{CopyConstructible},
    /// \namedrequirement{MoveConstructible},
    /// \namedrequirement{CopyAssignable}, or
    /// \namedrequirement{MoveAssignable}.
    ///
    HPX_CXX_CORE_EXPORT class condition_variable_any
    {
    private:
        using mutex_type =
            lcos::local::detail::condition_variable_data::mutex_type;
        using data_type =
            hpx::intrusive_ptr<lcos::local::detail::condition_variable_data>;

    public:
        ///
        /// \brief Constructs an object of type \a hpx::condition_variable_any
        ///
        condition_variable_any()
          : data_(data_type(
                new lcos::local::detail::condition_variable_data, false))
        {
        }

        ///
        /// \brief Destroys the object of type \a hpx::condition_variable_any.
        ///
        /// It is only safe to invoke the destructor if all threads have been
        /// notified. It is not required that they have exited their respective
        /// wait functions: some threads may still be waiting to reacquire the
        /// associated lock, or may be waiting to be scheduled to run after
        /// reacquiring it.
        ///
        /// The programmer must ensure that no threads attempt to wait on
        /// \a *this once the destructor has been started, especially when the
        /// waiting threads are calling the wait functions in a loop or are using
        /// the overloads of the wait functions that take a predicate.
        ///
        /// Preconditions: There is no thread blocked on *this. [Note: That is,
        ///                all threads have been notified; they could subsequently
        ///                block on the lock specified in the wait.This relaxes
        ///                the usual rules, which would have required all wait
        ///                calls to happen before destruction. Only the notification
        ///                to unblock the wait needs to happen before destruction.
        ///                The user should take care to ensure that no threads wait
        ///                on \a *this once the destructor has been started,
        ///                especially when the waiting threads are calling the
        ///                \a wait functions in a loop or using the overloads of
        ///                \a wait, \a wait_for, or \a wait_until that take a
        ///                predicate. end note]
        ///
        /// IOW, \a ~condition_variable_any() can execute before a signaled thread
        /// returns from a wait. If this happens with \a condition_variable_any, that
        /// waiting thread will attempt to lock the destructed mutex.
        /// To fix this, there must be shared ownership of the data members
        /// between the \a condition_variable_any object and the member functions
        /// wait (\a wait_for, etc.).
        ///
        ~condition_variable_any() = default;

        condition_variable_any(condition_variable_any const&) = delete;
        condition_variable_any(condition_variable_any&&) = delete;
        condition_variable_any& operator=(
            condition_variable_any const&) = delete;
        condition_variable_any& operator=(condition_variable_any&&) = delete;

        ///
        /// \brief If any threads are waiting on \a *this, calling \a notify_one
        /// unblocks one of the waiting threads.
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual
        ///       condition variable. This makes it impossible for
        ///       \a notify_one() to, for example, be delayed and unblock a
        ///       thread that started waiting just after the call to
        ///       \a notify_one() was made.
        ///
        ///       The notifying thread does not need to hold the lock on the
        ///       same mutex as the one held by the waiting thread(s); in fact
        ///       doing so is a pessimization, since the notified thread would
        ///       immediately block again, waiting for the notifying thread to
        ///       release the lock. However, some implementations (in
        ///       particular many implementations of pthreads) recognize this
        ///       situation and avoid this "hurry up and wait" scenario by
        ///       transferring the waiting thread from the condition variable's
        ///       queue directly to the queue of the mutex within the notify
        ///       call, without waking it up.
        ///
        ///       Notifying while under the lock may nevertheless be necessary
        ///       when precise scheduling of events is required, e.g. if the
        ///       waiting thread would exit the program if the condition is
        ///       satisfied, causing destruction of the notifying thread's
        ///       condition variable. A spurious wakeup after mutex unlock but
        ///       before notify would result in notify called on a destroyed
        ///       object.
        ///
        /// \param ec Used to hold error code value originated during the
        ///           operation. Defaults to \a throws -- A special 'throw on
        ///           error' \a error_code.
        ///
        /// \returns \a notify_one returns \a void.
        ///
        void notify_one(error_code& ec = throws) const
        {
            std::unique_lock<mutex_type> l(data_->mtx_);
            data_->cond_.notify_one(HPX_MOVE(l), ec);
        }

        ///
        /// \brief Unblocks all threads currently waiting for \a *this.
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single
        ///       total order that can be viewed as modification order of an
        ///       atomic variable: the order is specific to this individual
        ///       condition variable. This makes it impossible for
        ///       \a notify_one() to, for example, be delayed and unblock a
        ///       thread that started waiting just after the call to
        ///       \a notify_one() was made.
        ///
        ///       The notifying thread does not need to hold the lock on the
        ///       same mutex as the one held by the waiting thread(s); in fact
        ///       doing so is a pessimization, since the notified thread would
        ///       immediately block again, waiting for the notifying thread to
        ///       release the lock.
        ///
        /// \param ec Used to hold error code value originated during the
        /// operation. Defaults to \a throws -- A special 'throw on error'
        /// \a error_code.
        ///
        /// \returns \a notify_all returns \a void.
        ///
        void notify_all(error_code& ec = throws) const
        {
            std::unique_lock<mutex_type> l(data_->mtx_);
            data_->cond_.notify_all(HPX_MOVE(l), ec);
        }

        ///
        /// \brief \a wait causes the current thread to block until the
        /// condition variable is notified or a spurious wakeup occurs,
        /// optionally looping until some predicate is satisfied
        /// \a (bool(pred())==true).
        ///
        /// Atomically unlocks lock, blocks
        /// the current executing thread, and adds it to the list of threads
        /// waiting on \a *this. The thread will be unblocked when
        /// \a notify_all() or \a notify_one() is executed. It may also be
        /// unblocked spuriously. When unblocked, regardless of the reason,
        /// lock is reacquired and wait exits.
        ///
        /// \note If these functions fail to meet the postconditions (lock is
        ///       locked by the calling thread), std::terminate is called. For
        ///       example, this could happen if re-locking the mutex throws an
        ///       exception.
        ///
        ///       The effects of `notify_one()/notify_all()` and each of the
        ///       three atomic parts of `wait()/wait_for()/wait_until()`
        ///       `(unlock+wait, wakeup, and lock)` take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to `notify_one()` was made.
        ///
        /// \tparam Lock Type of \a lock.
        ///
        /// \param lock An object of type Lock that meets the
        ///             \namedrequirement{BasicLockable} requirements, which
        ///             must be locked by the current thread
        /// \param ec   Used to hold error code value originated during the
        ///             operation. Defaults to \a throws -- A special 'throw on
        ///             error' \a error_code.
        ///
        /// \returns \a wait returns \a void.
        ///
        template <typename Lock>
        void wait(Lock& lock, error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            auto const data = data_;    // keep data alive

            [[maybe_unused]] util::ignore_all_while_checking const ignore_lock;

            std::unique_lock<mutex_type> l(data->mtx_);
            unlock_guard<Lock> unlock(lock);

            // The following ensures that the inner lock will be unlocked
            // before the outer to avoid deadlock (fixes issue #3608)
            std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                l, std::adopt_lock);

            data->cond_.wait(l, ec);
        }

        ///
        /// \brief \a wait causes the current thread to block until the
        /// condition variable is notified or a spurious wakeup occurs,
        /// optionally looping until some predicate is satisfied
        /// \a (bool(pred())==true).
        ///
        /// Equivalent to
        /// \code
        ///     while (!pred()) {
        ///         wait(lock);
        ///     }
        /// \endcode
        ///
        /// This overload may be used to ignore spurious awakenings while
        /// waiting for a specific condition to become true. Note that lock must
        /// be acquired before entering this method, and it is reacquired after
        /// \a wait(lock) exits, which means that lock can be used to guard
        /// access to \a pred().
        ///
        /// \note If these functions fail to meet the postconditions (lock is
        ///       locked by the calling thread), std::terminate is called. For
        ///       example, this could happen if re-locking the mutex throws an
        ///       exception.
        ///
        ///       The effects of `notify_one()/notify_all()` and each of the
        ///       three atomic parts of `wait()/wait_for()/wait_until()`
        ///       `(unlock+wait, wakeup, and lock)` take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to `notify_one()` was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock an object of type Lock that meets the
        ///             \namedrequirement{BasicLockable} requirements, which
        ///             must be locked by the current thread
        /// \param pred predicate which returns `false` if the waiting should
        ///             be continued `(bool(pred()) == false)`.
        ///             The signature of the predicate function should be
        ///             equivalent to the following: `bool pred()`.
        ///
        /// \returns \a wait returns \a void.
        ///
        template <typename Lock, typename Predicate>
        void wait(Lock& lock, Predicate pred, error_code& /* ec */ = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            while (!pred())
            {
                wait(lock);
            }
        }

        ///
        /// \brief \a wait_until causes the current thread to block until the
        /// condition variable is notified, a specific time is reached, or a
        /// spurious wakeup occurs, optionally looping until some predicate is
        /// satisfied `(bool(pred()) == true)`.
        ///
        /// Atomically releases lock, blocks the current executing thread, and
        /// adds it to the list of threads waiting on \a *this. The thread will
        /// be unblocked when \a notify_all() or \a notify_one() is executed,
        /// or when the absolute time point \a abs_time is reached. It may also
        /// be unblocked spuriously. When unblocked, regardless of the reason,
        /// lock is reacquired and \a wait_until exits.
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to,
        ///       for example, be delayed and unblock a thread that started
        ///       waiting just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock Type of \a lock.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 requirements of
        ///                 \namedrequirement{BasicLockable}, which must be
        ///                 locked by the current thread
        /// \param abs_time represents the time when waiting should be stopped.
        /// \param ec       used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special
        ///                 'throw on error' \a error_code.
        ///
        /// \return cv_status \a hpx::cv_status::timeout if the absolute timeout
        ///                   specified by \a abs_time was reached,
        ///                   \a hpx::cv_status::no_timeout otherwise.
        ///
        template <typename Lock>
        cv_status wait_until(Lock& lock,
            hpx::chrono::steady_time_point const& abs_time,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            auto const data = data_;    // keep data alive

            [[maybe_unused]] util::ignore_all_while_checking const ignore_lock;

            std::unique_lock<mutex_type> l(data->mtx_);
            unlock_guard<Lock> unlock(lock);

            // The following ensures that the inner lock will be unlocked
            // before the outer to avoid deadlock (fixes issue #3608)
            std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                l, std::adopt_lock);

            threads::thread_restart_state const reason =
                data->cond_.wait_until(l, abs_time, ec);

            if (ec)
                return cv_status::error;

            // if the timer has hit, the waiting period timed out
            return (reason ==
                       threads::thread_restart_state::timeout) ?    //-V110
                cv_status::timeout :
                cv_status::no_timeout;
        }

        ///
        /// \brief \a wait_until causes the current thread to block until the
        /// condition variable is notified, a specific time is reached, or a
        /// spurious wakeup occurs, optionally looping until some predicate is
        /// satisfied `(bool(pred()) == true)`.
        ///
        /// Equivalent to
        /// \code
        ///     while (!pred()) {
        ///         if (wait_until(lock, timeout_time) == hpx::cv_status::timeout) {
        ///            return pred();
        ///         }
        ///     }
        ///     return true;
        /// \endcode
        ///
        /// This overload may be used to ignore spurious wakeups.
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to,
        ///       for example, be delayed and unblock a thread that started
        ///       waiting just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 requirements of \namedrequirement{BasicLockable},
        ///                 which must be locked by the current thread
        /// \param abs_time represents the time when waiting should be stopped.
        /// \param pred     predicate which returns \a false if the waiting
        ///                 should be continued
        ///                 `(bool(pred()) == false)`.
        ///                 The signature of the predicate function should be
        ///                 equivalent to the following: `bool pred();`.
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special
        ///                 'throw on error' \a error_code.
        ///
        /// \return bool \a false if the predicate \a pred still evaluates to
        ///              \a false after the \a abs_time timeout expired,
        ///              otherwise true. If the timeout had already expired,
        ///              evaluates and returns the result of \a pred.
        ///
        template <typename Lock, typename Predicate>
        bool wait_until(Lock& lock,
            hpx::chrono::steady_time_point const& abs_time, Predicate pred,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(lock);

            while (!pred())
            {
                if (wait_until(lock, abs_time, ec) == cv_status::timeout)
                    return pred();
            }
            return true;
        }

        ///
        /// \brief Atomically releases lock, blocks the current executing
        /// thread, and adds it to the list of threads waiting on \a *this. The
        /// thread will be unblocked when \a notify_all() or \a notify_one() is
        /// executed, or when the relative timeout \a rel_time expires. It may
        /// also be unblocked spuriously. When unblocked, regardless of the
        /// reason, \a lock is reacquired and \a wait_for() exits.
        ///
        /// \note Even if notified under lock, this overload makes no guarantees
        ///       about the state of the associated predicate when returning due
        ///       to timeout.
        ///
        ///       The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock Type of \a lock.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 \namedrequirement{BasicLockable} requirements,
        ///                 which must be locked by the current thread.
        /// \param rel_time an object of type \a hpx::chrono::duration
        ///                 representing the maximum time to spend waiting. Note
        ///                 that \a rel_time must be small enough not to
        ///                 overflow when added to
        ///                 \a hpx::chrono::steady_clock::now().
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special 'throw
        ///                 on error' \a error_code.
        ///
        /// \return cv_status \a hpx::cv_status::timeout if the relative timeout
        ///                   specified by \a rel_time expired,
        ///                   \a hpx::cv_status::no_timeout otherwise.
        ///
        template <typename Lock>
        cv_status wait_for(Lock& lock,
            hpx::chrono::steady_duration const& rel_time,
            error_code& ec = throws)
        {
            return wait_until(lock, rel_time.from_now(), ec);
        }

        ///
        /// \brief Equivalent to
        /// \code
        ///     return wait_until(lock,
        ///         hpx::chrono::steady_clock::now() + rel_time,
        ///         std::move(pred));
        /// \endcode
        /// This overload may be used to ignore spurious awakenings by looping
        /// until some predicate is satisfied `(bool(pred()) == true)`.
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 \namedrequirement{BasicLockable} requirements,which
        ///                 must be locked
        ///                 by the current thread.
        /// \param rel_time an object of type \a hpx::chrono::duration
        ///                 representing the maximum time to spend waiting. Note
        ///                 that \a rel_time must be small enough not to
        ///                 overflow when added to
        ///                 \a hpx::chrono::steady_clock::now().
        /// \param pred     predicate which returns \a false if the waiting
        ///                 should be continued
        ///                 `(bool(pred()) == false)`.
        ///                 The signature of the predicate function should be
        ///                 equivalent to the following: `bool pred();`.
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special 'throw
        ///                 on error' \a error_code.
        ///
        /// \return bool \a false if the predicate \a pred still
        ///              evaluates to \a false after the \a rel_time timeout
        ///              expired, otherwise \a true.
        ///
        template <typename Lock, typename Predicate>
        bool wait_for(Lock& lock, hpx::chrono::steady_duration const& rel_time,
            Predicate pred, error_code& ec = throws)
        {
            return wait_until(lock, rel_time.from_now(), HPX_MOVE(pred), ec);
        }

        ///
        /// \brief \a wait causes the current thread to block until the
        /// condition variable is notified or a spurious wakeup occurs,
        /// optionally looping until some predicate is satisfied
        /// \a (bool(pred())==true).
        ///
        /// An interruptible wait: registers the \a condition_variable_any for
        /// the duration of \a wait(), to be notified if a stop request is made
        /// on the given stoken's associated stop-state; it is then equivalent
        /// to
        /// \code
        ///     while (!stoken.stop_requested()) {
        ///         if (pred()) return true;
        ///         wait(lock);
        ///     }
        ///     return pred();
        /// \endcode
        /// Note that the returned value indicates whether \a pred evaluated to
        /// \a true, regardless of whether there was a stop requested or not.
        ///
        /// \note The effects of `notify_one()/notify_all()` and each of the
        ///       three atomic parts of `wait()/wait_for()/wait_until()`
        ///       `(unlock+wait, wakeup, and lock)` take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to `notify_one()` was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock   an object of type Lock that meets the
        ///               \namedrequirement{BasicLockable} requirements, which
        ///               must be locked by the current thread
        /// \param stoken a \a hpx::stop_token to register interruption for
        /// \param pred   predicate which returns `false` if the waiting should
        ///               be continued `(bool(pred()) == false)`.
        ///               The signature of the predicate function should be
        ///               equivalent to the following: `bool pred()`.
        /// \param ec     Used to hold error code value originated during the
        ///               operation. Defaults to \a throws -- A special 'throw
        ///               on error' \a error_code.
        ///
        /// \returns bool result of \a pred().
        ///
        template <typename Lock, typename Predicate>
        bool wait(Lock& lock, stop_token stoken, Predicate pred,
            error_code& ec = throws)
        {
            if (stoken.stop_requested())
            {
                return pred();
            }

            auto data = data_;    // keep data alive

            auto f = [&data, &ec] {
                std::unique_lock<mutex_type> l(data->mtx_);
                data->cond_.notify_all(HPX_MOVE(l), ec);
            };
            stop_callback<decltype(f)> cb(stoken, HPX_MOVE(f));

            while (!pred())
            {
                [[maybe_unused]] util::ignore_all_while_checking const
                    ignore_lock;

                std::unique_lock<mutex_type> l(data->mtx_);
                if (stoken.stop_requested())
                {
                    // pred() has already evaluated to false since we last
                    // an acquired lock
                    return false;
                }

                unlock_guard<Lock> unlock(lock);

                // The following ensures that the inner lock will be unlocked
                // before the outer to avoid deadlock (fixes issue #3608)
                std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                    l, std::adopt_lock);

                data->cond_.wait(l, ec);
            }

            return true;
        }

        ///
        /// \brief \a wait_until causes the current thread to block until the
        /// condition variable is notified, a specific time is reached, or a
        /// spurious wakeup occurs, optionally looping until some predicate is
        /// satisfied `(bool(pred()) == true)`.
        ///
        /// An interruptible wait: registers the \a condition_variable_any for
        /// the duration of \a wait_until(), to be notified if a stop request
        /// is made on the given stoken's associated stop-state; it is then
        /// equivalent to
        /// \code
        ///     while (!stoken.stop_requested()) {
        ///         if (pred())
        ///             return true;
        ///         if (wait_until(lock, timeout_time) == hpx::cv_status::timeout)
        ///             return pred();
        ///     }
        ///     return pred();
        /// \endcode
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to,
        ///       for example, be delayed and unblock a thread that started
        ///       waiting just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 requirements of \namedrequirement{BasicLockable},
        ///                 which must be locked by the current thread.
        /// \param stoken   a \a hpx::stop_token to register interruption for.
        /// \param abs_time represents the time when waiting should be stopped.
        /// \param pred     predicate which returns \a false if the waiting
        ///                 should be continued
        ///                 `(bool(pred()) == false)`.
        ///                 The signature of the predicate function should be
        ///                 equivalent to the following: `bool pred();`.
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special
        ///                 'throw on error' \a error_code.
        ///
        /// \return bool \a pred(), regardless of whether the timeout
        ///              was met or stop was requested.
        ///
        template <typename Lock, typename Predicate>
        bool wait_until(Lock& lock, stop_token stoken,
            hpx::chrono::steady_time_point const& abs_time, Predicate pred,
            error_code& ec = throws)
        {
            if (stoken.stop_requested())
            {
                return pred();
            }

            auto data = data_;    // keep data alive

            auto f = [&data, &ec] {
                std::unique_lock<mutex_type> l(data->mtx_);
                data->cond_.notify_all(HPX_MOVE(l), ec);
            };
            stop_callback<decltype(f)> cb(stoken, HPX_MOVE(f));

            while (!pred())
            {
                bool should_stop;
                {
                    [[maybe_unused]] util::ignore_all_while_checking const
                        ignore_lock;

                    std::unique_lock<mutex_type> l(data->mtx_);
                    if (stoken.stop_requested())
                    {
                        // pred() has already evaluated to false since we last
                        // acquired lock.
                        return false;
                    }

                    unlock_guard<Lock> unlock(lock);

                    // The following ensures that the inner lock will be unlocked
                    // before the outer to avoid deadlock (fixes issue #3608)
                    std::lock_guard<std::unique_lock<mutex_type>> unlock_next(
                        l, std::adopt_lock);

                    threads::thread_restart_state const reason =
                        data->cond_.wait_until(
                            l, abs_time,
                            [&]() {
                                // re-acquire the outer lock to make sure that
                                // the user-supplied predicate is protected
                                relock_guard<Lock> relock(unlock);
                                return pred();
                            },
                            "condition_variable::wait_until_stoken", ec);

                    if (ec)
                        return false;

                    should_stop =
                        (reason == threads::thread_restart_state::timeout) ||
                        stoken.stop_requested();
                }

                if (should_stop)
                {
                    return pred();
                }
            }
            return true;
        }

        ///
        /// \brief Equivalent to
        /// \code
        ///     return wait_until(lock, std::move(stoken),
        ///         hpx::chrono::steady_clock::now() + rel_time,
        ///         std::move(pred));
        /// \endcode
        ///
        /// \note The effects of \a notify_one()/notify_all() and each of the
        ///       three atomic parts of \a wait()/wait_for()/wait_until()
        ///       (unlock+wait, wakeup, and lock) take place in a single total
        ///       order that can be viewed as modification order of an atomic
        ///       variable: the order is specific to this individual condition
        ///       variable. This makes it impossible for \a notify_one() to, for
        ///       example, be delayed and unblock a thread that started waiting
        ///       just after the call to \a notify_one() was made.
        ///
        /// \tparam Lock      Type of \a lock.
        /// \tparam Predicate Type of \a pred.
        ///
        /// \param lock     an object of type \a Lock that meets the
        ///                 \namedrequirement{BasicLockable} requirements,
        ///                 which must be locked by the current thread.
        /// \param stoken   a \a hpx::stop_token to register interruption for.
        /// \param rel_time an object of type \a hpx::chrono::duration
        ///                 representing the maximum time to spend waiting. Note
        ///                 that \a rel_time must be small enough not to
        ///                 overflow when added to
        ///                 \a hpx::chrono::steady_clock::now().
        /// \param pred     predicate which returns \a false if the waiting
        ///                 should be continued
        ///                 `(bool(pred()) == false)`.
        ///                 The signature of the predicate function should be
        ///                 equivalent to the following: `bool pred();`.
        /// \param ec       Used to hold error code value originated during the
        ///                 operation. Defaults to \a throws -- A special 'throw
        ///                 on error' \a error_code.
        ///
        /// \return bool \a pred(), regardless of whether the timeout was met or
        ///              stop was requested.
        ///
        template <typename Lock, typename Predicate>
        bool wait_for(Lock& lock, stop_token stoken,
            hpx::chrono::steady_duration const& rel_time, Predicate pred,
            error_code& ec = throws)
        {
            return wait_until(
                lock, stoken, rel_time.from_now(), HPX_MOVE(pred), ec);
        }

    private:
        hpx::util::cache_aligned_data_derived<data_type> data_;
    };
}    // namespace hpx
