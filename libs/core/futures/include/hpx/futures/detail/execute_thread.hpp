//  Copyright (c) 2019-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/threading_base.hpp>

namespace hpx::threads::detail {

    // The caller keeps its reference to the thread, so the thread data stays
    // valid after this returns (also when the thread has terminated).
    HPX_CORE_EXPORT bool execute_thread(thread_id_ref_type const& thrd);
}    // namespace hpx::threads::detail
