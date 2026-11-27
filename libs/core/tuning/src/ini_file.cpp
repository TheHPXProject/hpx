//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace hpx::tuning {

    namespace {

        // the key as written in its section (see to_ini)
        std::string_view key_in_section(
            std::string_view key, std::string_view const section) noexcept
        {
            if (key.size() > section.size() &&
                key.substr(0, section.size()) == section &&
                key[section.size()] == '.')
            {
                key.remove_prefix(section.size() + 1);
                return key;
            }

            constexpr std::string_view prefix = "hpx.";
            if (key.substr(0, prefix.size()) == prefix)
                key.remove_prefix(prefix.size());
            return key;
        }

        // A comment as lines of at most max_comment_line characters: "# "
        // and the text, broken at blanks, the following lines indented
        // ("#   "); a word longer than a line is broken where the line ends.
        // HPX's ini reader cuts comments off with an expression that
        // recurses once per character, which overflows an HPX thread's stack
        // on lines of a few hundred characters.
        void write_comment(std::ostringstream& os, std::string_view text)
        {
            std::string_view prefix = "# ";
            while (true)
            {
                std::size_t const room = max_comment_line - prefix.size();
                if (text.size() <= room)
                {
                    os << prefix << text << "\n";
                    return;
                }
                std::size_t end = text.rfind(' ', room);
                if (end == std::string_view::npos || end == 0)
                    end = room;
                os << prefix << text.substr(0, end) << "\n";
                text.remove_prefix(end);
                while (!text.empty() && text.front() == ' ')
                    text.remove_prefix(1);
                if (text.empty())
                    return;
                prefix = "#   ";
            }
        }
    }    // namespace

    std::string to_ini(std::vector<ini_section> const& sections,
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
            write_comment(os, c);

        for (ini_section const& section : sections)
        {
            os << "\n[" << section.name << "]\n";
            for (tuned_setting const& s : section.settings)
            {
                write_comment(os,
                    s.is_default ? s.reason + " (HPX's default)" : s.reason);
                os << key_in_section(s.key, section.name) << " = " << s.value
                   << "\n";
            }
        }
        return os.str();
    }
}    // namespace hpx::tuning
