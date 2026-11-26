//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file machine_description.hpp
/// \brief The parts of the machine's topology the tuning rules depend on.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <vector>

namespace hpx::tuning {

    /// The parts of the machine's topology the tuning rules depend on.
    HPX_CXX_CORE_EXPORT struct machine_description
    {
        std::size_t sockets = 0;
        std::size_t numa_domains = 0;
        std::size_t cores = 0;
        std::size_t pus = 0;

        /// The pool sizes the settings are measured for, in workers, one
        /// per core: the whole domains of the machine (the cores of a NUMA
        /// domain, of a socket and of the machine), ascending, without
        /// repetitions. A pool of another size uses the default settings.
        std::vector<std::size_t> pool_sizes;
    };

    /// Describe the machine the given topology was discovered on.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT machine_description describe_machine(
        threads::topology const& topo);
}    // namespace hpx::tuning
