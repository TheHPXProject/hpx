//  Copyright (c) 2026 Dominic Marcello
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <vector>

#include <hpx/hpx_start.hpp>

// Keep this as the only HPX include to test the public facade in isolation.
int main()
{
    std::vector<std::string> const cfg{"hpx.os_threads=1"};
    return hpx_startup::user_main_config(cfg) == cfg ? 0 : 1;
}
