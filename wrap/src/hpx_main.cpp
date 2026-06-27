//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <string>
#include <vector>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/modules/init_runtime.hpp>
#else
namespace hpx_startup {

    std::vector<std::string> (*user_main_config_function)(
        std::vector<std::string> const&) = nullptr;
}    // namespace hpx_startup
#endif

namespace hpx_startup {

    void install_user_main_config();

    namespace {

        std::vector<std::string> (*prev_user_main_config_function)(
            std::vector<std::string> const&) = nullptr;

        std::vector<std::string> enable_run_main(
            std::vector<std::string> const& config)
        {
            std::vector<std::string> cfg(config);
            cfg.emplace(cfg.begin(), "hpx.run_hpx_main!=1");
            if (prev_user_main_config_function)
                return prev_user_main_config_function(cfg);
            return cfg;
        }
    }    // namespace

    void install_user_main_config()
    {
        prev_user_main_config_function = hpx_startup::user_main_config_function;
        hpx_startup::user_main_config_function = &enable_run_main;
    }
}    // namespace hpx_startup
