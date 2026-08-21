//  Copyright (c) 2021-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>

#include <hpx/parcelset_base/parcelset_base_fwd.hpp>

#include <cstdint>
#include <string>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset::detail {

    extern HPX_EXPORT parcelset::parcel (*create_parcel)();

    extern HPX_EXPORT locality (*create_locality)(std::string const& name);

    extern HPX_EXPORT bool (*locality_was_disconnected)(std::uint32_t);

    extern HPX_EXPORT parcel_write_handler_type (*set_parcel_write_handler)(
        parcel_write_handler_type const& f);

    extern HPX_EXPORT void (*put_parcel)(
        parcelset::parcel&& p, write_handler_type&& f);

    extern HPX_EXPORT void (*sync_put_parcel)(parcelset::parcel&& p);

    extern HPX_EXPORT void (*parcel_route_handler_func)(
        std::error_code const& ec, parcelset::parcel const& p);
}    // namespace hpx::parcelset::detail

#endif
