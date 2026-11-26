//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/suspending_fork_join_executor.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace hpx::execution::experimental::detail {

    using shared_data = suspending_fork_join_shared_data;
    using loop_schedule = fork_join_executor::loop_schedule;

    namespace {

        // The number of spin rounds per nanosecond, measured once. A round
        // is one look at an atomic and one pause, as in the wait loops below.
        // The clock is read only here, never while spinning.
        double measure_spin_rounds_per_ns() noexcept
        {
            constexpr std::uint64_t rounds = 4096;
            std::atomic<std::uint64_t> word(0);

            double best = (std::numeric_limits<double>::max)();
            for (int repetition = 0; repetition != 5; ++repetition)
            {
                auto const start = std::chrono::steady_clock::now();
                for (std::uint64_t k = 0; k != rounds; ++k)
                {
                    if (word.load(std::memory_order_acquire) != 0)
                        break;
                    HPX_SMT_PAUSE;
                }
                auto const elapsed = std::chrono::duration<double, std::nano>(
                    std::chrono::steady_clock::now() - start)
                                         .count();
                best = (std::min) (best, elapsed);
            }
            return best > 0 ? static_cast<double>(rounds) / best : 1.0;
        }

        std::uint64_t spin_rounds_for(std::chrono::nanoseconds const duration)
        {
            if (duration.count() <= 0)
                return 0;

            static double const rounds_per_ns = measure_spin_rounds_per_ns();
            return static_cast<std::uint64_t>(std::llround(
                static_cast<double>(duration.count()) * rounds_per_ns));
        }

        // A suspended thread that is woken up is scheduled on the worker of
        // the thread that wakes it, unless it has bound priority. The worker
        // threads and the calling thread are woken by threads that are
        // about to spin on their own cores, so they keep bound priority
        // while suspended: they are then resumed on the worker they ran on.
        class bind_while_suspended
        {
        public:
            bind_while_suspended() noexcept
              : self_(threads::get_self_id_data())
            {
                if (self_ != nullptr)
                {
                    priority_ = self_->get_priority();
                    if (priority_ != threads::thread_priority::bound)
                    {
                        self_->set_priority(threads::thread_priority::bound);
                    }
                }
            }

            bind_while_suspended(bind_while_suspended const&) = delete;
            bind_while_suspended(bind_while_suspended&&) = delete;
            bind_while_suspended& operator=(
                bind_while_suspended const&) = delete;
            bind_while_suspended& operator=(bind_while_suspended&&) = delete;

            ~bind_while_suspended()
            {
                if (self_ != nullptr &&
                    priority_ != threads::thread_priority::bound)
                {
                    self_->set_priority(priority_);
                }
            }

        private:
            threads::thread_data* self_;
            threads::thread_priority priority_ =
                threads::thread_priority::bound;
        };

        hpx::threads::mask_type full_pu_mask(
            threads::thread_pool_base const* pool)
        {
            auto const& rp = hpx::resource::get_partitioner();

            hpx::threads::mask_type mask = hpx::threads::mask_type();
            hpx::threads::resize(mask, hpx::threads::hardware_concurrency());

            std::size_t const offset = pool->get_thread_offset();
            std::size_t const count = pool->get_os_thread_count();
            for (std::size_t i = 0; i != count; ++i)
            {
                hpx::threads::set(mask, rp.get_pu_num(offset + i));
            }
            return mask;
        }
    }    // namespace

    ///////////////////////////////////////////////////////////////////////////
    hpx::threads::mask_type suspending_fork_join_default_pu_mask()
    {
        return full_pu_mask(threads::detail::get_self_or_default_pool());
    }

    bool suspending_fork_join_caller_on(
        hpx::threads::mask_cref_type mask) noexcept
    {
        if (threads::get_self_id_data() == nullptr)
        {
            return false;
        }
        std::size_t const worker = hpx::get_worker_thread_num();
        if (worker == static_cast<std::size_t>(-1))
        {
            return false;
        }
        return hpx::threads::test(
            mask, hpx::resource::get_partitioner().get_pu_num(worker));
    }

    ///////////////////////////////////////////////////////////////////////////
    suspending_fork_join_shared_data::suspending_fork_join_shared_data(
        threads::thread_priority const priority,
        threads::thread_stacksize const stacksize, loop_schedule const sched,
        std::chrono::nanoseconds const spin_duration,
        hpx::threads::mask_type const* pu_mask)
      : priority_(priority)
      , stacksize_(stacksize)
      , schedule_(sched)
      , spin_duration_(spin_duration)
      , pu_mask_(pu_mask ? *pu_mask : hpx::threads::mask_type())
      , has_pu_mask_(pu_mask != nullptr)
      , epoch_(0)
      , remaining_(0)
    {
    }

    bool suspending_fork_join_shared_data::start(char const* annotation)
    {
        std::lock_guard<std::mutex> l(start_mtx_);
        if (started_.load(std::memory_order_relaxed))
        {
            return false;
        }

        if (stacksize_ == threads::thread_stacksize::nostack)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "suspending_fork_join_shared_data::start",
                "the worker threads of a suspending_fork_join_executor "
                "suspend, they can't be stackless");
        }

        pool_ = threads::detail::get_self_or_default_pool();
        spin_rounds_ = spin_rounds_for(spin_duration_);
        if (!has_pu_mask_)
        {
            pu_mask_ = full_pu_mask(pool_);
        }

        init_threads(annotation);
        started_.store(true, std::memory_order_release);
        return true;
    }

    void suspending_fork_join_shared_data::init_threads(char const* annotation)
    {
        auto const& rp = hpx::resource::get_partitioner();

        // The slots are the PUs of the pool that are in the PU mask, in the
        // order of the pool's worker threads.
        std::size_t const offset = pool_->get_thread_offset();
        std::size_t const count = pool_->get_os_thread_count();

        std::vector<std::size_t> slot_workers;
        slot_workers.reserve(count);
        for (std::size_t i = 0; i != count; ++i)
        {
            if (hpx::threads::test(pu_mask_, rp.get_pu_num(offset + i)))
            {
                slot_workers.push_back(i);
            }
        }

        num_threads_ = slot_workers.size();
        if (num_threads_ == 0 || num_threads_ != hpx::threads::count(pu_mask_))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "suspending_fork_join_shared_data::init_threads",
                "the PU mask has to name PUs of the pool only, PU mask: {}, "
                "PUs of the pool in the mask: {}",
                pu_mask_, num_threads_);
        }

        // the slots of a region are encoded in 16 bits each (see epoch_)
        if (num_threads_ > 0xffff)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "suspending_fork_join_shared_data::init_threads",
                "a suspending_fork_join_executor supports at most 65535 "
                "PUs, PUs in the mask: {}",
                num_threads_);
        }

        // The calling thread takes part in the regions if it is a worker
        // thread of this pool and runs on one of the PUs of the mask. This
        // is decided by its PU, not by its worker number.
        if (threads::get_self_id_data() != nullptr &&
            hpx::get_thread_pool_num() == pool_->get_pool_index())
        {
            std::size_t const caller_worker =
                hpx::get_local_worker_thread_num();
            for (std::size_t slot = 0; slot != num_threads_; ++slot)
            {
                if (slot_workers[slot] == caller_worker)
                {
                    caller_slot_ = slot;
                    break;
                }
            }
        }

        num_workers_ = caller_slot_ == static_cast<std::size_t>(-1) ?
            num_threads_ :
            num_threads_ - 1;

        if (schedule_ == loop_schedule::dynamic && num_threads_ > 1)
        {
            queues_.resize(num_threads_);
        }

        // Create a worker thread for every slot but the one of the calling
        // thread. Their positions follow the order of their slots.
        parking_ = std::make_unique<hpx::util::cache_aligned_data<parking>[]>(
            num_workers_);
        alive_.store(num_workers_, std::memory_order_relaxed);
        std::size_t position = 0;
        for (std::size_t slot = 0; slot != num_threads_; ++slot)
        {
            if (slot == caller_slot_)
            {
                continue;
            }

            std::size_t const worker = slot_workers[slot];
            threads::thread_init_data data(
                threads::make_thread_function_nullary(
                    [this, slot, position]() noexcept {
                        worker_main(slot, position);
                    }),
                threads::thread_description(hpx::util::format("{}: thread ({})",
                    annotation ? annotation : "suspending_fork_join_executor",
                    rp.get_pu_num(offset + worker))),
                priority_,
                threads::thread_schedule_hint(
                    static_cast<std::int16_t>(worker)),
                stacksize_, threads::thread_schedule_state::pending, false,
                pool_->get_scheduler());

            threads::register_work(data, pool_);
            ++position;
        }
    }

    suspending_fork_join_shared_data::~suspending_fork_join_shared_data()
    {
        stop_workers();
    }

    void suspending_fork_join_shared_data::stop_workers() noexcept
    {
        if (stopped_.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }

        // Let a running region finish, and keep new ones from starting: the
        // flag stays set.
        hpx::util::yield_while(
            [&] {
                return in_region_.exchange(true, std::memory_order_acquire);
            },
            "suspending_fork_join_executor::stop_workers");

        if (num_workers_ == 0)
        {
            return;
        }

        // The worker threads may be suspended: wake them with a last epoch
        // that tells them to exit.
        publish_region({0, 0});

        // A worker thread leaves its last notification before it exits, so
        // the members it uses have to stay alive until it did. Its exit is
        // only a short time after it saw the stop request.
        hpx::util::yield_while(
            [&] { return alive_.load(std::memory_order_acquire) != 0; },
            "suspending_fork_join_executor::stop_workers");
    }

    ///////////////////////////////////////////////////////////////////////////
    // The shared data of all executors whose worker threads may still run.
    // A single pre-shutdown function of the runtime (see
    // hpx::register_pre_shutdown_function) stops them all.
    struct suspending_fork_join_shared_data::shutdown_registry
    {
        // a shared instance, see shared_suspending_fork_join_executor
        struct shared_entry
        {
            threads::thread_pool_base* pool;
            threads::thread_priority priority;
            threads::thread_stacksize stacksize;
            loop_schedule schedule;
            std::chrono::nanoseconds spin_duration;
            std::shared_ptr<shared_data> data;
        };

        std::mutex mtx;
        std::vector<std::weak_ptr<shared_data>> entries;
        std::vector<shared_entry> shared;
        bool registered = false;

        static shutdown_registry& get()
        {
            static shutdown_registry registry;
            return registry;
        }

        // register data, with mtx locked
        void add(std::weak_ptr<shared_data> data)
        {
            std::erase_if(entries, [](std::weak_ptr<shared_data> const& entry) {
                return entry.expired();
            });
            entries.push_back(HPX_MOVE(data));

            if (!registered)
            {
                // throws if it is too late to register a pre-shutdown
                // function
                registered = threads::detail::register_pre_shutdown(
                    &shutdown_registry::stop_all);
            }
        }

        static void stop_all() noexcept
        {
            std::vector<std::weak_ptr<shared_data>> stopping;

            // released after the worker threads were stopped
            std::vector<shared_entry> shared_stopping;
            {
                auto& registry = get();
                std::lock_guard<std::mutex> l(registry.mtx);
                stopping.swap(registry.entries);
                shared_stopping.swap(registry.shared);

                // a later runtime needs its own registration
                registry.registered = false;
            }

            for (auto const& entry : stopping)
            {
                if (auto const data = entry.lock())
                {
                    data->stop_workers();
                }
            }
        }
    };

    void suspending_fork_join_shared_data::stop_on_shutdown(
        std::weak_ptr<shared_data> data)
    {
        auto& registry = shutdown_registry::get();
        std::lock_guard<std::mutex> l(registry.mtx);
        registry.add(HPX_MOVE(data));
    }

    std::shared_ptr<suspending_fork_join_shared_data>
    suspending_fork_join_shared_instance(
        threads::thread_priority const priority,
        threads::thread_stacksize const stacksize, loop_schedule const schedule,
        std::chrono::nanoseconds const spin_duration,
        char const* annotation) noexcept
    {
        if (threads::get_self_id_data() == nullptr)
        {
            return {};
        }

        return hpx::detail::try_catch_exception_ptr(
            [&]() -> std::shared_ptr<shared_data> {
                auto* pool = threads::detail::get_self_or_default_pool();

                auto& registry = shared_data::shutdown_registry::get();
                std::lock_guard<std::mutex> l(registry.mtx);
                for (auto const& entry : registry.shared)
                {
                    if (entry.pool == pool && entry.priority == priority &&
                        entry.stacksize == stacksize &&
                        entry.schedule == schedule &&
                        entry.spin_duration == spin_duration)
                    {
                        return entry.data;
                    }
                }

                // The first use creates the instance. The lock keeps others
                // from creating one at the same time.
                auto data = std::make_shared<shared_data>(
                    priority, stacksize, schedule, spin_duration, nullptr);
                data->start(annotation);
                registry.add(data);
                registry.shared.push_back(
                    {pool, priority, stacksize, schedule, spin_duration, data});
                return data;
            },
            [](std::exception_ptr&&) -> std::shared_ptr<shared_data> {
                return {};
            });
    }

    void suspending_fork_join_shared_data::store_exception(
        std::exception_ptr&& ep) noexcept
    {
        std::lock_guard<hpx::spinlock> l(exception_mtx_);
        if (!exception_)
        {
            exception_ = HPX_MOVE(ep);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t suspending_fork_join_shared_data::region_workers(
        region_slots const slots) const noexcept
    {
        return caller_takes_part(slots) ? slots.count - 1 : slots.count;
    }

    std::size_t suspending_fork_join_shared_data::wake_index(
        region_slots const slots, std::size_t const slot) const noexcept
    {
        std::size_t const index = share_index(slots, slot);
        if (index >= slots.count)
        {
            return region_workers(slots);
        }

        // the calling thread is not woken
        if (caller_takes_part(slots) &&
            share_index(slots, caller_slot_) < index)
        {
            return index - 1;
        }
        return index;
    }

    std::size_t suspending_fork_join_shared_data::wake_position(
        region_slots const slots, std::size_t index) const noexcept
    {
        // skip the calling thread
        if (caller_takes_part(slots) &&
            share_index(slots, caller_slot_) <= index)
        {
            ++index;
        }

        // the positions follow the slots, skipping the one of the calling
        // thread
        std::size_t const slot = (slots.first + index) % num_threads_;
        return caller_slot_ < slot ? slot - 1 : slot;
    }

    // Start a region (or stop the worker threads, if slots.count is 0): the
    // worker threads see the new epoch. The suspended worker threads that
    // take part in the region are woken in a tree: the calling thread wakes
    // the first two of them (in the order of their slots in the region), and
    // the i-th one wakes those at 2i + 2 and 2i + 3 as soon as it sees the
    // new epoch, whether it was suspended or not. All of them are woken after
    // a number of steps that is logarithmic in their number. The other worker
    // threads are not woken.
    void suspending_fork_join_shared_data::publish_region(
        region_slots const slots) noexcept
    {
        // only the thread that holds in_region_ changes the epoch
        std::uint64_t const counter =
            (epoch_.data_.load(std::memory_order_relaxed) >> 32) + 1;
        epoch_.data_.store((counter << 32) |
                static_cast<std::uint64_t>(slots.first << 16) |
                static_cast<std::uint64_t>(slots.count),
            std::memory_order_seq_cst);
        wake_workers(wake_slots(slots), 0);
    }

    // Wake the worker threads at the places first and first + 1 of the given
    // region (see publish_region), if they are suspended. This pairs with
    // wait_for_region: the waking thread has seen the new epoch, so either the
    // worker thread sees it as well, or this sees it parked.
    void suspending_fork_join_shared_data::wake_workers(
        region_slots const slots, std::size_t const first) noexcept
    {
        std::size_t const workers = region_workers(slots);
        for (std::size_t index = first; index < first + 2 && index < workers;
            ++index)
        {
            parking& p = parking_[wake_position(slots, index)].data_;
            if (p.parked.load(std::memory_order_seq_cst))
            {
                std::unique_lock<hpx::spinlock> l(p.mtx);
                p.cv.notify_all(HPX_MOVE(l));
            }
        }
    }

    // Wait until the epoch differs from seen, and store the new one in seen.
    // Returns false if the runtime aborted the waiting thread.
    bool suspending_fork_join_shared_data::wait_for_region(
        std::uint64_t& seen, std::size_t const position) noexcept
    {
        // spin first
        for (std::uint64_t k = 0; k != spin_rounds_; ++k)
        {
            if (std::uint64_t const epoch =
                    epoch_.data_.load(std::memory_order_acquire);
                epoch != seen)
            {
                seen = epoch;
                return true;
            }
            HPX_SMT_PAUSE;
        }

        // then suspend
        bind_while_suspended bind;

        parking& p = parking_[position].data_;
        std::unique_lock<hpx::spinlock> l(p.mtx);
        p.parked.store(true, std::memory_order_seq_cst);

        bool aborted = false;
        std::uint64_t epoch = 0;
        while ((epoch = epoch_.data_.load(std::memory_order_seq_cst)) == seen)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            if (p.cv.wait(l, "suspending_fork_join_executor::wait_for_region",
                    ec) == threads::thread_restart_state::abort ||
                ec)
            {
                // the runtime aborts the suspended threads at shutdown
                aborted = true;
                break;
            }
        }
        p.parked.store(false, std::memory_order_relaxed);

        seen = epoch;
        return !aborted;
    }

    // A worker thread has finished its share of the region. The last one
    // wakes the calling thread, if it suspended. This pairs with
    // wait_for_workers: either the caller sees no remaining worker threads,
    // or this sees it parked.
    void suspending_fork_join_shared_data::finish_share() noexcept
    {
        if (remaining_.data_.fetch_sub(1, std::memory_order_seq_cst) == 1 &&
            caller_parked_.load(std::memory_order_seq_cst))
        {
            std::unique_lock<hpx::spinlock> l(caller_mtx_);
            caller_cv_.notify_all(HPX_MOVE(l));
        }
    }

    void suspending_fork_join_shared_data::wait_for_workers() noexcept
    {
        // spin first
        for (std::uint64_t k = 0; k != spin_rounds_; ++k)
        {
            if (remaining_.data_.load(std::memory_order_acquire) == 0)
            {
                return;
            }
            HPX_SMT_PAUSE;
        }

        // then suspend
        bind_while_suspended bind;

        std::unique_lock<hpx::spinlock> l(caller_mtx_);
        caller_parked_.store(true, std::memory_order_seq_cst);
        while (remaining_.data_.load(std::memory_order_seq_cst) != 0)
        {
            // The worker threads use data on the stack of this thread, so it
            // can't leave before they are done, even if it was interrupted.
            hpx::error_code ec(hpx::throwmode::lightweight);
            caller_cv_.wait(
                l, "suspending_fork_join_executor::wait_for_workers", ec);
        }
        caller_parked_.store(false, std::memory_order_relaxed);
    }

    void suspending_fork_join_shared_data::worker_main(
        std::size_t const slot, std::size_t const position) noexcept
    {
        // The worker threads are background threads of their pool: while
        // they wait for the next region the pool counts as idle. Otherwise
        // a pool with an idle executor is never idle, and the termination
        // detection of the distributed runtime, which runs before the
        // pre-shutdown function that stops the worker threads, never ends.
        // A suspended background thread is not kept alive by the suspension
        // (whoever creates it has to), so the thread keeps itself alive
        // until it ends.
        hpx::threads::keep_alive_thread_id const keep_alive(
            hpx::threads::get_self_id(), true);
        hpx::threads::thread_data* const self =
            hpx::threads::get_self_id_data();
        hpx::threads::policies::scheduler_base* const scheduler =
            self->get_scheduler_base();
        self->set_is_background();
        scheduler->increment_background_thread_count();
        auto on_exit = hpx::experimental::scope_exit(
            [scheduler] { scheduler->decrement_background_thread_count(); });

        std::uint64_t seen = 0;
        while (true)
        {
            bool const run = wait_for_region(seen, position);

            region_slots const slots = decode_slots(seen);
            bool const stop = slots.count == 0;

            // pass the wake-up on, also when stopping
            region_slots const woken = wake_slots(slots);
            std::size_t const index = wake_index(woken, slot);
            bool const takes_part = index != region_workers(woken);
            if (takes_part)
            {
                wake_workers(woken, 2 * index + 2);
            }

            if (!run || stop)
            {
                break;
            }

            // A worker thread that doesn't take part in the region doesn't
            // count in remaining_. It may see a later region next.
            if (takes_part)
            {
                region_function_(region_body_, *this, share_index(slots, slot));
                finish_share();
            }
        }

        // nothing may touch this object after this
        alive_.fetch_sub(1, std::memory_order_release);
    }

    ///////////////////////////////////////////////////////////////////////////
    void suspending_fork_join_shared_data::execute_region(
        region_function_type* f, void* body, std::size_t const size,
        std::size_t const first, std::size_t const count)
    {
        if (stopped_.load(std::memory_order_relaxed))
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "suspending_fork_join_executor::execute_region",
                "the worker threads of this suspending_fork_join_executor "
                "have exited, the runtime is shutting down");
        }

        // protect against nested (or concurrent) use of this executor
        // instance
        if (in_region_.exchange(true, std::memory_order_acquire))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_request,
                "suspending_fork_join_executor::execute_region",
                "this instance of suspending_fork_join_executor is used in "
                "nested ways");
        }

        run_locked_region(f, body, size, first, count);
    }

    bool suspending_fork_join_shared_data::try_execute_region(
        region_function_type* f, void* body, std::size_t const size,
        std::size_t const first, std::size_t const count)
    {
        if (stopped_.load(std::memory_order_relaxed) ||
            in_region_.exchange(true, std::memory_order_acquire))
        {
            return false;
        }

        run_locked_region(f, body, size, first, count);
        return true;
    }

    // Run a region, in_region_ was set by the caller.
    void suspending_fork_join_shared_data::run_locked_region(
        region_function_type* f, void* body, std::size_t const size,
        std::size_t const first, std::size_t const count)
    {
        struct reset_in_region
        {
            std::atomic<bool>& in_region;
            ~reset_in_region()
            {
                in_region.store(false, std::memory_order_release);
            }
        };
        reset_in_region const on_exit{in_region_};

        // the slots of the region: all of them if count is 0
        std::size_t const n =
            count == 0 ? num_threads_ : (std::min) (count, num_threads_);
        region_slots const slots{first % num_threads_, n};

        exception_ = std::exception_ptr();
        region_function_ = f;
        region_body_ = body;
        region_size_ = size;
        region_threads_ = n;

        // the queues hold 32 bit indices, larger regions are split statically
        dynamic_ = !queues_.empty() && n > 1 &&
            size <= static_cast<std::size_t>(
                        (std::numeric_limits<std::uint32_t>::max)());
        if (dynamic_)
        {
            for (std::size_t index = 0; index != n; ++index)
            {
                queues_[index].data_.reset(
                    static_cast<std::uint32_t>((index * size) / n),
                    static_cast<std::uint32_t>(((index + 1) * size) / n));
            }
        }

        std::size_t const workers = region_workers(slots);
        if (workers != 0)
        {
            remaining_.data_.store(workers, std::memory_order_relaxed);
            publish_region(slots);
        }

        if (caller_takes_part(slots))
        {
            f(body, *this, share_index(slots, caller_slot_));
        }

        if (workers != 0)
        {
            wait_for_workers();
        }

        if (exception_)
        {
            std::rethrow_exception(std::exchange(exception_, {}));
        }
    }
}    // namespace hpx::execution::experimental::detail
