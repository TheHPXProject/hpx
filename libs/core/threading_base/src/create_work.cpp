//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/threading_base/create_work.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_init_data.hpp>

#include <cstddef>

namespace hpx::threads::detail {

    thread_id_ref_type create_work(policies::scheduler_base* scheduler,
        threads::thread_init_data& data, error_code& ec)
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
                "thread::detail::create_work", "invalid initial state: {}",
                data.initial_state);
            return invalid_thread_id;
        }
        }

#ifdef HPX_HAVE_THREAD_DESCRIPTION
        if (!data.description)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "thread::detail::create_work", "description is nullptr");
            return invalid_thread_id;
        }
#endif

        LTM_(info)
            .format("create_work: pool({}), scheduler({}), initial_state({}), "
                    "thread_priority({})",
                *scheduler->get_parent_pool(), *scheduler,
                get_thread_state_name(data.initial_state),
                get_thread_priority_name(data.priority))
#ifdef HPX_HAVE_THREAD_DESCRIPTION
            .format(", description({})", data.description)
#endif
            ;

        thread_self const* self = get_self_ptr();

#ifdef HPX_HAVE_THREAD_PARENT_REFERENCE
        if (nullptr == data.parent_id)
        {
            if (self)
            {
                data.parent_id = get_thread_id_data(self->get_thread_id());
                data.parent_phase = self->get_thread_phase();
            }
        }
        if (0 == data.parent_locality_id)
            data.parent_locality_id = detail::get_locality_id(hpx::throws);
#endif

        if (nullptr == data.scheduler_base)
            data.scheduler_base = scheduler;

        // Pass critical priority from parent to child.
        if (self)
        {
            if (data.priority == thread_priority::default_ &&
                thread_priority::high_recursive ==
                    get_thread_id_data(self->get_thread_id())->get_priority())
            {
                data.priority = thread_priority::high_recursive;
            }
        }

        // create the new thread
        if (data.priority == thread_priority::default_)
        {
            data.priority = thread_priority::normal;
        }

        HPX_ASSERT(!data.run_now);
        data.run_now = (thread_priority::high == data.priority ||
            thread_priority::high_recursive == data.priority ||
            thread_priority::bound == data.priority ||
            thread_priority::boost == data.priority);

        thread_id_ref_type id = invalid_thread_id;
        scheduler->create_thread(data, data.run_now ? &id : nullptr, ec);

        scheduler->do_some_work(data.schedulehint.mode() ==
                    hpx::threads::thread_schedule_hint_mode::numa ?
                static_cast<std::size_t>(-1) :
                data.schedulehint.hint);

        return id;
    }
}    // namespace hpx::threads::detail
