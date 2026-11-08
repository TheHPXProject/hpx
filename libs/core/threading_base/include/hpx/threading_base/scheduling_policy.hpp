//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file scheduling_policy.hpp
/// \brief The rules that decide when a scheduler wakes up a worker and what
///        an idle worker does, apart from the scheduler that applies them.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>
#include <hpx/threading_base/thread_queue_init_parameters.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
// The rules of a scheduler
//
// This is an overview of the rules, by the numbers used in the comments of
// this file. The scheduler supplies the facts and carries out the decisions.
// Each rule names the setting that controls it, with its default in
// parentheses.
//
// Two sets of rules decide the same way at the defaults:
// fixed_scheduling_rules, used by the local priority queue scheduler, has the
// defaults of the settings of rules 1 to 4 and 8 and of the pauses of rule 7
// built in, and scheduling_policy, used by the local-priority-tuned-fifo
// scheduler, reads them where it decides from tunable settings (see
// scheduler_tuning_parameters), which a tuner may change while the scheduler
// runs. Only that scheduler reads these settings from the configuration (see
// scheduling_policy_init_parameters). A setting is either per worker, or
// holds for the whole pool.
//
// New threads (wakes_one_for, wake_for_small)
//
//    Only schedulers that wake up workers for nano and small threads
//    themselves use these rules (see scheduler_base::wakes_up_for). Threads
//    with a priority other than normal or default wake up workers as before.
//
//    1. A new thread up to a given size wakes up at most one worker. Larger
//       and unsized threads wake up workers as before, and rules 2 to 4 don't
//       apply to them. The size unknown turns these rules off.
//       hpx.scheduler_policy.wake_one_max_size (small), pool
//
//    2. A thread queued for another worker wakes up only that one, and only
//       if it sleeps: if it is awake, it will find the thread.
//       hpx.scheduler_policy.wake_target_only (1), pool
//
//    A worker runs the threads it queued for itself once it is done with its
//    current one, which may take long. So it wakes up one sleeping worker if
//    rule 3 or 4 holds, and nobody otherwise:
//
//    3. No idle worker spins that would find the thread.
//       hpx.scheduler_policy.wake_if_nobody_spins (1), pool
//
//    4. Round-robin or NUMA placement put the thread there, as opposed to its
//       creator: it may belong to a caller that waits for it without
//       yielding.
//       hpx.scheduler_policy.wake_one_unless_placed_by_caller (1), pool
//
//    5. Placement itself is not decided here: the scheduler keeps it. A
//       thread without a hint goes to the next worker in round-robin order,
//       which may be the caller (see rule 4).
//
// Idle workers (idle_step_for, free_pass_after_wake)
//
//    Rules 6 and 7 hold for all schedulers, rule 8 for those with early
//    minima. scheduler_base applies them with the configured values (see
//    scheduler_base::idle_step_for); the local-priority-tuned-fifo scheduler
//    takes them from scheduling_policy.
//
//    6. The backoff period of an idle worker doubles with every period
//       without work, starting at 1 us, up to a maximum.
//       hpx.max_idle_backoff_time (1000 us), per worker
//
//    7. A period shorter than a minimum is spent spinning instead of sleeping,
//       since a timed wait can't be that short. A spinning worker counts
//       rounds of pause instructions, and it looks at its own queues every
//       round and at the queues it steals from every given interval.
//       hpx.min_idle_backoff_time (50 us), per worker
//       hpx.idle_backoff_spin_round_pauses (16), per worker
//       hpx.idle_backoff_steal_check_interval (8 us), per worker
//
//    8. A worker woken up by a notification may steal once without the early
//       minima of the scheduler.
//       hpx.scheduler_policy.free_pass_after_wake (1), pool

namespace hpx::threads::policies {

    /// What a scheduler does after it queued a new nano or small thread
    /// (see scheduling_policy::wake_for_small).
    HPX_CXX_CORE_EXPORT enum class wake_action : std::uint8_t {
        /// nothing, the calling worker runs the thread once it is done with
        /// its current one
        none = 0,

        /// wake up the worker the thread was queued for if that one sleeps
        target_if_sleeping = 1,

        /// wake up one sleeping worker
        one = 2,

        /// as for threads of other sizes (scheduler_base::do_some_work)
        all = 3
    };

