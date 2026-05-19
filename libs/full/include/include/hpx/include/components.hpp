//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/include/actions.hpp>

#include <hpx/runtime_components/component_factory.hpp>
#include <hpx/runtime_components/component_registry.hpp>
#include <hpx/runtime_components/derived_component_factory.hpp>
#include <hpx/runtime_components/distributed_metadata_base.hpp>
#include <hpx/runtime_components/new.hpp>

#include <hpx/modules/actions.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/runtime_distributed/copy_component.hpp>
#include <hpx/runtime_distributed/migrate_component.hpp>
#include <hpx/runtime_distributed/runtime_support.hpp>
#include <hpx/runtime_distributed/stubs/runtime_support.hpp>

#include <hpx/distribution_policies/binpacking_distribution_policy.hpp>
#include <hpx/distribution_policies/colocating_distribution_policy.hpp>
#include <hpx/distribution_policies/default_distribution_policy.hpp>
#include <hpx/distribution_policies/target_distribution_policy.hpp>
#include <hpx/distribution_policies/unwrapping_result_policy.hpp>
