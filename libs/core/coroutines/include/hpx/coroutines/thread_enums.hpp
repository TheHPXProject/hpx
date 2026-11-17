//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/coroutines/thread_enums.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/coroutines/detail/combined_tagged_state.hpp>
#include <hpx/modules/thread_support.hpp>

namespace hpx::threads {

    /// \cond NOINTERNAL
    // special type storing both state in one tagged structure
    HPX_CXX_CORE_EXPORT using thread_state =
        threads::detail::combined_tagged_state<thread_schedule_state,
            thread_restart_state>;
    /// \endcond

    /// Get the readable string representing the name of the given
    /// thread_state constant.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_thread_state_name(
        thread_state state) noexcept;
}    // namespace hpx::threads
