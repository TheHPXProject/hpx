//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/util/regex_from_pattern.hpp>

#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        [[nodiscard]] inline std::string regex_from_character_set(
            std::string::const_iterator& it,
            std::string::const_iterator const& end, error_code& ec)
        {
            std::string::const_iterator const start = it;
            std::string result(1, *it);    // copy '['
            if (++it != end)
            {
                if (*it == '!')
                {
                    result.append(1, '^');    // negated character set
                }
                else if (*it == ']')
                {
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "regex_from_character_set",
                        "Invalid pattern (empty character set) at: {}",
                        std::string(start, end));
                    return "";
                }
                else
                {
                    result.append(1, *it);    // append this character
                }

                // copy while in character set
                while (++it != end)
                {
                    result.append(1, *it);
                    if (*it == ']')
                        break;
                }
            }

            if (it == end || *it != ']')
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "regex_from_character_set",
                    "Invalid pattern (missing closing ']') at: {}",
                    std::string(start, end));
                return "";
            }

            return result;
        }
    }    // namespace detail

    std::string regex_from_pattern(std::string const& pattern, error_code& ec)
    {
        std::string result;
        auto const end = pattern.end();
        for (auto it = pattern.begin(); it != end; ++it)
        {
            switch (char const c = *it)
            {
            case '*':
                result.append(".*");
                break;

            case '?':
                result.append(1, '.');
                break;

            case '[':
            {
                std::string r = detail::regex_from_character_set(it, end, ec);
                if (ec)
                    return "";
                result.append(r);
            }
            break;

            case '\\':
                if (++it == end)
                {
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "regex_from_pattern", "Invalid escape sequence at: {}",
                        pattern);
                    return "";
                }
                result.append(1, *it);
                break;

            // escape regex special characters
            // NOLINTNEXTLINE(bugprone-branch-clone)
            case '+':
                [[fallthrough]];
            case '.':
                [[fallthrough]];
            case '(':
                [[fallthrough]];
            case ')':
                [[fallthrough]];
            case '{':
                [[fallthrough]];
            case '}':
                [[fallthrough]];
            case '^':
                [[fallthrough]];
            case '$':
                result.append("\\");
                [[fallthrough]];

            default:
                result.append(1, c);
                break;
            }
        }
        return result;
    }
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
