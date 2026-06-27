//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2020-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions/actions_fwd.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>

#if defined(HPX_HAVE_NETWORKING)

namespace hpx::actions::detail {

    HPX_CXX_EXPORT template <typename Action>
    struct register_action
    {
        register_action(register_action const&) = delete;
        register_action(register_action&&) = delete;
        register_action& operator=(register_action const&) = delete;
        register_action& operator=(register_action&&) = delete;

        register_action();
        ~register_action() = default;

        // defined in actions/transfer_action.hpp
        static base_action* create();

        // defined in async_distributed/transfer_continuation_action.hpp
        static base_action* create_cont();

        register_action& instantiate() noexcept;

        static register_action instance;
    };

    template <typename Action>
    register_action<Action> register_action<Action>::instance;

    template <typename Action>
    register_action<Action>::register_action()
    {
        char const* action_name =
            hpx::actions::detail::get_action_name<Action>();
        HPX_ASSERT(action_name != nullptr);
        if (action_name != nullptr)
        {
            action_registry::instance().register_factory(
                action_name, &create, &create_cont);
        }
    }

    template <typename Action>
    register_action<Action>& register_action<Action>::instantiate() noexcept
    {
        return *this;
    }
}    // namespace hpx::actions::detail

#endif
