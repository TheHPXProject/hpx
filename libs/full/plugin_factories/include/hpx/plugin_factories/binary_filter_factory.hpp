//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/plugin_factories/binary_filter_factory_base.hpp>
#include <hpx/plugin_factories/macros.hpp>
#include <hpx/plugin_factories/plugin_registry.hpp>
#include <hpx/plugin_factories/unique_plugin_name.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins {

    ///////////////////////////////////////////////////////////////////////////
    /// The \a message_handler_factory provides a minimal implementation of a
    /// message handler's factory. If no additional functionality is required
    /// this type can be used to implement the full set of minimally required
    /// functions to be exposed by a message handler's factory instance.
    ///
    /// \tparam BinaryFilter The message handler type this factory should be
    ///                        responsible for.
    HPX_CXX_EXPORT template <typename BinaryFilter>
    struct binary_filter_factory : binary_filter_factory_base
    {
        /// \brief Construct a new factory instance
        ///
        /// \param global   [in] The pointer to a \a hpx#util#section instance
        ///                 referencing the settings read from the [settings]
        ///                 section of the global configuration file (hpx.ini)
        ///                 This pointer may be nullptr if no such section has
        ///                 been found.
        /// \param local    [in] The pointer to a \a hpx#util#section instance
        ///                 referencing the settings read from the section
        ///                 describing this component type:
        ///                 [hpx.components.\<name\>], where \<name\> is the
        ///                 instance name of the component as given in the
        ///                 configuration files.
        /// \param isenabled
        ///
        /// \note The contents of both sections has to be cloned in order to
        ///       save the configuration setting for later use.
        binary_filter_factory(util::section const* global,
            util::section const* local, bool isenabled)
          : isenabled_(isenabled)
        {
            // store the configuration settings
            if (nullptr != global)
                global_settings_ = *global;
            if (nullptr != local)
                local_settings_ = *local;
        }

        ~binary_filter_factory() override = default;

        /// Create a new instance of a message handler
        ///
        /// return Returns the newly created instance of the message handler
        ///        supported by this factory
        serialization::binary_filter* create(bool compress,
            serialization::binary_filter* next_filter = nullptr) override
        {
            if (isenabled_)
                return new BinaryFilter(compress, next_filter);
            return nullptr;
        }

    protected:
        util::section global_settings_;
        util::section local_settings_;
        bool isenabled_;
    };
}    // namespace hpx::plugins
