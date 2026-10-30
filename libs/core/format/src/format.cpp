///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2017-2018 Agustin Berge
//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/format/api.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hpx::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    inline std::size_t format_atoi(
        std::string_view str, std::size_t* pos = nullptr) noexcept
    {
        // copy input to a null terminated buffer
        static constexpr std::size_t digits10 =
            std::numeric_limits<std::size_t>::digits10 + 1;
        char buffer[digits10 + 1] = {};
        std::memcpy(buffer, str.data(), (std::min) (str.size(), digits10));

        char const* first = buffer;
        char* last = buffer;

        // the buffer holds one digit more than a std::size_t can represent,
        // so saturate the way strtoull itself does when it runs out of range
        unsigned long long const value = std::strtoull(first, &last, 10);
        constexpr unsigned long long max_value =
            static_cast<unsigned long long>(
                (std::numeric_limits<std::size_t>::max)());
        std::size_t const r =
            static_cast<std::size_t>(value < max_value ? value : max_value);
        if (pos != nullptr)
            *pos = last - first;
        return r;
    }

    inline std::string_view format_substr(std::string_view str,
        std::size_t start, std::size_t end = std::string_view::npos) noexcept
    {
        return start < str.size() ? str.substr(start, end - start) :
                                    std::string_view{};
    }

    ///////////////////////////////////////////////////////////////////////////
    // replacement-field ::= '{' [arg-id] [':' format-spec] '}'
    struct format_field
    {
        std::size_t arg_id;
        std::string_view spec;
    };

    inline format_field parse_field(std::string_view field) noexcept
    {
        std::size_t const sep = field.find(':');
        if (sep != std::string_view::npos)
        {
            std::string_view const arg_id = format_substr(field, 0, sep);
            std::string_view const spec = format_substr(field, sep + 1);

            std::size_t const id = format_atoi(arg_id);
            return format_field{id, spec};
        }
        else
        {
            std::size_t const id = format_atoi(field);
            return format_field{id, ""};
        }
    }

    void format_to(std::ostream& os, std::string_view format_str,
        format_arg const* args, std::size_t count)
    {
        std::size_t index = 0;
        while (!format_str.empty())
        {
            if (format_str[0] == '{' || format_str[0] == '}')
            {
                if (format_str[1] == format_str[0])
                {
                    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
                    os.write(format_str.data(), 1);    // '{' or '}'
                }
                else
                {
                    if (format_str[0] == '}')
                    {
                        throw std::runtime_error("bad format string");
                    }
                    std::size_t const end = format_str.find('}');
                    std::string_view const field_str =
                        format_substr(format_str, 1, end);
                    format_field const field = parse_field(field_str);
                    format_str.remove_prefix(end - 1);

                    std::size_t const id =
                        field.arg_id ? field.arg_id - 1 : index;
                    if (id >= count)
                    {
                        throw std::runtime_error(
                            "bad format string (wrong number of arguments)");
                    }

                    args[id](os, field.spec);
                    ++index;
                }
                format_str.remove_prefix(2);
            }
            else
            {
                std::size_t const next = format_str.find_first_of("{}");
                std::size_t const cnt =
                    next != std::string_view::npos ? next : format_str.size();

                // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
                os.write(format_str.data(), static_cast<std::streamsize>(cnt));
                format_str.remove_prefix(cnt);
            }
        }
    }

    HPX_CORE_EXPORT std::string format(
        std::string_view format_str, format_arg const* args, std::size_t count)
    {
        std::ostringstream os;
        detail::format_to(os, format_str, args, count);
        return os.str();
    }
}    // namespace hpx::util::detail

namespace hpx::util {

    std::string const& format()
    {
        static std::string empty;
        return empty;
    }
}    // namespace hpx::util
