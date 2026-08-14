//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <hpx/actions/macros.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/async_distributed/macros.hpp>
#include <hpx/components_base/macros.hpp>

////////////////////////////////////////////////////////////////////////////////
// from server/channel.hpp
#define HPX_REGISTER_CHANNEL_DECLARATION(...)                                  \
    HPX_REGISTER_CHANNEL_DECLARATION_(__VA_ARGS__)                             \
/**/
#define HPX_REGISTER_CHANNEL_DECLARATION_(...)                                 \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_CHANNEL_DECLARATION_,                \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_CHANNEL_DECLARATION_1(type)                               \
    HPX_REGISTER_CHANNEL_DECLARATION_2(type, type)                             \
/**/
#define HPX_REGISTER_CHANNEL_DECLARATION_2(type, name)                         \
    using HPX_PP_CAT(__channel_, HPX_PP_CAT(type, name)) =                     \
        ::hpx::lcos::server::channel<type>;                                    \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        hpx::lcos::server::channel<type>::get_generation_action,               \
        HPX_PP_CAT(__channel_get_generation_action, HPX_PP_CAT(type, name)))   \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        hpx::lcos::server::channel<type>::set_generation_action,               \
        HPX_PP_CAT(__channel_set_generation_action, HPX_PP_CAT(type, name)))   \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        hpx::lcos::server::channel<type>::close_action,                        \
        HPX_PP_CAT(__channel_close_action, HPX_PP_CAT(type, name)))            \
    /**/

#define HPX_REGISTER_CHANNEL(...)                                              \
    HPX_REGISTER_CHANNEL_(__VA_ARGS__)                                         \
/**/
#define HPX_REGISTER_CHANNEL_(...)                                             \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_REGISTER_CHANNEL_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))        \
    /**/

#define HPX_REGISTER_CHANNEL_1(type)                                           \
    HPX_REGISTER_CHANNEL_2(type, type)                                         \
/**/
#define HPX_REGISTER_CHANNEL_2(type, name)                                     \
    using HPX_PP_CAT(__channel_, HPX_PP_CAT(type, name)) =                     \
        ::hpx::lcos::server::channel<type>;                                    \
    using HPX_PP_CAT(__channel_component_, name) =                             \
        ::hpx::components::component<HPX_PP_CAT(                               \
            __channel_, HPX_PP_CAT(type, name))>;                              \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY(                                    \
        HPX_PP_CAT(__channel_component_, name),                                \
        HPX_PP_CAT(__channel_component_, name),                                \
        HPX_PP_STRINGIZE(HPX_PP_CAT(__base_lco_with_value_channel_, name)))    \
    HPX_REGISTER_ACTION(                                                       \
        hpx::lcos::server::channel<type>::get_generation_action,               \
        HPX_PP_CAT(__channel_get_generation_action, HPX_PP_CAT(type, name)))   \
    HPX_REGISTER_ACTION(                                                       \
        hpx::lcos::server::channel<type>::set_generation_action,               \
        HPX_PP_CAT(__channel_set_generation_action, HPX_PP_CAT(type, name)))   \
    HPX_REGISTER_ACTION(hpx::lcos::server::channel<type>::close_action,        \
        HPX_PP_CAT(__channel_close_action, HPX_PP_CAT(type, name)))            \
    /**/
