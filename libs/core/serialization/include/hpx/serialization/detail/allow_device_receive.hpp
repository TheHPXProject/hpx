//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::serialization::detail {

    // Tags an input archive whose zero-copy chunks the receiving parcelport
    // can place into device memory directly, whatever memory the sender sent
    // them from. The receiving locality decides this itself (the flags of an
    // input archive are the sender's).
    HPX_CXX_CORE_EXPORT struct allow_device_receive
    {
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::allow_device_receive>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static constexpr void reset(
        serialization::detail::allow_device_receive*) noexcept
    {
    }
};
