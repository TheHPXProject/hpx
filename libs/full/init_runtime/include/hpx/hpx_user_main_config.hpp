//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/init_runtime/detail/main_winsocket.hpp>

#include <string>
#include <vector>

namespace hpx_startup {

    // Allow applications to add configuration settings if HPX_MAIN is set
    HPX_CXX_EXPORT HPX_EXPORT extern std::vector<std::string> (
        *user_main_config_function)(std::vector<std::string> const&);

    HPX_CXX_EXPORT inline std::vector<std::string> user_main_config(
        std::vector<std::string> const& cfg)
    {
        return user_main_config_function ? user_main_config_function(cfg) : cfg;
    }
}    // namespace hpx_startup
