//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/parcelports/init_all_parcelports.hpp>
#include <hpx/parcelports/static_parcelports.hpp>

#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    void (*init_static_parcelport_factories)(
        std::vector<plugins::parcelport_factory_base*>& factories) = nullptr;

    struct HPX_EXPORT init_parcelports
    {
        init_parcelports()
        {
            init_static_parcelport_factories =
                init_static_parcelport_factories_impl;
        }
    } init;

    // force linking with this module
    void init_all_parcelports() {}
}    // namespace hpx::parcelset

#endif
