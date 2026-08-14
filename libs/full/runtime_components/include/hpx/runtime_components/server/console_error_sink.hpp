//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>

#include <exception>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    // console logging happens here
    HPX_CXX_EXPORT void console_error_sink(std::exception_ptr const&);

    HPX_DEFINE_PLAIN_ACTION(
        HPX_CXX_EXPORT, console_error_sink, console_error_sink_action);
}    // namespace hpx::components::server

HPX_ACTION_HAS_CRITICAL_PRIORITY(
    hpx::components::server::console_error_sink_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::server::console_error_sink_action,
    console_error_sink_action)
