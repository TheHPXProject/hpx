//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::start
/// \headerfile hpx/init.hpp

#pragma once

#include <hpx/assert.hpp>
#include <hpx/hpx_start.hpp>
#include <hpx/hpx_user_main_config.hpp>
#include <hpx/init_runtime/detail/run_or_start.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/program_options.hpp>

#include <cstddef>
#include <utility>

#if defined(__FreeBSD__)
extern char** environ;    // available in executables only
#endif

#if defined(HPX_WINDOWS) && defined(HPX_HAVE_APEX)
#include <string>

namespace apex {

    // force linking of the application with APEX
    HPX_SYMBOL_IMPORT std::string& version();
}    // namespace apex
#endif

namespace hpx_startup {

    extern std::function<int(hpx::program_options::variables_map&)> const&
    get_main_func();
}

namespace hpx {

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as an HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution.
    inline bool start(
        std::function<int(hpx::program_options::variables_map&)> f, int argc,
        char** argv, init_params const& params)
    {
        return detail::start_impl(
            HPX_MOVE(f), argc, argv, params, HPX_PREFIX, environ);
    }

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as an HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution.
    inline bool start(std::function<int(int, char**)> f, int argc, char** argv,
        init_params const& params)
    {
        hpx::function<int(hpx::program_options::variables_map&)> main_f =
            hpx::bind_back(detail::init_helper, HPX_MOVE(f));
        return detail::start_impl(
            HPX_MOVE(main_f), argc, argv, params, HPX_PREFIX, environ);
    }

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as an HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution.
    inline bool start(int argc, char** argv, init_params const& params)
    {
#if defined(HPX_HAVE_RUN_MAIN_EVERYWHERE)
        init_params pars = params;
        pars.cfg.insert(pars.cfg.begin(), "hpx.run_hpx_main!=1");
        return detail::start_impl(hpx_startup::get_main_func(), argc, argv,
            pars, HPX_PREFIX, environ);
#else
        return detail::start_impl(hpx_startup::get_main_func(), argc, argv,
            params, HPX_PREFIX, environ);
#endif
    }

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as an HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution.
    inline bool start(
        std::nullptr_t, int argc, char** argv, init_params const& params)
    {
        hpx::function<int(hpx::program_options::variables_map&)> main_f;
        return detail::start_impl(
            HPX_MOVE(main_f), argc, argv, params, HPX_PREFIX, environ);
    }

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as an HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution.
    inline bool start(init_params const& params)
    {
#if defined(HPX_HAVE_RUN_MAIN_EVERYWHERE)
        init_params pars = params;
        pars.cfg.insert(pars.cfg.begin(), "hpx.run_hpx_main!=1");
        return detail::start_impl(hpx_startup::get_main_func(),
            hpx::local::detail::dummy_argc, hpx::local::detail::dummy_argv,
            pars, HPX_PREFIX, environ);
#else
        return detail::start_impl(hpx_startup::get_main_func(),
            hpx::local::detail::dummy_argc, hpx::local::detail::dummy_argv,
            params, HPX_PREFIX, environ);
#endif
    }
}    // namespace hpx
