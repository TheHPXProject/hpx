//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_init_data.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <forward_list>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>

#if defined(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION)
#include <string>
#endif

#include <hpx/config/warnings_prefix.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    namespace detail {

        HPX_CXX_CORE_EXPORT using get_locality_id_type =
            std::uint32_t(hpx::error_code&);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_get_locality_id(
            get_locality_id_type* f);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::uint32_t get_locality_id(
            hpx::error_code&);
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    /// A \a thread is the representation of a HPX thread. It's a first class
    /// object in HPX. In our implementation this is a user level thread running
    /// on top of one of the OS threads spawned by the \a thread-manager.
    ///
    /// A \a thread encapsulates:
    ///  - A thread status word (see the functions \a thread#get_state and
    ///    \a thread#set_state)
    ///  - A function to execute (the thread function)
    ///  - A frame (in this implementation this is a block of memory used as the
    ///    threads stack)
    ///  - A block of registers (not implemented yet)
    ///
    /// Generally, \a threads are not created or executed directly. All
    /// functionality related to the management of \a threads is implemented by
    /// the thread-manager.
    HPX_CXX_CORE_EXPORT class thread_data
      : public detail::thread_data_reference_counting
    {
    public:
        thread_data(thread_data const&) = delete;
        thread_data(thread_data&&) = delete;
        thread_data& operator=(thread_data const&) = delete;
        thread_data& operator=(thread_data&&) = delete;

    public:
        using spinlock_pool = util::spinlock_pool<thread_data>;

        /// The get_state function queries the state of this thread instance.
        ///
        /// \returns        This function returns the current state of this
        ///                 thread. It will return one of the values as defined
        ///                 by the \a thread_state enumeration.
        ///
        /// \note           This function will seldom be used directly. Most of
        ///                 the time the state of a thread will be retrieved
        ///                 by using the function \a threadmanager#get_state.
        thread_state get_state(std::memory_order const order =
                                   std::memory_order_acquire) const noexcept
        {
            return current_state_.load(order);
        }

        /// The set_state function changes the state of this thread instance.
        ///
        /// \param state    [in] The new state to be set for the thread.
        /// \param state_ex       [in]
        /// \param load_order     [in]
        /// \param exchange_order [in]
        ///
        /// \note           This function will seldom be used directly. Most of
        ///                 the time the state of a thread will have to be
        ///                 changed using the thread-manager. Moreover,
        ///                 changing the thread state using this function does
        ///                 not change its scheduling status. It only sets the
        ///                 thread's status word. To change the thread's
        ///                 scheduling status \a threadmanager#set_state should
        ///                 be used.
        thread_state set_state(thread_schedule_state const state,
            thread_restart_state state_ex = thread_restart_state::unknown,
            std::memory_order const load_order = std::memory_order_acquire,
            std::memory_order const exchange_order =
                std::memory_order_acq_rel) const noexcept
        {
            thread_state prev_state = current_state_.load(load_order);

            for (;;)
            {
                thread_state tmp = prev_state;

                // ABA prevention for state only (not for state_ex)
                std::int64_t tag = tmp.tag();
                if (state != tmp.state())
                    ++tag;

                if (state_ex == thread_restart_state::unknown)
                    state_ex = tmp.state_ex();

                if (HPX_LIKELY(current_state_.compare_exchange_strong(tmp,
                        thread_state(state, state_ex, tag), exchange_order)))
                {
                    return prev_state;
                }

                prev_state = tmp;
            }
        }

        bool set_state_tagged(thread_schedule_state const newstate,
            thread_state const& prev_state, thread_state& new_tagged_state,
            std::memory_order exchange_order =
                std::memory_order_acq_rel) const noexcept
        {
            new_tagged_state = thread_state(
                newstate, prev_state.state_ex(), prev_state.tag() + 1);

            thread_state tmp = prev_state;
            return current_state_.compare_exchange_strong(
                tmp, new_tagged_state, exchange_order);
        }

        /// The restore_state function changes the state of this thread
        /// instance depending on its current state. It will change the state
        /// atomically only if the current state is still the same as passed
        /// as the second parameter. Otherwise, it won't touch the thread state
        /// of this instance.
        ///
        /// \param new_state [in] The new state to be set for the thread.
        /// \param old_state [in] The old state of the thread which still has to
        ///                 be the current state.
        /// \param load_order    [in]
        /// \param load_exchange [in]
        ///
        /// \note           This function will seldom be used directly. Most of
        ///                 the time the state of a thread will have to be
        ///                 changed using the threadmanager. Moreover,
        ///                 changing the thread state using this function does
        ///                 not change its scheduling status. It only sets the
        ///                 thread's status word. To change the thread's
        ///                 scheduling status \a threadmanager#set_state should
        ///                 be used.
        ///
        /// \returns This function returns \a true if the state has been
        ///          changed successfully
        bool restore_state(thread_state const new_state,
            thread_state const old_state,
            std::memory_order const load_order = std::memory_order_relaxed,
            std::memory_order const load_exchange =
                std::memory_order_acq_rel) const noexcept
        {
            // ignore the state_ex while compare-exchanging
            thread_state const current_state = current_state_.load(load_order);
            thread_restart_state const state_ex = current_state.state_ex();

            // ABA prevention for state only (not for state_ex)
            std::int64_t tag = current_state.tag();
            if (new_state.state() != old_state.state())
                ++tag;

            thread_state old_tmp(old_state.state(), state_ex, old_state.tag());
            thread_state const new_tmp(new_state.state(), state_ex, tag);

            return current_state_.compare_exchange_strong(
                old_tmp, new_tmp, load_exchange);
        }

        bool restore_state(thread_schedule_state new_state,
            thread_restart_state const state_ex, thread_state old_state,
            std::memory_order const load_exchange =
                std::memory_order_acq_rel) const noexcept
        {
            // ABA prevention for state only (not for state_ex)
            std::int64_t tag = old_state.tag();
            if (new_state != old_state.state())
                ++tag;

            return current_state_.compare_exchange_strong(old_state,
                thread_state(new_state, state_ex, tag), load_exchange);
        }

    protected:
        /// The set_state function changes the extended state of this
        /// thread instance.
        ///
        /// \param new_state [in] The new extended state to be set for the
        ///                 thread.
        /// \param load_order [in]
        /// \param load_exchange [in]
        ///
        /// \note           This function will seldom be used directly. Most of
        ///                 the time the state of a thread will have to be
        ///                 changed using the threadmanager.
        thread_restart_state set_state_ex(thread_restart_state const new_state,
            std::memory_order const load_order = std::memory_order_acquire,
            std::memory_order const load_exchange =
                std::memory_order_acq_rel) const noexcept
        {
            thread_state prev_state = current_state_.load(load_order);

            for (;;)
            {
                thread_state tmp = prev_state;

                if (HPX_LIKELY(current_state_.compare_exchange_strong(tmp,
                        thread_state(tmp.state(), new_state, tmp.tag()),
                        load_exchange)))
                {
                    return prev_state.state_ex();
                }

                prev_state = tmp;
            }
        }

    public:
        /// Return the id of the component this thread is running in
        static constexpr std::uint64_t    // same as naming::address_type
        get_component_id() noexcept
        {
            return 0;
        }

#if !defined(HPX_HAVE_THREAD_DESCRIPTION)
        static constexpr threads::thread_description get_description() noexcept
        {
            return {"<unknown>"};
        }
        static constexpr threads::thread_description set_description(
            threads::thread_description /*value*/) noexcept
        {
            return {"<unknown>"};
        }

        static constexpr threads::thread_description
        get_lco_description() noexcept    //-V524
        {
            return {"<unknown>"};
        }
        static constexpr threads::thread_description set_lco_description(
            threads::thread_description /*value*/) noexcept    //-V524
        {
            return {"<unknown>"};
        }
#else
        threads::thread_description get_description() const;
        threads::thread_description set_description(
            threads::thread_description value);

        threads::thread_description get_lco_description() const;
        threads::thread_description set_lco_description(
            threads::thread_description value);
#endif

        static char const* get_safe_description(
            threads::thread_description const& description,
            char const* fallback = "<unknown>") noexcept;

#if defined(HPX_HAVE_TRACY)
    private:
        mutable char fiber_name_[64];
#endif

    public:
        char const* get_fiber_name() const noexcept;

#if !defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
        /// Return the locality of the parent thread
        static constexpr std::uint32_t get_parent_locality_id() noexcept
        {
            // this is the same as naming::invalid_locality_id
            return ~static_cast<std::uint32_t>(0);
        }

        /// Return the thread id of the parent thread
        static constexpr thread_id_type get_parent_thread_id() noexcept
        {
            return threads::invalid_thread_id;
        }

        /// Return the phase of the parent thread
        static constexpr std::size_t get_parent_thread_phase() noexcept
        {
            return 0;
        }
#else
        /// Return the locality of the parent thread
        std::uint32_t get_parent_locality_id() const noexcept
        {
            return parent_locality_id_;
        }

        /// Return the thread id of the parent thread
        thread_id_type get_parent_thread_id() const noexcept
        {
            return parent_thread_id_;
        }

        /// Return the phase of the parent thread
        std::size_t get_parent_thread_phase() const noexcept
        {
            return parent_thread_phase_;
        }
#endif

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
        void set_marked_state(thread_schedule_state mark) const noexcept
        {
            marked_state_ = mark;
        }
        thread_schedule_state get_marked_state() const noexcept
        {
            return marked_state_;
        }
#endif

#if !defined(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION)

#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
        static constexpr char const* get_backtrace() noexcept
        {
            return nullptr;
        }
        static constexpr char const* set_backtrace(char const*) noexcept
        {
            return nullptr;
        }
#else
        static constexpr util::backtrace const* get_backtrace() noexcept
        {
            return nullptr;
        }
        static constexpr util::backtrace const* set_backtrace(
            util::backtrace const*) noexcept
        {
            return nullptr;
        }
#endif

#else    // defined(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION

#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
        char const* get_backtrace() const noexcept
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));
            return backtrace_;
        }
        char const* set_backtrace(char const* value) noexcept
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));

            char const* bt = backtrace_;
            backtrace_ = value;
            return bt;
        }