    /// What an idle worker does for one backoff period (see
    /// scheduling_policy::idle_step_for).
    HPX_CXX_CORE_EXPORT struct idle_step
    {
        /// the backoff period
        std::chrono::microseconds period;

        /// zero: sleep for the period (unless woken up); otherwise spin for
        /// this many rounds, or until there is work
        std::uint64_t spin_rounds;

        /// the pause instructions per spin round
        std::uint32_t pauses_per_round;

        /// look at the queues to steal from every this many spin rounds
        std::uint64_t steal_check_rounds;
    };

    /// The values from the configuration that the settings of
    /// scheduling_policy start with, apart from those that are in
    /// thread_queue_init_parameters (rules 6 and 7). Read when a
    /// local-priority-tuned-fifo scheduler is created and given to that one
    /// only: the local priority queue scheduler doesn't read them.
    HPX_CXX_CORE_EXPORT struct scheduling_policy_init_parameters
    {
        // rule 1 (hpx.scheduler_policy.wake_one_max_size)
        threads::task_size wake_one_max_size_ = threads::task_size::small_;

        // rule 2 (hpx.scheduler_policy.wake_target_only)
        bool wake_target_only_ = true;

        // rule 3 (hpx.scheduler_policy.wake_if_nobody_spins)
        bool wake_if_nobody_spins_ = true;

        // rule 4 (hpx.scheduler_policy.wake_one_unless_placed_by_caller)
        bool wake_one_unless_placed_by_caller_ = true;

        // rule 8 (hpx.scheduler_policy.free_pass_after_wake)
        bool free_pass_after_wake_ = true;

        // rule 7 (hpx.idle_backoff_spin_round_pauses)
        std::uint32_t idle_backoff_spin_round_pauses_ = 16;
    };

    /// The rules for new threads (2 to 4) at the defaults of their settings,
    /// used by the local priority queue scheduler. They decide as
    /// scheduling_policy::wake_for_small does at the default settings; see
    /// there for the parameters.
    HPX_CXX_CORE_EXPORT struct fixed_scheduling_rules
    {
        explicit constexpr fixed_scheduling_rules(std::size_t) noexcept {}

        template <typename HasSpinningWorker>
        [[nodiscard]] wake_action wake_for_small(
            thread_queue_init_parameters const&, std::size_t const target,
            std::size_t const caller, thread_priority const priority,
            bool const placed_by_caller,
            HasSpinningWorker&& has_spinning_worker) const
        {
            if (priority != thread_priority::normal &&
                priority != thread_priority::default_)
            {
                return wake_action::all;
            }

            if (target != caller)
            {
                return wake_action::target_if_sleeping;
            }

            if (!placed_by_caller || !has_spinning_worker())
            {
                return wake_action::one;
            }
            return wake_action::none;
        }
    };

