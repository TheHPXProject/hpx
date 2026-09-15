//  Copyright (c) 2021-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/parcelset_base/detail/locality_interface_functions.hpp>
#include <hpx/parcelset_base/locality.hpp>

#include <cstdint>
#include <string>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset::detail {

    parcelset::parcel (*create_parcel)() = nullptr;

    locality (*create_locality)(std::string const& name) = nullptr;

    bool (*locality_was_disconnected)(std::uint32_t) = nullptr;

    parcel_write_handler_type (*set_parcel_write_handler)(
        parcel_write_handler_type const& f) = nullptr;

    void (*put_parcel)(parcelset::parcel&& p, write_handler_type&& f) = nullptr;

    void (*sync_put_parcel)(parcelset::parcel&& p) = nullptr;

    void (*parcel_route_handler_func)(
        std::error_code const& ec, parcelset::parcel const& p) = nullptr;
}    // namespace hpx::parcelset::detail

#endif
