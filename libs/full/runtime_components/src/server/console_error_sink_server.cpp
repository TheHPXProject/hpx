//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/runtime_components/server/console_error_sink.hpp>
#include <hpx/runtime_components/server/console_error_sink_singleton.hpp>

#include <exception>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    // implementation of this console error sink
    void console_error_sink(std::exception_ptr const& e)
    {
        // dispatch this error to registered functions
        get_error_dispatcher()(hpx::diagnostic_information(e));
    }
}    // namespace hpx::components::server

///////////////////////////////////////////////////////////////////////////////
// This must be in global namespace
HPX_REGISTER_ACTION_ID(hpx::components::server::console_error_sink_action,
    console_error_sink_action, hpx::actions::console_error_sink_action_id)
