//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/threading_base/scheduler_mode.hpp>
#include <hpx/threading_base/scheduler_state.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_init_data.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>
#include <hpx/threading_base/thread_queue_init_parameters.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>
#if defined(HPX_HAVE_SCHEDULER_LOCAL_STORAGE)
#include <hpx/modules/coroutines.hpp>
#endif

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::policies {

    namespace detail {

        enum class polling_status
        {
            /// Signals that a polling function currently has no more work to do
            idle = 0,

            /// Signals that a polling function still has outstanding work to
            /// poll for
            busy = 1
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The scheduler_base defines the interface to be implemented by all
    /// scheduler policies
    HPX_CXX_CORE_EXPORT struct scheduler_base
    {
        scheduler_base(scheduler_base const&) = delete;
        scheduler_base(scheduler_base&&) = delete;
        scheduler_base& operator=(scheduler_base const&) = delete;
        scheduler_base& operator=(scheduler_base&&) = delete;

        using pu_mutex_type = std::mutex;

        explicit scheduler_base(std::size_t num_threads,
            char const* description = "",
            thread_queue_init_parameters const& thread_queue_init =
                thread_queue_init_parameters{},
            scheduler_mode mode = scheduler_mode::nothing_special);

        virtual ~scheduler_base() = default;

        threads::thread_pool_base* get_parent_pool() const noexcept
        {
            HPX_ASSERT(parent_pool_ != nullptr);
            return parent_pool_;
        }

        void set_parent_pool(threads::thread_pool_base* p) noexcept
        {
            HPX_ASSERT(parent_pool_ == nullptr);
            parent_pool_ = p;
        }

        std::size_t global_to_local_thread_index(std::size_t n) const
        {
            return n - parent_pool_->get_thread_offset();
        }

        std::size_t local_to_global_thread_index(std::size_t n) const
        {
            return n + parent_pool_->get_thread_offset();
        }

        constexpr char const* get_description() const noexcept
        {
            return description_;
        }

        bool idle_callback(std::size_t num_thread);

        /// This function gets called by the thread-manager whenever new work
        /// has been added, allowing the scheduler to reactivate one or more of
        /// possibly idling OS threads
        void do_some_work(std::size_t);

        virtual void suspend(std::size_t num_thread);
        virtual void resume(std::size_t num_thread);

        std::size_t select_active_pu(
            std::size_t num_thread, bool allow_fallback = false);

        // allow to access/manipulate states
        std::atomic<hpx::state>& get_state(std::size_t num_thread);
        std::atomic<hpx::state> const& get_state(std::size_t num_thread) const;
        void set_all_states(hpx::state s);
        void set_all_states_at_least(hpx::state s);

        // return whether all states are at least at the given one
        bool has_reached_state(hpx::state s) const;
        bool is_state(hpx::state s) const;
        std::pair<hpx::state, hpx::state> get_minmax_state() const;

        ///////////////////////////////////////////////////////////////////////
        // get/set scheduler mode
        scheduler_mode get_scheduler_mode(std::size_t thread_num) const noexcept
        {
            return modes_[thread_num].data_.load(std::memory_order_relaxed);
        }

        // get/set scheduler mode
        bool has_scheduler_mode(
            scheduler_mode mode, std::size_t thread_num) const noexcept
        {
            return (modes_[thread_num].data_.load(std::memory_order_relaxed) &
                       mode) != 0;
        }

        // set mode flags that control scheduler behaviour
        // This set function is virtual so that flags may be overridden
        // by schedulers that do not support certain operations/modes.
        // All other mode set functions should call this one to ensure
        // that flags are always consistent
        virtual void set_scheduler_mode(
            scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept;

        // add a flag to the scheduler mode flags
        void add_scheduler_mode(
            scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept;

        // remove flag from scheduler mode
        void remove_scheduler_mode(
            scheduler_mode mode, hpx::threads::mask_cref_type pu_mask) noexcept;

        // add flag to scheduler mode
        void add_remove_scheduler_mode(scheduler_mode to_add_mode,
            scheduler_mode to_remove_mode,
            hpx::threads::mask_cref_type pu_mask) noexcept;

        // conditionally add or remove depending on set true/false
        void update_scheduler_mode(scheduler_mode mode, bool set,
            hpx::threads::mask_cref_type pu_mask) noexcept;

        pu_mutex_type& get_pu_mutex(std::size_t num_thread) noexcept
        {
            HPX_ASSERT(num_thread < pu_mtxs_.size());
            return pu_mtxs_[num_thread];
        }

        ///////////////////////////////////////////////////////////////////////
#ifdef HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        virtual std::uint64_t get_creation_time(bool reset) = 0;
        virtual std::uint64_t get_cleanup_time(bool reset) = 0;
#endif

#ifdef HPX_HAVE_THREAD_STEALING_COUNTS
        virtual std::int64_t get_num_pending_misses(
            std::size_t num_thread, bool reset) = 0;
        virtual std::int64_t get_num_pending_accesses(
            std::size_t num_thread, bool reset) = 0;

        virtual std::int64_t get_num_stolen_from_pending(
            std::size_t num_thread, bool reset) = 0;
        virtual std::int64_t get_num_stolen_to_pending(
            std::size_t num_thread, bool reset) = 0;
        virtual std::int64_t get_num_stolen_from_staged(
            std::size_t num_thread, bool reset) = 0;
        virtual std::int64_t get_num_stolen_to_staged(
            std::size_t num_thread, bool reset) = 0;
#endif

        virtual std::int64_t get_queue_length(
            std::size_t num_thread = static_cast<std::size_t>(-1)) const = 0;

        virtual std::int64_t get_thread_count(
            thread_schedule_state state = thread_schedule_state::unknown,
            thread_priority priority = thread_priority::default_,
            std::size_t num_thread = static_cast<std::size_t>(-1),
            bool reset = false) const = 0;

        // Queries whether a given core is idle
        virtual bool is_core_idle(std::size_t num_thread) const = 0;

        // count active background threads
        std::int64_t get_background_thread_count() const noexcept;
        void increment_background_thread_count() noexcept;
        void decrement_background_thread_count() noexcept;

        // Enumerate all matching threads
        virtual bool enumerate_threads(
            hpx::function<bool(thread_id_type)> const& f,
            thread_schedule_state state =
                thread_schedule_state::unknown) const = 0;

        virtual void abort_all_suspended_threads() = 0;

        virtual bool cleanup_terminated(bool delete_all) = 0;
        virtual bool cleanup_terminated(
            std::size_t num_thread, bool delete_all) = 0;

        virtual void create_thread(
            thread_init_data& data, thread_id_ref_type* id, error_code& ec) = 0;

        virtual void schedule_thread(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) = 0;

        virtual void schedule_thread_last(threads::thread_id_ref_type thrd,
            threads::thread_schedule_hint schedulehint,
            bool allow_fallback = false,
            thread_priority priority = thread_priority::default_) = 0;

        virtual void destroy_thread(threads::thread_data* thrd) = 0;

        virtual void on_start_thread(std::size_t num_thread) = 0;
        virtual void on_stop_thread(std::size_t num_thread) = 0;
        virtual void on_error(
            std::size_t num_thread, std::exception_ptr const& e) = 0;

#ifdef HPX_HAVE_THREAD_QUEUE_WAITTIME
        virtual std::int64_t get_average_thread_wait_time(
            std::size_t num_thread = std::size_t(-1)) const = 0;
        virtual std::int64_t get_average_task_wait_time(
            std::size_t num_thread = std::size_t(-1)) const = 0;
#endif

        virtual void reset_thread_distribution() noexcept {}

        std::ptrdiff_t get_stack_size(
            threads::thread_stacksize stacksize) const noexcept;

        using polling_function_ptr = detail::polling_status (*)();
        using polling_work_count_function_ptr = std::size_t (*)();

        static constexpr detail::polling_status null_polling_function() noexcept
        {
            return detail::polling_status::idle;
        }

        static constexpr std::size_t null_polling_work_count_function() noexcept
        {
            return 0;
        }

        void set_mpi_polling_functions(polling_function_ptr mpi_func,
            polling_work_count_function_ptr mpi_work_count_func);
        void clear_mpi_polling_function();
        void set_cuda_polling_functions(polling_function_ptr cuda_func,
            polling_work_count_function_ptr cuda_work_count_func);
        void clear_cuda_polling_function();
        void set_sycl_polling_functions(polling_function_ptr sycl_func,
            polling_work_count_function_ptr sycl_work_count_func);
        void clear_sycl_polling_function();

        detail::polling_status custom_polling_function() const;
        std::size_t get_polling_work_count() const;

        // almost all schedulers support direct execution
        virtual bool supports_direct_execution() const noexcept
        {
            return true;
        }

    protected:
        // the scheduler mode, protected from false sharing
        std::vector<util::cache_line_data<std::atomic<scheduler_mode>>> modes_;

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        // support for suspension on idle queues
        double max_idle_backoff_time_;
        struct idle_backoff_data
        {
            pu_mutex_type wait_mtx;
            std::condition_variable wait_cond;
            std::atomic<std::uint32_t> wait_count{0};
        };
        std::vector<util::cache_line_data<idle_backoff_data>> wait_count_data_;
#endif

        // support for suspension of pus
        std::vector<pu_mutex_type> suspend_mtxs_;
        std::vector<std::condition_variable> suspend_conds_;

        std::vector<pu_mutex_type> pu_mtxs_;

        std::vector<util::cache_line_data<std::atomic<hpx::state>>> states_;
        char const* description_;

        thread_queue_init_parameters thread_queue_init_;

        // the pool that owns this scheduler
        threads::thread_pool_base* parent_pool_;

        std::atomic<std::int64_t> background_thread_count_;

        std::atomic<polling_function_ptr> polling_function_mpi_;
        std::atomic<polling_function_ptr> polling_function_cuda_;
        std::atomic<polling_function_ptr> polling_function_sycl_;
        std::atomic<polling_work_count_function_ptr>
            polling_work_count_function_mpi_;
        std::atomic<polling_work_count_function_ptr>
            polling_work_count_function_cuda_;
        std::atomic<polling_work_count_function_ptr>
            polling_work_count_function_sycl_;

#if defined(HPX_HAVE_SCHEDULER_LOCAL_STORAGE)
    public:
        // manage scheduler-local data
        coroutines::detail::tss_data_node* find_tss_data(void const* key);
        void add_new_tss_node(void const* key,
            std::shared_ptr<coroutines::detail::tss_cleanup_function> const&
                func,
            void* tss_data);
        void erase_tss_node(void const* key, bool cleanup_existing);
        void* get_tss_data(void const* key);
        void set_tss_data(void const* key,
            std::shared_ptr<coroutines::detail::tss_cleanup_function> const&
                func,
            void* tss_data, bool cleanup_existing);

    protected:
        std::shared_ptr<coroutines::detail::tss_storage> thread_data_;
#endif
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, scheduler_base const& scheduler);
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
