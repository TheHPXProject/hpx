//  Copyright (c)      2025 Agustin Berge
//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file manage_runtime.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/condition_variable.hpp>
#include <hpx/hpx_init_params.hpp>
#include <hpx/modules/init_runtime.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <mutex>

namespace hpx {

    /// \brief Manages starting/stopping the HPX runtime system.
    ///
    /// An instance of this class may be used to start the HPX runtime system.
    class manage_runtime
    {
    public:
        /// Starts the runtime system.
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
        /// \returns            The function returns `true` if command line
        ///                     processing succeeded and the runtime system was
        ///                     started successfully. It will return `false`
        ///                     otherwise.
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
        ///
        /// \note               This function will block and wait for the runtime
        ///                     system to start before returning to the caller.
        bool start(int argc, char** argv,
            init_params const& init_args = init_params());

        /// Stops the runtime system.
        ///
        /// \returns            This function will always return zero.
        ///
        /// \note               The runtime system instance must have been
        ///                     previously started by a successful call to
        ///                     `start()`.
        ///
        /// \note               This function will block and wait for the runtime
        ///                     system to stop before returning to the caller.
        int stop();

        /// \returns            A pointer to the runtime system if an instance is
        ///                     running, otherwise `nullptr`.
        runtime* get_runtime_ptr() const noexcept
        {
            return rts_;
        }

    private:
        // Main HPX thread, does nothing but wait for the application to exit
        int hpx_main(int, char*[]);

    private:
        bool running_ = false;
        runtime* rts_ = nullptr;

        std::mutex startup_mtx_;
        std::condition_variable startup_cond_;

        spinlock stop_mtx_;
        condition_variable_any stop_cond_;
    };
}    // namespace hpx
