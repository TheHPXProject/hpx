//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks topology::get_cache_info: for each level of the first processing
// unit's caches, a cache has lines, is shared by at least one core and by
// at least as many processing units as cores, has the size get_cache_size
// gives for that processing unit alone, and a higher level is at least as
// large and shared by at least as many cores. Levels outside 1 to 5 and a
// processing unit that doesn't exist have no cache.

#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <iostream>

int main()
{
    auto const& topo = hpx::threads::create_topology();

    std::size_t const pu = 0;
    // the mask of the processing unit alone, for get_cache_size
    hpx::threads::mask_type mask = hpx::threads::mask_type();
    hpx::threads::resize(mask, topo.get_number_of_pus());
    hpx::threads::set(mask, pu);

    hpx::threads::cache_info previous;
    for (int level = 1; level <= 5; ++level)
    {
        hpx::threads::cache_info const info = topo.get_cache_info(pu, level);
        std::cout << "L" << level << ": " << info.size << " bytes, lines of "
                  << info.line_size << " bytes, shared by " << info.cores
                  << " cores (" << info.pus << " processing units)\n";
        if (info.size == 0)
        {
            continue;
        }

        HPX_TEST_LT(std::size_t(0), info.line_size);
        HPX_TEST_LT(std::size_t(0), info.cores);
        HPX_TEST_LTE(info.cores, info.pus);
        HPX_TEST_EQ(info.size, topo.get_cache_size(mask, level));

        if (previous.size != 0)
        {
            HPX_TEST_LTE(previous.size, info.size);
            HPX_TEST_LTE(previous.cores, info.cores);
        }
        previous = info;
    }

    // no such level, no such processing unit
    HPX_TEST_EQ(topo.get_cache_info(pu, 0).size, std::size_t(0));
    HPX_TEST_EQ(topo.get_cache_info(pu, 6).size, std::size_t(0));
    HPX_TEST_EQ(topo.get_cache_info(topo.get_number_of_pus() + 1, 1).size,
        std::size_t(0));

    return hpx::util::report_errors();
}
