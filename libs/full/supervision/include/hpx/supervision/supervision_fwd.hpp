//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstdint>
#include <functional>

namespace hpx::supervision {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline constexpr char const* const service_name =
        "/0/supervision/";

    namespace detail {

        HPX_CXX_EXPORT inline constexpr std::uint64_t supervision_manager_msb =
            0x100000011ULL;
        HPX_CXX_EXPORT inline constexpr std::uint64_t supervision_manager_lsb =
            0x000000011ULL;
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class HPX_EXPORT supervision_manager;

    namespace server {

        HPX_CXX_EXPORT struct HPX_EXPORT supervision_manager;
    }    // namespace server

    HPX_CXX_EXPORT HPX_EXPORT supervision_manager& get_supervision_manager();

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT enum class event : std::uint8_t;
    HPX_CXX_EXPORT struct lifecycle_state;

    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::output_archive& ar, lifecycle_state const&,
        unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::input_archive& ar, lifecycle_state&, unsigned int);

    HPX_CXX_EXPORT enum class publish_result : std::uint8_t;
    HPX_CXX_EXPORT struct lifecycle_event_notification;

    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::output_archive& ar,
        lifecycle_event_notification const&, unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void serialize(
        hpx::serialization::input_archive& ar, lifecycle_event_notification&,
        unsigned int);

    HPX_CXX_EXPORT using lifecycle_callback =
        std::function<bool(lifecycle_event_notification const&)>;

}    // namespace hpx::supervision
