//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file component_commandline.hpp
/// \page HPX_REGISTER_COMMANDLINE_MODULE
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/macros.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    namespace commandline_options_provider {

        HPX_CXX_EXPORT hpx::program_options::options_description
        add_commandline_options();
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The \a component_startup_shutdown provides a minimal implementation of
    /// a component's startup/shutdown function provider.
    ///
    HPX_CXX_EXPORT struct component_commandline : component_commandline_base
    {
        /// \brief Return any additional command line options valid for this
        ///        component
        ///
        /// \return The module is expected to fill a options_description object
        ///         with any additional command line options this component
        ///         will handle.
        ///
        /// \note   This function will be executed by the runtime system
        ///         during system startup.
        hpx::program_options::options_description add_commandline_options()
            override
        {
            return commandline_options_provider::add_commandline_options();
        }
    };
}    // namespace hpx::components
