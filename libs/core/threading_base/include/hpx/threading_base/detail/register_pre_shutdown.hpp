//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>

namespace hpx::threads::detail {

    // Lets modules below the runtime register pre-shutdown functions (see
    // hpx::register_pre_shutdown_function). The runtime installs the hook.
    HPX_CXX_CORE_EXPORT using register_pre_shutdown_type =
        hpx::function<void(hpx::move_only_function<void()>)>;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_register_pre_shutdown(
        register_pre_shutdown_type f);

    // Register f to be run by an HPX thread when the runtime starts to shut
    // down (in hpx::finalize). Returns false if no runtime installed the hook.
    // Throws invalid_status if it is too late to register a pre-shutdown
    // function.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool register_pre_shutdown(
        hpx::move_only_function<void()> f);
}    // namespace hpx::threads::detail
