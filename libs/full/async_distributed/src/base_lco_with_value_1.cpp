//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_HAVE_STATIC_LINKING)
#include <hpx/async_distributed/base_lco_with_value.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/async_distributed/macros.hpp>
#include <hpx/async_distributed/post.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(float, float,
    hpx::actions::base_lco_with_value_float_get,
    hpx::actions::base_lco_with_value_float_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(double, double,
    hpx::actions::base_lco_with_value_double_get,
    hpx::actions::base_lco_with_value_double_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::int8_t, int8_t,
    hpx::actions::base_lco_with_value_int8_get,
    hpx::actions::base_lco_with_value_int8_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::uint8_t, uint8_t,
    hpx::actions::base_lco_with_value_uint8_get,
    hpx::actions::base_lco_with_value_uint8_set)

#endif
