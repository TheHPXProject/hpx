//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2025 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/timing.hpp>

namespace hpx::local {

    /// \brief Wait for all local HPX threads to complete execution.
    ///
    /// This function blocks the calling thread until all HPX worker threads
    /// on the current locality have completed their work and become idle.
    /// It performs local termination detection by waiting for the thread
    /// manager to drain all pending tasks.
    ///
    /// \note This function should be called from an HPX thread or after
    ///       the HPX runtime has been initialized.
    ///
    /// \throws hpx::exception if called when the runtime is not in a valid
    ///         state (e.g., before initialization or after shutdown).
    ///
    /// \par Example:
    /// \code
    /// #include <hpx/hpx_init.hpp>
    /// #include <hpx/runtime_local/termination_detection.hpp>
    ///
    /// int hpx_main()
    /// {
    ///     // Launch some asynchronous work
    ///     for (int i = 0; i < 100; ++i)
    ///     {
    ///         hpx::post([]{ /* do work */ });
    ///     }
    ///
    ///     // Wait for all local threads to complete
    ///     hpx::local::termination_detection();
    ///
    ///     return hpx::finalize();
    /// }
    /// \endcode
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void termination_detection();

    /// \brief Wait for all local HPX threads to complete with a timeout.
    ///
    /// This function blocks the calling thread until either all HPX worker
    /// threads have completed their work, or the specified timeout duration
    /// has elapsed.
    ///
    /// \param timeout Maximum duration to wait for termination. The function
    ///                will return false if this duration elapses before all
    ///                threads complete.
    ///
    /// \returns true if all threads completed before the timeout, false if
    ///          the timeout elapsed.
    ///
    /// \throws hpx::exception if called when the runtime is not in a valid
    ///         state.
    ///
    /// \par Example:
    /// \code
    /// #include <hpx/hpx_init.hpp>
    /// #include <hpx/runtime_local/termination_detection.hpp>
    /// #include <chrono>
    ///
    /// int hpx_main()
    /// {
    ///     // Launch some work
    ///     hpx::post([]{ /* long running task */ });
    ///
    ///     // Wait up to 5 seconds
    ///     if (!hpx::local::termination_detection(std::chrono::seconds(5)))
    ///     {
    ///         std::cerr << "Warning: Work did not complete within timeout\n";
    ///         // Implement fallback strategy
    ///     }
    ///
    ///     return hpx::finalize();
    /// }
    /// \endcode
    ///
    /// \see termination_detection()
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool termination_detection(
        hpx::chrono::steady_duration const& timeout);

    /// \brief Wait for all local HPX threads to complete until a deadline.
    ///
    /// This function blocks the calling thread until either all HPX worker
    /// threads have completed their work, or the specified deadline time
    /// point is reached.
    ///
    /// \param deadline Absolute time point when waiting should stop. The
    ///                 function will return false if this time is reached
    ///                 before all threads complete.
    ///
    /// \returns true if all threads completed before the deadline, false if
    ///          the deadline was reached.
    ///
    /// \throws hpx::exception if called when the runtime is not in a valid
    ///         state.
    ///
    /// \par Example:
    /// \code
    /// #include <hpx/hpx_init.hpp>
    /// #include <hpx/runtime_local/termination_detection.hpp>
    /// #include <chrono>
    ///
    /// int hpx_main()
    /// {
    ///     auto deadline = std::chrono::steady_clock::now() +
    ///                     std::chrono::seconds(10);
    ///
    ///     hpx::post([]{ /* work */ });
    ///
    ///     if (!hpx::local::termination_detection(deadline))
    ///     {
    ///         std::cerr << "Deadline reached\n";
    ///     }
    ///
    ///     return hpx::finalize();
    /// }
    /// \endcode
    ///
    /// \see termination_detection(std::chrono::duration<double>)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool termination_detection(
        hpx::chrono::steady_time_point const& deadline);

    /// \brief Wait for all local HPX threads with cancellation support.
    ///
    /// This function blocks the calling thread until all HPX worker threads
    /// have completed, the timeout elapses, or a stop is requested via the
    /// stop_token. This enables cooperative cancellation of the wait
    /// operation.
    ///
    /// \param stop_token Token that can be used to request cancellation of
    ///                   the wait operation.
    /// \param timeout    Maximum duration to wait. Defaults to maximum
    ///                   duration (effectively infinite).
    ///
    /// \returns true if all threads completed, false if timeout elapsed or
    ///          stop was requested.
    ///
    /// \throws hpx::exception if called when the runtime is not in a valid
    ///         state.
    ///
    /// \par Example:
    /// \code
    /// #include <hpx/hpx_init.hpp>
    /// #include <hpx/runtime_local/termination_detection.hpp>
    /// #include <hpx/stop_token.hpp>
    /// #include <chrono>
    ///
    /// int hpx_main()
    /// {
    ///     hpx::stop_source stop_src;
    ///
    ///     // In shutdown handler
    ///     hpx::thread shutdown_thread([&stop_src]() {
    ///         hpx::this_thread::sleep_for(std::chrono::seconds(30));
    ///         stop_src.request_stop();
    ///     });
    ///
    ///     hpx::post([]{ /* work */ });
    ///
    ///     // Wait with cancellation support
    ///     bool completed = hpx::local::termination_detection(
    ///         stop_src.get_token(),
    ///         std::chrono::minutes(1)
    ///     );
    ///
    ///     if (!completed)
    ///     {
    ///         std::cerr << "Cancelled or timed out\n";
    ///     }
    ///
    ///     shutdown_thread.join();
    ///     return hpx::finalize();
    /// }
    /// \endcode
    ///
    /// \see termination_detection(std::chrono::duration<double>)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool termination_detection(
        hpx::stop_token stop_token,
        hpx::chrono::steady_duration const& timeout =
            hpx::chrono::steady_duration(
                (hpx::chrono::steady_clock::duration::max) ()));
    /// \endcond

}    // namespace hpx::local
