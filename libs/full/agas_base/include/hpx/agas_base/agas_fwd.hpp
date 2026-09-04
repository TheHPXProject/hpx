//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>

#include <cstdint>
#include <string>

namespace hpx::components {

    HPX_CXX_EXPORT using component_type = std::int32_t;
}

namespace hpx::agas {

    ////////////////////////////////////////////////////////////////////////
    // Base name used to register AGAS service instances
    HPX_CXX_EXPORT inline constexpr char const* const service_name =
        "/{}/agas/";

    // Fixed addresses of AGAS components
    HPX_CXX_EXPORT inline constexpr std::uint64_t booststrap_prefix = 0ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t primary_ns_msb =
        0x100000001ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t primary_ns_lsb =
        0x000000001ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t component_ns_msb =
        0x100000001ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t component_ns_lsb =
        0x000000002ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t symbol_ns_msb =
        0x100000001ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t symbol_ns_lsb =
        0x000000003ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t locality_ns_msb =
        0x100000001ULL;
    HPX_CXX_EXPORT inline constexpr std::uint64_t locality_ns_lsb =
        0x000000004ULL;

    HPX_CXX_EXPORT using iterate_types_function_type =
        hpx::function<void(std::string const&, components::component_type),
            true>;

    /// \brief Get the currently configured timeout for AGAS RPC operations.
    HPX_CXX_EXPORT HPX_EXPORT hpx::chrono::steady_duration
    get_rpc_timeout() noexcept;

    /// \brief Set the timeout for AGAS RPC operations.
    ///
    /// \param timeout The new duration for AGAS RPC timeouts.
    HPX_CXX_EXPORT HPX_EXPORT void set_rpc_timeout(
        hpx::chrono::steady_duration const& timeout) noexcept;

    HPX_CXX_EXPORT struct HPX_EXPORT component_namespace;
    HPX_CXX_EXPORT struct HPX_EXPORT locality_namespace;
    HPX_CXX_EXPORT struct HPX_EXPORT primary_namespace;
    HPX_CXX_EXPORT struct HPX_EXPORT symbol_namespace;

    namespace server {

        HPX_CXX_EXPORT struct HPX_EXPORT component_namespace;
        HPX_CXX_EXPORT struct HPX_EXPORT locality_namespace;
        HPX_CXX_EXPORT struct HPX_EXPORT primary_namespace;
        HPX_CXX_EXPORT struct HPX_EXPORT symbol_namespace;
    }    // namespace server
}    // namespace hpx::agas
