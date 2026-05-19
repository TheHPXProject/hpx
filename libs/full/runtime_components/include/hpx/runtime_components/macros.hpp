//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <hpx/components_base/macros.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

///////////////////////////////////////////////////////////////////////////////
// from component_factory.hpp

// This macro is used create and to register a minimal component factory with
// Hpx.Plugin.
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(...)                            \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_(__VA_ARGS__)                       \
    /**/

#define HPX_REGISTER_COMPONENT(...)                                            \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_(__VA_ARGS__)                       \
    /**/

#define HPX_REGISTER_ENABLED_COMPONENT_FACTORY(ComponentType, componentname)   \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_3(ComponentType, componentname,     \
        ::hpx::components::factory_state::enabled)                             \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
    /**/

#define HPX_REGISTER_DISABLED_COMPONENT_FACTORY(ComponentType, componentname)  \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_3(ComponentType, componentname,     \
        ::hpx::components::factory_state::disabled)                            \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_1(ComponentType)                \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_3(                                  \
        ComponentType, ComponentType, ::hpx::components::factory_state::check) \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_2(ComponentType, componentname) \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_3(                                  \
        ComponentType, componentname, ::hpx::components::factory_state::check) \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_3(                              \
    ComponentType, componentname, state)                                       \
    HPX_REGISTER_COMPONENT_HEAP(ComponentType)                                 \
    HPX_REGISTER_COMPONENT_FACTORY(componentname)                              \
    HPX_DEFINE_COMPONENT_NAME(ComponentType::type_holder, componentname)       \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_3(                                 \
        ComponentType, componentname, state)                                   \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC(...)                    \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_(__VA_ARGS__)               \
/**/

// same as above, just a better name

/// This macro is used create and to register a minimal component factory for
/// a component type which allows it to be remotely created using the
/// hpx::new_<> function.
/// This macro can be invoked with one, two or three arguments
#define HPX_REGISTER_COMPONENT_DYNAMIC(...)                                    \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_(__VA_ARGS__)               \
    /**/

#define HPX_REGISTER_ENABLED_COMPONENT_FACTORY_DYNAMIC(                        \
    ComponentType, componentname)                                              \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_3(ComponentType,            \
        componentname, ::hpx::components::factory_state::enabled)              \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
    /**/

#define HPX_REGISTER_DISABLED_COMPONENT_FACTORY_DYNAMIC(                       \
    ComponentType, componentname)                                              \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_3(ComponentType,            \
        componentname, ::hpx::components::factory_state::disabled)             \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
    /**/

#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_(...)                   \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_,  \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_1(ComponentType)        \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_3(                          \
        ComponentType, ComponentType, ::hpx::components::factory_state::check) \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_2(                      \
    ComponentType, componentname)                                              \
    HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_3(                          \
        ComponentType, componentname, ::hpx::components::factory_state::check) \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC_3(                      \
    ComponentType, componentname, state)                                       \
    HPX_REGISTER_COMPONENT_HEAP(ComponentType)                                 \
    HPX_DEFINE_COMPONENT_NAME(ComponentType::type_holder, componentname)       \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_3(                         \
        ComponentType, componentname, state)                                   \
    /**/

#else    // COMPUTE DEVICE CODE

#define HPX_REGISTER_COMPONENT(...)
#define HPX_REGISTER_ENABLED_COMPONENT_FACTORY(ComponentType, componentname)
#define HPX_REGISTER_DISABLED_COMPONENT_FACTORY(ComponentType, componentname)
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(...)
#define HPX_REGISTER_MINIMAL_COMPONENT_FACTORY_DYNAMIC(...)
#define HPX_REGISTER_COMPONENT_DYNAMIC(...)
#define HPX_REGISTER_ENABLED_COMPONENT_FACTORY_DYNAMIC(                        \
    ComponentType, componentname)
#define HPX_REGISTER_DISABLED_COMPONENT_FACTORY_DYNAMIC(                       \
    ComponentType, componentname)

#endif

///////////////////////////////////////////////////////////////////////////////
// from component_registry.hpp

/// This macro is used create and to register a minimal component registry with
/// Hpx.Plugin.

#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY(...)                           \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_(__VA_ARGS__)                      \
    /**/

#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_(...)                          \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_,         \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_2(                             \
    ComponentType, componentname)                                              \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_3(                                 \
        ComponentType, componentname, ::hpx::components::factory_state::check) \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_3(                             \
    ComponentType, componentname, state)                                       \
    using componentname##_component_registry_type =                            \
        hpx::components::component_registry<ComponentType, state>;             \
    HPX_REGISTER_COMPONENT_REGISTRY(                                           \
        componentname##_component_registry_type, componentname)                \
    template struct hpx::components::component_registry<ComponentType, state>; \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC(...)                   \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_(__VA_ARGS__)              \
    /**/

#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_(...)                  \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_, \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_2(                     \
    ComponentType, componentname)                                              \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_3(                         \
        ComponentType, componentname, ::hpx::components::factory_state::check) \
