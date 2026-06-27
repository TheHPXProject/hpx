//  Copyright Vladimir Prus 2004.
//  Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/plugin/config.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/plugin/virtual_constructor.hpp>

namespace hpx::util::plugin {

    namespace detail {

        HPX_CXX_CORE_EXPORT struct HPX_PLUGIN_EXPORT_API
            abstract_factory_item_base
        {
            virtual ~abstract_factory_item_base() = default;
            void create(int*******);    // dummy placeholder
        };

        // A template class that is given the base type of plugin and a set of
        // constructor parameter types and defines the appropriate virtual
        // 'create' function.
        HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename Base,
            typename Parameter>
        struct HPX_PLUGIN_EXPORT_API abstract_factory_item;

        template <typename BasePlugin, typename Base, typename... Parameters>
        struct HPX_PLUGIN_EXPORT_API abstract_factory_item<BasePlugin, Base,
            hpx::util::pack<Parameters...>> : public Base
        {
            using Base::create;
            virtual BasePlugin* create(
                dll_handle const& dll, Parameters... parameters) = 0;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BasePlugin>
    struct HPX_PLUGIN_EXPORT_API abstract_factory
      : detail::abstract_factory_item<BasePlugin,
            detail::abstract_factory_item_base,
            virtual_constructor_t<BasePlugin>>
    {
    };
}    // namespace hpx::util::plugin
