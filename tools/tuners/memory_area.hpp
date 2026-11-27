//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The memory area of hpx_tune (--memory): the rate at which a core copies
// data that fits its share of each cache level, and data in memory, as
// task_size_chunk_size uses it for the library's operations on builtin
// elements (hpx.task_size_chunk_size.copy_bytes_per_ns_l1 to _memory, see
// get_copy_bytes_per_ns).

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"

namespace hpx_tune {

    // the memory area's command line options (--memory-runs)
    hpx::program_options::options_description memory_options();

    // Time a copy on one core for a footprint in each cache level and one in
    // memory, in several children one after the other, and write the rates.
    area_result tune_memory(area_context const& context);

    // A child of the memory area (hpx_tune --child=memory ...): starts the
    // HPX runtime on one worker, times copies of the footprints of its
    // command line and writes the rates for its parent; with --memory-verify
    // it reads back the rates task_size_chunk_size uses instead.
    int memory_child_main(int argc, char* argv[]);

    // hpx_tune --check=<file>: a child started with the file reads back the
    // rate for each level's share and for memory; the check holds if they
    // are the file's.
    verify_result verify_memory(verify_context const& context);
}    // namespace hpx_tune
