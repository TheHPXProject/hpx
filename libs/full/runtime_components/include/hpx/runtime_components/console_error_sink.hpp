//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming.hpp>

#include <exception>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    // Stub function which applies the console_error_sink action.
    HPX_CXX_EXPORT HPX_EXPORT void console_error_sink(
        hpx::id_type const& dst, std::exception_ptr const& e);

    // Stub function which applies the console_error_sink action.
    HPX_CXX_EXPORT HPX_EXPORT void console_error_sink(
        std::exception_ptr const& e);
}    // namespace hpx::components
