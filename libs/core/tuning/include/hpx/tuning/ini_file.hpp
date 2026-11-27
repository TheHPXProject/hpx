//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file ini_file.hpp
/// \brief Tuned settings as the text of an ini file, section by section.

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace hpx::tuning {

    /// The longest comment line to_ini writes, in characters.
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t max_comment_line = 100;

    /// One section of an ini file: its name without the brackets (for
    /// instance "hpx.parcel" or "hpx.idle_tuning.local-priority-fifo.none.20")
    /// and its settings.
    HPX_CXX_CORE_EXPORT struct ini_section
    {
        std::string name;
        std::vector<tuned_setting> settings;
    };

    /// The sections as the text of an ini file: the license header of HPX's
    /// files with the given year (so that a generated file can be added to
    /// the repository as it is), the given comment lines (the host, the
    /// date, the tool's version, ...), then the sections in the given order.
    /// Each setting is preceded by a comment with its reason and, if it
    /// equals HPX's default, says so. A key is written relative to its
    /// section: the section's name and a dot are removed from its front if
    /// it starts with them, otherwise a leading "hpx." is (so that
    /// "hpx.parcel.tcp.enable" is written as "enable" in the section
    /// "hpx.parcel.tcp", and "hpx.min_idle_backoff_time" as
    /// "min_idle_backoff_time" in an idle tuning section). Comment lines are
    /// at most max_comment_line characters long, broken at blanks (HPX's
    /// ini reader overflows an HPX thread's stack on lines of a few hundred
    /// characters); a value is written as it is.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_ini(
        std::vector<ini_section> const& sections,
        std::vector<std::string> const& comments, int year);
}    // namespace hpx::tuning
