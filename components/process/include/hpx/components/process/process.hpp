// Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/components/process/child.hpp>

#include <type_traits>
#include <utility>

namespace hpx::components::process {

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    child execute(hpx::id_type const& id, Ts&&... ts)
    {
        return hpx::new_<child>(id, HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::components::process
