//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// What the areas of hpx_tune share with the driver (hpx_tune.cpp). Each area
// lives in files of its own (idle_area.hpp and .cpp, ...) and gives the
// driver its options and a function that measures and returns an
// area_result; the driver writes the results of all areas into one file.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/tuning.hpp>

#include <string>
#include <vector>

namespace hpx_tune {

    // what an area adds to the ini file: its sections and its comment lines
    // (the measurements the reader of the file should see)
    struct area_result
    {
        std::vector<hpx::tuning::ini_section> sections;
        std::vector<std::string> comments;
    };
}    // namespace hpx_tune
