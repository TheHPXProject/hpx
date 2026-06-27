//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2007 Richard D. Guidry Jr.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <mutex>

namespace hpx::naming::detail {

    HPX_CXX_EXPORT HPX_EXPORT hpx::future_or_value<gid_type>
    split_gid_if_needed(gid_type& id);

    HPX_CXX_EXPORT HPX_EXPORT gid_type split_gid_if_needed(
        hpx::launch::sync_policy, gid_type& id);

    HPX_CXX_EXPORT HPX_EXPORT hpx::future_or_value<gid_type>
    split_gid_if_needed_locked(
        std::unique_lock<gid_type::mutex_type>& l, gid_type& gid);
}    // namespace hpx::naming::detail
