//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/thread_support.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

// What a tuner can see of a running scheduler. A tuner decides the values of
// the tunable settings of a scheduler (see scheduler_tuning_parameters); to
// do that from what the scheduler does instead of from fixed values, it reads
// these counters from time to time (see scheduler_tuner::update) and compares
// them with its earlier reading. The scheduler counts where it makes its
// decisions:
//
// - which threads it gets, runs and steals, and of which size (task_size):
//   the share of sized threads decides how much the rules that depend on the
//   size matter, today those for waking up workers, later others;
// - whom it wakes up for a new thread: by the rules for threads up to
//   hpx.scheduler_policy.wake_one_max_size (nano and small by default, see
//   wake_action), or as before by the creator of the thread; and how often
//   that wakes up a sleeping worker, in particular while another worker is
//   awake and could have run the thread (what a placement that prefers awake
//   workers would save);
// - how its workers' idle periods end (spinning or sleeping, finding work or
//   not): what the backoff and spin settings decide;
// - how often its workers steal.
//
// A rule that comes to depend on the size adds counters of its own; the
// names of all counters are checked against this list
// (get_scheduler_counter_name).
//
// Only a scheduler with a tuner counts (tuned_scheduling_rules), the default
// scheduler does not. Nothing is counted inside a spin round, and no clock is
// read.

namespace hpx::threads::policies {

    /// The events a scheduler with tunable settings counts for its tuner (see
    /// scheduler_tuning_counters).
    HPX_CXX_CORE_EXPORT enum class scheduler_counter : std::uint8_t {
        /// threads created, by task size (unknown, nano, small, medium,
        /// large; see created_of)
        created_unknown,
        created_nano,
        created_small,
        created_medium,
        created_large,

        /// threads handed to a worker to run, by task size (see run_of)
        run_unknown,
        run_nano,
        run_small,
        run_medium,
        run_large,

        /// the decisions for new threads up to
        /// hpx.scheduler_policy.wake_one_max_size (see wake_action)
        wake_none,
        wake_target_if_sleeping,
        wake_one,
        wake_all,

        /// new threads the scheduler leaves to their creator (unsized ones
        /// and those above hpx.scheduler_policy.wake_one_max_size):
        /// create_thread and create_work call do_some_work for them, which
        /// wakes up sleeping workers as before; counted as decisions, not as
        /// wake ups
        wake_by_creator,

        /// a wake up by the rules (wake_target_if_sleeping, wake_one) that
        /// found a worker asleep
        woke_sleeper,

        /// a new thread woke up the sleeping worker it was queued for while
        /// another worker was spinning, or while the sentinel spun
        sleeper_woken_while_spinning,
        sleeper_woken_with_sentinel,

        /// how a worker's idle periods ended: while spinning it saw work in
        /// its own queues, or work to steal, or the spin ran out; it spun as
        /// the sentinel; before waiting it saw work; its wait was ended by a
        /// notification, or ran out
        idle_spin_own_work,
        idle_spin_work_to_steal,
        idle_spin_ran_out,
        idle_sentinel,
        idle_work_before_wait,
        idle_wait_notified,
        idle_wait_timed_out,

        /// threads stolen from the pending queues, by task size (see
        /// stolen_of)
        stolen_unknown,
        stolen_nano,
        stolen_small,
        stolen_medium,
        stolen_large,

        /// steals from the staged queues that got threads (converted before
        /// the threads exist, so not by size)
        steal_staged,

        count
    };

    namespace detail {

        // The counter for the given size in the group of five counters that
        // starts with unknown (unknown, nano, small, medium, large). The
        // size comes from the 3 bits of a schedule hint, which can hold more
        // values than there are sizes; an invalid one is counted as unknown,
        // so that it can't count another event or write past the counters.
        HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr scheduler_counter
        counter_of_size(scheduler_counter const unknown,
            threads::task_size const size) noexcept
        {
            HPX_ASSERT(size <= threads::task_size::large);
            std::uint8_t const offset = size <= threads::task_size::large ?
                static_cast<std::uint8_t>(size) :
                static_cast<std::uint8_t>(threads::task_size::unknown);
            return static_cast<scheduler_counter>(
                static_cast<std::uint8_t>(unknown) + offset);
        }
    }    // namespace detail

