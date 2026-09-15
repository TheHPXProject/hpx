//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/supervision_dispatch.hpp>

#include "late_component_worker.hpp"

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality.
// The boilerplate macros below are necessary to make the test_server component
// available through AGAS.
HPX_REGISTER_COMPONENT_MODULE()

HPX_REGISTER_ACTION(late_component_test_server_set_message_action)
HPX_REGISTER_ACTION(late_component_test_server_get_message_action)

HPX_REGISTER_ACTION(fenced_late_component_test_server_set_message_action)
HPX_REGISTER_ACTION(fenced_late_component_test_server_get_message_action)

using late_component_test_server_component =
    late_component::server::test_server;
HPX_REGISTER_COMPONENT(
    hpx::components::component<late_component_test_server_component>,
    late_component_test_server, hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(late_component_test_server_component)

#include <hpx/config/warnings_suffix.hpp>
