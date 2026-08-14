//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/plugin/macros.hpp>
#include <hpx/runtime_configuration/macros.hpp>

#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define HPX_DEF_UNIQUE_PLUGIN_NAME(PluginType, name)                           \
    namespace hpx::plugins {                                                   \
        template <>                                                            \
        struct unique_plugin_name<PluginType>                                  \
        {                                                                      \
            using type = char const*;                                          \
                                                                               \
            static constexpr type call(void) noexcept                          \
            {                                                                  \
                return HPX_PP_STRINGIZE(name);                                 \
            }                                                                  \
        };                                                                     \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/plugin_factory_base.hpp
// This macro is used to register the given plugin factory with Hpx.Plugin.
// This macro has to be used for each of the plugin factories. Ungated,
// matching the component analog (HPX_REGISTER_COMPONENT_FACTORY).
#define HPX_REGISTER_PLUGIN_FACTORY_BASE(FactoryType, pluginname)              \
    HPX_PLUGIN_EXPORT(HPX_PLUGIN_PLUGIN_PREFIX,                                \
        hpx::plugins::plugin_factory_base, FactoryType, pluginname, factory)   \
    HPX_INIT_REGISTRY_PLUGIN_FACTORY_STATIC(                                   \
        HPX_PLUGIN_PLUGIN_PREFIX, pluginname, factory)                         \
/**/

// This macro is used to define the required Hpx.Plugin entry points. This
// macro has to be used in exactly one compilation unit of a plugin module.
// Ungated: a plugin may live in a shared library, in a statically linked
// build, or directly in the application executable.
#define HPX_REGISTER_PLUGIN_MODULE()                                           \
    HPX_PLUGIN_EXPORT_LIST(HPX_PLUGIN_PLUGIN_PREFIX, factory)                  \
    HPX_REGISTER_PLUGIN_REGISTRY_MODULE()                                      \
    /**/

#define HPX_REGISTER_PLUGIN_MODULE_DYNAMIC()                                   \
    HPX_PLUGIN_EXPORT_LIST(HPX_PLUGIN_PLUGIN_PREFIX, factory)                  \
    HPX_REGISTER_PLUGIN_REGISTRY_MODULE_DYNAMIC()

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/binary_filter_factory_base.hpp
// This macro is used to register the given component factory with
// Hpx.Plugin. This macro has to be used for each of the binary filter
// factories.
#define HPX_REGISTER_BINARY_FILTER_FACTORY_BASE(FactoryType, pluginname)       \
    HPX_PLUGIN_EXPORT(HPX_PLUGIN_PLUGIN_PREFIX,                                \
        hpx::plugins::plugin_factory_base, FactoryType, pluginname, factory)   \
    HPX_INIT_REGISTRY_PLUGIN_FACTORY_STATIC(                                   \
        HPX_PLUGIN_PLUGIN_PREFIX, pluginname, factory)                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/binary_filter_factory.hpp
// This macro is used create and to register a minimal component factory with
// Hpx.Plugin.
#define HPX_REGISTER_BINARY_FILTER_FACTORY(BinaryFilter, pluginname)           \
    HPX_REGISTER_BINARY_FILTER_FACTORY_BASE(                                   \
        hpx::plugins::binary_filter_factory<BinaryFilter>, pluginname)         \
    HPX_DEF_UNIQUE_PLUGIN_NAME(                                                \
        hpx::plugins::binary_filter_factory<BinaryFilter>, pluginname)         \
    template struct hpx::plugins::binary_filter_factory<BinaryFilter>;         \
    HPX_REGISTER_PLUGIN_REGISTRY_2(BinaryFilter, pluginname)                   \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/plugin_registry.hpp
// This macro is used create and to register a minimal plugin registry with
// Hpx.Plugin.
#define HPX_REGISTER_PLUGIN_REGISTRY(...)                                      \
    HPX_REGISTER_PLUGIN_REGISTRY_(__VA_ARGS__)                                 \
    /**/

#define HPX_REGISTER_PLUGIN_REGISTRY_(...)                                     \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_PLUGIN_REGISTRY_,                    \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_PLUGIN_REGISTRY_2(PluginType, pluginname)                 \
    HPX_REGISTER_PLUGIN_REGISTRY_5(                                            \
        PluginType, pluginname, HPX_PLUGIN_NAME, "hpx", "hpx")                 \
    /**/
#define HPX_REGISTER_PLUGIN_REGISTRY_4(                                        \
    PluginType, pluginname, pluginsection, pluginsuffix)                       \
    HPX_REGISTER_PLUGIN_REGISTRY_5(                                            \
        PluginType, pluginname, HPX_PLUGIN_NAME, pluginsection, pluginsuffix)  \
    /**/