#else
        util::backtrace const* get_backtrace() const noexcept
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));
            return backtrace_;
        }
        util::backtrace const* set_backtrace(
            util::backtrace const* value) noexcept
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));

            util::backtrace const* bt = backtrace_;
            backtrace_ = value;
            return bt;
        }
#endif

        // Generate full backtrace for captured stack
        std::string backtrace()
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));

            std::string bt;
            if (0 != backtrace_)
            {
#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
                bt = *backtrace_;
#else
                bt = backtrace_->trace();
#endif
            }
            return bt;
        }
#endif

        constexpr thread_priority get_priority() const noexcept
        {
            return priority_;
        }
        void set_priority(thread_priority priority) noexcept
        {
            priority_ = priority;
        }

        // handle thread interruption
        constexpr bool interruption_requested() const noexcept
        {
            return requested_interrupt_;
        }

        constexpr bool interruption_enabled() const noexcept
        {
            return enabled_interrupt_;
        }

        bool set_interruption_enabled(bool enable) noexcept
        {
            using std::swap;
            swap(enabled_interrupt_, enable);
            return enable;
        }

        void interrupt(bool const flag = true)
        {
            std::unique_lock<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));
            if (flag && !enabled_interrupt_)
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::thread_not_interruptable,
                    "thread_data::interrupt",
                    "interrupts are disabled for this thread");
            }
            requested_interrupt_ = flag;
        }

        bool interruption_point(bool throw_on_interrupt = true);

        bool add_thread_exit_callback(function<void()> const& f);
        void run_thread_exit_callbacks();
        void free_thread_exit_callbacks();

        // no need to protect the variables related to scoped children as those
        // are supposed to be accessed by ourselves only
        bool runs_as_child(
            std::memory_order mo = std::memory_order_acquire) const noexcept
        {
            return runs_as_child_.load(mo);
        }

        HPX_FORCEINLINE constexpr bool is_stackless() const noexcept
        {
            return is_stackless_;
        }

        void destroy_thread() override;

        constexpr policies::scheduler_base* get_scheduler_base() const noexcept
        {
            return scheduler_base_;
        }

        constexpr std::uint16_t get_last_worker_thread_num() const noexcept
        {
            return last_worker_thread_num_;
        }

        void set_last_worker_thread_num(
            std::uint16_t last_worker_thread_num) noexcept
        {
            last_worker_thread_num_ = last_worker_thread_num;
        }

        constexpr std::ptrdiff_t get_stack_size() const noexcept
        {
            return stacksize_enum_ == thread_stacksize::nostack ?
                (std::numeric_limits<std::ptrdiff_t>::max)() :
                stacksize_;
        }

        thread_stacksize get_stack_size_enum() const noexcept
        {
            return stacksize_enum_;
        }

        template <typename ThreadQueue>
        constexpr ThreadQueue& get_queue() noexcept
        {
            return *static_cast<ThreadQueue*>(queue_);
        }

        /// \brief Execute the thread function
        ///
        /// \returns        This function returns the thread state the thread
        ///                 should be scheduled from this point on. The thread
        ///                 manager will use the returned value to set the
        ///                 thread's scheduling status.
        inline coroutine_type::result_type operator()(
            hpx::execution_base::this_thread::detail::agent_storage*
                agent_storage);

        /// \brief Directly execute the thread function (inline)
        ///
        /// \returns        This function returns the thread state the thread
        ///                 should be scheduled from this point on. The thread
        ///                 manager will use the returned value to set the
        ///                 thread's scheduling status.
        inline coroutine_type::result_type invoke_directly();

        virtual thread_id_type get_thread_id() const
        {
            return thread_id_type{const_cast<thread_data*>(this)};
        }

