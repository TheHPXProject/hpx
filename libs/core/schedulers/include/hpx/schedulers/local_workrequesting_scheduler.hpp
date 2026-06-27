//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/affinity.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/schedulers/lockfree_queue_backends.hpp>
#include <hpx/schedulers/thread_queue.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
// The scheduler implemented here is adapted from the ideas described in chapter
// 3 of Andreas Prell's PhD thesis 'Embracing Explicit Communication in
// Work-Stealing Runtime Systems' (see: https://epub.uni-bayreuth.de/2990/).
// While it's being described as a work-stealing scheduler, it relies on a
// different working principle if compared to the classic work-stealing. Instead
// of actively pulling work from the work queues of neighboring cores it relies
// on a push model. Cores that run out of work post steal requests that are
// handled by cores that have work available by actively sending tasks to the
// requesting core.
//
// When a worker runs out of tasks, it becomes a thief by sending steal requests
// to selected victim workers, those either reply with tasks or signal that they
// have no tasks left. A steal request is a message containing the thief's ID, a
// reference to a channel for sending tasks from victim to thief, and other
// information needed for thread coordination.
//
// When the runtime system starts up, every worker allocates two channels: a
// channel for receiving steal requests and a channel for receiving tasks. A
// reference to the latter is stored in steal requests, and workers use this
// reference to send tasks. By "owning" two channels, workers are able to
// receive steal requests and tasks independently of other workers, which in
// turn enables efficient channel implementations based on single-consumer
// queues. The total number of channels grows linearly with the number of
// workers: n workers allocate 2n channels to communicate with each other.
//
// Matching traditional work stealing, we allow one outstanding steal request
// per worker. This decision has two important consequences: (1) The number of
// steal requests is bounded by n, the number of workers. (2) A thief will never
// receive tasks from more than one victim at a time. It follows from (1) that a
// channel capacity of n - 1 is sufficient to deal with other workers' steal
// requests since no more than n - 1 thieves may request tasks from a single
// victim. We actually increase the capacity to n so that steal requests can be
// returned to their senders, for instance, in case of repeated failure. (2)
// implies that, at any given time, a task channel has at most one sender and
// one receiver, meeting the requirements for an SPSC implementation.
//
// In summary, every worker allocates two specialized channels: an MPSC channel
// where it receives steal requests and an SPSC channel where it receives tasks.
// Steal requests are forwarded rather than acknowledged, letting workers steal
// on behalf of others upon receiving steal requests that cannot be handled.
// Random victim selection fits in well with forwarding steal requests, but may
// cause a lot of communication if only few workers have tasks left. Stealing
// half of a victim's tasks (steal-half) is straightforward to implement with
// private task queues, especially when shared memory is available, in which
// case tasks do not need to be copied. While steal-half is important to tackle
// fine-grained parallelism, polling is necessary to achieve short message
// handling delays when workers schedule long-running tasks.

namespace hpx::threads::policies {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    HPX_CXX_CORE_EXPORT using default_local_workrequesting_scheduler_terminated_queue =
        lockfree_lifo;
#else
    HPX_CXX_CORE_EXPORT using default_local_workrequesting_scheduler_terminated_queue =
        lockfree_fifo;
#endif

    namespace detail {

        ////////////////////////////////////////////////////////////////////////
        inline unsigned int random_seed() noexcept
        {
            static std::random_device rd;
            return rd();
        }

        ////////////////////////////////////////////////////////////////////////
        struct workrequesting_init_parameter
        {
            workrequesting_init_parameter(std::size_t num_queues,
                detail::affinity_data const& affinity_data,
                std::size_t num_high_priority_queues = static_cast<std::size_t>(
                    -1),
                thread_queue_init_parameters const& thread_queue_init =
                    thread_queue_init_parameters{},
                char const* description = "local_workrequesting_scheduler")
              : num_queues_(num_queues)
              , num_high_priority_queues_(
                    num_high_priority_queues == static_cast<std::size_t>(-1) ?
                        num_queues :
                        num_high_priority_queues)
              , thread_queue_init_(thread_queue_init)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            workrequesting_init_parameter(std::size_t num_queues,
                detail::affinity_data const& affinity_data,
                char const* description)
              : num_queues_(num_queues)
              , num_high_priority_queues_(num_queues)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            std::size_t num_queues_;
            std::size_t num_high_priority_queues_;
            thread_queue_init_parameters thread_queue_init_;
            detail::affinity_data const& affinity_data_;
            char const* description_;
        };

        struct workrequesting_task_data
        {
            explicit workrequesting_task_data(
                std::uint16_t num_thread = static_cast<std::uint16_t>(
                    -1)) noexcept
              : num_thread_(num_thread)
            {
            }

            // core number this task data originated from
            std::uint16_t num_thread_;
            hpx::detail::small_vector<thread_id_ref_type, 1> tasks_;
        };

        using workrequesting_task_channel =
            lcos::local::channel_spsc<workrequesting_task_data,
                lcos::local::channel_mode::dont_support_close>;

        ////////////////////////////////////////////////////////////////////////
        struct workrequesting_steal_request
        {
            enum class state : std::uint16_t
            {
                working = 0,
                idle = 2,
                failed = 4
            };

            workrequesting_steal_request() = default;

            workrequesting_steal_request(std::size_t const num_thread,
                workrequesting_task_channel* channel, mask_cref_type victims,
                bool idle, bool const stealhalf)
              : channel_(channel)
              , victims_(victims)
              , num_thread_(static_cast<std::uint16_t>(num_thread))
              , attempt_(static_cast<std::uint16_t>(count(victims) - 1))
              , state_(idle ? state::idle : state::working)
              , stealhalf_(stealhalf)
            {
            }

            workrequesting_task_channel* channel_ = nullptr;
            mask_type victims_ = mask_type();
            std::uint16_t num_thread_ = static_cast<std::uint16_t>(-1);
            std::uint16_t attempt_ = 0;
            state state_ = state::failed;
            // true ? attempt steal-half : attempt steal-one
            bool stealhalf_ = true;
        };

