//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <algorithm>
#include <cstddef>

int main()
{
    hpx::tuning::machine_description const m =
        hpx::tuning::describe_machine(hpx::threads::get_topology());

    HPX_TEST_LT(std::size_t(0), m.cores);
    HPX_TEST_LTE(m.cores, m.pus);
    HPX_TEST_LT(std::size_t(0), m.sockets);
    HPX_TEST_LT(std::size_t(0), m.numa_domains);

    // whole domains, ascending, unique, the last one the whole machine
    HPX_TEST(!m.pool_sizes.empty());
    HPX_TEST(std::is_sorted(m.pool_sizes.begin(), m.pool_sizes.end()));
    HPX_TEST(std::adjacent_find(m.pool_sizes.begin(), m.pool_sizes.end()) ==
        m.pool_sizes.end());
    HPX_TEST_EQ(m.pool_sizes.back(), m.cores);
    HPX_TEST_LT(std::size_t(0), m.pool_sizes.front());

    // the caches, from L1 up, each at least as large as the one below and
    // shared by at least as many cores
    for (std::size_t i = 0; i != m.caches.size(); ++i)
    {
        hpx::tuning::cache_level const& c = m.caches[i];
        HPX_TEST_LT(std::size_t(0), c.size);
        HPX_TEST_LT(std::size_t(0), c.line_size);
        HPX_TEST_LT(std::size_t(0), c.cores);
        HPX_TEST_LTE(c.cores, c.pus);
        if (i != 0)
        {
            HPX_TEST_LT(m.caches[i - 1].level, c.level);
            HPX_TEST_LTE(m.caches[i - 1].size, c.size);
            HPX_TEST_LTE(m.caches[i - 1].cores, c.cores);
        }
    }

    return hpx::util::report_errors();
}
