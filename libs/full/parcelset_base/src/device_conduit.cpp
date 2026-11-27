//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/parcelset_base/device_conduit.hpp>

namespace hpx::parcelset {

    device_conduit::~device_conduit() = default;
}    // namespace hpx::parcelset

#endif
