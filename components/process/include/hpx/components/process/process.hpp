// Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <hpx/components/process/child.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::components::process {

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    child execute(hpx::id_type const& id, Ts&&... ts)
    {
        return hpx::new_<child>(id, HPX_FORWARD(Ts, ts)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Launch an executable as a new HPX locality that connects to the
    ///        currently running (bootstrap) locality via TCP.
    ///
    /// This function spawns \p to_launch as a child process configured to join
    /// the calling HPX runtime as an additional, connecting locality. It forces
    /// the TCP parcelport for bootstrapping, forwards AGAS server address/port
    /// information from the current runtime's configuration, lets the operating
    /// system pick an ephemeral parcel port for the new locality, and blocks
    /// the caller until the spawned locality signals that it has finished
    /// starting up (via a named latch derived from the executable name and,
    /// optionally,
    /// \p spawn_id).
    ///
    /// \param to_launch  Path to the executable to launch as the connecting
    ///                    locality.
    /// \param outer_args Additional command-line arguments appended after the
    ///                    mandatory HPX bootstrap arguments
    ///                    (`--hpx:ignore-batch-env`,
    ///                    `--hpx:ini=hpx.parcel.tcp.priority=1000`,
    ///                    `--hpx:ini=hpx.parcel.bootstrap=tcp`). Defaults to an
    ///                    empty list.
    /// \param outer_env  Additional environment variable entries in
    ///                    `"NAME=value"` form (or just `"NAME"` to set an empty
    ///                    value) that are merged into (and override) the
    ///                    inherited environment before launching the child
    ///                    process. Defaults to an empty list.
    /// \param spawn_id   Optional identifier used to disambiguate the startup
    ///                    latch name when multiple instances of the same
    ///                    executable are launched concurrently. When present,
    ///                    the latch name is `"<exe-stem>/<spawn_id>"`;
    ///                    otherwise it is just `"<exe-stem>"`.
    ///
    /// \returns A \c child object representing the launched connecting-locality
    ///          process, once it has signaled startup completion.
    ///
    /// \throws hpx::exception (or a derived type) if the child process could
    ///         not be started (\c process::throw_on_error() is used).
    ///
    /// \note The following environment variables are set (overriding any
    ///       inherited values) before launching:
    ///       - `HPX_AGAS_SERVER_ADDRESS` / `HPX_AGAS_SERVER_PORT` ? taken from
    ///         the current runtime's `hpx.agas.address` / `hpx.agas.port`
    ///         configuration entries.
    ///       - `HPX_PARCEL_SERVER_ADDRESS` ? same address as the AGAS server.
    ///       - `HPX_PARCEL_SERVER_PORT` ? set to `"0"` so the OS assigns an
    ///         ephemeral port.
    ///       - `HPX_ON_STARTUP_WAIT_ON_LATCH` ? the startup latch name
    ///         described above, used to synchronize with the caller.
    HPX_PROCESS_EXPORT child launch_connecting_locality(
        std::string const& to_launch,
        std::vector<std::string> const& outer_args = {},
        std::vector<std::string> const& outer_env = {},
        std::optional<std::uint64_t> spawn_id = nullopt);
}    // namespace hpx::components::process
