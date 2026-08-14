//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2018 Thomas Heller
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/threading_base/register_thread.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>
#if !defined(HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION)
#include <hpx/modules/debugging.hpp>
#endif

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set the thread state of the \a thread referenced by the
    ///         thread_id \a id.
    ///
    /// \param id         [in] The thread id of the thread the state should
    ///                   be modified for.
    /// \param state      [in] The new state to be set for the thread
    ///                   referenced by the \a id parameter.
    /// \param stateex    [in] The new extended state to be set for the
    ///                   thread referenced by the \a id parameter.
    /// \param priority   [in]
    /// \param retry_on_active [in]
    ///
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             If the thread referenced by the parameter \a id
    ///                   is in \a thread_state#active state this function
    ///                   schedules a new thread which will set the state of
    ///                   the thread as soon as it is not active anymore. The
    ///                   function returns \a thread_state#active in this case.
    ///
    /// \returns          This function returns the previous state of the
    ///                   thread referenced by the \a id parameter. It will
    ///                   return one of the values as defined by the
    ///                   \a thread_state enumeration. If the
    ///                   thread is not known to the thread-manager the
    ///                   return value will be \a thread_state#unknown.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_state set_thread_state(
        thread_id_type const& id,
        thread_schedule_state state = thread_schedule_state::pending,
        thread_restart_state stateex = thread_restart_state::signaled,
        thread_priority priority = thread_priority::normal,
        bool retry_on_active = true, hpx::error_code& ec = hpx::throws);

    ///////////////////////////////////////////////////////////////////////
    /// \brief  Set the thread state of the \a thread referenced by the
    ///         thread_id \a id.
    ///
    /// Set a timer to set the state of the given \a thread to the given
    /// new value after it expired (at the given time)
    ///
    /// \param id         [in] The thread id of the thread the state should
    ///                   be modified for.
    /// \param abs_time   [in] Absolute point in time for the new thread to be
    ///                   run
    /// \param started    [in,out] A helper variable allowing to track the
    ///                   state of the timer helper thread
    /// \param state      [in] The new state to be set for the thread
    ///                   referenced by the \a id parameter.
    /// \param stateex    [in] The new extended state to be set for the
    ///                   thread referenced by the \a id parameter.
    /// \param priority        [in]
    /// \param retry_on_active [in]
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_id_ref_type set_thread_state(
        thread_id_type const& id,
        hpx::chrono::steady_time_point const& abs_time,
        std::atomic<bool>* started,
        thread_schedule_state state = thread_schedule_state::pending,
        thread_restart_state stateex = thread_restart_state::timeout,
        thread_priority priority = thread_priority::normal,
        bool retry_on_active = true, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT inline thread_id_ref_type set_thread_state(
        thread_id_type const& id,
        hpx::chrono::steady_time_point const& abs_time,
        thread_schedule_state state = thread_schedule_state::pending,
        thread_restart_state stateex = thread_restart_state::timeout,
        thread_priority priority = thread_priority::normal,
        bool retry_on_active = true, hpx::error_code& /*ec*/ = throws)
    {
        return set_thread_state(id, abs_time, nullptr, state, stateex, priority,
            retry_on_active, throws);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set the thread state of the \a thread referenced by the
    ///         thread_id \a id.
    ///
    /// Set a timer to set the state of the given \a thread to the given
    /// new value after it expired (after the given duration)
    ///
    /// \param id         [in] The thread id of the thread the state should
    ///                   be modified for.
    /// \param rel_time   [in] Time duration after which the new thread should
    ///                   be run
    /// \param state      [in] The new state to be set for the thread
    ///                   referenced by the \a id parameter.
    /// \param stateex    [in] The new extended state to be set for the
    ///                   thread referenced by the \a id parameter.
    /// \param priority        [in]
    /// \param retry_on_active [in]
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT inline thread_id_ref_type set_thread_state(
        thread_id_type const& id, hpx::chrono::steady_duration const& rel_time,
        thread_schedule_state state = thread_schedule_state::pending,
        thread_restart_state stateex = thread_restart_state::timeout,
        thread_priority priority = thread_priority::normal,
        bool retry_on_active = true, hpx::error_code& ec = hpx::throws)
    {
        return set_thread_state(id, rel_time.from_now(), state, stateex,
            priority, retry_on_active, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The function get_thread_backtrace is part of the thread related API
    /// allows to query the currently stored thread back trace (which is
    /// captured during thread suspension).
    ///
    /// \param id         [in] The thread id of the thread being queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns the currently captured stack
    ///                   back trace of the thread referenced by the \a id
    ///                   parameter. If the thread is not known to the
    ///                   thread-manager the return value will be the zero.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_backtrace(
        thread_id_type const& id, hpx::hpx::error_code& ec = hpx::throws);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* set_thread_backtrace(
        thread_id_type const& id, char const* bt = nullptr,
        hpx::error_code& ec = hpx::throws);
#else
#if !defined(DOXYGEN)
    // backtracing support
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT util::backtrace const*
    get_thread_backtrace(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT util::backtrace const*
    set_thread_backtrace(thread_id_type const& id,
        util::backtrace const* bt = nullptr, hpx::error_code& ec = hpx::throws);
#endif
#endif

    ///////////////////////////////////////////////////////////////////////////
    /// The function get_thread_state is part of the thread related API. It
    /// queries the state of one of the threads known to the thread-manager.
    ///
    /// \param id         [in] The thread id of the thread the state should
    ///                   be modified for.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns the thread state of the
    ///                   thread referenced by the \a id parameter. If the
    ///                   thread is not known to the thread-manager the return
    ///                   value will be \a terminated.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_state get_thread_state(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// The function get_thread_phase is part of the thread related API.
    /// It queries the phase of one of the threads known to the thread-manager.
    ///
    /// \param id         [in] The thread id of the thread the phase should
    ///                   be modified for.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns the thread phase of the
    ///                   thread referenced by the \a id parameter. If the
    ///                   thread is not known to the thread-manager the return
    ///                   value will be ~0.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_thread_phase(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// Returns whether the given thread can be interrupted at this point.
    ///
    /// \param id         [in] The thread id of the thread which should be
    ///                   queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if the given thread
    ///                   can be interrupted at this point in time. It will
    ///                   return \a false otherwise.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_thread_interruption_enabled(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    /// Set whether the given thread can be interrupted at this point.
    ///
    /// \param id         [in] The thread id of the thread which should
    ///                   receive the new value.
    /// \param enable     [in] This value will determine the new interruption
    ///                   enabled status for the given thread.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns the previous value of
    ///                   whether the given thread could have been interrupted.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool set_thread_interruption_enabled(
        thread_id_type const& id, bool enable,
        hpx::error_code& ec = hpx::throws);

    /// Returns whether the given thread has been flagged for interruption.
    ///
    /// \param id         [in] The thread id of the thread which should be
    ///                   queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if the given thread
    ///                   was flagged for interruption. It will return
    ///                   \a false otherwise.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool get_thread_interruption_requested(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    /// Flag the given thread for interruption.
    ///
    /// \param id         [in] The thread id of the thread which should be
    ///                   interrupted.
    /// \param flag       [in] The flag encodes whether the thread should be
    ///                   interrupted (if it is \a true), or 'uninterrupted'
    ///                   (if it is \a false).
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CORE_EXPORT void interrupt_thread(
        thread_id_type const& id, bool flag, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT inline void interrupt_thread(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws)
    {
        interrupt_thread(id, true, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Interrupt the current thread at this point if it was canceled. This will
    /// throw a thread_interrupted exception, which will cancel the thread.
    ///
    /// \param id         [in] The thread id of the thread which should be
    ///                   interrupted.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws the
    ///                   function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the parameter
    ///                   \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void interruption_point(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    ///////////////////////////////////////////////////////////////////////////
    /// Return priority of the given thread
    ///
    /// \param id         [in] The thread id of the thread whose priority
    ///                   is queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CORE_EXPORT threads::thread_priority get_thread_priority(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// Return stack size of the given thread
    ///
    /// \param id         [in] The thread id of the thread whose priority
    ///                   is queried.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to
    ///                   \a hpx#throws this function doesn't
    ///                   throw but returns the result code using the
    ///                   parameter \a ec. Otherwise, it throws an instance
    ///                   of hpx#exception.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ptrdiff_t get_stack_size(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void run_thread_exit_callbacks(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool add_thread_exit_callback(
        thread_id_type const& id, hpx::function<void()> const& f,
        hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void free_thread_exit_callbacks(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_thread_data(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t set_thread_data(
        thread_id_type const& id, std::size_t data,
        hpx::error_code& ec = hpx::throws);

#if defined(HPX_HAVE_LIBCDS)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_libcds_data(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t set_libcds_data(
        thread_id_type const& id, std::size_t data,
        hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    get_libcds_hazard_pointer_data(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    set_libcds_hazard_pointer_data(thread_id_type const& id, std::size_t data,
        hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    get_libcds_dynamic_hazard_pointer_data(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    set_libcds_dynamic_hazard_pointer_data(thread_id_type const& id,
        std::size_t data, hpx::error_code& ec = hpx::throws);
#endif

    // Handle thread recursion count
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t&
    get_continuation_recursion_count() noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
    reset_continuation_recursion_count() noexcept;
    /// \endcond

    /// Returns a pointer to the pool that was used to run the current thread
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside  a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_pool_base* get_pool(
        thread_id_type const& id, hpx::error_code& ec = hpx::throws);
}    // namespace hpx::threads

namespace hpx::this_thread {

    ///////////////////////////////////////////////////////////////////////////
    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// the thread state passed as the parameter.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside  a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_restart_state suspend(
        threads::thread_schedule_state state, threads::thread_id_type nextid,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws);

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// the thread state passed as the parameter.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT inline threads::thread_restart_state suspend(
        threads::thread_schedule_state state =
            threads::thread_schedule_state::pending,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws)
    {
        return suspend(state, threads::invalid_thread_id, description, ec);
    }

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// \a suspended and schedules a wakeup for this threads at the given
    /// time.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_restart_state suspend(
        hpx::chrono::steady_time_point const& abs_time,
        threads::thread_id_type id,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws);

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// \a suspended and schedules a wakeup for this threads at the given
    /// time.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT inline threads::thread_restart_state suspend(
        hpx::chrono::steady_time_point const& abs_time,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws)
    {
        return suspend(abs_time, threads::invalid_thread_id, description, ec);
    }

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// \a suspended and schedules a wakeup for this threads after the given
    /// duration.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT inline threads::thread_restart_state suspend(
        hpx::chrono::steady_duration const& rel_time,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws)
    {
        return suspend(
            rel_time.from_now(), threads::invalid_thread_id, description, ec);
    }

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// \a suspended and schedules a wakeup for this threads after the given
    /// duration.
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT inline threads::thread_restart_state suspend(
        hpx::chrono::steady_duration const& rel_time,
        threads::thread_id_type const& id,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws)
    {
        return suspend(rel_time.from_now(), id, description, ec);
    }

    /// The function \a suspend will return control to the thread manager
    /// (suspends the current thread). It sets the new state of this thread to
    /// \a suspended and schedules a wakeup for this threads after the given
    /// time (specified in milliseconds).
    ///
    /// \note Must be called from within a HPX-thread.
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    ///
    HPX_CXX_CORE_EXPORT inline threads::thread_restart_state suspend(
        std::uint64_t ms,
        threads::thread_description const& description =
            threads::thread_description("this_thread::suspend"),
        hpx::error_code& ec = hpx::throws)
    {
        return suspend(std::chrono::milliseconds(ms),
            threads::invalid_thread_id, description, ec);
    }

    /// Returns a pointer to the pool that was used to run the current thread
    ///
    /// \throws If <code>&ec != &throws</code>, never throws, but will set \a ec
    ///         to an appropriate value when an error occurs. Otherwise, this
    ///         function will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#yield_aborted if it is signaled with \a
    ///            wait_aborted.
    ///         If called outside a HPX-thread, this function will throw an
    ///         \a hpx#exception with an error code of \a
    ///            hpx#error#null_thread_id.
    ///         If this function is called while the thread-manager is not
    ///         running, it will throw an \a hpx#exception with an error code of
    ///         \a hpx#error#invalid_status.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_pool_base* get_pool(
        hpx::error_code& ec = hpx::throws);

    /// \cond NOINTERNAL
    // returns the remaining available stack space
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ptrdiff_t
    get_available_stack_space() noexcept;

    // returns whether the remaining stack-space is at least as large as
    // requested
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool has_sufficient_stack_space(
        std::size_t space_needed = static_cast<std::size_t>(
            8 * HPX_THREADS_STACK_OVERHEAD)) noexcept;
    /// \endcond
}    // namespace hpx::this_thread

#include <hpx/config/warnings_suffix.hpp>
