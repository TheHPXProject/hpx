//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/tuning_table.hpp>

#include <string>
#include <vector>

namespace hpx::tuning {

    std::string section_name(tuning_row const& row)
    {
        std::string const tuner = row.tuner.empty() ? "none" : row.tuner;
        return "hpx.idle_tuning." + row.scheduler + "." + tuner + "." +
            std::to_string(row.pool_size);
    }

    std::vector<ini_section> to_sections(std::vector<tuning_row> const& rows)
    {
        std::vector<ini_section> sections;
        sections.reserve(rows.size());
        for (tuning_row const& row : rows)
            sections.push_back(ini_section{section_name(row), row.settings});
        return sections;
    }

    std::string to_ini(std::vector<tuning_row> const& rows,
        std::vector<std::string> const& comments, int const year)
    {
        return to_ini(to_sections(rows), comments, year);
    }
}    // namespace hpx::tuning
