// Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/components/process/export_definitions.hpp>
#include <hpx/components/process/util/child.hpp>
#include <hpx/components/process/util/execute.hpp>

#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::components::process::server {

    ///////////////////////////////////////////////////////////////////////////
    class HPX_PROCESS_EXPORT child : public component_base<child>
    {
    public:
        template <typename... Ts>
        explicit child(Ts&&... ts)
          : child_(process::util::execute(HPX_FORWARD(Ts, ts)...))
        {
        }

        void terminate();
        int wait_for_exit();

        HPX_DEFINE_COMPONENT_ACTION(child, terminate)
        HPX_DEFINE_COMPONENT_ACTION(child, wait_for_exit)

    private:
        process::util::child child_;
    };
}    // namespace hpx::components::process::server

#include <hpx/config/warnings_suffix.hpp>

HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::process::server::child::terminate_action,
    hpx_components_process_server_child_terminate_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::process::server::child::wait_for_exit_action,
    hpx_components_process_server_child_wait_for_exit)