#if !defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        virtual std::size_t get_thread_phase() const noexcept
        {
            return 0;
        }
#else
        virtual std::size_t get_thread_phase() const noexcept = 0;
#endif
        virtual std::size_t get_thread_data() const = 0;
        virtual std::size_t set_thread_data(std::size_t data) = 0;

#if defined(HPX_HAVE_LIBCDS)
        virtual std::size_t get_libcds_data() const = 0;
        virtual std::size_t set_libcds_data(std::size_t data) = 0;
        virtual std::size_t get_libcds_hazard_pointer_data() const = 0;
        virtual std::size_t set_libcds_hazard_pointer_data(
            std::size_t data) = 0;
        virtual std::size_t get_libcds_dynamic_hazard_pointer_data() const = 0;
        virtual std::size_t set_libcds_dynamic_hazard_pointer_data(
            std::size_t data) = 0;
#endif

        virtual void init() = 0;
        virtual void rebind(thread_init_data& init_data) = 0;

        hpx::tracing::task_timer_data get_timer_data() const noexcept
        {
            return this->timer_data_;
        }
        void set_timer_data(hpx::tracing::task_timer_data data) noexcept
        {
            this->timer_data_ = HPX_MOVE(data);
        }

        // Construct a new \a thread
        thread_data(thread_init_data& init_data, void* queue,
            std::ptrdiff_t stacksize, bool is_stackless = false,
            thread_id_addref addref = thread_id_addref::yes);

        ~thread_data() override;
        virtual void destroy() noexcept = 0;

    protected:
        void rebind_base(thread_init_data& init_data);

    private:
        thread_priority priority_;

        bool requested_interrupt_;
        bool enabled_interrupt_;
        bool ran_exit_funcs_;
        bool const is_stackless_;

        // support scoped child execution
        std::atomic<bool> runs_as_child_;

        std::uint16_t last_worker_thread_num_;

        thread_stacksize stacksize_enum_;
        HPX_NO_UNIQUE_ADDRESS hpx::tracing::task_timer_data timer_data_;
        std::int32_t stacksize_;

        mutable std::atomic<thread_state> current_state_;

        // Singly linked list (heap-allocated)
        std::forward_list<hpx::function<void()>> exit_funcs_;

        // reference to scheduler which created/manages this thread
        policies::scheduler_base* scheduler_base_;

        void* queue_;

        ///////////////////////////////////////////////////////////////////////
        // Debugging/logging information
