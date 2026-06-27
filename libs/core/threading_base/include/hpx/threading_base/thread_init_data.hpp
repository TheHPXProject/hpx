//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class thread_init_data
    {
    public:
        thread_init_data()
          : func()
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
          , description()
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
          , parent_locality_id(0)
          , parent_id(nullptr)
          , parent_phase(0)
#endif
          , timer_data()
          , schedulehint()
          , priority(thread_priority::normal)
          , stacksize(thread_stacksize::default_)
          , initial_state(thread_schedule_state::pending)
          , run_now(false)
          , scheduler_base(nullptr)
        {
            if (initial_state == thread_schedule_state::staged)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_init_data::thread_init_data",
                    "threads shouldn't have 'staged' as their initial state");
            }
        }

        thread_init_data& operator=(thread_init_data const& rhs) = delete;
        thread_init_data& operator=(thread_init_data&& rhs) noexcept
        {
            func = HPX_MOVE(rhs.func);
            schedulehint = rhs.schedulehint;
            priority = rhs.priority;
            stacksize = rhs.stacksize;
            initial_state = rhs.initial_state;
            run_now = rhs.run_now;
            scheduler_base = rhs.scheduler_base;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            description = HPX_MOVE(rhs.description);
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            parent_locality_id = rhs.parent_locality_id;
            parent_id = HPX_MOVE(rhs.parent_id);
            parent_phase = rhs.parent_phase;
#endif
            timer_data = HPX_MOVE(rhs.timer_data);
            return *this;
        }

        thread_init_data(thread_init_data const& rhs) = delete;
        thread_init_data(thread_init_data&& rhs) noexcept
          : func(HPX_MOVE(rhs.func))
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
          , description(HPX_MOVE(rhs.description))
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
          , parent_locality_id(rhs.parent_locality_id)
          , parent_id(HPX_MOVE(rhs.parent_id))
          , parent_phase(rhs.parent_phase)
#endif
          , timer_data(HPX_MOVE(rhs.timer_data))
          , schedulehint(rhs.schedulehint)
          , priority(rhs.priority)
          , stacksize(rhs.stacksize)
          , initial_state(rhs.initial_state)
          , run_now(rhs.run_now)
          , scheduler_base(rhs.scheduler_base)
        {
        }

        template <typename F>
        thread_init_data(F&& f,
            [[maybe_unused]] threads::thread_description const& desc,
            thread_priority priority_ = thread_priority::normal,
            thread_schedule_hint os_thread = thread_schedule_hint(),
            thread_stacksize stacksize_ = thread_stacksize::default_,
            thread_schedule_state initial_state_ =
                thread_schedule_state::pending,
            bool run_now_ = false,
            policies::scheduler_base* scheduler_base_ = nullptr)
          : func(HPX_FORWARD(F, f))
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
          , description(desc)
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
          , parent_locality_id(0)
          , parent_id(nullptr)
          , parent_phase(0)
#endif
          , timer_data(setup_timer_data(desc, 0, threads::thread_id_type{}))
          , schedulehint(os_thread)
          , priority(priority_)
          , stacksize(stacksize_)
          , initial_state(initial_state_)
          , run_now(run_now_)
          , scheduler_base(scheduler_base_)
        {
            if (initial_state == thread_schedule_state::staged)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_init_data::thread_init_data",
                    "threads shouldn't have 'staged' as their initial state");
            }
        }

        threads::thread_function_type func;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        threads::thread_description description;
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
        std::uint32_t parent_locality_id;
        threads::thread_id_type parent_id;
        std::size_t parent_phase;
#endif
        HPX_NO_UNIQUE_ADDRESS hpx::tracing::task_timer_data timer_data;

        static hpx::tracing::task_timer_data setup_timer_data(
            [[maybe_unused]] threads::thread_description const& desc,
            [[maybe_unused]] std::uint32_t parent_locality_id,
            [[maybe_unused]] threads::thread_id_type const& parent_id)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION) &&                                    \
    defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            return hpx::tracing::create_task_timer(
                desc, parent_locality_id, parent_id);
#else
            return hpx::tracing::task_timer_data{};
#endif
        }

        static hpx::tracing::task_timer_data setup_timer_data(
            [[maybe_unused]] thread_init_data const& data)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION) &&                                    \
    defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            return hpx::tracing::create_task_timer(
                data.description, data.parent_locality_id, data.parent_id);
#else
            return hpx::tracing::task_timer_data{};
#endif
        }

        thread_schedule_hint schedulehint;
        thread_priority priority;
        thread_stacksize stacksize;
        thread_schedule_state initial_state;
        bool run_now;

        policies::scheduler_base* scheduler_base;
    };
}    // namespace hpx::threads
