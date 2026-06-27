//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx_finalize.hpp
/// \page hpx::finalize, hpx::disconnect
/// \headerfile hpx/init.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>

#include <hpx/config/warnings_prefix.hpp>

/// \namespace hpx
namespace hpx {
    /// \brief Main function to gracefully terminate the HPX runtime system.
    ///
    /// The function \a hpx::finalize is the main way to (gracefully) exit any
    /// HPX application. It must be called at least once, but can be called
    /// multiple times as well. However, only the first invocation will have
    /// effect. It will notify all connected localities to finish execution.
    /// Only after all other localities have exited this function will return,
    /// allowing to exit the console locality as well.
    ///
    /// During the execution of this function the runtime system will invoke
    /// all registered shutdown functions (see \a hpx::init) on all localities.
    ///
    /// \param shutdown_timeout This parameter allows to specify a timeout (in
    ///           microseconds), specifying how long any of the connected
    ///           localities should wait for pending tasks to be executed.
    ///           After this timeout, all suspended HPX-threads will be aborted.
    ///           Note, that this function will not abort any running
    ///           HPX-threads. In any case the shutdown will not proceed as long
    ///           as there is at least one pending/running HPX-thread.
    ///
    ///           The default value (`-1.0`) will try to find a globally set
    ///           timeout value (can be set as the configuration parameter
    ///           `hpx.shutdown_timeout`), and if that is not set or `-1.0` as
    ///           well, it will disable any timeout, each connected
    ///           locality will wait for all existing HPX-threads to terminate.
    ///
    /// \param localwait This parameter allows to specify a local wait time
    ///           (in microseconds) before the connected localities will be
    ///           notified and the overall shutdown process starts.
    ///
    ///           The default value (`-1.0`) will try to find a globally set
    ///           wait time value (can be set as the configuration parameter
    ///           "hpx.finalize_wait_time"), and if this is not set or `-1.0`
    ///           as well, it will disable any addition local wait time before
    ///           proceeding.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise, it throws an instance of
    ///           hpx::exception.
    ///
    /// This function will block and wait for all connected localities to exit
    /// before returning to the caller. It should be the last HPX-function
    /// called by any application.
    ///
    /// Using this function is an alternative to \a hpx::disconnect, these
    /// functions do not need to be called both.
    HPX_CXX_EXPORT HPX_EXPORT int finalize(double shutdown_timeout,
        double localwait = -1.0, hpx::error_code& ec = throws);

    /// \brief Main function to gracefully terminate the HPX runtime system.
    ///
    /// The function \a hpx::finalize is the main way to (gracefully) exit any
    /// HPX application. It must be called at least once, but can be called
    /// multiple times as well. However, only the first invocation will have
    /// effect. It will notify all connected localities to finish execution.
    /// Only after all other localities have exited this function will return,
    /// allowing to exit the console locality as well.
    ///
    /// During the execution of this function the runtime system will invoke
    /// all registered shutdown functions (see \a hpx::init) on all localities.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise, it throws an instance of
    ///           hpx::exception.
    ///
    /// This function will block and wait for all connected localities to exit
    /// before returning to the caller. It should be the last HPX-function
    /// called by any application.
    ///
    /// Using this function is an alternative to \a hpx::disconnect, these
    /// functions do not need to be called both.
    HPX_CXX_EXPORT inline int finalize(hpx::error_code& ec = throws)
    {
        return finalize(-1.0, -1.0, ec);
    }

    /// \brief Terminate any application non-gracefully.
    ///
    /// The function \a hpx::terminate is the non-graceful way to exit any
    /// application immediately. It can be called from any locality and will
    /// terminate all localities currently used by the application.
    ///
    /// \note    This function will cause HPX to call `std::terminate()` on
    ///          all localities associated with this application. If the function
    ///          is called not from an HPX thread it will fail and return an error
    ///          using the argument \a ec.
    HPX_CXX_EXPORT [[noreturn]] HPX_EXPORT void terminate();

    /// \brief Disconnect this locality from the application.
    ///
    /// The function \a hpx::disconnect can be used to disconnect a locality
    /// from a running HPX application.
    ///
    /// During the execution of this function the runtime system will invoke
    /// all registered shutdown functions (see \a hpx::init) on this locality.
    //
    /// \param shutdown_timeout This parameter allows to specify a timeout (in
    ///           microseconds), specifying how long this locality should wait
    ///           for pending tasks to be executed. After this timeout, all
    ///           suspended HPX-threads will be aborted.
    ///           Note, that this function will not abort any running
    ///           HPX-threads. In any case the shutdown will not proceed as long
    ///           as there is at least one pending/running HPX-thread.
    ///
    ///           The default value (`-1.0`) will try to find a globally set
    ///           timeout value (can be set as the configuration parameter
    ///           "hpx.shutdown_timeout"), and if that is not set or `-1.0` as
    ///           well, it will disable any timeout, each connected
    ///           locality will wait for all existing HPX-threads to terminate.
    ///
    /// \param localwait This parameter allows to specify a local wait time
    ///           (in microseconds) before the connected localities will be
    ///           notified and the overall shutdown process starts.
    ///
    ///           The default value (`-1.0`) will try to find a globally set
    ///           wait time value (can be set as the configuration parameter
    ///           `hpx.finalize_wait_time`), and if this is not set or `-1.0`
    ///           as well, it will disable any addition local wait time before
    ///           proceeding.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise, it throws an instance of
    ///           hpx::exception.
    ///
    /// This function will block and wait for this locality to finish executing
    /// before returning to the caller. It should be the last HPX-function
    /// called by any locality being disconnected.
    ///
    HPX_CXX_EXPORT HPX_EXPORT int disconnect(double shutdown_timeout,
        double localwait = -1.0, hpx::error_code& ec = throws);

    /// \brief Disconnect this locality from the application.
    ///
    /// The function \a hpx::disconnect can be used to disconnect a locality
    /// from a running HPX application.
    ///
    /// During the execution of this function the runtime system will invoke
    /// all registered shutdown functions (see \a hpx::init) on this locality.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise, it throws an instance of
    ///           hpx::exception.
    ///
    /// This function will block and wait for this locality to finish executing
    /// before returning to the caller. It should be the last HPX-function
    /// called by any locality being disconnected.
    ///
    HPX_CXX_EXPORT inline int disconnect(hpx::error_code& ec = throws)
    {
        return disconnect(-1.0, -1.0, ec);
    }

    /// \brief Stop the runtime system
    ///
    /// \returns            The function returns the value, which has been
    ///                     returned from the user supplied main HPX function
    ///                     (usually `hpx_main`).
    ///
    /// This function will block and wait for this locality to finish executing
    /// before returning to the caller. It should be the last HPX-function
    /// called on every locality. This function should be used only if the
    /// runtime system was started using `hpx::start`.
    ///
    HPX_CXX_EXPORT HPX_EXPORT int stop(hpx::error_code& ec = throws);
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
