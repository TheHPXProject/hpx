//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file tuning_table.hpp
/// \brief The table of tuned settings by scheduler, tuner and pool size, and
///        its form as an ini file.

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace hpx::tuning {

    /// The settings for the pools of one size that use one scheduler and
    /// one tuner. A pool whose scheduler, tuner and number of workers match
    /// no row keeps the default settings.
    HPX_CXX_CORE_EXPORT struct tuning_row
    {
        /// the scheduler as --hpx:queuing names it (local-priority-fifo, ...)
        std::string scheduler;
        /// the tuner as hpx.scheduler_tuner names it, empty or "none"
        /// without one
        std::string tuner;
        /// the number of workers of the pool
        std::size_t pool_size = 0;
        std::vector<tuned_setting> settings;
    };

    /// The ini section of a row: hpx.idle_tuning.<scheduler>.<tuner>.<size>,
    /// the tuner "none" if the row has none.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string section_name(
        tuning_row const& row);

    /// The rows as ini sections, one per row, named by section_name, so
    /// that they can be written into one file with the sections of other
    /// areas (see ini_file.hpp).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<ini_section> to_sections(
        std::vector<tuning_row> const& rows);

    /// The table as the text of an ini file: to_ini of ini_file.hpp with
    /// the sections of to_sections. The keys of the idle settings are
    /// written without the leading "hpx.".
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_ini(
        std::vector<tuning_row> const& rows,
        std::vector<std::string> const& comments, int year);
}    // namespace hpx::tuning
