//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::serialization::detail {

    // The GPU the object being loaded from an input archive should place
    // device data on, -1 if none was requested (the device current on the
    // loading thread is used then). Set by the receiving side for each parcel
    // from its destination (see hpx::traits::component_target and
    // hpx::detail::result_device); an input archive may hold several parcels,
    // so the value is set for every parcel.
    HPX_CXX_CORE_EXPORT struct receive_device
    {
        int device = -1;
    };
}    // namespace hpx::serialization::detail

template <>
struct hpx::util::extra_data_helper<hpx::serialization::detail::receive_device>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static constexpr void reset(
        serialization::detail::receive_device* p) noexcept
    {
        p->device = -1;
    }
};
