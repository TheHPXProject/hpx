//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/format.hpp>

#include <hpx/batch_environments/detail/flux_environment.hpp>

#include <cstddef>
#include <string>

namespace hpx::util::batch_environments::detail {

    // FLUX_TASK_RANK: number of physical nodes
    // FLUX_JOB_NNODES: number of nodes in the job
    flux_environment::flux_environment()
      : node_num_(static_cast<std::size_t>(-1))
      , num_localities_(0)
      , valid_(false)
    {
        char const* num_nodes = std::getenv("FLUX_JOB_NNODES");
        valid_ = num_nodes != nullptr;
        if (valid_)
        {
            // Get the number of localities
            num_localities_ = from_string<std::size_t>(num_nodes);

            // Initialize our node number, if available
            char const* var = std::getenv("FLUX_TASK_RANK");
            if (var != nullptr)
            {
                node_num_ = from_string<std::size_t>(var);
            }
            else
            {
                valid_ = false;
            }
        }
    }
}    // namespace hpx::util::batch_environments::detail
