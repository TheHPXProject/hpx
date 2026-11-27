//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle area of hpx_tune (--idle): the scheduler's idle settings, one
// section per scheduler and tuner for pools of any size
// (hpx.idle_tuning.<scheduler>.<tuner>), or per scheduler, tuner and pool
// size for the sizes asked for (hpx.idle_tuning.<scheduler>.<tuner>.<pool
// size>). It measures in the
// tool's own process and never starts the HPX runtime, whose idle workers
// would disturb its measurements. Its check of a written file (hpx_tune
// --check) starts children, which run the runtime with the file.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"

namespace hpx_tune {

    // the idle area's command line options (--configuration, --pool-size)
    hpx::program_options::options_description idle_options();

    // Measure and derive the idle settings (phases 2 and 4 of the design)
    // for the configurations given on the command line (default: the
    // default scheduler), for the pool sizes given there, or else for pools
    // of any size.
    area_result tune_idle(area_context const& context);

    // Check a written file (hpx_tune --check): for each scheduler and tuner
    // with a row, a child started with the file runs two pools, one of a
    // size with a row of its own (if the file has one) and one of a size
    // without; each pool's idle settings have to be those of the row the
    // runtime's lookup takes: the row of the pool's size, else the row for
    // pools of any size, else the plain setting, setting by setting.
    verify_result verify_idle(verify_context const& context);

    // a child of the idle area (--child=idle): starts the runtime and
    // writes each pool's idle settings for the parent
    int idle_child_main(int argc, char* argv[]);
}    // namespace hpx_tune
