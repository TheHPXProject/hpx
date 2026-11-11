////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/util/sed_transform.hpp>

#include <memory>
#include <regex>
#include <string>

namespace hpx::util {

    bool parse_sed_expression(
        std::string const& input, std::string& search, std::string& replace)
    {
        if (input.size() <= 2)
            return false;

        // s/search/replace/
        // ^^
        if ('s' != input.at(0) || '/' != input.at(1))
            return false;

        constexpr std::string::size_type search_begin = 2;
        std::string::size_type search_end = search_begin;

        // s/search/replace/
        //   ^^^^^^
        while (true)
        {
            // See if we're at the end of the string.
            if ((input.size() - 1) < search_end)
                return false;

            // If this is an escape, then don't check the next character.
            if ('\\' == input.at(search_end++))
            {
                // See if we're at the end of the string.
                if ((input.size() - 1) < ++search_end)
                    return false;
            }

            // The next character is read below, make sure it exists.
            if ((input.size() - 1) < search_end)
                return false;

            if ('/' == input.at(search_end))
            {
                --search_end;
                break;
            }
        }

        // s/search/replace/
        //         ^
        if ('/' != input.at(search_end + 1))
            return false;

        std::string::size_type const replace_begin = search_end + 2;
        std::string::size_type replace_end = input.size() - 1;

        // s/search/replace/
        //                 ^ (optional)
        if ('/' == input.at(replace_end))
            --replace_end;

        search = input.substr(search_begin, (search_end - search_begin) + 1);
        replace =
            input.substr(replace_begin, (replace_end - replace_begin) + 1);

        return true;
    }

    struct sed_transform::command
    {
        command(std::string const& search, std::string replace)
          : search_(search)
          , replace_(HPX_MOVE(replace))
        {
        }

        std::regex search_;
        std::string replace_;
    };

    sed_transform::sed_transform(std::string const& search, std::string replace)
      : command_(std::make_shared<command>(search, HPX_MOVE(replace)))
    {
    }

    sed_transform::sed_transform(std::string const& expression)
    {
        std::string search, replace;

        if (parse_sed_expression(expression, search, replace))
        {
            command_ = std::make_shared<command>(search, HPX_MOVE(replace));
        }
    }

    std::string sed_transform::operator()(std::string const& input) const
    {
        if (!command_)
            return input;

        return std::regex_replace(input, command_->search_, command_->replace_,
            std::regex_constants::match_default |
                std::regex_constants::format_sed);
    }
}    // namespace hpx::util