    /// The rules a scheduler applies when it decides whether to wake up a
    /// worker for new work and what an idle worker does next. The scheduler
    /// supplies the facts (queue lengths, whether workers spin or sleep) and
    /// carries out the decision (it keeps the queues and the sleep mask). The
    /// numbers and switches the rules use are tunable settings (see
    /// scheduler_tuning_parameters), which a tuner may change while the
    /// scheduler runs.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT scheduling_policy
    {
    public:
        /// The pause instructions of a spin round the spin rate is measured
        /// with, and of the rounds of the schedulers that don't tune them
        /// (see scheduler_base).
        static constexpr std::uint32_t calibration_pauses_per_round = 16;

        /// Register the settings of the rules for idle workers (6 to 8), with
        /// the values of the given configuration. Done by every scheduler.
        void register_idle_parameters(scheduler_tuning_parameters& parameters,
            thread_queue_init_parameters const& init,
            scheduling_policy_init_parameters const& policy_init);

        /// Register the settings of the rules for new threads (1 to 4), with
        /// the values of the given configuration. Done only by schedulers
        /// that wake up workers for nano and small threads themselves (see
        /// scheduler_base::wakes_up_for).
        void register_wake_parameters(scheduler_tuning_parameters& parameters,
            scheduling_policy_init_parameters const& policy_init);

        /// Rule 1: new threads of this size wake up at most one worker (see
        /// wake_for_small), larger and unsized ones wake up workers as
        /// before (hpx.scheduler_policy.wake_one_max_size).
        [[nodiscard]] bool wakes_one_for(
            scheduler_tuning_parameters const& parameters,
            threads::task_size const size) const noexcept
        {
            HPX_ASSERT(wake_one_max_size_ != scheduler_tuning_parameters::npos);
            return size != threads::task_size::unknown &&
                static_cast<double>(size) <= parameters.get(wake_one_max_size_);
        }

        /// Rules 2 to 4: whom to wake up after a new nano or small thread was
        /// queued on the worker target by the worker caller (the calling
        /// worker, or -1 for a thread outside of the pool).
        /// placed_by_caller tells whether the creator chose the worker (as
        /// opposed to round-robin or NUMA placement). has_spinning_worker
        /// returns whether an idle worker spins that would find the thread;
        /// it is called only if needed.
        template <typename HasSpinningWorker>
        [[nodiscard]] wake_action wake_for_small(
            scheduler_tuning_parameters const& parameters,
            std::size_t const target, std::size_t const caller,
            thread_priority const priority, bool const placed_by_caller,
            HasSpinningWorker&& has_spinning_worker) const
        {
            // threads with a priority wake up workers as before
            if (priority != thread_priority::normal &&
                priority != thread_priority::default_)
            {
                return wake_action::all;
            }

            // rule 2: a thread queued for another worker wakes up only that
            // one, if it sleeps; it has nothing else to do
            // (hpx.scheduler_policy.wake_target_only)
            if (target != caller)
            {
                return parameters.get(wake_target_only_) != 0.0 ?
                    wake_action::target_if_sleeping :
                    wake_action::all;
            }

            // The calling worker runs the threads it queued for itself once
            // it is done with its current one. But it may keep running for
            // a long time without getting to them (a task that posts work
            // and goes on with its own), so one sleeping worker is woken up
            // (which then spins for a while, waking up resets its backoff):
            // - rule 3: if no idle worker spins that would find them
            //   (hpx.scheduler_policy.wake_if_nobody_spins),
            // - rule 4: if round-robin placement put them there: they may
            //   belong to a caller that waits for them without yielding
            //   (hpx.scheduler_policy.wake_one_unless_placed_by_caller).
            if ((!placed_by_caller &&
                    parameters.get(wake_one_unless_placed_by_caller_) != 0.0) ||
                (parameters.get(wake_if_nobody_spins_) != 0.0 &&
                    !has_spinning_worker()))
            {
                return wake_action::one;
            }
            return wake_action::none;
        }

        /// Rules 6 and 7: what the given worker does for its backoff period
        /// after count periods without work. The period doubles with every
        /// one, up to hpx.max_idle_backoff_time. Periods shorter than
        /// hpx.min_idle_backoff_time are spent spinning: a timed wait lasts
        /// at least the time the OS takes to wake the thread up (a
        /// millisecond or more on Windows, the timer slack of 50 us on
        /// Linux), so a short sleep would be a long one. A spinning worker
        /// looks at its own queues every round, but at the queues it steals
        /// from only every hpx.idle_backoff_steal_check_interval
        /// microseconds, since that reads the length of the queue of every
        /// victim. The spin counts rounds instead of reading the clock:
        /// times are converted to rounds with the rate measured at startup
        /// (spin_rounds_per_us, for rounds of calibration_pauses_per_round
        /// pauses), because a round lasts from a few nanoseconds (yield on
        /// AArch64) to most of a microsecond (pause on recent Intel cores).
        [[nodiscard]] idle_step idle_step_for(
            scheduler_tuning_parameters const& parameters,
            std::size_t const worker, std::uint32_t const count,
            double const spin_rounds_per_us) const noexcept
        {
            constexpr double max_exponent =
                std::numeric_limits<double>::max_exponent - 1;
            double const exponent =
                (std::min) (static_cast<double>(count), max_exponent);

            idle_step step{};
            step.period = std::chrono::microseconds(std::lround(
                (std::min) (parameters.get(worker, max_idle_backoff_time_),
                    std::pow(2.0, exponent))));

            if (static_cast<double>(step.period.count()) >=
                parameters.get(worker, min_idle_backoff_time_))
            {
                return step;    // sleep
            }

            // a round of more pauses lasts longer than the measured one
            step.pauses_per_round = static_cast<std::uint32_t>(
                parameters.get(worker, pauses_per_round_));
            double const rounds_per_us = spin_rounds_per_us *
                calibration_pauses_per_round / step.pauses_per_round;

            step.spin_rounds = (std::max) (std::uint64_t(1),
                static_cast<std::uint64_t>(std::llround(
                    static_cast<double>(step.period.count()) * rounds_per_us)));
            step.steal_check_rounds = (std::max) (std::uint64_t(1),
                static_cast<std::uint64_t>(
                    std::llround(parameters.get(worker, steal_check_interval_) *
                        rounds_per_us)));
            return step;
        }

        /// Rule 8: a worker woken up by a notification may steal once
        /// without the early minima of the scheduler
        /// (hpx.scheduler_policy.free_pass_after_wake).
        [[nodiscard]] bool free_pass_after_wake(
            scheduler_tuning_parameters const& parameters) const noexcept
        {
            return parameters.get(free_pass_after_wake_) != 0.0;
        }

    private:
        std::size_t wake_one_max_size_ = scheduler_tuning_parameters::npos;
        std::size_t wake_target_only_ = scheduler_tuning_parameters::npos;
        std::size_t wake_if_nobody_spins_ = scheduler_tuning_parameters::npos;
        std::size_t wake_one_unless_placed_by_caller_ =
            scheduler_tuning_parameters::npos;
        std::size_t max_idle_backoff_time_ = scheduler_tuning_parameters::npos;
        std::size_t min_idle_backoff_time_ = scheduler_tuning_parameters::npos;
        std::size_t steal_check_interval_ = scheduler_tuning_parameters::npos;
        std::size_t pauses_per_round_ = scheduler_tuning_parameters::npos;
        std::size_t free_pass_after_wake_ = scheduler_tuning_parameters::npos;
    };

