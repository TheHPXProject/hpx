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

namespace hpx::execution::experimental {

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
    hpx::threads::mask_type suspending_fork_join_executor::default_pu_mask()
    {
        return full_pu_mask(threads::detail::get_self_or_default_pool());
    }

    ///////////////////////////////////////////////////////////////////////////
    suspending_fork_join_executor::shared_data::shared_data(
        threads::thread_priority const priority,
        threads::thread_stacksize const stacksize, loop_schedule const sched,
        std::chrono::nanoseconds const spin_duration,
        hpx::threads::mask_type const* pu_mask, char const* annotation)
      : pool_(threads::detail::get_self_or_default_pool())
      , priority_(priority)
      , stacksize_(stacksize)
      , schedule_(sched)
      , spin_rounds_(spin_rounds_for(spin_duration))
      , pu_mask_(pu_mask ? *pu_mask : full_pu_mask(pool_))
      , epoch_(0)
      , remaining_(0)
    {
        if (stacksize_ == threads::thread_stacksize::nostack)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "suspending_fork_join_executor::shared_data::shared_data",
                "the worker threads of a suspending_fork_join_executor "
                "suspend, they can't be stackless");
        }

        init_threads(annotation);
    }

    void suspending_fork_join_executor::shared_data::init_threads(
        char const* annotation)
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
                "suspending_fork_join_executor::shared_data::init_threads",
                "the PU mask has to name PUs of the pool only, PU mask: {}, "
                "PUs of the pool in the mask: {}",
                pu_mask_, num_threads_);
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
        // thread.
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

    suspending_fork_join_executor::shared_data::~shared_data()
    {
        stop_workers();
    }

    void suspending_fork_join_executor::shared_data::stop_workers() noexcept
    {
        if (stopped_.exchange(true, std::memory_order_acq_rel) ||
            num_workers_ == 0)
        {
            return;
        }

        // The worker threads may be suspended: wake them with a last epoch
        // that tells them to exit.
        stop_.store(true, std::memory_order_relaxed);
        publish_region();

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
    struct suspending_fork_join_executor::shared_data::shutdown_registry
    {
        std::mutex mtx;
        std::vector<std::weak_ptr<shared_data>> entries;
        bool registered = false;

        static shutdown_registry& get()
        {
            static shutdown_registry registry;
            return registry;
        }

        static void stop_all() noexcept
        {
            std::vector<std::weak_ptr<shared_data>> stopping;
            {
                auto& registry = get();
                std::lock_guard<std::mutex> l(registry.mtx);
                stopping.swap(registry.entries);

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

    void suspending_fork_join_executor::shared_data::stop_on_shutdown(
        std::weak_ptr<shared_data> data)
    {
        auto& registry = shutdown_registry::get();
        std::lock_guard<std::mutex> l(registry.mtx);

        std::erase_if(
            registry.entries, [](std::weak_ptr<shared_data> const& entry) {
                return entry.expired();
            });
        registry.entries.push_back(HPX_MOVE(data));

        if (!registry.registered)
        {
            // throws if it is too late to register a pre-shutdown function
            registry.registered = threads::detail::register_pre_shutdown(
                &shutdown_registry::stop_all);
        }
    }

    void suspending_fork_join_executor::shared_data::store_exception(
        std::exception_ptr&& ep) noexcept
    {
        std::lock_guard<hpx::spinlock> l(exception_mtx_);
        if (!exception_)
        {
            exception_ = HPX_MOVE(ep);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Start a region: the worker threads see the new epoch. The suspended
    // worker threads are woken in a tree: the calling thread wakes the worker
    // threads at positions 0 and 1, and the one at position p wakes those at
    // 2p + 2 and 2p + 3 as soon as it sees the new epoch, whether it was
    // suspended or not. All of them are woken after a number of steps that is
    // logarithmic in the number of worker threads.
    void suspending_fork_join_executor::shared_data::publish_region() noexcept
    {
        epoch_.data_.fetch_add(1, std::memory_order_seq_cst);
        wake_workers(0);
    }

    // Wake the worker threads at positions first and first + 1, if they are
    // suspended. This pairs with wait_for_region: the waking thread has seen
    // the new epoch, so either the worker thread sees it as well, or this sees
    // it parked.
    void suspending_fork_join_executor::shared_data::wake_workers(
        std::size_t const first) noexcept
    {
        for (std::size_t position = first;
            position < first + 2 && position < num_workers_; ++position)
        {
            parking& p = parking_[position].data_;
            if (p.parked.load(std::memory_order_seq_cst))
            {
                std::unique_lock<hpx::spinlock> l(p.mtx);
                p.cv.notify_all(HPX_MOVE(l));
            }
        }
    }

    // Wait until the epoch differs from seen. Returns false if the worker
    // thread has to exit.
    bool suspending_fork_join_executor::shared_data::wait_for_region(
        std::uint64_t const seen, std::size_t const position) noexcept
    {
        // spin first
        for (std::uint64_t k = 0; k != spin_rounds_; ++k)
        {
            if (epoch_.data_.load(std::memory_order_acquire) != seen)
            {
                return !stop_.load(std::memory_order_relaxed);
            }
            HPX_SMT_PAUSE;
        }

        // then suspend
        bool aborted = false;
        {
            bind_while_suspended bind;

            parking& p = parking_[position].data_;
            std::unique_lock<hpx::spinlock> l(p.mtx);
            p.parked.store(true, std::memory_order_seq_cst);
            while (epoch_.data_.load(std::memory_order_seq_cst) == seen)
            {
                hpx::error_code ec(hpx::throwmode::lightweight);
                if (p.cv.wait(l,
                        "suspending_fork_join_executor::wait_for_region",
                        ec) == threads::thread_restart_state::abort ||
                    ec)
                {
                    // the runtime aborts the suspended threads at shutdown
                    aborted = true;
                    break;
                }
            }
            p.parked.store(false, std::memory_order_relaxed);
        }

        return !aborted && !stop_.load(std::memory_order_relaxed);
    }

    // A worker thread has finished its share of the region. The last one
    // wakes the calling thread, if it suspended. This pairs with
    // wait_for_workers: either the caller sees no remaining worker threads,
    // or this sees it parked.
    void suspending_fork_join_executor::shared_data::finish_share() noexcept
    {
        if (remaining_.data_.fetch_sub(1, std::memory_order_seq_cst) == 1 &&
            caller_parked_.load(std::memory_order_seq_cst))
        {
            std::unique_lock<hpx::spinlock> l(caller_mtx_);
            caller_cv_.notify_all(HPX_MOVE(l));
        }
    }

    void suspending_fork_join_executor::shared_data::wait_for_workers() noexcept
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

    void suspending_fork_join_executor::shared_data::worker_main(
        std::size_t const slot, std::size_t const position) noexcept
    {
        std::uint64_t seen = 0;
        while (true)
        {
            bool const run = wait_for_region(seen, position);

            // pass the wake-up on, also when stopping
            wake_workers(2 * position + 2);

            if (!run)
            {
                break;
            }

            ++seen;
            region_function_(region_body_, *this, slot);
            finish_share();
        }

        // nothing may touch this object after this
        alive_.fetch_sub(1, std::memory_order_release);
    }

    ///////////////////////////////////////////////////////////////////////////
    void suspending_fork_join_executor::shared_data::execute_region(
        region_function_type* f, void* body, std::size_t const size)
    {
        if (stopped_.load(std::memory_order_relaxed))
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "suspending_fork_join_executor::execute_region",
                "the worker threads of this suspending_fork_join_executor "
                "have exited, the runtime is shutting down");
        }

        // protect against nested use of this executor instance
        if (in_region_)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_request,
                "suspending_fork_join_executor::execute_region",
                "this instance of suspending_fork_join_executor is used in "
                "nested ways");
        }

        struct reset_in_region
        {
            bool& in_region;
            ~reset_in_region()
            {
                in_region = false;
            }
        };

        in_region_ = true;
        reset_in_region const on_exit{in_region_};

        exception_ = std::exception_ptr();
        region_function_ = f;
        region_body_ = body;
        region_size_ = size;

        // the queues hold 32 bit indices, larger regions are split statically
        dynamic_ = !queues_.empty() &&
            size <= static_cast<std::size_t>(
                        (std::numeric_limits<std::uint32_t>::max)());
        if (dynamic_)
        {
            for (std::size_t slot = 0; slot != num_threads_; ++slot)
            {
                queues_[slot].data_.reset(
                    static_cast<std::uint32_t>((slot * size) / num_threads_),
                    static_cast<std::uint32_t>(
                        ((slot + 1) * size) / num_threads_));
            }
        }

        if (num_workers_ != 0)
        {
            remaining_.data_.store(num_workers_, std::memory_order_relaxed);
            publish_region();
        }

        if (caller_slot_ != static_cast<std::size_t>(-1))
        {
            f(body, *this, caller_slot_);
        }

        if (num_workers_ != 0)
        {
            wait_for_workers();
        }

        if (exception_)
        {
            std::rethrow_exception(std::exchange(exception_, {}));
        }
    }
}    // namespace hpx::execution::experimental
