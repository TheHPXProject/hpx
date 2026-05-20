//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/async_distributed/macros.hpp>

////////////////////////////////////////////////////////////////////////////////
// from async_colocated.hpp
#define HPX_REGISTER_ASYNC_COLOCATED_DECLARATION(Action, Name)                 \
    HPX_UTIL_REGISTER_UNIQUE_FUNCTION_DECLARATION(                             \
        void(hpx::id_type, hpx::id_type),                                      \
        (hpx::util::functional::detail::async_continuation_impl<               \
            typename hpx::detail::async_colocated_bound_action<Action>::type,  \
            hpx::util::unused_type>),                                          \
        Name)                                                                  \
    /**/

#define HPX_REGISTER_ASYNC_COLOCATED(Action, Name)                             \
    HPX_UTIL_REGISTER_UNIQUE_FUNCTION(void(hpx::id_type, hpx::id_type),        \
        (hpx::util::functional::detail::async_continuation_impl<               \
            typename hpx::detail::async_colocated_bound_action<Action>::type,  \
            hpx::util::unused_type>),                                          \
        Name)                                                                  \
    /**/

////////////////////////////////////////////////////////////////////////////////
// from register_post_colocated.hpp
#define HPX_REGISTER_APPLY_COLOCATED_DECLARATION(Action, Name)
#define HPX_REGISTER_APPLY_COLOCATED(action, name)
