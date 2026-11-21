//  Copyright (c) 2026 Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/include/components.hpp>

struct registered_here : hpx::components::component_base<registered_here>
{
};

struct registered_elsewhere
  : hpx::components::component_base<registered_elsewhere>
{
};

HPX_COMPONENT_REGISTERED_ELSEWHERE(registered_elsewhere)

static_assert(!hpx::traits::component_registered_elsewhere_v<registered_here>);
static_assert(
    hpx::traits::component_registered_elsewhere_v<registered_elsewhere>);

using registered_here_type = hpx::components::component<registered_here>;
HPX_REGISTER_COMPONENT(registered_here_type, registered_here)
#endif

int main()
{
    return 0;
}
