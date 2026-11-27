//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/threading_base/set_thread_affinity.hpp>
#include <hpx/threading_base/set_thread_state.hpp>
#include <hpx/threading_base/thread_helpers.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>

#include <cstdint>

namespace hpx::threads {

    thread_state set_thread_affinity(thread_id_type const& id,
        std::int16_t target_pu, thread_priority priority, error_code& ec)
    {
        if (HPX_UNLIKELY(id == invalid_thread_id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "threads::set_thread_affinity", "null thread id encountered");
            return thread_state{
                thread_schedule_state::unknown, thread_restart_state::unknown};
        }

        if (target_pu < 0 ||
            target_pu >=
                static_cast<std::int16_t>(hpx::threads::hardware_concurrency()))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "threads::set_thread_affinity", "invalid target pu number: {}",
                target_pu);
            return thread_state{
                thread_schedule_state::unknown, thread_restart_state::unknown};
        }

        thread_schedule_hint schedulehint(target_pu);

        thread_state previous_state = detail::set_thread_state(id,
            thread_schedule_state::pending, thread_restart_state::signaled,
            priority, schedulehint, true, ec);

        return previous_state;
    }

}    // namespace hpx::threads

namespace hpx::this_thread {

    void set_affinity(std::int16_t target_pu, threads::thread_priority priority,
        error_code& ec)
    {
        hpx::threads::set_thread_affinity(
            threads::get_self_id(), target_pu, priority, ec);
        if (ec)
            return;

        hpx::this_thread::suspend(
            hpx::threads::thread_schedule_state::suspended);
    }

}    // namespace hpx::this_thread
