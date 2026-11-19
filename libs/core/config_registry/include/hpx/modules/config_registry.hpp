//  Copyright (c) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#if !defined(HPX_MODULE_STATIC_LINKING)
#if defined(HPX_CORE_EXPORTS)
#define HPX_CONFIG_REGISTRY_EXPORT __declspec(dllexport)
#else
#define HPX_CONFIG_REGISTRY_EXPORT __declspec(dllimport)
#endif
#endif
#elif defined(__NVCC__) || defined(__CUDACC__)
#define HPX_CONFIG_REGISTRY_EXPORT /* empty */
#elif defined(HPX_CORE_EXPORTS)
#define HPX_CONFIG_REGISTRY_EXPORT __attribute__((visibility("default")))
#endif

// make sure we have reasonable defaults
#if !defined(HPX_CONFIG_REGISTRY_EXPORT)
#define HPX_CONFIG_REGISTRY_EXPORT /* empty */
#endif

namespace hpx::config_registry {

    struct module_config
    {
        std::string module_name;
        std::vector<std::string> config_entries;
    };

    [[nodiscard]] HPX_CONFIG_REGISTRY_EXPORT std::vector<module_config> const&
    get_module_configs();

    HPX_CONFIG_REGISTRY_EXPORT void add_module_config(
        module_config const& config);

    struct HPX_CONFIG_REGISTRY_EXPORT add_module_config_helper
    {
        explicit add_module_config_helper(module_config const& config);
    };
}    // namespace hpx::config_registry
