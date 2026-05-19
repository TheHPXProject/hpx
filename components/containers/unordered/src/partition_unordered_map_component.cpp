//  Copyright (c) 2014-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file src/components/containers/unordered/partition_unordered_map_component.cpp

/// This file defines the necessary component boilerplate code which is
/// required for proper functioning of components in the context of HPX.

#include <hpx/config.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/components/containers/unordered/partition_unordered_map_component.hpp>
#include <hpx/components/containers/unordered/unordered_map.hpp>

#include <hpx/config/warnings_prefix.hpp>

HPX_DISTRIBUTED_METADATA(hpx::server::unordered_map_config_data,
    hpx_server_unordered_map_config_data)

HPX_REGISTER_COMPONENT_MODULE()
