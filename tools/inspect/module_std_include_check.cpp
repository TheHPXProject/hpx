//  module_std_include_check implementation  --------------------------------//

//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/format.hpp>

#include <algorithm>
#include <cstddef>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "function_hyper.hpp"
#include "module_std_include_check.hpp"

namespace {

    // List of HPX modules that have been converted to guard standard includes.
    // Modules are added here as their include guards are implemented.
    constexpr char const* const converted_modules[] = {
        "config",
        "preprocessor",
        "properties",
    };

    struct std_header
    {
        std::string_view name;
        std::string_view guard;
        bool macro;
    };

    // Central definition in cmake/standard_headers.inc.
    constexpr std_header standard_headers[] = {
#define HPX_STD_HEADER(name, guard, macro) std_header{name, guard, macro != 0},
#include "../../cmake/standard_headers.inc"
#undef HPX_STD_HEADER
    };

    std_header const* find_standard_header(std::string_view header) noexcept
    {
        for (auto const& h : standard_headers)
        {
            if (h.name == header)
            {
                return &h;
            }
        }
        return nullptr;
    }

    bool is_standard_header(std::string_view header) noexcept
    {
        return find_standard_header(header) != nullptr;
    }

    bool is_macro_header(std::string_view header) noexcept
    {
        auto const* h = find_standard_header(header);
        return h != nullptr && h->macro;
    }

    // Determine whether path belongs to an include directory of a converted
    // module, e.g. libs/core/<module>/include/...
    bool is_converted_module_header(
        std::string const& file_path, std::string& found_module)
    {
        std::string path_normalized = file_path;
        std::replace(path_normalized.begin(), path_normalized.end(), '\\', '/');

        for (char const* mod : converted_modules)
        {
            std::string const core_prefix =
                std::string("libs/core/") + mod + "/include/";
            std::string const full_prefix =
                std::string("libs/full/") + mod + "/include/";

            if (path_normalized.find(core_prefix) != std::string::npos ||
                path_normalized.find(full_prefix) != std::string::npos)
            {
                found_module = mod;
                return true;
            }
        }
        return false;
    }

    // Regex matching guard condition:
    // #if defined(HPX_HAVE_TEXTUAL_STD_INCLUDES)
    // #ifdef HPX_HAVE_TEXTUAL_STD_INCLUDES
    // (with backwards-compatible logical equivalents).
    // Note: requires full match of the directive condition.
    std::regex const guard_regex(
        R"(^\s*#\s*(?:if\s+(?:defined\s*\(\s*HPX_HAVE_TEXTUAL_STD_INCLUDES\s*\)|)"
        R"(HPX_HAVE_TEXTUAL_STD_INCLUDES\b|)"
        R"(!\s*\(\s*defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*&&)"
        R"(\s*defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*\)|)"
        R"(!\s*\(\s*defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*&&)"
        R"(\s*defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*\)|)"
        R"(!defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*\|\|)"
        R"(\s*!defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)|)"
        R"(!defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*\|\|)"
        R"(\s*!defined\s*\(\s*HPX_COMPILE_BMI\s*\))|)"
        R"(ifdef\s+HPX_HAVE_TEXTUAL_STD_INCLUDES\b)\s*$)");

    std::regex const if_regex(R"(^\s*#\s*(if|ifdef|ifndef)\b)");
    std::regex const elif_else_regex(
        R"(^\s*#\s*(elif|elifdef|elifndef|else)\b)");
    std::regex const endif_regex(R"(^\s*#\s*endif\b)");
    std::regex const include_regex(
        R"(^\s*#\s*include\s*<([a-zA-Z0-9_/.]+)>(.*)$)");

