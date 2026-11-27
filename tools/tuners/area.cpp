//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include "area.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace hpx_tune {

    namespace {

        // in a function, so that it exists before the first registrar runs,
        // whatever the order of the static initialization of the files
        std::vector<area>& areas()
        {
            static std::vector<area> registered;
            return registered;
        }
    }    // namespace

    std::vector<area> registered_areas()
    {
        std::vector<area> sorted = areas();
        std::stable_sort(
            sorted.begin(), sorted.end(), [](area const& lhs, area const& rhs) {
                return lhs.order < rhs.order;
            });
        return sorted;
    }

    area_registrar::area_registrar(area a)
    {
        if (a.name.empty() || a.tune == nullptr)
            throw std::logic_error("hpx_tune: an area needs a name and tune");
        for (area const& registered : areas())
        {
            if (registered.name == a.name)
            {
                throw std::logic_error(
                    "hpx_tune: area '" + a.name + "' registered twice");
            }
        }
        areas().push_back(HPX_MOVE(a));
    }
}    // namespace hpx_tune
