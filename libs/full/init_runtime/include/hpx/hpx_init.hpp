//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx_init.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/hpx_finalize.hpp>
#include <hpx/hpx_init_params.hpp>
#include <hpx/hpx_suspend.hpp>
#include <hpx/hpx_user_main_config.hpp>

#include <cstddef>
#include <functional>

///////////////////////////////////////////////////////////////////////////////
/// \namespace hpx
namespace hpx {

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread. This overload will not call
    /// `hpx_main`.
    ///
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::init function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from the user supplied \p f.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise, it will be executed as specified by the
    ///                     parameter\p mode.
    inline int init(std::function<int(hpx::program_options::variables_map&)> f,
        int argc, char** argv, init_params const& params = init_params());

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread. This overload will not call
    /// `hpx_main`.
    ///
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::init function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from the user supplied \p f.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise, it will be executed as specified by the
    ///                     parameter\p mode.
    inline int init(std::function<int(int, char**)> f, int argc, char** argv,
        init_params const& params = init_params());

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads) should be called from the users `main()`
    /// function. This overload expects a user-defined function named `hpx_main`
    /// at global scope, which will be used as the entry point for the HPX
    /// application.
    ///
    ///
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::init function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from `hpx_main` (or 0 when executed in
    ///                     worker mode).
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise, it will be executed as specified by the
    ///                     parameter\p mode.
    inline int init(
        int argc, char** argv, init_params const& params = init_params());

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread. This overload will not call
    /// `hpx_main`.
    ///
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::init function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from the user supplied \p f.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise, it will be executed as specified by the
    ///                     parameter\p mode.
    inline int init(std::nullptr_t f, int argc, char** argv,
        init_params const& params = init_params());

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    /// This overload expects a user-defined function named `hpx_main` at global
    /// scope, which will be used as the entry point for the HPX application.
    ///
    /// \param params       [in] The parameters to the \a hpx::init function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from `hpx_main` (or 0 when executed in
    ///                     worker mode).
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`. If not
    ///                     command line arguments are passed, console mode is
    ///                     assumed.
    ///
    /// \note               If no command line arguments are passed the HPX
    ///                     runtime system will not support any of the default
    ///                     command line options as described in the section
    ///                     'HPX Command Line Options'.
#if !defined(HPX_HAVE_STATIC_LINKING)
    inline int init(init_params const& params = init_params());
#else
    int init(init_params const& params = init_params());
#endif
}    // namespace hpx

#if !defined(DOXYGEN)
///////////////////////////////////////////////////////////////////////////////
// Pull in the implementation of the inlined hpx::init functions if we're not
// compiling the core HPX library.
#if !defined(HPX_EXPORTS)
#include <hpx/init_runtime/init_impl.hpp>
#endif
#endif
