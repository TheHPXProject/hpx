//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The chunk area of hpx_tune (--chunk): the minimal run time of a chunk that
// task_size_chunk_size uses for the loops of each executor, one section per
// executor and pool size (hpx.task_size_chunk_size.<executor>.<pool size>)
// and one per executor for the pool sizes not measured. The regions are
// timed in children of the tool (child.hpp), as they need the HPX runtime:
// several per pool size (--chunk-runs), whose median is used. The area runs
// after the idle area.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"

#include <cstddef>

namespace hpx_tune {

    // the chunk area's command line options (--chunk-pool-size,
    // --chunk-rule, --chunk-regions, --chunk-runs)
    hpx::program_options::options_description chunk_options();

    // Time the executors' regions for the pool sizes given on the command
    // line (or half a NUMA domain and the machine's whole domains, one
    // worker per core) and derive the minimal run time of a chunk of each
    // executor by the rule given.
    area_result tune_chunk(hpx::program_options::variables_map const& vm,
        hpx::tuning::machine_description const& machine, std::size_t repeats,
        bool verbose);

    // A child of the chunk area (hpx_tune --child=chunk ...): starts the HPX
    // runtime with the HPX options of its command line, times the regions
    // on all its workers and writes them for its parent.
    int chunk_child_main(int argc, char* argv[]);
}    // namespace hpx_tune
