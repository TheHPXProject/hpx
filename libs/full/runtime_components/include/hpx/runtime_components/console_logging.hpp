//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/logging.hpp>

#include <cstddef>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    HPX_CXX_EXPORT HPX_EXPORT void console_logging(
        logging_destination dest, std::size_t level, std::string const& msg);
    HPX_CXX_EXPORT HPX_EXPORT void cleanup_logging();
    HPX_CXX_EXPORT HPX_EXPORT void activate_logging();
}    // namespace hpx::components
