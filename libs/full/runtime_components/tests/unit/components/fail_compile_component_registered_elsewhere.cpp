//  Copyright (c) 2026 Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Registering a component marked as registered by another module must fail.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/include/components.hpp>

struct registered_elsewhere
  : hpx::components::component_base<registered_elsewhere>
{
};

HPX_COMPONENT_REGISTERED_ELSEWHERE(registered_elsewhere)

using registered_elsewhere_type =
    hpx::components::component<registered_elsewhere>;
HPX_REGISTER_COMPONENT(registered_elsewhere_type, registered_elsewhere)
#endif

int main()
{
    return 0;
}
