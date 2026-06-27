//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/preprocessor.hpp>

///////////////////////////////////////////////////////////////////////////////
// from hpx/component_base/component_commandline.hpp
#define HPX_DEFINE_COMPONENT_COMMANDLINE_OPTIONS(add_options_function)         \
    namespace hpx::components::commandline_options_provider {                  \
        hpx::program_options::options_description add_commandline_options()    \
        {                                                                      \
            return add_options_function();                                     \
        }                                                                      \
    }                                                                          \
    /***/

/**
 * @brief Macro to register a command-line module with the HPX runtime.
 *
 * This macro facilitates the registration of a command-line module with the HPX
 * runtime system. A command-line module typically provides additional command-line
 * options that can be used to configure the HPX application.
 *
 * @param add_options_function The function that adds custom command-line options.
 */
#define HPX_REGISTER_COMMANDLINE_MODULE(add_options_function)                  \
    HPX_REGISTER_COMMANDLINE_OPTIONS()                                         \
    HPX_REGISTER_COMMANDLINE_REGISTRY(                                         \
        hpx::components::component_commandline, commandline_options)           \
    HPX_DEFINE_COMPONENT_COMMANDLINE_OPTIONS(add_options_function)             \
    /**/
#define HPX_REGISTER_COMMANDLINE_MODULE_DYNAMIC(add_options_function)          \
    HPX_REGISTER_COMMANDLINE_OPTIONS_DYNAMIC()                                 \
    HPX_REGISTER_COMMANDLINE_REGISTRY_DYNAMIC(                                 \
        hpx::components::component_commandline, commandline_options)           \
    HPX_DEFINE_COMPONENT_COMMANDLINE_OPTIONS(add_options_function)             \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from hpx/component_base/component_startup_shutdown.hpp
