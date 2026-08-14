//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/create_thread.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_init_data.hpp>

#include <cstddef>

namespace hpx::threads::detail {

    void create_thread(policies::scheduler_base* scheduler,
        threads::thread_init_data& data, threads::thread_id_ref_type& id,
        error_code& ec)
    {
        // verify parameters
        switch (data.initial_state)
        {
        // NOLINTNEXTLINE(bugprone-branch-clone)
        case thread_schedule_state::pending:
            [[fallthrough]];
        case thread_schedule_state::pending_do_not_schedule:
            [[fallthrough]];
        case thread_schedule_state::pending_boost:
            [[fallthrough]];
        case thread_schedule_state::suspended:
            break;

        default:
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "threads::detail::create_thread", "invalid initial state: {}",
                data.initial_state);
            return;
        }
        }

#ifdef HPX_HAVE_THREAD_DESCRIPTION
        if (!data.description)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "threads::detail::create_thread", "description is nullptr");
            return;
        }
#endif

        thread_self const* self = get_self_ptr();

        void const* parent_task_id = nullptr;
#ifdef HPX_HAVE_THREAD_PARENT_REFERENCE
        if (nullptr == data.parent_id)
        {
            if (self)
            {
                data.parent_id = get_thread_id_data(self->get_thread_id());
                data.parent_phase = self->get_thread_phase();
                parent_task_id = data.parent_id.get();
            }
        }
        else
        {
            parent_task_id = data.parent_id.get();
        }

        if (0 == data.parent_locality_id)
            data.parent_locality_id = detail::get_locality_id(hpx::throws);
#endif

        if (nullptr == data.scheduler_base)
            data.scheduler_base = scheduler;

        // Pass critical priority from parent to child (but only if there is
        // none is explicitly specified).
        if (self)
        {
            if (data.priority == thread_priority::default_ &&
                thread_priority::high_recursive ==
                    get_thread_id_data(self->get_thread_id())->get_priority())
            {
                data.priority = thread_priority::high_recursive;
            }
        }

        if (data.priority == thread_priority::default_)
            data.priority = thread_priority::normal;

#ifdef HPX_HAVE_THREAD_DESCRIPTION
        hpx::tracing::task_staged(threads::thread_data::get_safe_description(
                                      data.description, "thread"),
            parent_task_id);
#else
        hpx::tracing::task_staged("thread", parent_task_id);
#endif

        // create the new thread
        scheduler->create_thread(data, &id, ec);

        LTM_(info)
            .format("create_thread: pool({}), scheduler({}), thread({}), "
                    "initial_state({}), run_now({})",
                *scheduler->get_parent_pool(), *scheduler, id,
                get_thread_state_name(data.initial_state), data.run_now)
#ifdef HPX_HAVE_THREAD_DESCRIPTION
            .format(", description({})", data.description)
#endif
            ;

        scheduler->do_some_work(data.schedulehint.mode ==
                    hpx::threads::thread_schedule_hint_mode::numa ?
                static_cast<std::size_t>(-1) :
                data.schedulehint.hint);
    }
}    // namespace hpx::threads::detail
