//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assertion/source_location.hpp>

#include <string>

namespace hpx::assertion {

    /// The signature for an assertion handler
    HPX_CXX_CORE_EXPORT using assertion_handler =
        void (*)(hpx::source_location const& loc, char const* expr,
            std::string const& msg);

    /// Set the assertion handler to be used within a program. If the handler has been
    /// set already once, the call to this function will be ignored.
    /// \note This function is not thread safe
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT assertion_handler set_assertion_handler(
        assertion_handler handler);
}    // namespace hpx::assertion
