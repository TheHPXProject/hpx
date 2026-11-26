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

    return hpx::util::report_errors();
}
