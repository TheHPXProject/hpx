//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2012-2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/parcelset/macros.hpp>

#include <cstddef>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(...)                      \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_(__VA_ARGS__)                 \
/**/
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_(...)                     \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_,    \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
/**/

// obsolete
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION2(                         \
    Value, RemoteValue, Name)                                                  \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_3(Value, RemoteValue, Name)   \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_1(Value)                  \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_4(                            \
        Value, Value, Value, managed_component_tag)                            \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_2(Value, Name)            \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_4(                            \
        Value, Value, Name, managed_component_tag)                             \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_3(                        \
    Value, RemoteValue, Name)                                                  \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_4(                            \
        Value, RemoteValue, Name, managed_component_tag)                       \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION_4(                        \
    Value, RemoteValue, Name, Tag)                                             \
    typedef ::hpx::lcos::base_lco_with_value<Value, RemoteValue,               \
        ::hpx::traits::detail::Tag>                                            \
        HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name), Tag);               \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        HPX_PP_CAT(                                                            \
            HPX_PP_CAT(base_lco_with_value_, Name), Tag)::set_value_action,    \
        HPX_PP_CAT(HPX_PP_CAT(set_value_action_, Name), Tag))                  \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        HPX_PP_CAT(                                                            \
            HPX_PP_CAT(base_lco_with_value_, Name), Tag)::get_value_action,    \
        HPX_PP_CAT(HPX_PP_CAT(get_value_action_, Name), Tag))                  \
    HPX_ACTION_USES_MESSAGE_COALESCING_NOTHROW_DECLARATION(                    \
        HPX_PP_CAT(                                                            \
            HPX_PP_CAT(base_lco_with_value_, Name), Tag)::set_value_action,    \
        "lco_set_value_action", std::size_t(-1), std::size_t(-1))              \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BASE_LCO_WITH_VALUE(...)                                  \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_(__VA_ARGS__)                             \
/**/
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_(...)                                 \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BASE_LCO_WITH_VALUE_,                \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_1(Value)                              \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_4(                                        \
        Value, Value, Value, managed_component_tag)                            \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_2(Value, Name)                        \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_4(                                        \
        Value, Value, Name, managed_component_tag)                             \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_3(Value, RemoteValue, Name)           \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_4(                                        \
        Value, RemoteValue, Name, managed_component_tag)                       \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_4(Value, RemoteValue, Name, Tag)      \
    typedef ::hpx::lcos::base_lco_with_value<Value, RemoteValue,               \
        ::hpx::traits::detail::Tag>                                            \
        HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name), Tag);               \
    HPX_REGISTER_ACTION(HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name),     \
                            Tag)::set_value_action,                            \
        HPX_PP_CAT(HPX_PP_CAT(set_value_action_, Name), Tag))                  \
    HPX_REGISTER_ACTION(HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name),     \
                            Tag)::get_value_action,                            \
        HPX_PP_CAT(HPX_PP_CAT(get_value_action_, Name), Tag))                  \
    HPX_ACTION_USES_MESSAGE_COALESCING_NOTHROW_DEFINITION(                     \
        HPX_PP_CAT(                                                            \
            HPX_PP_CAT(base_lco_with_value_, Name), Tag)::set_value_action,    \
        "lco_set_value_action", std::size_t(-1), std::size_t(-1))              \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(...)                               \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_(__VA_ARGS__)                          \
/**/
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_(...)                              \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_,             \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
/**/

// obsolete
#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID2(                                  \
    Value, RemoteValue, Name, ActionIdGet, ActionIdSet)                        \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_6(Value, RemoteValue, Name,            \
        ActionIdGet, ActionIdSet, managed_component_tag)                       \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_4(                                 \
    Value, Name, ActionIdGet, ActionIdSet)                                     \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_6(                                     \
        Value, Value, Name, ActionIdGet, ActionIdSet, managed_component_tag)   \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_5(                                 \
    Value, RemoteValue, Name, ActionIdGet, ActionIdSet)                        \
    HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_6(Value, RemoteValue, Name,            \
        ActionIdGet, ActionIdSet, managed_component_tag)                       \
    /**/

#define HPX_REGISTER_BASE_LCO_WITH_VALUE_ID_6(                                 \
    Value, RemoteValue, Name, ActionIdGet, ActionIdSet, Tag)                   \
    typedef ::hpx::lcos::base_lco_with_value<Value, RemoteValue,               \
        ::hpx::traits::detail::Tag>                                            \
        HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name), Tag);               \
    HPX_REGISTER_ACTION_ID(HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name),  \
                               Tag)::set_value_action,                         \
        HPX_PP_CAT(HPX_PP_CAT(set_value_action_, Name), Tag), ActionIdSet)     \
    HPX_REGISTER_ACTION_ID(HPX_PP_CAT(HPX_PP_CAT(base_lco_with_value_, Name),  \
                               Tag)::get_value_action,                         \
        HPX_PP_CAT(HPX_PP_CAT(get_value_action_, Name), Tag), ActionIdGet)     \
    HPX_ACTION_USES_MESSAGE_COALESCING_NOTHROW_DEFINITION(                     \
        HPX_PP_CAT(                                                            \
            HPX_PP_CAT(base_lco_with_value_, Name), Tag)::set_value_action,    \
        "lco_set_value_action", std::size_t(-1), std::size_t(-1))              \
    /**/