#define HPX_DEFINE_COMPONENT_STARTUP_SHUTDOWN(startup_, shutdown_)             \
    namespace hpx::components::startup_shutdown_provider {                     \
        bool HPX_PP_CAT(HPX_PLUGIN_COMPONENT_PREFIX, _startup)(                \
            startup_function_type & startup_func, bool& pre_startup)           \
        {                                                                      \
            hpx::function<bool(startup_function_type&, bool&)> tmp =           \
                static_cast<bool (*)(startup_function_type&, bool&)>(          \
                    startup_);                                                 \
            if (!!tmp)                                                         \
            {                                                                  \
                return tmp(startup_func, pre_startup);                         \
            }                                                                  \
            return false;                                                      \
        }                                                                      \
        bool HPX_PP_CAT(HPX_PLUGIN_COMPONENT_PREFIX, _shutdown)(               \
            shutdown_function_type & shutdown_func, bool& pre_shutdown)        \
        {                                                                      \
            hpx::function<bool(shutdown_function_type&, bool&)> tmp =          \
                static_cast<bool (*)(shutdown_function_type&, bool&)>(         \
                    shutdown_);                                                \
            if (!!tmp)                                                         \
            {                                                                  \
                return tmp(shutdown_func, pre_shutdown);                       \
            }                                                                  \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    /***/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(startup, shutdown)               \
    HPX_DEFINE_COMPONENT_STARTUP_SHUTDOWN(startup, shutdown)                   \
    namespace hpx::components::startup_shutdown_provider {                     \
        using HPX_PP_CAT(HPX_PLUGIN_COMPONENT_PREFIX, _provider) =             \
            component_startup_shutdown<HPX_PP_CAT(HPX_PLUGIN_COMPONENT_PREFIX, \
                                           _startup),                          \
                HPX_PP_CAT(HPX_PLUGIN_COMPONENT_PREFIX, _shutdown)>;           \
    }                                                                          \
    namespace hpx::components {                                                \
        template struct component_startup_shutdown<                            \
            startup_shutdown_provider::HPX_PP_CAT(                             \
                HPX_PLUGIN_COMPONENT_PREFIX, _startup),                        \
            startup_shutdown_provider::HPX_PP_CAT(                             \
                HPX_PLUGIN_COMPONENT_PREFIX, _shutdown)>;                      \
    }                                                                          \
    /**/

#define HPX_REGISTER_STARTUP_SHUTDOWN_MODULE(startup, shutdown)                \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS()                                  \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(startup, shutdown)                   \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY(                                    \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/
#define HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_DYNAMIC(startup, shutdown)        \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS_DYNAMIC()                          \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(startup, shutdown)                   \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY_DYNAMIC(                            \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/

/**
 * @brief Macro to register a startup module with the HPX runtime.
 *
 * This macro facilitates the registration of a startup module with the HPX
 * runtime system. A startup module typically contains initialization code
 * that should be executed when the HPX runtime starts.
 *
 * @param startup The name of the startup function to be registered.
 */
#define HPX_REGISTER_STARTUP_MODULE(startup)                                   \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS()                                  \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(startup, 0)                          \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY(                                    \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/
#define HPX_REGISTER_STARTUP_MODULE_DYNAMIC(startup)                           \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS_DYNAMIC()                          \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(startup, 0)                          \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY_DYNAMIC(                            \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/

#define HPX_REGISTER_SHUTDOWN_MODULE(shutdown)                                 \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS()                                  \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(0, shutdown)                         \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY(                                    \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/
#define HPX_REGISTER_SHUTDOWN_MODULE_DYNAMIC(shutdown)                         \
    HPX_REGISTER_STARTUP_SHUTDOWN_FUNCTIONS_DYNAMIC()                          \
    HPX_REGISTER_STARTUP_SHUTDOWN_MODULE_(0, shutdown)                         \
    HPX_REGISTER_STARTUP_SHUTDOWN_REGISTRY_DYNAMIC(                            \
        hpx::components::startup_shutdown_provider::HPX_PP_CAT(                \
            HPX_PLUGIN_COMPONENT_PREFIX, _provider),                           \
        startup_shutdown)                                                      \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from hpx/component_base/component_type.hpp
#define HPX_DEFINE_GET_COMPONENT_TYPE(component)                               \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        HPX_ALWAYS_EXPORT components::component_type                           \
        component_type_database<component>::get() noexcept                     \
        {                                                                      \
            return value;                                                      \
        }                                                                      \
        template <>                                                            \
        HPX_ALWAYS_EXPORT void component_type_database<component>::set(        \
            components::component_type t)                                      \
        {                                                                      \
            value = t;                                                         \
        }                                                                      \
    }                                                                          \
    /**/

#define HPX_DEFINE_GET_COMPONENT_TYPE_TEMPLATE(template_, component)           \
    namespace hpx::traits {                                                    \
        HPX_PP_STRIP_PARENS(template_)                                         \
        struct component_type_database<HPX_PP_STRIP_PARENS(component)>         \
        {                                                                      \
            inline static components::component_type value =                   \
                to_int(hpx::components::component_enum_type::invalid);         \
                                                                               \
            HPX_ALWAYS_EXPORT static components::component_type get() noexcept \
            {                                                                  \
                return value;                                                  \
            }                                                                  \
            HPX_ALWAYS_EXPORT static void set(components::component_type t)    \
            {                                                                  \
                value = t;                                                     \
            }                                                                  \
        };                                                                     \
    }                                                                          \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(component, type)                  \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        HPX_ALWAYS_EXPORT components::component_type                           \
        component_type_database<component>::get() noexcept                     \
        {                                                                      \
            return type;                                                       \
        }                                                                      \
        template <>                                                            \
        HPX_ALWAYS_EXPORT void component_type_database<component>::set(        \
            components::component_type)                                        \
        {                                                                      \
            HPX_ASSERT(false);                                                 \
        }                                                                      \
    }                                                                          \
    /**/

#define HPX_DEFINE_COMPONENT_NAME(...) HPX_DEFINE_COMPONENT_NAME_(__VA_ARGS__)

#define HPX_DEFINE_COMPONENT_NAME_(...)                                        \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_DEFINE_COMPONENT_NAME_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))   \
    /**/

#define HPX_DEFINE_COMPONENT_NAME_2(Component, name)                           \
    namespace hpx::components {                                                \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const*                                          \
        get_component_name<Component, void>() noexcept                         \
        {                                                                      \
            return HPX_PP_STRINGIZE(name);                                     \
        }                                                                      \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const*                                          \
        get_component_base_name<Component, void>() noexcept                    \
        {                                                                      \
            return nullptr;                                                    \
        }                                                                      \
    }                                                                          \
    /**/

#define HPX_DEFINE_COMPONENT_NAME_3(Component, name, base_name)                \
    namespace hpx::components {                                                \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const*                                          \
        get_component_name<Component, void>() noexcept                         \
        {                                                                      \
            return HPX_PP_STRINGIZE(name);                                     \
        }                                                                      \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const*                                          \
        get_component_base_name<Component, void>() noexcept                    \
        {                                                                      \
            return base_name;                                                  \
        }                                                                      \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from hpx/component_base/server/component_heap.hpp
#define HPX_REGISTER_COMPONENT_HEAP(Component)                                 \
    namespace hpx::components::detail {                                        \
        template <>                                                            \
        HPX_ALWAYS_EXPORT Component::heap_type&                                \
        component_heap_helper<Component>(...)                                  \
        {                                                                      \
            util::reinitializable_static<Component::heap_type> heap;           \
            return heap.get();                                                 \
        }                                                                      \
    }                                                                          \
    /**/
