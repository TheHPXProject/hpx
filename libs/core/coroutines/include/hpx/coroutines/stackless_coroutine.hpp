//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/coroutines/coroutine.hpp>
#include <hpx/coroutines/detail/coroutine_self.hpp>
#include <hpx/coroutines/thread_enums.hpp>
#include <hpx/coroutines/thread_id_type.hpp>
#include <hpx/modules/functional.hpp>
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
#include <hpx/coroutines/detail/tss.hpp>
#endif

#include <cstddef>
#include <limits>
#include <utility>

namespace hpx::threads::coroutines {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class stackless_coroutine
    {
    private:
        friend struct detail::coroutine_accessor;

        enum class context_state
        {
            running,    // context running.
            ready,      // context at yield point.
            exited      // context is finished.
        };

        static constexpr std::ptrdiff_t const default_stack_size = -1;

        constexpr bool running() const noexcept
        {
            return state_ == context_state::running;
        }

        constexpr bool exited() const noexcept
        {
            return state_ == context_state::exited;
        }

    public:
        using thread_id_type = hpx::threads::thread_id;

        using result_type = std::pair<thread_schedule_state, thread_id_type>;
        using arg_type = thread_restart_state;

        using functor_type = hpx::move_only_function<result_type(arg_type)>;

        stackless_coroutine(functor_type&& f, thread_id_type id,
            std::ptrdiff_t /*stack_size*/ = default_stack_size) noexcept
          : f_(HPX_MOVE(f))
          , state_(context_state::ready)
          , id_(HPX_MOVE(id))
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
          , phase_(0)
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
          , thread_data_(nullptr)
#else
          , thread_data_(0)
#endif
          , continuation_recursion_count_(0)
        {
        }

        ~stackless_coroutine()
        {
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
            detail::delete_tss_storage(thread_data_);
#else
            thread_data_ = 0;
#endif
        }

        stackless_coroutine(stackless_coroutine const& src) = delete;
        stackless_coroutine& operator=(stackless_coroutine const& src) = delete;
        stackless_coroutine(stackless_coroutine&& src) = delete;
        stackless_coroutine& operator=(stackless_coroutine&& src) = delete;

        constexpr thread_id_type get_thread_id() const noexcept
        {
            return id_;
        }

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        constexpr std::size_t get_thread_phase() const noexcept
        {
            return phase_;
        }
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        std::size_t get_thread_data() const
        {
            if (!thread_data_)
                return 0;
            return detail::get_tss_thread_data(thread_data_);
        }
#else
        constexpr std::size_t get_thread_data() const noexcept
        {
            return thread_data_;
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        std::size_t set_thread_data(std::size_t data)
        {
            return detail::set_tss_thread_data(thread_data_, data);
        }
#else
        std::size_t set_thread_data(std::size_t const data) const noexcept
        {
            std::size_t const olddata = thread_data_;
            thread_data_ = data;
            return olddata;
        }
#endif

#if defined(HPX_HAVE_LIBCDS)
        std::size_t get_libcds_data() const
        {
            return libcds_data_;
        }

        std::size_t set_libcds_data(std::size_t data)
        {
            std::swap(data, libcds_data_);
            return data;
        }

        std::size_t get_libcds_hazard_pointer_data() const
        {
            return libcds_hazard_pointer_data_;
        }

        std::size_t set_libcds_hazard_pointer_data(std::size_t data)
        {
            std::swap(data, libcds_hazard_pointer_data_);
            return data;
        }

        std::size_t get_libcds_dynamic_hazard_pointer_data() const
        {
            return libcds_dynamic_hazard_pointer_data_;
        }

        std::size_t set_libcds_dynamic_hazard_pointer_data(std::size_t data)
        {
            std::swap(data, libcds_dynamic_hazard_pointer_data_);
            return data;
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        detail::tss_storage* get_thread_tss_data(bool create_if_needed) const
        {
            if (!thread_data_ && create_if_needed)
                thread_data_ = detail::create_tss_storage();
            return thread_data_;
        }
#endif

        void rebind(functor_type&& f, thread_id_type id) noexcept
        {
            HPX_ASSERT(exited());

            f_ = HPX_MOVE(f);
            id_ = HPX_MOVE(id);

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            phase_ = 0;
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
            HPX_ASSERT(thread_data_ == nullptr);
#else
            HPX_ASSERT(thread_data_ == 0);
#endif
            state_ = context_state::ready;
        }

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        void reset_tss()
        {
            detail::delete_tss_storage(thread_data_);
        }
#else
        void reset_tss() const noexcept
        {
            thread_data_ = 0;
        }
#endif

        void reset()
        {
            HPX_ASSERT(exited());

            util::detail::reset_function(f_);

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            phase_ = 0;
#endif
            id_.reset();
        }

    public:
        HPX_FORCEINLINE result_type operator()(arg_type arg = arg_type());

        explicit constexpr operator bool() const noexcept
        {
            return !exited();
        }

        constexpr bool is_ready() const noexcept
        {
            return state_ == context_state::ready;
        }

        static constexpr std::ptrdiff_t get_available_stack_space() noexcept
        {
            return (std::numeric_limits<std::ptrdiff_t>::max)();
        }

        std::size_t& get_continuation_recursion_count() noexcept
        {
            return continuation_recursion_count_;
        }

    protected:
        functor_type f_;
        context_state state_;
        thread_id_type id_;

#ifdef HPX_HAVE_THREAD_PHASE_INFORMATION
        std::size_t phase_;
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        mutable detail::tss_storage* thread_data_;
#else
        mutable std::size_t thread_data_;
#endif
        std::size_t continuation_recursion_count_;
#if defined(HPX_HAVE_LIBCDS)
        mutable std::size_t libcds_data_;
        mutable std::size_t libcds_hazard_pointer_data_;
        mutable std::size_t libcds_dynamic_hazard_pointer_data_;
#endif
    };
}    // namespace hpx::threads::coroutines

////////////////////////////////////////////////////////////////////////////////
#include <hpx/coroutines/detail/coroutine_stackless_self.hpp>

namespace hpx::threads::coroutines {

    HPX_FORCEINLINE stackless_coroutine::result_type
    stackless_coroutine::operator()(arg_type const arg)
    {
        HPX_ASSERT(is_ready());

        result_type result;

        {
            // A stackless coroutine may be run directly by a parent waiting on
            // it, so the caller's self is put back on exit, not cleared.
            detail::coroutine_self* old_self =
                detail::coroutine_self::get_self();
            detail::coroutine_stackless_self self(this);

            detail::coroutine_self::set_self(&self);
            auto on_exit = hpx::experimental::scope_exit(
                [old_self] { detail::coroutine_self::set_self(old_self); });

            {
                state_ = context_state::running;
                auto on_exit_inner = hpx::experimental::scope_exit(
                    [this] { state_ = context_state::exited; });

                result = f_(arg);    // invoke wrapped function

                // we always have to run to completion
                HPX_ASSERT(
                    result.first == threads::thread_schedule_state::terminated);
            }

            reset_tss();
            reset();
        }

        return result;
    }
}    // namespace hpx::threads::coroutines