    void strip_line_comments(std::string const& line, bool& in_block_comment,
        bool& in_raw_string, std::string& raw_delimiter,
        std::string& clean_line)
    {
        clean_line.clear();
        std::size_t i = 0;
        std::size_t const n = line.size();
        bool in_string = false;
        bool in_char = false;

        while (i < n)
        {
            if (in_block_comment)
            {
                if (i + 1 < n && line[i] == '*' && line[i + 1] == '/')
                {
                    in_block_comment = false;
                    i += 2;
                    if (clean_line.empty() || clean_line.back() != ' ')
                    {
                        clean_line += ' ';
                    }
                }
                else
                {
                    ++i;
                }
            }
            else if (in_raw_string)
            {
                std::string const end_marker = ")" + raw_delimiter + "\"";
                if (line.substr(i, end_marker.size()) == end_marker)
                {
                    in_raw_string = false;
                    raw_delimiter.clear();
                    i += end_marker.size();
                }
                else
                {
                    ++i;
                }
            }
            else if (in_string)
            {
                if (line[i] == '\\' && i + 1 < n)
                {
                    clean_line += line[i];
                    clean_line += line[i + 1];
                    i += 2;
                }
                else if (line[i] == '"')
                {
                    in_string = false;
                    clean_line += line[i];
                    ++i;
                }
                else
                {
                    clean_line += line[i];
                    ++i;
                }
            }
            else if (in_char)
            {
                if (line[i] == '\\' && i + 1 < n)
                {
                    clean_line += line[i];
                    clean_line += line[i + 1];
                    i += 2;
                }
                else if (line[i] == '\'')
                {
                    in_char = false;
                    clean_line += line[i];
                    ++i;
                }
                else
                {
                    clean_line += line[i];
                    ++i;
                }
            }
            else
            {
                if (i + 1 < n && line[i] == '/' && line[i + 1] == '*')
                {
                    in_block_comment = true;
                    i += 2;
                }
                else if (i + 1 < n && line[i] == '/' && line[i + 1] == '/')
                {
                    break;
                }
                else if (i + 1 < n && line[i] == 'R' && line[i + 1] == '"')
                {
                    std::size_t paren_pos = line.find('(', i + 2);
                    if (paren_pos != std::string::npos)
                    {
                        raw_delimiter = line.substr(i + 2, paren_pos - (i + 2));
                        in_raw_string = true;
                        i = paren_pos + 1;
                    }
                    else
                    {
                        clean_line += line[i];
                        ++i;
                    }
                }
                else if (line[i] == '"')
                {
                    in_string = true;
                    clean_line += line[i];
                    ++i;
                }
                else if (line[i] == '\'')
                {
                    // In C++, an apostrophe between digits is a numeric digit separator,
                    // not the start of a character literal (e.g. 1'000).
                    bool const is_digit_separator = !clean_line.empty() &&
                        (std::isxdigit(
                             static_cast<unsigned char>(clean_line.back())) ||
                            clean_line.back() == '.') &&
                        (i + 1 < n &&
                            (std::isxdigit(
                                 static_cast<unsigned char>(line[i + 1])) ||
                                line[i + 1] == '\''));

                    if (is_digit_separator)
                    {
                        clean_line += line[i];
                        ++i;
                    }
                    else
                    {
                        in_char = true;
                        clean_line += line[i];
                        ++i;
                    }
                }
                else
                {
                    clean_line += line[i];
                    ++i;
                }
            }
        }
    }

}    // unnamed namespace

namespace boost { namespace inspect {

    module_std_include_check::module_std_include_check()
      : m_errors(0)
    {
        register_signature(".h");
        register_signature(".hpp");
        register_signature(".hxx");
        register_signature(".ipp");
    }

    void module_std_include_check::inspect(string const& library_name,
        path const& full_path, string const& contents)
    {
        std::string mod_name;
        if (!is_converted_module_header(full_path.string(), mod_name))
        {
            return;
        }

        if (contents.find("hpxinspect:nostdincludeguardfile") != string::npos)
        {
            return;
        }

        struct if_frame
        {
            bool this_if_was_guard;
            bool in_else_of_guard;
        };

        std::vector<if_frame> if_stack;

        auto is_guarded = [&if_stack]() -> bool {
            for (auto const& frame : if_stack)
            {
                if (frame.this_if_was_guard && !frame.in_else_of_guard)
                {
                    return true;
                }
            }
            return false;
        };

        std::istringstream stream(contents);
        std::string line;
        std::size_t line_number = 0;
        bool in_block_comment = false;
        bool in_raw_string = false;
        std::string raw_delimiter;

        while (std::getline(stream, line))
        {
            ++line_number;

            std::string clean_line;
            strip_line_comments(line, in_block_comment, in_raw_string,
                raw_delimiter, clean_line);

            if (std::regex_match(clean_line, guard_regex))
            {
                if_stack.push_back({true, false});
                continue;
            }

            if (std::regex_search(clean_line, if_regex))
            {
                if_stack.push_back({false, false});
                continue;
            }

            if (std::regex_search(clean_line, elif_else_regex))
            {
                if (!if_stack.empty() && if_stack.back().this_if_was_guard)
                {
                    if_stack.back().in_else_of_guard = true;
                }
                continue;
            }

            if (std::regex_search(clean_line, endif_regex))
            {
                if (!if_stack.empty())
                {
                    if_stack.pop_back();
                }
                continue;
            }

            std::smatch matches;
            if (std::regex_match(clean_line, matches, include_regex))
            {
                std::string const header = matches[1].str();

                if (line.find("hpxinspect:nostdincludeguard") !=
                    std::string::npos)
                {
                    continue;
                }

                if (is_standard_header(header) && !is_macro_header(header) &&
                    !is_guarded())
                {
                    ++m_errors;
                    std::string lineloc =
                        linelink(full_path, hpx::util::to_string(line_number));
                    error(library_name, full_path,
                        "unguarded #include (" + header + ") on line " +
                            lineloc +
                            " (must be guarded with "
                            "#if defined(HPX_HAVE_TEXTUAL_STD_INCLUDES))");
                }
            }
        }
    }
}}    // namespace boost::inspect