/**/
#define HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_3(                     \
    ComponentType, componentname, state)                                       \
    using componentname##_component_registry_type =                            \
        hpx::components::component_registry<ComponentType, state>;             \
    HPX_REGISTER_COMPONENT_REGISTRY_DYNAMIC(                                   \
        componentname##_component_registry_type, componentname)                \
    template struct hpx::components::component_registry<ComponentType, state>; \
    /**/

#if !defined(HPX_COMPUTE_DEVICE_CODE)
///////////////////////////////////////////////////////////////////////////////
// from derived_component_factory.hpp

/// This macro is used create and to register a minimal component factory with
/// Hpx.Plugin. This macro may be used if the registered component factory is
/// the only factory to be exposed from a particular module. If more than one
/// factory needs to be exposed the \a HPX_REGISTER_COMPONENT_FACTORY and
/// \a HPX_REGISTER_COMPONENT_MODULE macros should be used instead.
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY(...)                            \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY_(__VA_ARGS__)                       \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_DERIVED_COMPONENT_FACTORY_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_3(                              \
    ComponentType, componentname, basecomponentname)                           \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY_4(ComponentType, componentname,     \
        basecomponentname, ::hpx::components::factory_state::check)            \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_4(                              \
    ComponentType, componentname, basecomponentname, state)                    \
    HPX_REGISTER_COMPONENT_HEAP(ComponentType)                                 \
    HPX_REGISTER_COMPONENT_FACTORY(componentname)                              \
    HPX_DEFINE_COMPONENT_NAME(                                                 \
        ComponentType::type_holder, componentname, basecomponentname)          \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_3(                                 \
        ComponentType, componentname, state)                                   \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(...)                    \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_(__VA_ARGS__)               \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_(...)                   \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_,  \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_3(                      \
    ComponentType, componentname, basecomponentname)                           \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_4(ComponentType,            \
        componentname, basecomponentname,                                      \
        ::hpx::components::factory_state::check)                               \
    HPX_DEFINE_GET_COMPONENT_TYPE(ComponentType::wrapped_type)                 \
    /**/

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_4(                      \
    ComponentType, componentname, basecomponentname, state)                    \
    HPX_REGISTER_COMPONENT_HEAP(ComponentType)                                 \
    HPX_DEFINE_COMPONENT_NAME(                                                 \
        ComponentType::type_holder, componentname, basecomponentname)          \
    HPX_REGISTER_MINIMAL_COMPONENT_REGISTRY_DYNAMIC_3(                         \
        ComponentType, componentname, state)                                   \
    /**/

#else    // COMPUTE DEVICE CODE

#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY(...)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_(...)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_3(                              \
    ComponentType, componentname, basecomponentname)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_4(                              \
    ComponentType, componentname, basecomponentname, state)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(...)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_(...)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_3(                      \
    ComponentType, componentname, basecomponentname)
#define HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC_4(                      \
    ComponentType, componentname, basecomponentname, state)

#endif

////////////////////////////////////////////////////////////////////////////////
// from distributed_metadata_base.hpp
#define HPX_DISTRIBUTED_METADATA_DECLARATION(...)                              \
    HPX_DISTRIBUTED_METADATA_DECLARATION_(__VA_ARGS__)                         \
    /**/
#define HPX_DISTRIBUTED_METADATA_DECLARATION_(...)                             \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_DISTRIBUTED_METADATA_DECLARATION_,            \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_DISTRIBUTED_METADATA_DECLARATION_1(config)                         \
    HPX_DISTRIBUTED_METADATA_DECLARATION_2(config, config)                     \
    /**/
#define HPX_DISTRIBUTED_METADATA_DECLARATION_2(config, name)                   \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::components::server::distributed_metadata_base<                  \
            config>::get_action,                                               \
        HPX_PP_CAT(__distributed_metadata_get_action_, name))                  \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::base_lco_with_value<config>::set_value_action,            \
        HPX_PP_CAT(__set_value_distributed_metadata_config_data_, name))       \
    /**/

#define HPX_DISTRIBUTED_METADATA(...)                                          \
    HPX_DISTRIBUTED_METADATA_(__VA_ARGS__)                                     \
    /**/
#define HPX_DISTRIBUTED_METADATA_(...)                                         \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_DISTRIBUTED_METADATA_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))    \
    /**/

#define HPX_DISTRIBUTED_METADATA_1(config)                                     \
    HPX_DISTRIBUTED_METADATA_2(config, config)                                 \
    /**/
#define HPX_DISTRIBUTED_METADATA_2(config, name)                               \
    HPX_REGISTER_ACTION(::hpx::components::server::distributed_metadata_base<  \
                            config>::get_action,                               \
        HPX_PP_CAT(__distributed_metadata_get_action_, name))                  \
    HPX_REGISTER_ACTION(                                                       \
        ::hpx::lcos::base_lco_with_value<config>::set_value_action,            \
        HPX_PP_CAT(__set_value_distributed_metadata_config_data_, name))       \
    typedef ::hpx::components::component<                                      \
        ::hpx::components::server::distributed_metadata_base<config>>          \
        HPX_PP_CAT(__distributed_metadata_, name);                             \
    HPX_REGISTER_COMPONENT(HPX_PP_CAT(__distributed_metadata_, name))          \
    /**/
