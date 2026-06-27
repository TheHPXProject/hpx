//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/functional.hpp>

#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstdint>
#include <vector>

namespace hpx::parcelset::detail {

    HPX_CXX_EXPORT using put_parcel_type = hpx::move_only_function<void(
        parcelset::parcel&&, write_handler_type&&)>;

    HPX_CXX_EXPORT void HPX_EXPORT parcel_await_apply(parcelset::parcel&& p,
        write_handler_type&& f, std::uint32_t archive_flags,
        put_parcel_type pp);

    HPX_CXX_EXPORT using put_parcels_type = hpx::move_only_function<void(
        std::vector<parcelset::parcel>&&, std::vector<write_handler_type>&&)>;

    HPX_CXX_EXPORT void HPX_EXPORT parcels_await_apply(
        std::vector<parcelset::parcel>&& p, std::vector<write_handler_type>&& f,
        std::uint32_t archive_flags, put_parcels_type pp);
}    // namespace hpx::parcelset::detail

#endif
