//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>

#include <cstdint>

namespace hpx::threads::detail {

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT thread_state set_thread_state(
        thread_id_type const& id, thread_schedule_state new_state,
        thread_restart_state new_state_ex, thread_priority priority,
        thread_schedule_hint schedulehint = thread_schedule_hint(),
        bool retry_on_active = true, error_code& ec = throws);

    /// Set the state of the given thread only while it is still in the
    /// suspension whose state carries the tag \a suspension_tag. If the thread
    /// is still running towards that suspension (it is active with the tag
    /// before it), wait until it has suspended. If that suspension has ended
    /// already, leave the thread alone.
    ///
    /// \returns true if the state of the thread has been changed.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool set_suspended_thread_state(
        thread_id_type const& id, std::int64_t suspension_tag,
        thread_schedule_state new_state, thread_restart_state new_state_ex,
        thread_priority priority,
        thread_schedule_hint schedulehint = thread_schedule_hint());
}    // namespace hpx::threads::detail
