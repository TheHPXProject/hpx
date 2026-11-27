//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/threading_base/detail/register_pre_shutdown.hpp>

#include <utility>

namespace hpx::threads::detail {

    namespace {

        register_pre_shutdown_type& register_pre_shutdown_hook()
        {
            static register_pre_shutdown_type hook;
            return hook;
        }
    }    // namespace

    void set_register_pre_shutdown(register_pre_shutdown_type f)
    {
        register_pre_shutdown_hook() = HPX_MOVE(f);
    }

    bool register_pre_shutdown(hpx::move_only_function<void()> f)
    {
        auto const& hook = register_pre_shutdown_hook();
        if (hook.empty())
        {
            return false;
        }
        hook(HPX_MOVE(f));
        return true;
    }
}    // namespace hpx::threads::detail