    /// The rules of scheduling_policy together with their tunable settings
    /// and the tuner that decides their values, used by the
    /// local-priority-tuned-fifo scheduler. wake_for_small has the interface of
    /// fixed_scheduling_rules::wake_for_small.
    HPX_CXX_CORE_EXPORT class tuned_scheduling_rules
    {
    public:
        explicit tuned_scheduling_rules(std::size_t const num_workers)
          : parameters_(num_workers)
        {
        }

        /// Register the settings of all rules, with the values of the given
        /// configuration.
        void register_parameters(thread_queue_init_parameters const& init,
            scheduling_policy_init_parameters const& policy_init)
        {
            policy_.register_idle_parameters(parameters_, init, policy_init);
            policy_.register_wake_parameters(parameters_, policy_init);
        }

        /// Set once the scheduler is constructed, before its workers start:
        /// the tuner that decides the values of the settings (see
        /// hpx.scheduler_tuner).
        void set_tuner(std::unique_ptr<scheduler_tuner> tuner)
        {
            HPX_ASSERT(tuner);
            tuner_ = HPX_MOVE(tuner);
            tuner_->attach(parameters_);
        }

        [[nodiscard]] scheduler_tuning_parameters& parameters() noexcept
        {
            return parameters_;
        }
        [[nodiscard]] scheduler_tuning_parameters const& parameters()
            const noexcept
        {
            return parameters_;
        }

        [[nodiscard]] scheduling_policy const& policy() const noexcept
        {
            return policy_;
        }

        template <typename HasSpinningWorker>
        [[nodiscard]] wake_action wake_for_small(
            thread_queue_init_parameters const&, std::size_t const target,
            std::size_t const caller, thread_priority const priority,
            bool const placed_by_caller,
            HasSpinningWorker&& has_spinning_worker) const
        {
            return policy_.wake_for_small(parameters_, target, caller, priority,
                placed_by_caller,
                HPX_FORWARD(HasSpinningWorker, has_spinning_worker));
        }

    private:
        scheduler_tuning_parameters parameters_;
        scheduling_policy policy_;
        std::unique_ptr<scheduler_tuner> tuner_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
