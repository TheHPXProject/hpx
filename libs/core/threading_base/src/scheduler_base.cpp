//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/execution_base.hpp>

#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/scheduler_mode.hpp>
#include <hpx/threading_base/scheduler_state.hpp>
#include <hpx/threading_base/thread_init_data.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>

#if defined(HPX_HAVE_SCHEDULER_LOCAL_STORAGE)
#include <hpx/modules/coroutines.hpp>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    scheduler_base::scheduler_base(std::size_t num_threads,
        char const* description,
        thread_queue_init_parameters const& thread_queue_init,
        scheduler_mode mode)
      : modes_(num_threads)
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
      , max_idle_backoff_time_(thread_queue_init.max_idle_backoff_time_)
      , wait_count_data_(num_threads)
#endif
      , suspend_mtxs_(num_threads)
      , suspend_conds_(num_threads)
      , pu_mtxs_(num_threads)
      , states_(num_threads)
      , description_(description)
      , thread_queue_init_(thread_queue_init)
      , parent_pool_(nullptr)
      , background_thread_count_(0)
      , polling_function_mpi_(&null_polling_function)
      , polling_function_cuda_(&null_polling_function)
      , polling_function_sycl_(&null_polling_function)
      , polling_work_count_function_mpi_(&null_polling_work_count_function)
      , polling_work_count_function_cuda_(&null_polling_work_count_function)
      , polling_work_count_function_sycl_(&null_polling_work_count_function)
    {
        for (std::size_t i = 0; i != num_threads; ++i)
        {
            modes_[i].data_.store(mode, std::memory_order::relaxed);
            states_[i].data_.store(
                hpx::state::initialized, std::memory_order::relaxed);
        }
    }

    bool scheduler_base::idle_callback([[maybe_unused]] std::size_t num_thread)
    {
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        if (modes_[num_thread].data_.load(std::memory_order_relaxed) &
            policies::scheduler_mode::enable_idle_backoff)
        {
            HPX_TRACING_MARK_EVENT("idle_callback");
            // Put this thread to sleep for some time, additionally it gets
            // woken up on new work.

            auto& data = wait_count_data_[num_thread].data_;

            std::unique_lock<pu_mutex_type> l(data.wait_mtx, std::try_to_lock);
            if (!l)
            {
                return false;
            }

            // Exponential back-off with a maximum sleep time.
            constexpr double max_exponent =
                std::numeric_limits<double>::max_exponent - 1;
            double const exponent =
                (std::min) (static_cast<double>(data.wait_count), max_exponent);

            std::chrono::microseconds const period(std::lround(
                (std::min) (max_idle_backoff_time_, std::pow(2.0, exponent))));

            ++data.wait_count;

            if (data.wait_cond.wait_for(l, period) ==    //-V1089
                std::cv_status::no_timeout)
            {
                // reset counter if thread was woken up
                data.wait_count = 0;
            }
            return true;
        }
#endif
        return false;
    }

    /// This function gets called by the thread-manager whenever new work
    /// has been added, allowing the scheduler to reactivate one or more of
    /// possibly idling OS threads
    void scheduler_base::do_some_work([[maybe_unused]] std::size_t num_thread)
    {
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        HPX_TRACING_MARK_EVENT("do_some_work");

        if (static_cast<std::size_t>(-1) == num_thread)
        {
            auto const size = wait_count_data_.size();
            for (std::size_t i = 0; i != size; ++i)
            {
                if (modes_[i].data_.load(std::memory_order_relaxed) &
                    policies::scheduler_mode::enable_idle_backoff)
                {
                    wait_count_data_[i].data_.wait_count = 0;
                    wait_count_data_[i].data_.wait_cond.notify_one();
                }
            }
        }
        else if (modes_[num_thread].data_.load(std::memory_order_relaxed) &
            policies::scheduler_mode::enable_idle_backoff)
        {
            auto const size = wait_count_data_.size();
            for (std::size_t i = 0; i != size; ++i)
            {
                wait_count_data_[i].data_.wait_count = 0;
                wait_count_data_[i].data_.wait_cond.notify_one();
            }
        }
#endif
    }

    void scheduler_base::suspend(std::size_t num_thread)
    {
        HPX_ASSERT(num_thread < suspend_conds_.size());

        states_[num_thread].data_.store(hpx::state::sleeping);
        std::unique_lock<pu_mutex_type> l(suspend_mtxs_[num_thread]);
        suspend_conds_[num_thread].wait(l);    //-V1089

        // Only set running if still in hpx::state::sleeping. Can be set with
        // non-blocking/locking functions to stopping or terminating, in which
        // case the state is left untouched.
        hpx::state expected = hpx::state::sleeping;
        states_[num_thread].data_.compare_exchange_strong(
            expected, hpx::state::running);

        HPX_ASSERT(expected == hpx::state::sleeping ||
            expected == hpx::state::stopping ||
            expected == hpx::state::terminating);
    }

    void scheduler_base::resume(std::size_t num_thread)
    {
        if (num_thread == static_cast<std::size_t>(-1))
        {
            for (std::condition_variable& c : suspend_conds_)
            {
                c.notify_one();
            }
        }
        else
        {
            HPX_ASSERT(num_thread < suspend_conds_.size());
            suspend_conds_[num_thread].notify_one();
        }
    }

    std::size_t scheduler_base::select_active_pu(
        std::size_t num_thread, bool allow_fallback)
    {
        if (modes_[num_thread].data_.load(std::memory_order_relaxed) &
            threads::policies::scheduler_mode::enable_elasticity)
        {
            std::size_t states_size = states_.size();

            if (!allow_fallback)
            {
                // Try indefinitely as long as at least one thread is available
                // for scheduling. Increase allowed state if no threads are
                // available for scheduling.
                auto max_allowed_state = hpx::state::suspended;

                hpx::util::yield_while([this, states_size, &num_thread,
                                           &max_allowed_state]() {
                    std::size_t num_allowed_threads = 0;

                    for (std::size_t offset = 0; offset < states_size; ++offset)
                    {
                        std::size_t const num_thread_local =
                            (num_thread + offset) % states_size;

                        {
                            std::unique_lock<pu_mutex_type> l(
                                pu_mtxs_[num_thread_local], std::try_to_lock);

                            if (l.owns_lock())
                            {
                                if (states_[num_thread_local].data_.load(
                                        std::memory_order_relaxed) <=
                                    max_allowed_state)
                                {
                                    num_thread = num_thread_local;
                                    return false;
                                }
                            }
                        }

                        if (states_[num_thread_local].data_.load(
                                std::memory_order_relaxed) <= max_allowed_state)
                        {
                            ++num_allowed_threads;
                        }
                    }

                    if (0 == num_allowed_threads)
                    {
                        if (max_allowed_state <= hpx::state::suspended)
                        {
                            max_allowed_state = hpx::state::sleeping;
                        }
                        else if (max_allowed_state <= hpx::state::sleeping)
                        {
                            max_allowed_state = hpx::state::stopping;
                        }
                        else
                        {
                            // All threads are terminating or stopped. Just
                            // return num_thread to avoid infinite loop.
                            return false;
                        }
                    }

                    // Yield after trying all pus, then try again
                    return true;
                });

                return num_thread;
            }

            // Try all pus only once if fallback is allowed
            HPX_ASSERT(num_thread != static_cast<std::size_t>(-1));
            for (std::size_t offset = 0; offset < states_size; ++offset)
            {
                std::size_t const num_thread_local =
                    (num_thread + offset) % states_size;

                std::unique_lock<pu_mutex_type> l(
                    pu_mtxs_[num_thread_local], std::try_to_lock);

                if (l.owns_lock() &&
                    states_[num_thread_local].data_.load(
                        std::memory_order_relaxed) <= hpx::state::suspended)
                {
                    return num_thread_local;
                }
            }
        }

        return num_thread;
    }

    // allow to access/manipulate states
    std::atomic<hpx::state>& scheduler_base::get_state(std::size_t num_thread)
    {
        HPX_ASSERT(num_thread < states_.size());
        return states_[num_thread].data_;
    }

    std::atomic<hpx::state> const& scheduler_base::get_state(
        std::size_t num_thread) const
    {
        HPX_ASSERT(num_thread < states_.size());
        return states_[num_thread].data_;
    }

    void scheduler_base::set_all_states(hpx::state s)
    {
        for (auto& state : states_)
        {
            state.data_.store(s);
        }
    }

    void scheduler_base::set_all_states_at_least(hpx::state s)
    {
        for (auto& state : states_)
        {
            if (state.data_.load(std::memory_order_relaxed) < s)
            {
                state.data_.store(s, std::memory_order_release);
            }
        }
    }

    // return whether all states are at least at the given one
    bool scheduler_base::has_reached_state(hpx::state s) const
    {
        for (auto const& state : states_)
        {
            if (state.data_.load(std::memory_order_relaxed) < s)
                return false;
        }
        return true;
    }

    bool scheduler_base::is_state(hpx::state s) const
    {
        for (auto const& state : states_)
        {
            if (state.data_.load(std::memory_order_relaxed) != s)
                return false;
        }
        return true;
    }

    std::pair<hpx::state, hpx::state> scheduler_base::get_minmax_state() const
    {
        std::pair<hpx::state, hpx::state> result(
            hpx::state::last_valid_runtime_state,
            hpx::state::first_valid_runtime_state);

        for (auto const& state_iter : states_)
        {
            hpx::state s = state_iter.data_.load(std::memory_order_relaxed);
            result.first = (std::min) (result.first, s);
            result.second = (std::max) (result.second, s);
        }

        return result;
    }

    // get/set scheduler mode
    void scheduler_base::set_scheduler_mode(
        scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept
    {
        HPX_ASSERT(hpx::threads::count(pu_mask) <= modes_.size());

        // distribute the same value across all cores
        std::size_t const size = hpx::threads::mask_size(pu_mask);
        for (std::size_t i = 0, j = 0; i != size; ++i)
        {
            if (hpx::threads::test(pu_mask, i))
            {
                modes_[j++].data_.store(mode, std::memory_order_release);
            }
        }
        do_some_work(static_cast<std::size_t>(-1));
    }

    void scheduler_base::add_scheduler_mode(
        scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept
    {
        HPX_ASSERT(hpx::threads::count(pu_mask) <= modes_.size());

        // distribute the same value across all cores
        std::size_t const size = hpx::threads::mask_size(pu_mask);
        for (std::size_t i = 0, j = 0; i != size; ++i)
        {
            if (hpx::threads::test(pu_mask, i))
            {
                auto const old_mode =
                    modes_[j].data_.load(std::memory_order::relaxed);
                modes_[j++].data_.store(
                    old_mode | mode, std::memory_order_release);
            }
        }
    }

    void scheduler_base::remove_scheduler_mode(
        scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept
    {
        HPX_ASSERT(hpx::threads::count(pu_mask) <= modes_.size());

        std::size_t const size = hpx::threads::mask_size(pu_mask);
        for (std::size_t i = 0, j = 0; i != size; ++i)
        {
            if (hpx::threads::test(pu_mask, i))
            {
                auto const old_mode =
                    modes_[j].data_.load(std::memory_order::relaxed);
                modes_[j++].data_.store(
                    static_cast<scheduler_mode>(old_mode & ~mode),
                    std::memory_order_release);
            }
        }
    }

    void scheduler_base::add_remove_scheduler_mode(scheduler_mode to_add_mode,
        scheduler_mode to_remove_mode,
        hpx::threads::mask_cref_type pu_mask) noexcept
    {
        HPX_ASSERT(hpx::threads::count(pu_mask) <= modes_.size());

        std::size_t const size = hpx::threads::mask_size(pu_mask);
        for (std::size_t i = 0, j = 0; i != size; ++i)
        {
            if (hpx::threads::test(pu_mask, i))
            {
                auto const old_mode =
                    modes_[j].data_.load(std::memory_order::relaxed);
                modes_[j++].data_.store(
                    static_cast<scheduler_mode>(
                        (old_mode | to_add_mode) & ~to_remove_mode),
                    std::memory_order_release);
            }
        }
    }

    void scheduler_base::update_scheduler_mode(scheduler_mode mode, bool set,
        hpx::threads::mask_cref_type pu_mask) noexcept
    {
        if (set)
        {
            add_scheduler_mode(mode, pu_mask);
        }
        else
        {
            remove_scheduler_mode(mode, pu_mask);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t scheduler_base::get_background_thread_count() const noexcept
    {
        return background_thread_count_;
    }

    void scheduler_base::increment_background_thread_count() noexcept
    {
        ++background_thread_count_;
    }

    void scheduler_base::decrement_background_thread_count() noexcept
    {
        --background_thread_count_;
    }

#if defined(HPX_HAVE_SCHEDULER_LOCAL_STORAGE)
    coroutines::detail::tss_data_node* scheduler_base::find_tss_data(
        void const* key)
    {
        if (!thread_data_)
            return nullptr;
        return thread_data_->find(key);
    }

    void scheduler_base::add_new_tss_node(void const* key,
        std::shared_ptr<coroutines::detail::tss_cleanup_function> const& func,
        void* tss_data)
    {
        if (!thread_data_)
        {
            thread_data_ = std::make_shared<coroutines::detail::tss_storage>();
        }
        thread_data_->insert(key, func, tss_data);
    }

    void scheduler_base::erase_tss_node(void const* key, bool cleanup_existing)
    {
        if (thread_data_)
            thread_data_->erase(key, cleanup_existing);
    }

    void* scheduler_base::get_tss_data(void const* key)
    {
        if (coroutines::detail::tss_data_node* const current_node =
                find_tss_data(key))
        {
            return current_node->get_value();
        }
        return nullptr;
    }

    void scheduler_base::set_tss_data(void const* key,
        std::shared_ptr<coroutines::detail::tss_cleanup_function> const& func,
        void* tss_data, bool cleanup_existing)
    {
        if (coroutines::detail::tss_data_node* const current_node =
                find_tss_data(key))
        {
            if (func || (tss_data != 0))
                current_node->reinit(func, tss_data, cleanup_existing);
            else
                erase_tss_node(key, cleanup_existing);
        }
        else if (func || (tss_data != 0))
        {
            add_new_tss_node(key, func, tss_data);
        }
    }
#endif

    std::ptrdiff_t scheduler_base::get_stack_size(
        threads::thread_stacksize stacksize) const noexcept
    {
        if (stacksize == thread_stacksize::current)
        {
            stacksize = get_self_stacksize_enum();
        }

        HPX_ASSERT(stacksize != thread_stacksize::current);

        switch (stacksize)
        {
        case thread_stacksize::small_:
            return thread_queue_init_.small_stacksize_;

        case thread_stacksize::medium:
            return thread_queue_init_.medium_stacksize_;

        case thread_stacksize::large:
            return thread_queue_init_.large_stacksize_;

        case thread_stacksize::huge:
            return thread_queue_init_.huge_stacksize_;

        case thread_stacksize::nostack:
            return (std::numeric_limits<std::ptrdiff_t>::max)();

        default:
            HPX_ASSERT_MSG(
                false, util::format("Invalid stack size {1}", stacksize));
            break;
        }

        return thread_queue_init_.small_stacksize_;
    }

    void scheduler_base::set_mpi_polling_functions(
        polling_function_ptr mpi_func,
        polling_work_count_function_ptr mpi_work_count_func)
    {
        polling_function_mpi_.store(mpi_func, std::memory_order_relaxed);
        polling_work_count_function_mpi_.store(
            mpi_work_count_func, std::memory_order_relaxed);
    }

    void scheduler_base::clear_mpi_polling_function()
    {
        polling_function_mpi_.store(
            &null_polling_function, std::memory_order_relaxed);
        polling_work_count_function_mpi_.store(
            &null_polling_work_count_function, std::memory_order_relaxed);
    }

    void scheduler_base::set_cuda_polling_functions(
        polling_function_ptr cuda_func,
        polling_work_count_function_ptr cuda_work_count_func)
    {
        polling_function_cuda_.store(cuda_func, std::memory_order_relaxed);
        polling_work_count_function_cuda_.store(
            cuda_work_count_func, std::memory_order_relaxed);
    }

    void scheduler_base::clear_cuda_polling_function()
    {
        polling_function_cuda_.store(
            &null_polling_function, std::memory_order_relaxed);
        polling_work_count_function_cuda_.store(
            &null_polling_work_count_function, std::memory_order_relaxed);
    }

    void scheduler_base::set_sycl_polling_functions(
        polling_function_ptr sycl_func,
        polling_work_count_function_ptr sycl_work_count_func)
    {
        polling_function_sycl_.store(sycl_func, std::memory_order_relaxed);
        polling_work_count_function_sycl_.store(
            sycl_work_count_func, std::memory_order_relaxed);
    }

    void scheduler_base::clear_sycl_polling_function()
    {
        polling_function_sycl_.store(
            &null_polling_function, std::memory_order_relaxed);
        polling_work_count_function_sycl_.store(
            &null_polling_work_count_function, std::memory_order_relaxed);
    }

    detail::polling_status scheduler_base::custom_polling_function() const
    {
        detail::polling_status status = detail::polling_status::idle;
#if defined(HPX_HAVE_MODULE_ASYNC_MPI)
        if ((*polling_function_mpi_.load(std::memory_order_relaxed))() ==
            detail::polling_status::busy)
        {
            status = detail::polling_status::busy;
        }
#endif
#if defined(HPX_HAVE_MODULE_ASYNC_CUDA)
        if ((*polling_function_cuda_.load(std::memory_order_relaxed))() ==
            detail::polling_status::busy)
        {
            status = detail::polling_status::busy;
        }
#endif
#if defined(HPX_HAVE_MODULE_ASYNC_SYCL)
        if ((*polling_function_sycl_.load(std::memory_order_relaxed))() ==
            detail::polling_status::busy)
        {
            status = detail::polling_status::busy;
        }
#endif
        return status;
    }

    std::size_t scheduler_base::get_polling_work_count() const
    {
        std::size_t work_count = 0;
#if defined(HPX_HAVE_MODULE_ASYNC_MPI)
        work_count +=
            polling_work_count_function_mpi_.load(std::memory_order_relaxed)();
#endif
#if defined(HPX_HAVE_MODULE_ASYNC_CUDA)
        work_count +=
            polling_work_count_function_cuda_.load(std::memory_order_relaxed)();
#endif
#if defined(HPX_HAVE_MODULE_ASYNC_SYCL)
        work_count +=
            polling_work_count_function_sycl_.load(std::memory_order_relaxed)();
#endif
        return work_count;
    }

    std::ostream& operator<<(std::ostream& os, scheduler_base const& scheduler)
    {
        os << scheduler.get_description() << "(" << &scheduler << ")";

        return os;
    }
}    // namespace hpx::threads::policies