#ifdef HPX_HAVE_THREAD_DESCRIPTION
        threads::thread_description description_;
        threads::thread_description lco_description_;
#endif

#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
        mutable thread_schedule_state marked_state_;
#endif

#ifdef HPX_HAVE_THREAD_PARENT_REFERENCE
        std::uint32_t parent_locality_id_;
        thread_id_type parent_thread_id_;
        std::size_t parent_thread_phase_;
#endif

#ifdef HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION
#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
        char const* backtrace_ = nullptr;
#else
        util::backtrace const* backtrace_ = nullptr;
#endif
#endif
    };

    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE constexpr thread_data*
    get_thread_id_data(thread_id_ref_type const& tid) noexcept
    {
        return static_cast<thread_data*>(tid.get().get());
    }

    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE constexpr thread_data*
    get_thread_id_data(thread_id_type const& tid) noexcept
    {
        return static_cast<thread_data*>(tid.get());
    }

#if defined(HPX_HAVE_TRACY)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT tracing::region_init_data
    get_region_init_data(thread_data const* thrdptr);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT tracing::fiber_region_init_data
    get_fiber_region_init_data(thread_data const* thrdptr);
#elif defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT tracing::region_init_data
    get_region_init_data(thread_data const* thrdptr);

    HPX_CXX_CORE_EXPORT constexpr tracing::fiber_region_init_data
    get_fiber_region_init_data(thread_data const*) noexcept
    {
        return {};
    }
