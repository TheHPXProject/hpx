//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/tuning/machine_description.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace hpx::tuning {

    machine_description describe_machine(threads::topology const& topo)
    {
        machine_description m;
        m.sockets = topo.get_number_of_sockets();
        m.numa_domains = topo.get_number_of_numa_nodes();
        m.cores = topo.get_number_of_cores();
        m.pus = topo.get_number_of_pus();

        // The whole domains: the first NUMA domain and the first socket stand
        // for all of them (a machine with domains of different sizes gets
        // the size of its first one).
        if (m.numa_domains > 1)
            m.pool_sizes.push_back(topo.get_number_of_numa_node_cores(0));
        if (m.sockets > 1)
            m.pool_sizes.push_back(topo.get_number_of_socket_cores(0));
        m.pool_sizes.push_back(m.cores);

        std::sort(m.pool_sizes.begin(), m.pool_sizes.end());
        m.pool_sizes.erase(
            std::unique(m.pool_sizes.begin(), m.pool_sizes.end()),
            m.pool_sizes.end());
        m.pool_sizes.erase(
            std::remove(m.pool_sizes.begin(), m.pool_sizes.end(), 0),
            m.pool_sizes.end());
        return m;
    }
}    // namespace hpx::tuning
