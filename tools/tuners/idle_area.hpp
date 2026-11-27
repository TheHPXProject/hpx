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
// would disturb its measurements.

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
}    // namespace hpx_tune
