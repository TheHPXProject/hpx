//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/static_reinit.hpp>

#include <hpx/runtime_components/server/console_error_sink_singleton.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    struct error_dispatcher_tag
    {
    };

    console_error_dispatcher& get_error_dispatcher()
    {
        util::reinitializable_static<console_error_dispatcher,
            error_dispatcher_tag>
            disp;
        return disp.get();
    }
}    // namespace hpx::components::server
