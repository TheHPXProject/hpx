//  Copyright Vladimir Prus 2004.
//  Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/plugin/config.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/plugin/abstract_factory.hpp>
#include <hpx/plugin/plugin_wrapper.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::plugin {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename Concrete,
            typename Base, typename Parameter>
        struct HPX_PLUGIN_EXPORT_API concrete_factory_item;

        template <typename BasePlugin, typename Concrete, typename Base,
            typename... Parameters>
        struct HPX_PLUGIN_EXPORT_API concrete_factory_item<BasePlugin, Concrete,
            Base, hpx::util::pack<Parameters...>> : public Base
        {
            [[nodiscard]] BasePlugin* create(
                dll_handle const& dll, Parameters... parameters) override
            {
                return new plugin_wrapper<Concrete, Parameters...>(
                    dll, parameters...);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename Concrete>
    struct HPX_PLUGIN_EXPORT_API concrete_factory
      : detail::concrete_factory_item<BasePlugin, Concrete,
            abstract_factory<BasePlugin>, virtual_constructor_t<BasePlugin>>
    {
    };
}    // namespace hpx::util::plugin