        using workrequesting_steal_request_channel =
            lcos::local::channel_mpsc<workrequesting_steal_request,
                lcos::local::channel_mode::dont_support_close>;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // The local_workrequesting_scheduler maintains several queues of work
    // items (threads) per OS thread, where this OS thread pulls its next work
    // from.
    HPX_CXX_CORE_EXPORT template <typename Mutex = std::mutex,
        typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = concurrentqueue_fifo,
        typename TerminatedQueuing =
            default_local_workrequesting_scheduler_terminated_queue>
    class local_workrequesting_scheduler final : public scheduler_base
    {
    public:
        using has_periodic_maintenance = std::false_type;
        using thread_queue_type = thread_queue<Mutex, PendingQueuing,
            StagedQueuing, TerminatedQueuing>;
        using init_parameter_type = detail::workrequesting_init_parameter;

    private:
        ////////////////////////////////////////////////////////////////////////
        using task_data = detail::workrequesting_task_data;
        using task_channel = detail::workrequesting_task_channel;

        using steal_request = detail::workrequesting_steal_request;
        using steal_request_channel =
            detail::workrequesting_steal_request_channel;

        ////////////////////////////////////////////////////////////////////////
        struct scheduler_data
        {
            scheduler_data() = default;

            scheduler_data(scheduler_data const&) = delete;
            scheduler_data(scheduler_data&&) = delete;
            scheduler_data& operator=(scheduler_data const&) = delete;
            scheduler_data& operator=(scheduler_data&&) = delete;

            ~scheduler_data()
            {
                delete queue_;
                delete high_priority_queue_;
                delete bound_queue_;
                delete requests_;
                delete tasks_;
            }

            // interval at which we re-decide on whether we should steal just
            // one task or half of what's available
            static constexpr std::uint16_t num_steal_adaptive_interval_ = 25;

            void init(std::size_t num_thread, std::size_t size,
                thread_queue_init_parameters const& queue_init,
                bool need_high_priority_queue)
            {
                if (queue_ == nullptr)
                {
                    num_thread_ = static_cast<std::uint16_t>(num_thread);

                    // initialize queues
                    queue_ = new thread_queue_type(queue_init);
                    if (need_high_priority_queue)
                    {
                        high_priority_queue_ =
                            new thread_queue_type(queue_init);
                    }
                    bound_queue_ = new thread_queue_type(queue_init);

                    // initialize channels needed for work stealing
                    requests_ = new steal_request_channel(size);
                    tasks_ = new task_channel(1);
                }
            }

            // initial affinity mask for this core
            mask_type victims_ = mask_type();

            // queues for threads scheduled on this core
            thread_queue_type* queue_ = nullptr;
            thread_queue_type* high_priority_queue_ = nullptr;
            thread_queue_type* bound_queue_ = nullptr;

            // channel for posting steal requests to this core (use
            // hpx::spinlock)
            steal_request_channel* requests_ = nullptr;

            // one channel per steal request per core
            task_channel* tasks_ = nullptr;

            // the number of outstanding steal requests
            std::uint16_t requested_ = 0;

            // core number this scheduler data instance refers to
            std::uint16_t num_thread_ = static_cast<std::uint16_t>(-1);

            // adaptive stealing
            std::uint16_t num_recent_steals_ = 0;
            std::uint16_t num_recent_tasks_executed_ = 0;
            bool stealhalf_ = true;

#if defined(HPX_HAVE_WORKREQUESTING_LAST_VICTIM)
            // core number the last stolen tasks originated from
            std::uint16_t last_victim_ = static_cast<std::uint16_t>(-1);
#endif
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
            // collect some statistics
            std::uint32_t steal_requests_sent_ = 0;
            std::uint32_t steal_requests_received_ = 0;
            std::uint32_t steal_requests_discarded_ = 0;
#endif
        };

    public:
        explicit local_workrequesting_scheduler(init_parameter_type const& init,
            bool deferred_initialization = true)
          : scheduler_base(init.num_queues_, init.description_,
                init.thread_queue_init_,
                policies::scheduler_mode::fast_idle_mode)
          , data_(init.num_queues_)
          , low_priority_queue_(thread_queue_init_)
          , curr_queue_(0)
          , gen_(detail::random_seed())
          , affinity_data_(init.affinity_data_)
          , num_queues_(init.num_queues_)
          , num_high_priority_queues_(init.num_high_priority_queues_)
        {
            HPX_ASSERT(init.num_queues_ != 0);
            HPX_ASSERT(num_high_priority_queues_ != 0);
            HPX_ASSERT(num_high_priority_queues_ <= num_queues_);

            if (!deferred_initialization)
            {
                for (std::size_t i = 0; i != init.num_queues_; ++i)
                {
                    data_[i].data_.init(i, init.num_queues_,
                        this->thread_queue_init_,
                        i < num_high_priority_queues_);
                }
            }
        }

        local_workrequesting_scheduler(
            local_workrequesting_scheduler const&) = delete;
        local_workrequesting_scheduler(
            local_workrequesting_scheduler&&) = delete;
        local_workrequesting_scheduler& operator=(
            local_workrequesting_scheduler const&) = delete;
        local_workrequesting_scheduler& operator=(
            local_workrequesting_scheduler&&) = delete;

        ~local_workrequesting_scheduler() override = default;

        static constexpr std::string_view get_scheduler_name() noexcept
        {
            return "local_workrequesting_scheduler";
        }

#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto const& d = data_[i].data_;
                if (i < num_high_priority_queues_)
                {
                    time += d.high_priority_queue_->get_creation_time(reset);
                }
                time += d.queue_->get_creation_time(reset);
                time += d.bound_queue_->get_creation_time(reset);
            }

            return time + low_priority_queue_.get_creation_time(reset);
        }

        std::uint64_t get_cleanup_time(bool reset) override
        {
            std::uint64_t time = 0;

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto const& d = data_[i].data_;
                if (i < num_high_priority_queues_)
                {
                    time += d.high_priority_queue_->get_cleanup_time(reset);
                }
                time += d.queue_->get_cleanup_time(reset);
                time += d.bound_queue_->get_cleanup_time(reset);
            }

