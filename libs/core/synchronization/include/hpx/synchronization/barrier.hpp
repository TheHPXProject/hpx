//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  The algorithm was taken from http://locklessinc.com/articles/barriers/

/// \page hpx::barrier
/// \headerfile hpx/barrier.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/synchronization/detail/condition_variable.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// \cond NOINTERN
    namespace detail {

        HPX_CXX_CORE_EXPORT struct empty_oncompletion
        {
            constexpr void operator()() const noexcept {}
        };

        struct barrier_data;

        HPX_CORE_EXPORT void intrusive_ptr_add_ref(barrier_data* p) noexcept;
        HPX_CORE_EXPORT void intrusive_ptr_release(barrier_data* p) noexcept;

        HPX_CXX_CORE_EXPORT struct barrier_data
        {
            using mutex_type = hpx::spinlock;

            barrier_data() noexcept
              : count_(1)
            {
            }

            barrier_data(barrier_data const&) = delete;
            barrier_data(barrier_data&&) = delete;
            barrier_data& operator=(barrier_data const&) = delete;
            barrier_data& operator=(barrier_data&&) = delete;

            ~barrier_data() = default;

            mutable mutex_type mtx_;

        private:
            friend HPX_CORE_EXPORT void intrusive_ptr_add_ref(
                barrier_data*) noexcept;
            friend HPX_CORE_EXPORT void intrusive_ptr_release(
                barrier_data*) noexcept;

            hpx::util::atomic_count count_;
        };
    }    // namespace detail
    /// \endcond

    /// A barrier is a thread coordination mechanism whose lifetime consists of
    /// a sequence of barrier phases, where each phase allows at most an
    /// expected number of threads to block until the expected number of threads
    /// arrive at the barrier. [ Note: A barrier is useful for managing repeated
    /// tasks that are handled by multiple threads. - end note ]

    /// Each barrier phase consists of the following steps:
    ///
    ///   - The expected count is decremented by each call to arrive or
    ///     arrive_and_drop.
    ///   - When the expected count reaches zero, the phase completion step is
    ///     run. For the specialization with the default value of the
    ///     CompletionFunction template parameter, the completion step is run
    ///     as part of the call to arrive or arrive_and_drop that caused the
    ///     expected count to reach zero. For other specializations, the
    ///     completion step is run on one of the threads that arrived at the
    ///     barrier during the phase.
    ///   - When the completion step finishes, the expected count is reset to
    ///     what was specified by the expected argument to the constructor,
    ///     possibly adjusted by calls to arrive_and_drop, and the next phase
    ///     starts.
    ///
    /// Each phase defines a phase synchronization point. Threads that arrive
    /// at the barrier during the phase can block on the phase synchronization
    /// point by calling wait, and will remain blocked until the phase
    /// completion step is run.

    /// The phase completion step that is executed at the end of each phase has
    /// the following effects:
    ///
    ///   - Invokes the completion function, equivalent to completion().
    ///   - Unblocks all threads that are blocked on the phase synchronization
    ///     point.
    ///
    /// The end of the completion step strongly happens before the returns from
    /// all calls that were unblocked by the completion step. For
    /// specializations that do not have the default value of the
    /// CompletionFunction template parameter, the behavior is undefined if any
    /// of the barrier object's member functions other than wait are called
    /// while the completion step is in progress.
    ///
    /// Concurrent invocations of the member functions of barrier, other than
    /// its destructor, do not introduce data races. The member functions
    /// arrive and arrive_and_drop execute atomically.
    ///
    /// CompletionFunction shall meet the Cpp17MoveConstructible (Table 28) and
    /// Cpp17Destructible (Table 32) requirements.
    /// std::is_nothrow_invocable_v<CompletionFunction&> shall be true.
    ///
    /// The default value of the CompletionFunction template parameter is an
    /// unspecified type, such that, in addition to satisfying the requirements
    /// of CompletionFunction, it meets the Cpp17DefaultConstructible
    /// requirements (Table 27) and completion() has no effects.
    ///
    /// barrier::arrival_token is an unspecified type, such that it meets the
    /// Cpp17MoveConstructible (Table 28), Cpp17MoveAssignable (Table 30), and
    /// Cpp17Destructible (Table 32) requirements.
    ///
    HPX_CXX_CORE_EXPORT template <typename OnCompletion =
                                      detail::empty_oncompletion>
    class barrier
    {
    public:
        /// \cond NOINTERNAL
        barrier(barrier const&) = delete;
        barrier(barrier&&) = delete;
        barrier& operator=(barrier const&) = delete;
        barrier& operator=(barrier&&) = delete;
        /// \endcond

    private:
        using mutex_type = hpx::spinlock;

    public:
        using arrival_token = bool;

        // Returns:        The maximum expected count that the implementation
        //                 supports.
        static constexpr std::ptrdiff_t(max)() noexcept
        {
            return (std::numeric_limits<std::ptrdiff_t>::max)();
        }

        /// Preconditions:  expected >= 0 is true and expected <= max() is true.
        ///
        /// Effects:        Sets both the initial expected count for each
        ///                 barrier phase and the current expected count for the
        ///                 first phase to expected. Initializes completion with
        ///                 std::move(f). Starts the first phase. [Note: If
        ///                 expected is 0 this object can only be destroyed.-
        ///                 end note]
        ///
        /// Throws:         Any exception thrown by CompletionFunction's move
        ///                 constructor.
        constexpr explicit barrier(
            std::ptrdiff_t expected, OnCompletion completion = OnCompletion())
          : mtx_(new detail::barrier_data(), false)
          , expected_(expected)
          , arrived_(expected)
          , completion_(HPX_MOVE(completion))
          , phase_(false)
        {
            // different versions of clang-format disagree
            // clang-format off
            HPX_ASSERT(expected >= 0 && expected <= (max)());
            // clang-format on
        }

        ~barrier() = default;

    private:
        /// \cond NOINTERNAL
        [[nodiscard]] arrival_token arrive_locked(
            std::unique_lock<mutex_type>& l, std::ptrdiff_t update = 1)
        {
            HPX_ASSERT_OWNS_LOCK(l);
            HPX_ASSERT_LOCKED(l, arrived_ >= update);

            bool const old_phase = phase_;
            std::ptrdiff_t const result = (arrived_ -= update);
            std::ptrdiff_t const new_expected = expected_;
            if (result == 0)
            {
                completion_();
                arrived_ = new_expected;
                phase_ = !old_phase;
                cond_.notify_all(HPX_MOVE(l));
            }
            return old_phase;
        }
        /// \endcond

    public:
        /// Preconditions:  update > 0 is true, and update is less than or equal
        ///                 to the expected count for the current barrier phase.
        ///
        /// Effects:        Constructs an object of type arrival_token that is
        ///                 associated with the phase synchronization point for
        ///                 the current phase. Then, decrements the expected
        ///                 count by update.
        ///
        /// Synchronization: The call to arrive strongly happens before the
        ///                 start of the phase completion step for the current
        ///                 phase.
        ///
        /// \returns:       The constructed arrival_token object.
        ///
        /// \throws system_error: when an exception is required
        ///                 ([thread.req.exception]).
        ///
        /// Error conditions: Any of the error conditions allowed for mutex
        ///                 types([thread.mutex.requirements.mutex]).
        /// [Note: This call can cause the completion step for the current phase
        ///        to start.- end note]
        [[nodiscard]] arrival_token arrive(std::ptrdiff_t update = 1)
        {
            auto const mtx = mtx_;    // keep alive
            std::unique_lock<mutex_type> l(mtx->mtx_);
            return arrive_locked(l, update);
        }

        /// Preconditions:  arrival is associated with the phase synchronization
        ///                 point for the current phase or the immediately
        ///                 preceding phase of the same barrier object.
        ///
        /// Effects:        Blocks at the synchronization point associated with
        ///                 HPX_MOVE(arrival) until the phase completion step
        ///                 of the synchronization point's phase is run. [ Note:
        ///                 If arrival is associated with the synchronization
        ///                 point for a previous phase, the call returns
        ///                 immediately. - end note ]
        ///
        /// \throws system_error: when an exception is required
        ///                 ([thread.req.exception]).
        /// Error conditions: Any of the error conditions allowed for mutex
        ///                 types ([thread.mutex.requirements.mutex]).
        void wait(arrival_token&& old_phase) const
        {
            auto const mtx = mtx_;    // keep alive
            std::unique_lock<mutex_type> l(mtx->mtx_);
            while (phase_ == old_phase)
            {
                cond_.wait(l, "barrier::wait");
            }
        }

        /// Effects:        Equivalent to: wait(arrive()).
        void arrive_and_wait()
        {
            auto const mtx = mtx_;    // keep alive
            std::unique_lock<mutex_type> l(mtx->mtx_);
            arrival_token const old_phase = arrive_locked(l, 1);
            while (phase_ == old_phase)
            {
                cond_.wait(l, "barrier::wait");
            }
        }

        /// Preconditions:  The expected count for the current barrier phase is
        ///                 greater than zero.
        ///
        /// Effects:        Decrements the initial expected count for all
        ///                 subsequent phases by one. Then decrements the
        ///                 expected count for the current phase by one.
        ///
        /// Synchronization: The call to arrive_and_drop strongly happens before
        ///                 the start of the phase completion step for the
        ///                 current phase.
        ///
        /// \throws system_error: when an exception is required
        ///                 ([thread.req.exception]).Error conditions:
        ///                 Any of the error conditions allowed for mutex
        ///                 types ([thread.mutex.requirements.mutex]).
        ///                 [Note: This call can cause the completion
        ///                 step for the current phase to start.- end note]
        void arrive_and_drop()
        {
            auto const mtx = mtx_;    // keep alive
            std::unique_lock<mutex_type> l(mtx->mtx_);
            HPX_ASSERT_LOCKED(l, expected_ > 0);
            --expected_;
            [[maybe_unused]] bool result = arrive_locked(l, 1);
        }

    private:
        hpx::intrusive_ptr<detail::barrier_data> mtx_;
        mutable hpx::lcos::local::detail::condition_variable cond_;

        std::ptrdiff_t expected_;
        std::ptrdiff_t arrived_;
        OnCompletion completion_;
        bool phase_;
    };

    /// \cond NOINTERNAL
    namespace lcos::local {

        //////////////////////////////////////////////////////////////////////////
        // A barrier can be used to synchronize a specific number of threads,
        // blocking all the entering threads until all the threads have
        // entered the barrier.
        //
        // \note   A \a barrier is not a LCO in the sense that it has no global
        //         id, and it can't be triggered using the action (parcel)
        //         mechanism. It is just a low level synchronization primitive
        //         allowing to synchronize a given number of \a threads.
        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT barrier
        {
            using mutex_type = hpx::spinlock;

            static constexpr std::size_t barrier_flag =
                static_cast<std::size_t>(1)
                << (CHAR_BIT * sizeof(std::size_t) - 1);

        public:
            // Creates a barrier with a given size, rank is locality id
            //
            // \param expected The number of participating threads
            //
            explicit barrier(std::size_t expected);

            barrier(barrier const&) = delete;
            barrier(barrier&&) = delete;
            barrier& operator=(barrier const&) = delete;
            barrier& operator=(barrier&&) = delete;

            ~barrier();

            // The function \a wait will block the number of entering \a threads
            // (as given by the constructor parameter \a number_of_threads),
            // releasing all waiting threads as soon as the last \a thread
            // entered this function.
            void wait();

            // The function \a count_up will increase the number of \a threads
            // to be waited in \a wait function.
            void count_up();

            // The function \a reset will reset the number of \a threads as
            // given by the function parameter \a number_of_threads. the newer
            // coming \a threads executing the function \a wait will be waiting
            // until \a total_ is equal to  \a barrier_flag.
            // The last \a thread exiting the \a wait function will notify the
            // newer \a threads waiting and the newer \a threads will get the
            // reset \a number_of_threads_. The function \a reset can be
            // executed while previous \a threads executing waiting after they
            // have been waken up. Thus \a total_ can not be reset to \a
            // barrier_flag which will break the comparison condition under the
            // function \a wait.
            void reset(std::size_t number_of_threads);

        private:
            std::size_t number_of_threads_;
            std::size_t total_;

            mutable mutex_type mtx_;
            hpx::lcos::local::detail::condition_variable cond_;
        };
    }    // namespace lcos::local
    /// \endcond
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
