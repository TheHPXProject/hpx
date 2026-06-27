//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/coroutines/thread_enums.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/coroutines/config/defines.hpp>
#include <hpx/coroutines/detail/combined_tagged_state.hpp>

#include <cstdint>
#include <iosfwd>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    // clang-format off

    /// \enum thread_schedule_state
    ///
    /// The \a thread_schedule_state enumerator encodes the current state of a
    /// \a thread instance
    HPX_CXX_CORE_EXPORT enum class thread_schedule_state : std::int8_t
    {
        unknown = 0,
        active = 1,                   /*!< thread is currently active (running,
                                           has resources) */
        pending = 2,                  /*!< thread is pending (ready to run, but
                                           no hardware resource available) */
        suspended = 3,                /*!< thread has been suspended (waiting
                                           for synchronization event, but still
                                           known and under control of the
                                           thread-manager) */
        depleted = 4,                 /*!< thread has been depleted (deeply
                                           suspended, it is not known to the
                                           thread-manager) */
        terminated = 5,               /*!< thread has been stopped and may be
                                           garbage collected */
        staged = 6,                   /*!< this is not a real thread state, but
                                           allows to reference staged task
                                           descriptions, which eventually will
                                           be converted into thread objects */
        pending_do_not_schedule = 7,  /*!< this is not a real thread state,
                                          but allows to create a thread in
                                          pending state without scheduling it
                                          (internal, do not use) */
        pending_boost = 8,            /*!< this is not a real thread state,
                                          but allows to suspend a thread in
                                          pending state without high priority
                                          rescheduling */
        deleted = 9                   /*!< thread has been stopped and was
                                          deleted */
    };
    // clang-format on

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, thread_schedule_state t);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns the name of the given state
    ///
    /// Get the readable string representing the name of the given
    /// thread_state constant.
    ///
    /// \param state this represents the thread state.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_state_name(
        thread_schedule_state state) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    // clang-format off

    /// This enumeration lists all possible thread-priorities for HPX threads.
    ///
    HPX_CXX_CORE_EXPORT enum class thread_priority : std::int8_t
    {
        unknown = -1,
        default_ = 0, /*!< Will assign the priority of the
            task to the default (normal) priority. */
        low = 1,     /*!< Task goes onto a special low
            priority queue and will not be executed until all high/normal
            priority tasks are done, even if they are added after the low
            priority task. */
        normal = 2,  /*!< Task will be executed when it is
            taken from the normal priority queue, this is usually a first
            in-first-out ordering of tasks (depending on scheduler choice).
            This is the default priority. */
        high_recursive = 3, /*!< The task is a high priority
            task and any child tasks spawned by this task will be made high
            priority as well - unless they are specifically flagged as non
            default priority. */
        boost = 4, /*!< Same as \a thread_priority_high
            except that the thread will fall back to \a thread_priority_normal
            if resumed after being suspended. */
        high = 5,  /*!< Task goes onto a special high
            priority queue and will be executed before normal/low priority
            tasks are taken (some schedulers modify the behavior slightly and
            the documentation for those should be consulted). */
        bound = 6,          /*!< Task goes onto a special
            high priority queue and will never be stolen by another thread
            after initial assignment. This should be used for thread placement
            tasks such as OpenMP type for loops. */
        initially_bound = 7,          /*!< Task initially goes onto a special
            high priority queue and will never be stolen by another thread
            after initial assignment. If the thread is rescheduled later, it
            will use normal priority. */

        /// \cond NOINTERNAL
        // obsolete, kept for compatibility only
        critical = high_recursive,
        /// \endcond
    };
    // clang-format on

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, thread_priority t);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief Return the thread priority name.
    ///
    /// Get the readable string representing the name of the given thread_priority
    /// constant.
    ///
    /// \param priority this represents the thread priority.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_priority_name(
        thread_priority priority) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_restart_state
    ///
    /// The \a thread_restart_state enumerator encodes the reason why a
    /// thread is being restarted
    HPX_CXX_CORE_EXPORT enum class thread_restart_state : std::int8_t {
        unknown = 0,
        signaled = 1,     ///< The thread has been signaled
        timeout = 2,      ///< The thread has been reactivated after a timeout
        terminate = 3,    ///< The thread needs to be terminated
        abort = 4         ///< The thread needs to be aborted
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, thread_restart_state t);

    /// Get the readable string representing the name of the given
    /// thread_restart_state constant.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_state_ex_name(
        thread_restart_state state) noexcept;

    /// \cond NOINTERNAL
    // special type storing both state in one tagged structure
    HPX_CXX_CORE_EXPORT using thread_state =
        threads::detail::combined_tagged_state<thread_schedule_state,
            thread_restart_state>;
    /// \endcond

    /// Get the readable string representing the name of the given
    /// thread_state constant.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_state_name(
        thread_state state) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_stacksize
    ///
    /// A \a thread_stacksize references any of the possible stack-sizes for
    /// HPX threads.
    HPX_CXX_CORE_EXPORT enum class thread_stacksize : std::int8_t {
        unknown = -1,
        small_ = 1,     ///< use small stack size (the underscore is to work
                        ///  around 'small' being defined to char on Windows)
        medium = 2,     ///< use medium sized stack size
        large = 3,      ///< use large stack size
        huge = 4,       ///< use very large stack size
        nostack = 5,    ///< this thread does not suspend
                        ///< (does not need a stack)
        current = 6,    ///< use size of current thread's stack

        default_ = small_,    ///< use default stack size
        minimal = small_,     ///< use minimally stack size
        maximal = huge,       ///< use maximally stack size
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, thread_stacksize t);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns the stack size name.
    ///
    /// Get the readable string representing the given stack size
    /// constant.
    ///
    /// \param size this represents the stack size
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_stack_size_enum_name(
        thread_stacksize size) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_schedule_hint_mode
    ///
    /// The type of hint given when creating new tasks.
    HPX_CXX_CORE_EXPORT enum class thread_schedule_hint_mode : std::int8_t {
        /// A hint that leaves the choice of scheduling entirely up to the
        /// scheduler.
        none = 0,

        /// A hint that tells the scheduler to prefer scheduling a task on the
        /// local thread number associated with this hint. Local thread numbers
        /// are indexed from zero. It is up to the scheduler to decide how to
        /// interpret thread numbers that are larger than the number of threads
        /// available to the scheduler. Typically, thread numbers will wrap
        /// around when too large.
        thread = 1,

        /// A hint that tells the scheduler to prefer scheduling a task on the
        /// NUMA domain associated with this hint. NUMA domains are indexed from
        /// zero. It is up to the scheduler to decide how to interpret NUMA
        /// domain indices that are larger than the number of available NUMA
        /// domains to the scheduler. Typically, indices will wrap around when
        /// too large.
        numa = 2,
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_placement_hint
    ///
    /// The type of hint given to the scheduler related to thread placement
    HPX_CXX_CORE_EXPORT enum class thread_placement_hint : std::int8_t {
        /// No hint is specified. The implementation is free to chose what
        /// placement methods to use.
        none = 0,

        /// A hint that tells the scheduler to prefer spreading thread placement
        /// on a depth-first basis (i.e. consecutively scheduled threads are
        /// placed on the same core).
        depth_first = 1,

        /// A hint that tells the scheduler to prefer spreading thread placement
        /// on a breadth-first basis (i.e. consecutively scheduled threads are
        /// placed on the neighboring cores).
        breadth_first = 2,

        /// A hint that tells the scheduler to prefer spreading thread placement
        /// on a depth-first basis (i.e. consecutively scheduled threads are
        /// placed on the same core). Threads are being scheduled in reverse
        /// order.
        depth_first_reverse = 5,

        /// A hint that tells the scheduler to prefer spreading thread placement
        /// on a breadth-first basis (i.e. consecutively scheduled threads are
        /// placed on the neighboring cores). Threads are being scheduled in
        /// reverse order.
        breadth_first_reverse = 6
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_sharing_hint
    ///
    /// The type of hint given to the scheduler related to whether it is ok to
    /// share the invoked function object between threads
    HPX_CXX_CORE_EXPORT enum class thread_sharing_hint : std::int8_t {
        /// No hint is specified. The implementation is free to chose what
        /// sharing methods to use.
        none = 0,

        /// A hint that tells the scheduler to avoid sharing the given function
        /// (object) between threads.
        do_not_share_function = 1,

        /// A hint that tells the scheduler to avoid combining tasks on the same
        /// thread. This is important for tasks that may synchronize between
        /// each other, which could lead to deadlocks if those tasks are
        /// combined running by the same thread.
        do_not_combine_tasks = 2
    };

    HPX_CXX_CORE_EXPORT constexpr bool do_not_share_function(
        thread_sharing_hint hint) noexcept
    {
        return static_cast<std::int8_t>(hint) &
            static_cast<std::int8_t>(
                thread_sharing_hint::do_not_share_function);
    }

    HPX_CXX_CORE_EXPORT constexpr bool do_not_combine_tasks(
        thread_sharing_hint hint) noexcept
    {
        return static_cast<std::int8_t>(hint) &
            static_cast<std::int8_t>(thread_sharing_hint::do_not_combine_tasks);
    }

    HPX_CXX_CORE_EXPORT constexpr thread_sharing_hint operator|(
        thread_sharing_hint lhs, thread_sharing_hint rhs) noexcept
    {
        return static_cast<thread_sharing_hint>(
            static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \enum thread_placement_hint
    ///
    /// The type of hint given to the scheduler related running a thread as a
    /// child directly in the context of the parent thread
    HPX_CXX_CORE_EXPORT enum class thread_execution_hint : std::int8_t {
        /// No hint is specified. Always run the thread in its own execution
        /// environment.
        none = 0,

        /// Attempt to run the thread in the execution context of the parent
        /// thread.
        run_as_child = 1,
    };

    HPX_CXX_CORE_EXPORT constexpr bool run_as_child(
        thread_execution_hint hint) noexcept
    {
        return static_cast<std::int8_t>(hint) &
            static_cast<std::int8_t>(thread_execution_hint::run_as_child);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Default value to use for runs-as-child mode (if \a true, then futures
    /// will attempt to execute associated threads directly if they have not
    /// started running).
#if defined(HPX_COROUTINES_HAVE_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD)
    HPX_CXX_CORE_EXPORT inline constexpr thread_execution_hint
        default_runs_as_child_hint = thread_execution_hint::run_as_child;
#else
    HPX_CXX_CORE_EXPORT inline constexpr thread_execution_hint
        default_runs_as_child_hint = thread_execution_hint::none;
#endif

    ///////////////////////////////////////////////////////////////////////////
    /// \brief A hint given to a scheduler to guide where a task should be
    /// scheduled.
    ///
    /// A scheduler is free to ignore the hint, or modify the hint to suit the
    /// resources available to the scheduler.
    HPX_CXX_CORE_EXPORT struct thread_schedule_hint
    {
        /// Construct a default hint with mode thread_schedule_hint_mode::none.
        constexpr thread_schedule_hint() noexcept
          : placement_mode_bits(
                static_cast<std::int8_t>(thread_placement_hint::none))
          , sharing_mode_bits(
                static_cast<std::int8_t>(thread_sharing_hint::none))
          , runs_as_child_mode_bits(
                static_cast<std::int8_t>(default_runs_as_child_hint))
        {
        }

        /// Construct a hint with mode thread_schedule_hint_mode::thread and the
        /// given hint as the local thread number.
        constexpr explicit thread_schedule_hint(std::int16_t const thread_hint,
            thread_placement_hint placement = thread_placement_hint::none,
            thread_execution_hint runs_as_child = default_runs_as_child_hint,
            thread_sharing_hint sharing = thread_sharing_hint::none) noexcept
          : hint(thread_hint)
          , mode(thread_hint >= 0 ? thread_schedule_hint_mode::thread :
                                    thread_schedule_hint_mode::none)
          , placement_mode_bits(static_cast<std::int8_t>(placement))
          , sharing_mode_bits(static_cast<std::int8_t>(sharing))
          , runs_as_child_mode_bits(static_cast<std::int8_t>(runs_as_child))
        {
        }

        /// Construct a hint with the given mode and hint. The numerical hint is
        /// unused when the mode is thread_schedule_hint_mode::none.
        constexpr thread_schedule_hint(thread_schedule_hint_mode const mode,
            std::int16_t const hint,
            thread_placement_hint placement = thread_placement_hint::none,
            thread_execution_hint runs_as_child = default_runs_as_child_hint,
            thread_sharing_hint sharing = thread_sharing_hint::none) noexcept
          : hint(hint)
          , mode(mode)
          , placement_mode_bits(static_cast<std::int8_t>(placement))
          , sharing_mode_bits(static_cast<std::int8_t>(sharing))
          , runs_as_child_mode_bits(static_cast<std::int8_t>(runs_as_child))
        {
        }

        /// Construct a hint with the given placement hint.
        explicit constexpr thread_schedule_hint(
            thread_placement_hint placement) noexcept
          : placement_mode_bits(static_cast<std::int8_t>(placement))
          , sharing_mode_bits(
                static_cast<std::int8_t>(thread_sharing_hint::none))
          , runs_as_child_mode_bits(
                static_cast<std::int8_t>(default_runs_as_child_hint))
        {
        }

        /// Construct a hint with the given execution hint.
        explicit constexpr thread_schedule_hint(
            thread_execution_hint runs_as_child) noexcept
          : placement_mode_bits(
                static_cast<std::int8_t>(thread_placement_hint::none))
          , sharing_mode_bits(
                static_cast<std::int8_t>(thread_sharing_hint::none))
          , runs_as_child_mode_bits(static_cast<std::int8_t>(runs_as_child))
        {
        }

        /// Construct a hint with the given sharing hint.
        explicit constexpr thread_schedule_hint(
            thread_sharing_hint sharing) noexcept
          : placement_mode_bits(
                static_cast<std::int8_t>(thread_placement_hint::none))
          , sharing_mode_bits(static_cast<std::int8_t>(sharing))
          , runs_as_child_mode_bits(
                static_cast<std::int8_t>(default_runs_as_child_hint))
        {
        }

        /// \cond NOINTERNAL
        constexpr bool operator==(
            thread_schedule_hint const& rhs) const noexcept
        {
            return mode == rhs.mode && hint == rhs.hint &&
                placement_mode() == rhs.placement_mode() &&
                sharing_mode() == rhs.sharing_mode() &&
                runs_as_child_mode() == rhs.runs_as_child_mode();
        }

        constexpr bool operator!=(
            thread_schedule_hint const& rhs) const noexcept
        {
            return !(*this == rhs);
        }
        /// \endcond

        // gcc V9 requires to store the bits as integers instead of directly
        // storing the enums
        [[nodiscard]] constexpr thread_placement_hint placement_mode()
            const noexcept
        {
            return static_cast<thread_placement_hint>(placement_mode_bits);
        }
        void placement_mode(thread_placement_hint bits) noexcept
        {
            placement_mode_bits = static_cast<std::uint8_t>(bits);
        }

        [[nodiscard]] constexpr thread_sharing_hint sharing_mode()
            const noexcept
        {
            return static_cast<thread_sharing_hint>(sharing_mode_bits);
        }
        void sharing_mode(thread_sharing_hint bits) noexcept
        {
            sharing_mode_bits = static_cast<std::uint8_t>(bits);
        }

        [[nodiscard]] constexpr thread_execution_hint runs_as_child_mode()
            const noexcept
        {
            return static_cast<thread_execution_hint>(runs_as_child_mode_bits);
        }
        void runs_as_child_mode(thread_execution_hint bits) noexcept
        {
            runs_as_child_mode_bits = static_cast<std::uint8_t>(bits);
        }

        void schedule_hint(std::int16_t core) noexcept
        {
            mode = thread_schedule_hint_mode::thread;
            hint = core;
        }

        /// The hint associated with the mode. The interpretation of this hint
        /// depends on the given mode.
        std::int16_t hint = -1;

        /// The mode of the scheduling hint.
        thread_schedule_hint_mode mode = thread_schedule_hint_mode::none;

        /// The mode of the desired thread placement.
        std::uint8_t placement_mode_bits : 5;

        /// The mode of the desired sharing hint
        std::uint8_t sharing_mode_bits : 2;

        /// The thread will run as a child directly in the context of the
        /// current thread
        std::uint8_t runs_as_child_mode_bits : 1;
    };
}    // namespace hpx::threads
