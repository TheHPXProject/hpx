//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/tuning/json.hpp>

#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::tuning {

    std::string json_string(std::string_view const text)
    {
        constexpr char hex[] = "0123456789abcdef";

        std::string result = "\"";
        result.reserve(text.size() + 2);
        for (char const c : text)
        {
            switch (c)
            {
            case '"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    auto const code = static_cast<unsigned char>(c);
                    result += "\\u00";
                    result += hex[code >> 4];
                    result += hex[code & 0xf];
                }
                else
                {
                    result += c;
                }
                break;
            }
        }
        result += '"';
        return result;
    }

    std::string json_number(double const value)
    {
        if (!std::isfinite(value))
            return "null";

        char buffer[32];
        auto const [end, error] =
            std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (error != std::errc())
            return "null";
        return std::string(buffer, end);
    }

    std::string json_object(
        std::vector<std::pair<std::string, std::string>> const& members)
    {
        std::string result = "{";
        for (auto const& [name, value] : members)
        {
            if (result.size() != 1)
                result += ", ";
            result += json_string(name);
            result += ": ";
            result += value;
        }
        result += '}';
        return result;
    }

    std::string json_array(std::vector<std::string> const& elements)
    {
        std::string result = "[";
        for (std::string const& element : elements)
        {
            if (result.size() != 1)
                result += ", ";
            result += element;
        }
        result += ']';
        return result;
    }

    std::string to_json(summary const& s)
    {
        return json_object({
            {"count", std::to_string(s.count)},
            {"min", json_number(s.min)},
            {"q1", json_number(s.q1)},
            {"median", json_number(s.median)},
            {"q3", json_number(s.q3)},
            {"p90", json_number(s.p90)},
            {"max", json_number(s.max)},
        });
    }

    std::string to_json(tuned_setting const& setting)
    {
        return json_object({
            {"key", json_string(setting.key)},
            {"value", json_string(setting.value)},
            {"reason", json_string(setting.reason)},
            {"is_default", setting.is_default ? "true" : "false"},
        });
    }

    std::string to_json(host_fingerprint const& fingerprint)
    {
        std::vector<std::pair<std::string, std::string>> members;
        members.reserve(fingerprint.entries.size());
        for (fingerprint_entry const& entry : fingerprint.entries)
        {
            members.emplace_back(entry.key,
                json_object({
                    {"value", json_string(entry.value)},
                    {"compared", entry.compared ? "true" : "false"},
                }));
        }
        return json_object(members);
    }
}    // namespace hpx::tuning
