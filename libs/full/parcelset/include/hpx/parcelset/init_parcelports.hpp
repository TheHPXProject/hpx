//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/plugin_factories.hpp>

#include <vector>

namespace hpx::parcelset {

    HPX_CXX_EXPORT extern HPX_EXPORT void (*init_static_parcelport_factories)(
        std::vector<plugins::parcelport_factory_base*>& factories);
}

#endif
