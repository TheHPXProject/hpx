//  Copyright (c) 2007-2055 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <string>
#include <vector>

namespace hpx_startup {

    // Allow applications to add configuration settings if HPX_MAIN is set
    HPX_EXPORT extern std::vector<std::string> (*user_main_config_function)(
        std::vector<std::string> const&);

    inline std::vector<std::string> user_main_config(
        std::vector<std::string> const& cfg)
    {
        return user_main_config_function ? user_main_config_function(cfg) : cfg;
    }
}    // namespace hpx_startup
