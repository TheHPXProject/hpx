//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_HAVE_STATIC_LINKING)
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/async_distributed/base_lco_with_value.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/async_distributed/macros.hpp>
#include <hpx/async_distributed/post.hpp>

#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::naming::gid_type, gid_type,
    hpx::actions::base_lco_with_value_gid_get,
    hpx::actions::base_lco_with_value_gid_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::vector<hpx::naming::gid_type>,
    vector_gid_type, hpx::actions::base_lco_with_value_vector_gid_get,
    hpx::actions::base_lco_with_value_vector_gid_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::id_type, hpx::naming::gid_type,
    id_type, hpx::actions::base_lco_with_value_id_get,
    hpx::actions::base_lco_with_value_id_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::id_type, naming_id_type,
    hpx::actions::base_lco_with_value_id_type_get,
    hpx::actions::base_lco_with_value_id_type_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::vector<hpx::id_type>,
    std::vector<hpx::naming::gid_type>, vector_id_gid_type,
    hpx::actions::base_lco_with_value_vector_id_gid_get,
    hpx::actions::base_lco_with_value_vector_id_gid_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::vector<hpx::id_type>, vector_id_type,
    hpx::actions::base_lco_with_value_vector_id_get,
    hpx::actions::base_lco_with_value_vector_id_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::util::unused_type, unused_type,
    hpx::actions::base_lco_with_value_unused_get,
    hpx::actions::base_lco_with_value_unused_set)

#endif
