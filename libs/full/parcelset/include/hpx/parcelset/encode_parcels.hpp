//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011-2015 Thomas Heller
//  Copyright (c) 2007 Richard D Guidry Jr
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Katelyn Kufahl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcelset_base.hpp>

#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <cstdint>

namespace hpx::parcelset {

    HPX_CXX_EXPORT std::size_t encode_parcels(parcelport& pp, parcel const* ps,
        std::size_t num_parcels, parcel_buffer& buffer,
        std::uint32_t archive_flags, std::uint64_t max_outbound_size);
}    // namespace hpx::parcelset

#endif
