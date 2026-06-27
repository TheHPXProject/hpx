//  Copyright (c) 2007-#include <hpx/plugin_factories/macros.hpp>2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/plugin_factories/macros.hpp>
#include <hpx/plugin_factories/plugin_factory_base.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins {

    ///////////////////////////////////////////////////////////////////////////
    /// The \a plugin_factory_base has to be used as a base class for all
    /// plugin factories.
    HPX_CXX_EXPORT struct HPX_EXPORT binary_filter_factory_base
      : plugin_factory_base
    {
        ~binary_filter_factory_base() override = default;

        /// Create a new instance of a binary filter
        ///
        /// return Returns the newly created instance of the binary filter
        ///        supported by this factory
        virtual serialization::binary_filter* create(bool compress,
            serialization::binary_filter* next_filter = nullptr) = 0;
    };
}    // namespace hpx::plugins
