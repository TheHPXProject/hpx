//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/components/process/child.hpp>
#include <hpx/components/process/server/child.hpp>

#include <hpx/components/process/util/terminate.hpp>
#include <hpx/components/process/util/wait_for_exit.hpp>

#include <functional>

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_ACTION(hpx::components::process::server::child::terminate_action,
    hpx_components_process_server_child_terminate_action)

HPX_REGISTER_ACTION(
    hpx::components::process::server::child::wait_for_exit_action,
    hpx_components_process_server_child_wait_for_exit)

namespace hpx::components::process::server {

    void child::terminate()
    {
        process::util::terminate(child_);
    }

    int child::wait_for_exit()
    {
        int (*f)(process::util::child const&) =
            &process::util::wait_for_exit<process::util::child>;
        return hpx::run_as_os_thread(f, std::ref(child_)).get();
    }
}    // namespace hpx::components::process::server
