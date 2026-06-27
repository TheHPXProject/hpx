//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <iostream>
#include <ostream>

namespace hpx::iostreams {

    template <typename Char = char, typename Sink = void>
    using ostream = std::basic_ostream<Char>;
}    // namespace hpx::iostreams

namespace hpx {

    using std::cerr;
    using std::cout;
    using std::endl;
    using std::flush;

    HPX_DEPRECATED_V(2, 0,
        "In local-only builds hpx::async_flush is equivalent to std::flush")
    inline std::ostream& async_flush(std::ostream& os)
    {
        return os << std::flush;
    }

    HPX_DEPRECATED_V(
        2, 0, "In local-only builds hpx::async_endl is equivalent to std::endl")
    inline std::ostream& async_endl(std::ostream& os)
    {
        return os << std::endl;
    }

    inline std::ostream& consolestream = std::cerr;

    inline std::ostream& get_consolestream() noexcept
    {
        return std::cerr;
    }
}    // namespace hpx
