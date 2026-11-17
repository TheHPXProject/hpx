//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#pragma once

#include <hpx/config.hpp>
#include <hpx/components/iostreams/export_definitions.hpp>
#include <hpx/components/iostreams/ostream.hpp>

#include <ostream>
#include <sstream>
#include <string>

namespace hpx {

    // as in local-only builds, hpx::endl and hpx::flush are std::endl and
    // std::flush
    using std::endl;
    using std::flush;

    HPX_IOSTREAMS_EXPORT extern iostreams::ostream<> cout;
    HPX_IOSTREAMS_EXPORT extern iostreams::ostream<> cerr;

    // special stream which writes to a predefine stringstream on the console
    HPX_IOSTREAMS_EXPORT extern iostreams::ostream<> consolestream;
    HPX_IOSTREAMS_EXPORT std::stringstream const& get_consolestream();
}    // namespace hpx
