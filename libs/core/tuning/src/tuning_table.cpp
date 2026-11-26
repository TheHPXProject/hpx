//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/tuning/tuning_table.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace hpx::tuning {

    std::string section_name(tuning_row const& row)
    {
        std::string const tuner = row.tuner.empty() ? "none" : row.tuner;
        return "hpx.idle_tuning." + row.scheduler + "." + tuner + "." +
            std::to_string(row.pool_size);
    }

    std::string to_ini(std::vector<tuning_row> const& rows,
        std::vector<std::string> const& comments, int const year)
    {
        std::ostringstream os;
        os << "# Copyright (c) " << year << " TheHPXProject\n"
           << "#\n"
           << "# SPDX-License-Identifier: BSL-1.0\n"
           << "# Distributed under the Boost Software License, Version 1.0. "
              "(See accompanying\n"
           << "# file LICENSE_1_0.txt or copy at "
              "http://www.boost.org/LICENSE_1_0.txt)\n";
        if (!comments.empty())
            os << "#\n";
        for (std::string const& c : comments)
            os << "# " << c << "\n";

        constexpr std::string_view prefix = "hpx.";
        for (tuning_row const& row : rows)
        {
            os << "\n[" << section_name(row) << "]\n";
            for (tuned_setting const& s : row.settings)
            {
                os << "# " << s.reason;
                if (s.is_default)
                    os << " (HPX's default)";
                os << "\n";
                std::string_view key = s.key;
                if (key.substr(0, prefix.size()) == prefix)
                    key.remove_prefix(prefix.size());
                os << key << " = " << s.value << "\n";
            }
        }
        return os.str();
    }
}    // namespace hpx::tuning