#define HPX_REGISTER_PLUGIN_REGISTRY_5(                                        \
    PluginType, pluginname, pluginstring, pluginsection, pluginsuffix)         \
    inline constexpr char __##pluginname##_string[] =                          \
        HPX_PP_STRINGIZE(pluginstring);                                        \
    inline constexpr char __##pluginname##_section[] = pluginsection;          \
    inline constexpr char __##pluginname##_suffix[] = pluginsuffix;            \
    using __##pluginname##_plugin_registry_type =                              \
        hpx::plugins::plugin_registry<PluginType, __##pluginname##_string,     \
            __##pluginname##_section, __##pluginname##_suffix>;                \
    HPX_REGISTER_PLUGIN_BASE_REGISTRY(                                         \
        __##pluginname##_plugin_registry_type, pluginname)                     \
    HPX_DEF_UNIQUE_PLUGIN_NAME(                                                \
        __##pluginname##_plugin_registry_type, pluginname)                     \
    template struct hpx::plugins::plugin_registry<PluginType,                  \
        __##pluginname##_string, __##pluginname##_section,                     \
        __##pluginname##_suffix>;                                              \
    /**/

#if defined(HPX_HAVE_NETWORKING)

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/parcelport_factory.hpp
// This macro is used create and to register a minimal parcelport factory with
// Hpx.Plugin.
#define HPX_REGISTER_PARCELPORT_(Parcelport, pluginname, pp)                   \
    using HPX_PP_CAT(pluginname, _plugin_factory_type) =                       \
        hpx::plugins::parcelport_factory<Parcelport>;                          \
    HPX_DEF_UNIQUE_PLUGIN_NAME(                                                \
        HPX_PP_CAT(pluginname, _plugin_factory_type), pp)                      \
    template struct hpx::plugins::parcelport_factory<Parcelport>;              \
    HPX_EXPORT hpx::plugins::parcelport_factory_base* HPX_PP_CAT(              \
        pluginname, _factory_init)(                                            \
        std::vector<hpx::plugins::parcelport_factory_base*> & factories)       \
    {                                                                          \
        static HPX_PP_CAT(pluginname, _plugin_factory_type)                    \
            factory(factories);                                                \
        return &factory;                                                       \
    }                                                                          \
    /**/

#define HPX_REGISTER_PARCELPORT(Parcelport, pluginname)                        \
    HPX_REGISTER_PARCELPORT_(                                                  \
        Parcelport, HPX_PP_CAT(parcelport_, pluginname), pluginname)

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/message_handler_factory_base.hpp
// This macro is used to register the given component factory with
// Hpx.Plugin. This macro has to be used for each of the message handler
// factories.
#define HPX_REGISTER_MESSAGE_HANDLER_FACTORY_BASE(FactoryType, pluginname)     \
    HPX_PLUGIN_EXPORT(HPX_PLUGIN_PLUGIN_PREFIX,                                \
        hpx::plugins::plugin_factory_base, FactoryType, pluginname, factory)   \
    HPX_INIT_REGISTRY_PLUGIN_FACTORY_STATIC(                                   \
        HPX_PLUGIN_PLUGIN_PREFIX, pluginname, factory)                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from libs/plugin_factories/message_handler_factory.hpp
// This macro is used create and to register a minimal component factory with
// Hpx.Plugin.
#define HPX_REGISTER_MESSAGE_HANDLER_FACTORY(MessageHandler, pluginname)       \
    HPX_REGISTER_MESSAGE_HANDLER_FACTORY_BASE(                                 \
        hpx::plugins::message_handler_factory<MessageHandler>, pluginname)     \
    HPX_DEF_UNIQUE_PLUGIN_NAME(                                                \
        hpx::plugins::message_handler_factory<MessageHandler>, pluginname)     \
    template struct hpx::plugins::message_handler_factory<MessageHandler>;     \
    HPX_REGISTER_PLUGIN_REGISTRY_2(MessageHandler, pluginname)                 \
    /**/

#endif
