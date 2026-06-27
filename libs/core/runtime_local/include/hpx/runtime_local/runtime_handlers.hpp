//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2017 Shoshana Jakobovits
//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <string>

namespace hpx::detail {

    HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void assertion_handler(
        hpx::source_location const& loc, char const* expr,
        std::string const& msg);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool enable_parent_task_handler();
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void test_failure_handler();
#if defined(HPX_HAVE_VERIFY_LOCKS)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void registered_locks_error_handler();
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool register_locks_predicate();
#endif
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::thread_pool_base*
    get_default_pool();
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threads::mask_type get_pu_mask(
        threads::topology& topo, std::size_t thread_num);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT ::asio::io_context&
    get_default_timer_service();
}    // namespace hpx::detail