            return time + low_priority_queue_.get_cleanup_time(reset);
        }
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count += d.high_priority_queue_->get_num_pending_misses(
                            reset);
                    }
                    count += d.queue_->get_num_pending_misses(reset);
                    count += d.bound_queue_->get_num_pending_misses(reset);
                }

                return count +
                    low_priority_queue_.get_num_pending_misses(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count += d.high_priority_queue_->get_num_pending_misses(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_pending_misses(reset);
            }
            count += d.queue_->get_num_pending_misses(reset);
            return count + d.bound_queue_->get_num_pending_misses(reset);
        }

        std::int64_t get_num_pending_accesses(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_num_pending_accesses(
                                reset);
                    }
                    count += d.queue_->get_num_pending_accesses(reset);
                    count += d.bound_queue_->get_num_pending_accesses(reset);
                }

                return count +
                    low_priority_queue_.get_num_pending_accesses(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count +=
                    d.high_priority_queue_->get_num_pending_accesses(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_pending_accesses(reset);
            }
            count += d.queue_->get_num_pending_accesses(reset);
            return count + d.bound_queue_->get_num_pending_accesses(reset);
        }

        std::int64_t get_num_stolen_from_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_num_stolen_from_pending(
                                reset);
                    }
                    count += d.queue_->get_num_stolen_from_pending(reset);
                    count += d.bound_queue_->get_num_stolen_from_pending(reset);
                }

                return count +
                    low_priority_queue_.get_num_stolen_from_pending(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count +=
                    d.high_priority_queue_->get_num_stolen_from_pending(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_stolen_from_pending(reset);
            }
            count += d.queue_->get_num_stolen_from_pending(reset);
            return count + d.bound_queue_->get_num_stolen_from_pending(reset);
        }

        std::int64_t get_num_stolen_to_pending(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_num_stolen_to_pending(
                                reset);
                    }
                    count += d.queue_->get_num_stolen_to_pending(reset);
                    count += d.bound_queue_->get_num_stolen_to_pending(reset);
                }

                return count +
                    low_priority_queue_.get_num_stolen_to_pending(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count +=
                    d.high_priority_queue_->get_num_stolen_to_pending(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_stolen_to_pending(reset);
            }
            count += d.queue_->get_num_stolen_to_pending(reset);
            return count + d.bound_queue_->get_num_stolen_to_pending(reset);
        }

        std::int64_t get_num_stolen_from_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_num_stolen_from_staged(
                                reset);
                    }
                    count += d.queue_->get_num_stolen_from_staged(reset);
                    count += d.bound_queue_->get_num_stolen_from_staged(reset);
                }

                return count +
                    low_priority_queue_.get_num_stolen_from_staged(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count +=
                    d.high_priority_queue_->get_num_stolen_from_staged(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_stolen_from_staged(reset);
            }
            count += d.queue_->get_num_stolen_from_staged(reset);
            return count + d.bound_queue_->get_num_stolen_from_staged(reset);
        }

        std::int64_t get_num_stolen_to_staged(
            std::size_t num_thread, bool reset) override
        {
            std::int64_t count = 0;
            if (num_thread == std::size_t(-1))
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_num_stolen_to_staged(
                                reset);
                    }
                    count += d.queue_->get_num_stolen_to_staged(reset);
                    count += d.bound_queue_->get_num_stolen_to_staged(reset);
                }

                return count +
                    low_priority_queue_.get_num_stolen_to_staged(reset);
            }

            auto const& d = data_[num_thread].data_;
            if (num_thread < num_high_priority_queues_)
            {
                count +=
                    d.high_priority_queue_->get_num_stolen_to_staged(reset);
            }
            if (num_thread == num_queues_ - 1)
            {
                count += low_priority_queue_.get_num_stolen_to_staged(reset);
            }
            count += d.queue_->get_num_stolen_to_staged(reset);
            return count + d.bound_queue_->get_num_stolen_to_staged(reset);
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        void abort_all_suspended_threads() override
        {
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                data_[i].data_.queue_->abort_all_suspended_threads();
                data_[i].data_.bound_queue_->abort_all_suspended_threads();
            }
        }

        ///////////////////////////////////////////////////////////////////////
        bool cleanup_terminated(bool delete_all) override
        {
            bool empty = true;
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto& d = data_[i].data_;
                if (i < num_high_priority_queues_)
                {
                    empty = d.high_priority_queue_->cleanup_terminated(
                                delete_all) &&
                        empty;
                }
                empty = d.queue_->cleanup_terminated(delete_all) && empty;
                empty = d.bound_queue_->cleanup_terminated(delete_all) && empty;
            }
            return low_priority_queue_.cleanup_terminated(delete_all) && empty;
        }

        bool cleanup_terminated(
            std::size_t num_thread, bool delete_all) override
        {
            auto& d = data_[num_thread].data_;
            bool empty = d.queue_->cleanup_terminated(delete_all);
            empty = d.bound_queue_->cleanup_terminated(delete_all) && empty;
            if (!delete_all)
                return empty;

            if (num_thread < num_high_priority_queues_)
            {
                empty =
                    d.high_priority_queue_->cleanup_terminated(delete_all) &&
                    empty;
            }

            if (num_thread == num_queues_ - 1)
            {
                return low_priority_queue_.cleanup_terminated(delete_all) &&
                    empty;
            }
            return empty;
        }

        ///////////////////////////////////////////////////////////////////////
        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(thread_init_data& data, thread_id_ref_type* id,
            error_code& ec) override
        {
            std::size_t num_thread =
                data.schedulehint.mode == thread_schedule_hint_mode::thread ?
                data.schedulehint.hint :
                static_cast<std::size_t>(-1);

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }

            num_thread = select_active_pu(num_thread);

            data.schedulehint.schedule_hint(
                static_cast<std::int16_t>(num_thread));

            // now create the thread
            switch (data.priority)
            {
            case thread_priority::high_recursive:
            case thread_priority::high:
            case thread_priority::boost:
            {
                if (data.priority == thread_priority::boost)
                {
                    data.priority = thread_priority::normal;
                }

                if (num_thread >= num_high_priority_queues_)
                {
                    num_thread %= num_high_priority_queues_;
                }

                // we never stage high priority threads, so there is no need to
                // call wait_or_add_new for those.
                data_[num_thread].data_.high_priority_queue_->create_thread(
                    data, id, ec);
                break;
            }

            case thread_priority::low:
                low_priority_queue_.create_thread(data, id, ec);
                break;

            case thread_priority::initially_bound:
            case thread_priority::bound:
                HPX_ASSERT(num_thread < num_queues_);
                data_[num_thread].data_.bound_queue_->create_thread(
                    data, id, ec);
                break;

            case thread_priority::default_:
            case thread_priority::normal:
                HPX_ASSERT(num_thread < num_queues_);
                data_[num_thread].data_.queue_->create_thread(data, id, ec);
                break;

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_workrequesting_scheduler::create_thread",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
        }

        // Retrieve the next viable steal request from our channel
        bool try_receiving_steal_request(
            scheduler_data& d, steal_request& req) noexcept
        {
            bool ret = d.requests_->get(&req);

            // 26813: Use 'bitwise and' to check if a flag is set.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26813)
#endif
            while (ret && req.state_ == steal_request::state::failed)
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
            {
                // forget the received steal request
                --data_[req.num_thread_].data_.requested_;

                // there should have been exactly one outstanding steal request
                HPX_ASSERT(data_[req.num_thread_].data_.requested_ == 0);

                // try to retrieve next steal request
                ret = d.requests_->get(&req);
            }

            // No special treatment for other states
            HPX_ASSERT(
                (ret && req.state_ != steal_request::state::failed) || !ret);

            return ret;
        }

        // Pass steal request on to another worker. Returns true if we have
        // handled our own steal request.
        bool decline_or_forward_steal_request(
            scheduler_data& d, steal_request& req) noexcept
        {
            HPX_ASSERT(req.attempt_ < num_queues_);

            if (req.num_thread_ == d.num_thread_)
            {
                // Steal request was either returned by another worker or
                // picked up by us.

                // 26813: Use 'bitwise and' to check if a flag is set.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26813)
#endif
                if (req.state_ == steal_request::state::idle ||
                    d.queue_->get_pending_queue_length(
                        std::memory_order_relaxed) != 0)
                {
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
                    ++d.steal_requests_discarded_;
#endif
                    // we have work now, drop this steal request
                    --d.requested_;

                    // there should have been exactly one outstanding steal
                    // request
                    HPX_ASSERT(d.requested_ == 0);
                }
                else
                {
                    // Continue circulating the steal request if it makes sense
                    req.state_ = steal_request::state::idle;
                    req.victims_ = d.victims_;
                    req.attempt_ =
                        static_cast<std::uint16_t>(count(d.victims_) - 1);

                    std::size_t victim = next_victim(d, req);
                    data_[victim].data_.requests_->set(HPX_MOVE(req));
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
                    ++d.steal_requests_sent_;
#endif
                }

                return true;
            }

            // send this steal request on to the next (random) core
            ++req.attempt_;
            set(req.victims_, d.num_thread_);    // don't ask a core twice

            HPX_ASSERT(req.attempt_ == count(req.victims_) - 1);

            std::size_t victim = next_victim(d, req);
            data_[victim].data_.requests_->set(HPX_MOVE(req));
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
            ++d.steal_requests_sent_;
#endif
            return false;
        }

        // decline_or_forward_one_steal_requests is only called when a worker
        // has nothing else to do but to relay steal requests, which means the
        // worker is idle.
        void decline_or_forward_one_steal_requests(scheduler_data& d) noexcept
        {
            if (!d.requests_->is_empty())
            {
                steal_request req;
                if (try_receiving_steal_request(d, req))
                {
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
                    ++d.steal_requests_received_;
#endif
                    decline_or_forward_steal_request(d, req);
                }
            }
        }

        // Handle a steal request by sending tasks in return or passing it on to
        // another worker. Returns true if the request was satisfied.
        bool handle_steal_request(
            scheduler_data& d, steal_request& req) noexcept
        {
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
            ++d.steal_requests_received_;
#endif
            if (req.num_thread_ == d.num_thread_)
            {
                // got back our own steal request.
                HPX_ASSERT(req.state_ != steal_request::state::failed);

                // Defer the decision to decline_steal_request
                decline_or_forward_steal_request(d, req);
                return false;
            }

            // Send tasks from our queue to the requesting core, depending on
            // what's requested, either one task or half of the available tasks
            std::size_t max_num_to_steal = 1;
            if (req.stealhalf_)
            {
                max_num_to_steal = d.queue_->get_pending_queue_length(
                                       std::memory_order_relaxed) /
                    2;
            }

            if (max_num_to_steal != 0)
            {
                task_data thrds(d.num_thread_);
                thrds.tasks_.reserve(max_num_to_steal);

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                thread_id_ref_type thrd;
                while (max_num_to_steal-- != 0 &&
                    d.queue_->get_next_thread(thrd, false, true))
                {
                    d.queue_->increment_num_stolen_from_pending();
                    thrds.tasks_.push_back(HPX_MOVE(thrd));
                    thrd = thread_id_ref_type{};
                }
#else
                d.queue_->get_next_threads(
                    thrds.tasks_.begin(), thrds.tasks_.size(), false, true);
#endif

                // we are ready to send at least one task
                if (!thrds.tasks_.empty())
                {
                    // send these tasks to the core that has sent the steal
                    // request
                    req.channel_->set(HPX_MOVE(thrds));

                    // wake the thread up so that it can pick up the stolen
                    // tasks
                    do_some_work(req.num_thread_);

                    return true;
                }
            }

            // There's nothing we can do with this steal request except pass
            // it on to a different worker
            decline_or_forward_steal_request(d, req);
            return false;
        }

        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(std::size_t num_thread, bool running,
            thread_id_ref_type& thrd, bool allow_stealing)
        {
            HPX_ASSERT(num_thread < num_queues_);

            auto const get_thread = [](thread_queue_type* this_queue,
                                        thread_id_ref_type& thrd) {
                bool result = false;
                if (this_queue->get_pending_queue_length(
                        std::memory_order_relaxed) != 0)
                {
                    result = this_queue->get_next_thread(thrd);
#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                    this_queue->increment_num_pending_accesses();
                    if (!result)
                        this_queue->increment_num_pending_misses();
#endif
                }
                return result;
            };

            auto& d = data_[num_thread].data_;

            if (num_thread < num_high_priority_queues_ &&
                get_thread(d.high_priority_queue_, thrd))
            {
                ++d.num_recent_tasks_executed_;
                return true;
            }

            if (get_thread(d.bound_queue_, thrd) || get_thread(d.queue_, thrd))
            {
                // We found a task to run, however before running it we handle
                // steal requests (assuming that there is more work left that
                // could be used to satisfy steal requests).
                if (!d.requests_->is_empty())
                {
                    if (allow_stealing)
                    {
                        steal_request req;
                        while (try_receiving_steal_request(d, req))
                        {
                            if (!handle_steal_request(d, req))
                                break;
                        }
                    }
                    else
                    {
                        decline_or_forward_one_steal_requests(d);
                    }
                }

                ++d.num_recent_tasks_executed_;
                return true;
            }

            // Give up if we have work to convert.
            if (d.queue_->get_staged_queue_length(std::memory_order_relaxed) !=
                    0 ||
                !running)
            {
                return false;
            }

            if (num_thread == num_queues_ - 1 &&
                get_thread(&low_priority_queue_, thrd))
            {
                ++d.num_recent_tasks_executed_;
                return true;
            }

            return false;
        }

        // Schedule the passed thread
        void schedule_thread(thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) override
        {
            auto num_thread = static_cast<std::size_t>(-1);
            if (schedulehint.mode == thread_schedule_hint_mode::thread)
            {
                num_thread = schedulehint.hint;
            }
            else
            {
                allow_fallback = false;
            }

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            HPX_ASSERT(get_thread_id_data(thrd)->get_scheduler_base() == this);
            HPX_ASSERT(num_thread < num_queues_);

            switch (priority)
            {
            case thread_priority::high_recursive:
            case thread_priority::high:
            case thread_priority::boost:
            {
                std::size_t num = num_thread;
                if (num >= num_high_priority_queues_)
                {
                    num %= num_high_priority_queues_;
                }

                data_[num].data_.high_priority_queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
                break;
            }

            case thread_priority::low:
                low_priority_queue_.schedule_thread(HPX_MOVE(thrd));
                break;

            case thread_priority::default_:
            case thread_priority::normal:
                data_[num_thread].data_.queue_->schedule_thread(HPX_MOVE(thrd));
                break;

            case thread_priority::initially_bound:
            case thread_priority::bound:
                data_[num_thread].data_.bound_queue_->schedule_thread(
                    HPX_MOVE(thrd));
                break;

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_workrequesting_scheduler::schedule_thread",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
        }

        void schedule_thread_last(thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) override
        {
            auto num_thread = static_cast<std::size_t>(-1);
            if (schedulehint.mode == thread_schedule_hint_mode::thread)
            {
                num_thread = schedulehint.hint;
            }
            else
            {
                allow_fallback = false;
            }

            if (static_cast<std::size_t>(-1) == num_thread)
            {
                num_thread = curr_queue_++ % num_queues_;
            }
            else if (num_thread >= num_queues_)
            {
                num_thread %= num_queues_;
            }

            num_thread = select_active_pu(num_thread, allow_fallback);

            HPX_ASSERT(get_thread_id_data(thrd)->get_scheduler_base() == this);
            HPX_ASSERT(num_thread < num_queues_);

            switch (priority)
            {
            case thread_priority::high_recursive:
            case thread_priority::high:
            case thread_priority::boost:
            {
                std::size_t num = num_thread;
                if (num >= num_high_priority_queues_)
                {
                    num %= num_high_priority_queues_;
                }

                data_[num].data_.high_priority_queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
                break;
            }

            case thread_priority::low:
                low_priority_queue_.schedule_thread(HPX_MOVE(thrd), true);
                break;

            case thread_priority::default_:
            case thread_priority::normal:
                data_[num_thread].data_.queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
                break;

            case thread_priority::initially_bound:
            case thread_priority::bound:
                data_[num_thread].data_.bound_queue_->schedule_thread(
                    HPX_MOVE(thrd), true);
                break;

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_workrequesting_scheduler::schedule_thread_last",
                    "unknown thread priority value (thread_priority::unknown)");
            }
            }
        }

        /// Destroy the passed thread as it has been terminated
        void destroy_thread(threads::thread_data* thrd) override
        {
            HPX_ASSERT(thrd->get_scheduler_base() == this);
            thrd->get_queue<thread_queue_type>().destroy_thread(thrd);
        }

        ///////////////////////////////////////////////////////////////////////
        // This returns the current length of the queues (work items and new
        // items)
        std::int64_t get_queue_length(std::size_t num_thread) const override
        {
            // Return queue length of one specific queue.
            std::int64_t count = 0;
            if (static_cast<std::size_t>(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < num_queues_);
                auto const& d = data_[num_thread].data_;

                if (num_thread < num_high_priority_queues_)
                {
                    count += d.high_priority_queue_->get_queue_length();
                }
                if (num_thread == num_queues_ - 1)
                {
                    count += low_priority_queue_.get_queue_length();
                }
                count += d.queue_->get_queue_length();
                return count + d.bound_queue_->get_queue_length();
            }

            // Cumulative queue lengths of all queues.
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto const& d = data_[i].data_;
                if (i < num_high_priority_queues_)
                {
                    count += d.high_priority_queue_->get_queue_length();
                }
                count += d.queue_->get_queue_length();
                count += d.bound_queue_->get_queue_length();
            }
            return count + low_priority_queue_.get_queue_length();
        }

        ///////////////////////////////////////////////////////////////////////
        // Queries the current thread count of the queues.
        std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority priority = thread_priority::default_,
            std::size_t num_thread = static_cast<std::size_t>(-1),
            bool /* reset */ = false) const override
        {
            // Return thread count of one specific queue.
            std::int64_t count = 0;
            if (static_cast<std::size_t>(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < num_queues_);

                auto const& d = data_[num_thread].data_;
                switch (priority)
                {
                case thread_priority::default_:
                {
                    if (num_thread < num_high_priority_queues_)
                    {
                        count = d.high_priority_queue_->get_thread_count(state);
                    }
                    if (num_thread == num_queues_ - 1)
                    {
                        count += low_priority_queue_.get_thread_count(state);
                    }
                    count += d.queue_->get_thread_count(state);
                    return count + d.bound_queue_->get_thread_count(state);
                }

                case thread_priority::low:
                {
                    if (num_queues_ - 1 == num_thread)
                        return low_priority_queue_.get_thread_count(state);
                    break;
                }

                case thread_priority::normal:
                    return d.queue_->get_thread_count(state);

                case thread_priority::initially_bound:
                case thread_priority::bound:
                    return d.bound_queue_->get_thread_count(state);

                case thread_priority::boost:
                case thread_priority::high:
                case thread_priority::high_recursive:
                {
                    if (num_thread < num_high_priority_queues_)
                    {
                        return d.high_priority_queue_->get_thread_count(state);
                    }
                    break;
                }

                case thread_priority::unknown:
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "local_workrequesting_scheduler::get_thread_count",
                        "unknown thread priority value "
                        "(thread_priority::unknown)");
                }
                }
                return 0;
            }

            // Return the cumulative count for all queues.
            switch (priority)
            {
            case thread_priority::default_:
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    auto const& d = data_[i].data_;
                    if (i < num_high_priority_queues_)
                    {
                        count +=
                            d.high_priority_queue_->get_thread_count(state);
                    }
                    count += d.queue_->get_thread_count(state);
                    count += d.bound_queue_->get_thread_count(state);
                }
                count += low_priority_queue_.get_thread_count(state);
                break;
            }

            case thread_priority::low:
                return low_priority_queue_.get_thread_count(state);

            case thread_priority::normal:
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    count += data_[i].data_.queue_->get_thread_count(state);
                }
                break;
            }

            case thread_priority::initially_bound:
            case thread_priority::bound:
            {
                for (std::size_t i = 0; i != num_queues_; ++i)
                {
                    count +=
                        data_[i].data_.bound_queue_->get_thread_count(state);
                }
                break;
            }

            case thread_priority::boost:
            case thread_priority::high:
            case thread_priority::high_recursive:
            {
                for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
                {
                    count +=
                        data_[i].data_.high_priority_queue_->get_thread_count(
                            state);
                }
                break;
            }

            case thread_priority::unknown:
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "local_workrequesting_scheduler::get_thread_count",
                    "unknown thread priority value "
                    "(thread_priority::unknown)");
            }
            }
            return count;
        }

        // Queries whether a given core is idle
        bool is_core_idle(std::size_t num_thread) const override
        {
            auto& data = data_[num_thread].data_;
            if (num_thread < num_queues_)
            {
                if (data.bound_queue_->get_queue_length() != 0)
                    return false;
                if (data.queue_->get_queue_length() != 0)
                    return false;
            }

            if (num_thread < num_high_priority_queues_ &&
                data.high_priority_queue_->get_queue_length() != 0)
            {
                return false;
            }
            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // Enumerate matching threads from all queues
        bool enumerate_threads(hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state =
                thread_schedule_state::unknown) const override
        {
            bool result = true;
            for (std::size_t i = 0; i != num_high_priority_queues_; ++i)
            {
                result = result &&
                    data_[i].data_.high_priority_queue_->enumerate_threads(
                        f, state);
            }

            result = result && low_priority_queue_.enumerate_threads(f, state);

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                result = result &&
                    data_[i].data_.queue_->enumerate_threads(f, state);
                result = result &&
                    data_[i].data_.bound_queue_->enumerate_threads(f, state);
            }
            return result;
        }

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        ///////////////////////////////////////////////////////////////////////
        // Queries the current average thread wait time of the queues.
        std::int64_t get_average_thread_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override
        {
            // Return average thread wait time of one specific queue.
            std::uint64_t wait_time = 0;
            std::uint64_t count = 0;
            if (std::size_t(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < num_queues_);

                auto const& d = data_[num_thread].data_;
                if (num_thread < num_high_priority_queues_)
                {
                    wait_time =
                        d.high_priority_queue_->get_average_thread_wait_time();
                    ++count;
                }

                if (num_thread == num_queues_ - 1)
                {
                    wait_time +=
                        low_priority_queue_.get_average_thread_wait_time();
                    ++count;
                }

                wait_time += d.queue_->get_average_thread_wait_time();
                wait_time += d.bound_queue_->get_average_thread_wait_time();
                return wait_time / (count + 1);
            }

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto const& d = data_[num_thread].data_;
                if (num_thread < num_high_priority_queues_)
                {
                    wait_time +=
                        d.high_priority_queue_->get_average_thread_wait_time();
                }
                wait_time += d.queue_->get_average_thread_wait_time();
                wait_time += d.bound_queue_->get_average_thread_wait_time();
                ++count;
            }

            return (wait_time +
                       low_priority_queue_.get_average_thread_wait_time()) /
                (count + 1);
        }

        ///////////////////////////////////////////////////////////////////////
        // Queries the current average task wait time of the queues.
        std::int64_t get_average_task_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override
        {
            // Return average task wait time of one specific queue.
            // Return average thread wait time of one specific queue.
            std::uint64_t wait_time = 0;
            std::uint64_t count = 0;
            if (std::size_t(-1) != num_thread)
            {
                HPX_ASSERT(num_thread < num_queues_);

                auto const& d = data_[num_thread].data_;
                if (num_thread < num_high_priority_queues_)
                {
                    wait_time =
                        d.high_priority_queue_->get_average_task_wait_time();
                    ++count;
                }

                if (num_thread == num_queues_ - 1)
                {
                    wait_time +=
                        low_priority_queue_.get_average_task_wait_time();
                    ++count;
                }

                wait_time += d.queue_->get_average_task_wait_time();
                wait_time += d.bound_queue_->get_average_task_wait_time();
                return wait_time / (count + 1);
            }

            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                auto const& d = data_[num_thread].data_;
                if (num_thread < num_high_priority_queues_)
                {
                    wait_time +=
                        d.high_priority_queue_->get_average_task_wait_time();
                }
                wait_time += d.queue_->get_average_task_wait_time();
                wait_time += d.bound_queue_->get_average_task_wait_time();
                ++count;
            }

            return (wait_time +
                       low_priority_queue_.get_average_task_wait_time()) /
                (count + 1);
        }
