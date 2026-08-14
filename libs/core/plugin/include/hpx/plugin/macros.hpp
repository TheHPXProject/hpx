// Copyright Vladimir Prus 2004.
// Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Make HPX inspect tool happy: hpxinspect:nounnamed

#pragma once

#include <hpx/config.hpp>
#include <hpx/plugin/config.hpp>
#include <hpx/plugin/config/defines.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_NAME_2(name1, name2)                                        \
    HPX_PP_CAT(name1, HPX_PP_CAT(_, name2))                                    \
    /**/

#define HPX_PLUGIN_NAME_3(name, base, cname)                                   \
    HPX_PP_CAT(cname, HPX_PP_CAT(_, HPX_PP_CAT(name, HPX_PP_CAT(_, base))))    \
    /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_LIST_NAME_(prefix, name, base)                              \
    HPX_PP_CAT(HPX_PP_CAT(prefix, _exported_plugins_list_),                    \
        HPX_PLUGIN_NAME_2(name, base))                                         \
    /**/

#define HPX_PLUGIN_EXPORTER_NAME_(prefix, name, base, cname)                   \
    HPX_PP_CAT(HPX_PP_CAT(prefix, _plugin_exporter_),                          \
        HPX_PLUGIN_NAME_3(name, base, cname))                                  \
    /**/

#define HPX_PLUGIN_EXPORTER_INSTANCE_NAME_(prefix, name, base, cname)          \
    HPX_PP_CAT(HPX_PP_CAT(prefix, _plugin_exporter_instance_),                 \
        HPX_PLUGIN_NAME_3(name, base, cname))                                  \
    /**/

#define HPX_PLUGIN_FORCE_LOAD_NAME_(prefix, name, base)                        \
    HPX_PP_CAT(HPX_PP_CAT(prefix, _exported_plugins_force_load_),              \
        HPX_PLUGIN_NAME_2(name, base))                                         \
    /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_LIST_NAME(name, base)                                       \
    HPX_PLUGIN_LIST_NAME_(HPX_PLUGIN_SYMBOLS_PREFIX, name, base)               \
    /**/

#define HPX_PLUGIN_EXPORTER_NAME(name, base, cname)                            \
    HPX_PLUGIN_EXPORTER_NAME_(HPX_PLUGIN_SYMBOLS_PREFIX, name, base, cname)    \
    /**/

#define HPX_PLUGIN_EXPORTER_INSTANCE_NAME(name, base, cname)                   \
    HPX_PLUGIN_EXPORTER_INSTANCE_NAME_(                                        \
        HPX_PLUGIN_SYMBOLS_PREFIX, name, base, cname)                          \
    /**/

#define HPX_PLUGIN_FORCE_LOAD_NAME(name, base)                                 \
    HPX_PLUGIN_FORCE_LOAD_NAME_(HPX_PLUGIN_SYMBOLS_PREFIX, name, base)         \
    /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_LIST_NAME_DYNAMIC(name, base)                               \
    HPX_PLUGIN_LIST_NAME_(HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, base), /**/

#define HPX_PLUGIN_EXPORTER_NAME_DYNAMIC(name, base, cname)                    \
    HPX_PLUGIN_EXPORTER_NAME_(                                                 \
        HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, base, cname)                  \
    /**/

#define HPX_PLUGIN_EXPORTER_INSTANCE_NAME_DYNAMIC(name, base, cname)           \
    HPX_PLUGIN_EXPORTER_INSTANCE_NAME_(                                        \
        HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, base, cname)                  \
    /**/

#define HPX_PLUGIN_FORCE_LOAD_NAME_DYNAMIC(name, base)                         \
    HPX_PLUGIN_FORCE_LOAD_NAME_(HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, base) \
    /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_EXPORT_(                                                    \
    prefix, name, BaseType, ActualType, actualname, classname)                 \
    extern "C" HPX_PLUGIN_EXPORT_API std::map<std::string, hpx::any_nonser>*   \
        HPX_PLUGIN_API HPX_PLUGIN_LIST_NAME_(prefix, name, classname)();       \
                                                                               \
    namespace {                                                                \
        struct HPX_PLUGIN_EXPORTER_NAME_(prefix, name, actualname, classname)  \
        {                                                                      \
            HPX_PLUGIN_EXPORTER_NAME_(prefix, name, actualname, classname)()   \
            {                                                                  \
                static hpx::util::plugin::concrete_factory<BaseType,           \
                    ActualType>                                                \
                    cf;                                                        \
                hpx::util::plugin::abstract_factory<BaseType>* w = &cf;        \
                std::string actname(HPX_PP_STRINGIZE(actualname));             \
                std::transform(actname.begin(), actname.end(),                 \
                    actname.begin(), [](char c) { return std::tolower(c); });  \
                HPX_PLUGIN_LIST_NAME_(prefix, name, classname)                 \
                ()->insert(std::make_pair(actname, hpx::any_nonser(w)));       \
            }                                                                  \
        } HPX_PLUGIN_EXPORTER_INSTANCE_NAME_(                                  \
            prefix, name, actualname, classname);                              \
    }                                                                          \
    /**/

#define HPX_PLUGIN_EXPORT(name, BaseType, ActualType, actualname, classname)   \
    HPX_PLUGIN_EXPORT_(HPX_PLUGIN_SYMBOLS_PREFIX, name, BaseType, ActualType,  \
        actualname, classname)                                                 \
    /**/

#define HPX_PLUGIN_EXPORT_DYNAMIC(                                             \
    name, BaseType, ActualType, actualname, classname)                         \
    HPX_PLUGIN_EXPORT_(HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, BaseType,      \
        ActualType, actualname, classname)                                     \
    /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_PLUGIN_EXPORT_LIST_(prefix, name, classname)                       \
    extern "C" HPX_PLUGIN_EXPORT_API std::map<std::string, hpx::any_nonser>*   \
        HPX_PLUGIN_API                                                         \
        HPX_PLUGIN_LIST_NAME_(prefix, name, classname)()                       \
    {                                                                          \
        static std::map<std::string, hpx::any_nonser> r;                       \
        return &r;                                                             \
    }                                                                          \
    extern "C" HPX_PLUGIN_EXPORT_API void HPX_PLUGIN_FORCE_LOAD_NAME_(         \
        prefix, name, classname)()                                             \
    {                                                                          \
        HPX_PLUGIN_LIST_NAME_(prefix, name, classname)();                      \
    }                                                                          \
    /**/

#define HPX_PLUGIN_EXPORT_LIST(name, classname)                                \
    HPX_PLUGIN_EXPORT_LIST_(HPX_PLUGIN_SYMBOLS_PREFIX, name, classname)        \
    /**/

#define HPX_PLUGIN_EXPORT_LIST_DYNAMIC(name, classname)                        \
    HPX_PLUGIN_EXPORT_LIST_(                                                   \
        HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC, name, classname)                    \
    /**/
