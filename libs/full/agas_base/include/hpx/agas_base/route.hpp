//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcelset.hpp>

#include <hpx/agas_base/agas_fwd.hpp>

namespace hpx::agas::server {

    HPX_CXX_EXPORT extern HPX_EXPORT void (*route)(
        primary_namespace& server, parcelset::parcel&& p);
}    // namespace hpx::agas::server

#endif