#endif

        // return a random victim for the current stealing operation
        std::size_t random_victim(steal_request const& req) noexcept
        {
            std::size_t result;

            {
                // generate at most 3 random numbers before resorting to more
                // expensive algorithm
                std::uniform_int_distribution<std::int16_t> uniform(
                    0, static_cast<std::int16_t>(num_queues_ - 1));

                int attempts = 0;
                do
                {
                    result = uniform(gen_);
                    if (result != req.num_thread_ &&
                        !test(req.victims_, result))
                    {
                        HPX_ASSERT(result < num_queues_);
                        return result;
                    }
                } while (++attempts < 3);
            }

            // to avoid infinite trials we randomly select one of the possible
            // victims
            std::uniform_int_distribution<std::int16_t> uniform(0,
                static_cast<std::int16_t>(
                    num_queues_ - count(req.victims_) - 1));

            // generate one more random number
            std::size_t selected_victim = uniform(gen_);
            for (std::size_t i = 0; i != num_queues_; ++i)
            {
                if (!test(req.victims_, i))
                {
                    if (selected_victim == 0)
                    {
                        result = i;
                        break;
                    }
                    --selected_victim;
                }
            }

            HPX_ASSERT(result < num_queues_ && result != req.num_thread_ &&
                !test(req.victims_, result));

            return result;
        }

        // return the number of the next victim core
        std::size_t next_victim([[maybe_unused]] scheduler_data& d,
            steal_request const& req) noexcept
        {
            std::size_t victim;

            // return thief if max steal attempts has been reached or no more
            // cores are available for stealing
            if (req.attempt_ == num_queues_ - 1)
            {
                // Return steal request to thief
                victim = req.num_thread_;
            }
            else
            {
                HPX_ASSERT(
                    req.num_thread_ == d.num_thread_ || req.attempt_ != 0);

#if defined(HPX_HAVE_WORKREQUESTING_LAST_VICTIM)
                if (d.last_victim_ != std::uint16_t(-1))
                {
                    victim = d.last_victim_;
                }
                else
#endif
                {
                    victim = random_victim(req);
                }
            }

            // couldn't find victim, return steal request to thief
            if (victim == static_cast<std::size_t>(-1))
            {
                victim = req.num_thread_;
                HPX_ASSERT(victim != d.num_thread_);
            }

            HPX_ASSERT(victim < num_queues_);
            HPX_ASSERT(req.attempt_ < num_queues_);

            return victim;
        }

        // Every worker can have at most MAXSTEAL pending steal requests. A
        // steal request with idle == false indicates that the requesting worker
        // is still busy working on some tasks. A steal request with idle ==
        // true indicates that the requesting worker is in fact idle and has
        // nothing to work on.
        void send_steal_request(scheduler_data& d, bool idle = true) noexcept
        {
            if (d.requested_ == 0)
            {
                // Estimate work-stealing efficiency during the last interval;
                // switch strategies if the value is below a threshold
                if (d.num_recent_steals_ >=
                    scheduler_data::num_steal_adaptive_interval_)
                {
                    double const ratio =
                        static_cast<double>(d.num_recent_tasks_executed_) /
                        d.num_steal_adaptive_interval_;

                    d.num_recent_steals_ = 0;
                    d.num_recent_tasks_executed_ = 0;

                    if (ratio >= 2.)
                    {
                        d.stealhalf_ = true;
                    }
                    else
                    {
                        if (d.stealhalf_)
                        {
                            d.stealhalf_ = false;
                        }
                        else if (ratio <= 1.)
                        {
                            d.stealhalf_ = true;
                        }
                    }
                }

                steal_request req(
                    d.num_thread_, d.tasks_, d.victims_, idle, d.stealhalf_);
                std::size_t victim = next_victim(d, req);

                ++d.requested_;
                data_[victim].data_.requests_->set(HPX_MOVE(req));
#if defined(HPX_HAVE_WORKREQUESTING_STEAL_STATISTICS)
                ++d.steal_requests_sent_;
#endif
            }
        }

        // Try receiving tasks that are sent by another core as a response to
        // one of our steal requests. This returns true if new tasks were
        // received.
        static bool try_receiving_tasks(scheduler_data& d, std::size_t& added,
            thread_id_ref_type* next_thrd)
        {
            task_data thrds{};
            if (d.tasks_->get(&thrds))
            {
                // keep track of number of outstanding steal requests, there
                // should have been at most one
                --d.requested_;
                HPX_ASSERT(d.requested_ == 0);

                // if at least one thrd was received
                if (!thrds.tasks_.empty())
                {
                    // Schedule all but the first received task in reverse order
                    // to maintain the sequence of tasks as pulled from the
                    // victims queue.
                    for (std::size_t i = thrds.tasks_.size() - 1; i != 0; --i)
                    {
                        HPX_ASSERT(thrds.tasks_[i]);
                        d.queue_->schedule_thread(
                            HPX_MOVE(thrds.tasks_[i]), true);

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                        d.queue_->increment_num_stolen_to_pending();
#endif
                        ++added;
                    }

#if defined(HPX_HAVE_WORKREQUESTING_LAST_VICTIM)
                    // store the originating core for the next stealing
                    // operation
                    d.last_victim_ = thrds.num_thread_;
                    HPX_ASSERT(d.last_victim_ != d.num_thread_);
#endif
                    // the last of the received tasks will be either directly
                    // executed or normally scheduled
                    if (next_thrd != nullptr)
                    {
                        // directly return the last thread as it should be run
                        // immediately
                        ++d.num_recent_tasks_executed_;
                        *next_thrd = HPX_MOVE(thrds.tasks_.front());
                    }
                    else
                    {
                        d.queue_->schedule_thread(
                            HPX_MOVE(thrds.tasks_.front()), true);

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
                        d.queue_->increment_num_stolen_to_pending();
#endif
                        ++added;
                    }

                    ++d.num_recent_steals_;
                    return true;
                }
            }
            return false;
        }

        // This is a function which gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function has to
        // be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running,
            [[maybe_unused]] std::int64_t& idle_loop_count, bool allow_stealing,
            std::size_t& added, thread_id_ref_type* next_thrd = nullptr)
        {
            HPX_ASSERT(num_thread < num_queues_);

            added = 0;

            auto& d = data_[num_thread].data_;

            // We don't need to call wait_or_add_new for high priority or bound
            // threads as these threads are never created 'staged'.

            bool result =
                d.queue_->wait_or_add_new(running, added, allow_stealing);

            // check if work was available
            if (0 != added)
            {
                return result;
            }

            if (num_thread == num_queues_ - 1)
            {
                result = low_priority_queue_.wait_or_add_new(running, added) &&
                    result;
            }

            // check if we have been disabled or if no stealing is requested (or
            // not possible)
            if (!running || num_queues_ == 1)
            {
                return !running;
            }

            // attempt to steal more work
            if (allow_stealing)
            {
                send_steal_request(d);
                HPX_ASSERT(d.requested_ != 0);
            }

            if (!d.tasks_->is_empty() &&
                try_receiving_tasks(d, added, next_thrd))
            {
                return false;
            }

            // if we did not receive any new task, decline or forward all
            // pending steal requests, if there are any
            if (HPX_UNLIKELY(!d.requests_->is_empty()))
            {
                decline_or_forward_one_steal_requests(d);
            }

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
            // no new work is available, are we deadlocked?
            if (HPX_UNLIKELY(get_minimal_deadlock_detection_enabled() &&
                    LHPX_ENABLED(error)))
            {
                bool suspended_only = true;

                for (std::size_t i = 0; suspended_only && i != num_queues_; ++i)
                {
                    suspended_only =
                        data_[i].data_.queue_->dump_suspended_threads(
                            i, idle_loop_count, running);
                }

                if (HPX_UNLIKELY(suspended_only))
                {
                    if (running)
                    {
                        LTM_(error)    //-V128
                            << "queue(" << num_thread << "): "
                            << "no new work available, are we "
                               "deadlocked?";
                    }
                    else
                    {
                        LHPX_CONSOLE_(
                            hpx::util::logging::level::error)    //-V128
                            << "  [TM] "                         //-V128
                            << "queue(" << num_thread << "): "
                            << "no new work available, are we "
                               "deadlocked?\n";
                    }
                }
            }
#endif
            return result;
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t num_thread) override
        {
            hpx::threads::detail::set_local_thread_num_tss(num_thread);
            hpx::threads::detail::set_thread_pool_num_tss(
                parent_pool_->get_pool_id().index());

            auto& d = data_[num_thread].data_;
            d.init(num_thread, num_queues_, this->thread_queue_init_,
                num_thread < num_high_priority_queues_);

            d.queue_->on_start_thread(num_thread);
            d.bound_queue_->on_start_thread(num_thread);
            if (num_thread < num_high_priority_queues_)
            {
                d.high_priority_queue_->on_start_thread(num_thread);
            }

            if (num_thread == num_queues_ - 1)
            {
                low_priority_queue_.on_start_thread(num_thread);
            }

            // Initially set all bits, code below resets the bits corresponding
            // to cores that can serve as a victim for the current core. A set
            // bit in this mask means 'do not steal from this core'.
            resize(d.victims_, num_queues_);
            reset(d.victims_);
            set(d.victims_, num_thread);
        }

        void on_stop_thread(std::size_t num_thread) override
        {
            if (num_thread < data_.size())
            {
                auto& d = data_[num_thread].data_;

                d.queue_->on_stop_thread(num_thread);
                d.bound_queue_->on_stop_thread(num_thread);
                if (num_thread < num_high_priority_queues_)
                {
                    d.high_priority_queue_->on_stop_thread(num_thread);
                }
            }

            if (num_thread == num_queues_ - 1)
            {
                low_priority_queue_.on_stop_thread(num_thread);
            }
        }

        void on_error(
            std::size_t num_thread, std::exception_ptr const& e) override
        {
            if (num_thread < data_.size())
            {
                auto& d = data_[num_thread].data_;

                d.queue_->on_error(num_thread, e);
                d.bound_queue_->on_error(num_thread, e);

                if (num_thread < num_high_priority_queues_)
                {
                    d.high_priority_queue_->on_error(num_thread, e);
                }
            }

            if (num_thread == num_queues_ - 1)
            {
                low_priority_queue_.on_error(num_thread, e);
            }
        }

        void reset_thread_distribution() noexcept override
        {
            curr_queue_.store(0, std::memory_order_release);
        }

        void set_scheduler_mode(scheduler_mode mode,
            hpx::threads::mask_cref_type pu_mask) noexcept override
        {
            // we should not disable stealing for this scheduler, this would
            // possibly lead to deadlocks
            scheduler_base::set_scheduler_mode(mode |
                    policies::scheduler_mode::enable_stealing |
                    policies::scheduler_mode::enable_stealing_numa,
                pu_mask);
        }

    protected:
        std::vector<util::cache_line_data<scheduler_data>> data_;
        thread_queue_type low_priority_queue_;

        std::atomic<std::size_t> curr_queue_;

        std::mt19937 gen_;

        detail::affinity_data const& affinity_data_;
        std::size_t const num_queues_;
        std::size_t const num_high_priority_queues_;
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
