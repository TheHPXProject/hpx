//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx_init_params.hpp
/// \page hpx::init_params
/// \headerfile hpx/init.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/init_runtime_local.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <cstring>
#include <functional>
#include <string>
#include <vector>

#if !defined(DOXYGEN)
///////////////////////////////////////////////////////////////////////////////
// One of these functions must be implemented by the application for the
// console locality.
//
// int hpx_main();
// int hpx_main(int argc, char* argv[]);
// int hpx_main(hpx::program_options::variables_map& vm);
#endif

///////////////////////////////////////////////////////////////////////////////
/// \cond NOINTERNAL
namespace hpx_startup {
    // As an alternative, the user can provide a function hpx_startup::user_main,
    // which is semantically equivalent to the plain old C-main.
    int user_main();
    int user_main(int argc, char** argv);
}    // namespace hpx_startup
/// \endcond

///////////////////////////////////////////////////////////////////////////////
/// \namespace hpx
namespace hpx {
    /// \cond NOINTERNAL
    namespace resource {
        // Utilities to init the thread_pools of the resource partitioner
        using rp_callback_type = hpx::function<void(hpx::resource::partitioner&,
            hpx::program_options::variables_map const&)>;
    }    // namespace resource
    /// \endcond

#if !defined(DOXYGEN)
    typedef int (*hpx_main_type)(hpx::program_options::variables_map&);
    typedef int (*hpx_user_main_type)(int argc, char** argv);
#endif

    /// \struct init_params
    /// \brief Parameters used to initialize the HPX runtime through
    ///        \a hpx::init and \a hpx::start
    ///
    /// \var init_params::desc_cmdline
    /// This parameter may hold the description of additional command line
    /// arguments understood by the application. These options will be prepended
    /// to the default command line options understood by \a hpx::init.
    ///
    /// \var init_params::cfg
    /// A list of configuration settings which will be added to the system
    /// configuration before the runtime instance is run. Each of the entries
    /// in this list must have the format of a fully defined key/value pair from
    /// an ini-file (for instance 'hpx.component.enabled=1').
    ///
    /// \var init_params::startup
    /// A function to be executed inside an HPX thread before \p f is called.
    /// If this parameter is not given, no function will be executed.
    ///
    /// \var init_params::shutdown
    /// A function to be executed inside an HPX thread while hpx::finalize is
    /// executed. If this parameter is not given, no function will be executed.
    ///
    /// \var init_params::mode
    /// The mode the created runtime environment should be initialized in.
    /// There has to be exactly one locality in each HPX application which is
    /// executed in console mode (\a hpx::runtime_mode::console), all other
    /// localities have to be run in worker mode (\a hpx::runtime_mode::worker).
    ///
    /// \var init_params::rp_mode
    /// The mode the resource partitioner should be created in. If none is
    /// specified, the \a hpx::resource::partitioner_mode::default_ will be used.
    ///
    /// \var init_params::rp_callback
    /// This callback is called after the resource partitioner creation. It may
    /// be used to initialize thread pools. If none is specified no function will
    /// be executed.
    struct init_params
    {
        init_params()
        {
            std::strncpy(hpx::local::detail::app_name, HPX_APPLICATION_STRING,
                sizeof(hpx::local::detail::app_name) - 1);
        }

        // Parameters
        std::reference_wrapper<hpx::program_options::options_description const>
            desc_cmdline =
                hpx::local::detail::default_desc(HPX_APPLICATION_STRING);
        std::vector<std::string> cfg;
        std::function<void()> startup;
        std::function<void()> shutdown;
        hpx::runtime_mode mode = ::hpx::runtime_mode::default_;
        hpx::resource::partitioner_mode rp_mode =
            ::hpx::resource::partitioner_mode::default_;
        hpx::resource::rp_callback_type rp_callback;
    };
}    // namespace hpx
