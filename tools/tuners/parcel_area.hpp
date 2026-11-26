//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The parcel area of hpx_tune (--parcel): the parcel layer's settings
// (hpx.parcel and its parcelports' sections), measured between two
// localities in children that start the HPX runtime; the tool's own process
// doesn't. Built only with networking.

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"

namespace hpx_tune {

    // the parcel area's command line options (--launcher, --placement, ...)
    hpx::program_options::options_description parcel_options();

    // Measure the configurations (phase 3, children of two localities) and
    // derive the parcel layer's settings (phase 4); adds the parcel area's
    // entries to the host fingerprint.
    area_result tune_parcel(area_context const& context);

    // run as a child of the parcel area (--child=parcel:..., see child.hpp):
    // starts the HPX runtime and measures one configuration
    int run_as_child(int argc, char* argv[]);
}    // namespace hpx_tune
#endif
