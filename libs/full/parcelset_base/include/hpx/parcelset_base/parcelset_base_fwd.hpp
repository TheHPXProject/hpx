//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>

#include <cstdint>
#include <system_error>

namespace hpx::parcelset {

    HPX_CXX_EXPORT class HPX_EXPORT parcelport;
    HPX_CXX_EXPORT class HPX_EXPORT locality;
    HPX_CXX_EXPORT class HPX_EXPORT parcel;

    HPX_CXX_EXPORT extern HPX_EXPORT parcel empty_parcel;

    /// The type of the function that can be registered as a parcel write handler
    /// using the function \a hpx::set_parcel_write_handler.
    ///
    /// \note A parcel write handler is a function which is called by the
    ///       parcel layer whenever a parcel has been sent by the underlying
    ///       networking library and if no explicit parcel handler function was
    ///       specified for the parcel.
    HPX_CXX_EXPORT using parcel_write_handler_type =
        hpx::function<bool(std::error_code const&, parcelset::parcel const&)>;

    HPX_CXX_EXPORT using write_handler_type =
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>;

    ////////////////////////////////////////////////////////////////////////
    /// Type of background work to perform
    HPX_CXX_EXPORT enum class parcelport_background_mode : std::uint8_t {
        /// perform buffer flush operations
        flush_buffers = 0x01,
        /// perform send operations (includes buffer flush)
        send = 0x03,
        /// perform receive operations
        receive = 0x04,
        /// perform all operations
        all = 0x07
    };

    HPX_CXX_EXPORT inline bool operator&(
        parcelport_background_mode lhs, parcelport_background_mode rhs)
    {
        return static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs);
    }

#define HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG                    \
    "The unscoped hpx::parcelset::parcelport_background_mode names are "       \
    "deprecated. Please use "                                                  \
    "hpx::parcelset::parcelport_background_mode::<value> instead."

    HPX_DEPRECATED_V(1, 10, HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG)
    inline constexpr parcelport_background_mode
        parcelport_background_mode_flush_buffers =
            parcelport_background_mode::flush_buffers;
    HPX_DEPRECATED_V(1, 10, HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG)
    inline constexpr parcelport_background_mode
        parcelport_background_mode_send = parcelport_background_mode::send;
    HPX_DEPRECATED_V(1, 10, HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG)
    inline constexpr parcelport_background_mode
        parcelport_background_mode_receive =
            parcelport_background_mode::receive;
    HPX_DEPRECATED_V(1, 10, HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG)
    inline constexpr parcelport_background_mode parcelport_background_mode_all =
        parcelport_background_mode::all;

#undef HPX_PARCELPORT_BACKGROUND_MODE_ENUM_DEPRECATION_MSG

    HPX_CXX_EXPORT HPX_EXPORT char const* get_parcelport_background_mode_name(
        parcelport_background_mode mode);
}    // namespace hpx::parcelset
