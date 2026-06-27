//  Copyright (c) 2014 Anuj R. Sharma
//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file src/components/containers/partitioned_vector/partitioned_vector_component.cpp

/// This file defines the necessary component boilerplate code which is
/// required for proper functioning of components in the context of HPX.

#include <hpx/config.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/components/containers/partitioned_vector/export_definitions.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component.hpp>

HPX_DISTRIBUTED_METADATA(hpx::server::partitioned_vector_config_data,
    hpx_server_partitioned_vector_config_data)

HPX_REGISTER_COMPONENT_MODULE()
