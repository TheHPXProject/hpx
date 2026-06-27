//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>
#include <string>

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    // return the public IP address of the local node
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    resolve_public_ip_address();

    ///////////////////////////////////////////////////////////////////////
    // Take an ip v4 or v6 address and "standardize" it for comparison checks
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    cleanup_ip_address(std::string const& addr);

    ///////////////////////////////////////////////////////////////////////
    // Addresses are supposed to have the format <hostname>[:port]
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool split_ip_address(
        std::string const& v, std::string& host, std::uint16_t& port);
}    // namespace hpx::util