#else
    HPX_CXX_CORE_EXPORT constexpr tracing::region_init_data
    get_region_init_data(thread_data const*) noexcept
    {
        return {};
    }

    HPX_CXX_CORE_EXPORT constexpr tracing::fiber_region_init_data
    get_fiber_region_init_data(thread_data const*) noexcept
    {
        return {};
    }
#endif
}    // namespace hpx::threads

#include <hpx/config/warnings_suffix.hpp>

#include <hpx/threading_base/thread_data_stackful.hpp>
#include <hpx/threading_base/thread_data_stackless.hpp>

namespace hpx::threads {

    HPX_FORCEINLINE coroutine_type::result_type thread_data::operator()(
        hpx::execution_base::this_thread::detail::agent_storage* agent_storage)
    {
        // once a thread has started it can't be run directly anymore
        runs_as_child_.store(false, std::memory_order_release);

        if (is_stackless())
        {
            HPX_ASSERT(dynamic_cast<thread_data_stackless*>(this) != nullptr);
            return static_cast<thread_data_stackless*>(this)->call();
        }

        HPX_ASSERT(dynamic_cast<thread_data_stackful*>(this) != nullptr);
        return static_cast<thread_data_stackful*>(this)->call(agent_storage);
    }

    HPX_FORCEINLINE coroutine_type::result_type thread_data::invoke_directly()
    {
        HPX_ASSERT(runs_as_child(std::memory_order_relaxed));

        if (is_stackless())
        {
            HPX_ASSERT(dynamic_cast<thread_data_stackless*>(this) != nullptr);
            return static_cast<thread_data_stackless*>(this)->call();
        }

        HPX_ASSERT(dynamic_cast<thread_data_stackful*>(this) != nullptr);
        return static_cast<thread_data_stackful*>(this)->call_directly();
    }
}    // namespace hpx::threads