    /// The counter of threads created of the given size.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr scheduler_counter created_of(
        threads::task_size const size) noexcept
    {
        return detail::counter_of_size(
            scheduler_counter::created_unknown, size);
    }

    /// The counter of threads run of the given size.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr scheduler_counter run_of(
        threads::task_size const size) noexcept
    {
        return detail::counter_of_size(scheduler_counter::run_unknown, size);
    }

    /// The counter of threads stolen of the given size.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr scheduler_counter stolen_of(
        threads::task_size const size) noexcept
    {
        return detail::counter_of_size(scheduler_counter::stolen_unknown, size);
    }

    // each group of counters by size has one counter for every size
    static_assert(static_cast<std::uint8_t>(scheduler_counter::created_large) -
            static_cast<std::uint8_t>(scheduler_counter::created_unknown) ==
        static_cast<std::uint8_t>(threads::task_size::large));
    static_assert(static_cast<std::uint8_t>(scheduler_counter::run_large) -
            static_cast<std::uint8_t>(scheduler_counter::run_unknown) ==
        static_cast<std::uint8_t>(threads::task_size::large));
    static_assert(static_cast<std::uint8_t>(scheduler_counter::stolen_large) -
            static_cast<std::uint8_t>(scheduler_counter::stolen_unknown) ==
        static_cast<std::uint8_t>(threads::task_size::large));

    /// The name of the given counter, as used in the records of a tuner.
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT char const*
    get_scheduler_counter_name(scheduler_counter c) noexcept;

    /// Counters of the events of a scheduler, kept per worker. A worker's
    /// counters are written only by that worker, with relaxed loads and
    /// stores on lines of their own, so counting costs no atomic
    /// read-modify-write and no line shared with other workers. Events of
    /// threads that are not workers of the scheduler's pool (a thread created
    /// from outside the pool) go to one more set of counters, which is shared
    /// and updated atomically. A tuner reads the counters while they are
    /// written, so what it reads is approximate.
    ///
    /// The counters are 32 bits wide and wrap around: a tuner compares two
    /// readings (the later minus the earlier one, as a std::uint32_t), which
    /// is right as long as fewer than 2^32 events of a kind happen between
    /// them. A tuner that wants the totals since the start adds up these
    /// differences itself (see recording_tuner).
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT scheduler_tuning_counters
    {
    public:
        /// The number of counters.
        static constexpr std::size_t size =
            static_cast<std::size_t>(scheduler_counter::count);

        /// The index of the shared counters for threads that are not workers
        /// of the pool.
        static constexpr std::size_t external = static_cast<std::size_t>(-1);

        explicit scheduler_tuning_counters(std::size_t num_workers);

        // owned by the scheduler's rules, which hand out references to them
        scheduler_tuning_counters(scheduler_tuning_counters const&) = delete;
        scheduler_tuning_counters(scheduler_tuning_counters&&) = delete;
        scheduler_tuning_counters& operator=(
            scheduler_tuning_counters const&) = delete;
        scheduler_tuning_counters& operator=(
            scheduler_tuning_counters&&) = delete;

        /// The number of workers the counters are kept for.
        [[nodiscard]] std::size_t num_workers() const noexcept
        {
            return values_.size();
        }

        /// Count an event of the given worker, or of a thread that is not a
        /// worker (any worker number out of range, see external).
        void add(std::size_t const worker, scheduler_counter const c,
            std::uint32_t const n = 1) noexcept
        {
            auto const i = static_cast<std::size_t>(c);
            HPX_ASSERT(i < size);
            if (worker < values_.size())
            {
                // only this worker writes its counters
                auto& value = values_[worker].data_[i];
                value.store(value.load(std::memory_order_relaxed) + n,
                    std::memory_order_relaxed);
            }
            else
            {
                external_.data_[i].fetch_add(n, std::memory_order_relaxed);
            }
        }

        /// The value of the given counter of the given worker, or of the
        /// shared counters (external); it wraps around (see above).
        [[nodiscard]] std::uint32_t get(
            std::size_t const worker, scheduler_counter const c) const noexcept
        {
            auto const i = static_cast<std::size_t>(c);
            HPX_ASSERT(i < size);
            return worker < values_.size() ?
                values_[worker].data_[i].load(std::memory_order_relaxed) :
                external_.data_[i].load(std::memory_order_relaxed);
        }

    private:
        using counter_values = std::array<std::atomic<std::uint32_t>, size>;
        std::vector<util::aligned_line_data<counter_values>> values_;
        util::aligned_line_data<counter_values> external_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
