//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file json.hpp
/// \brief The measurements and settings of the tuning routines as JSON text,
///        for a report that tools read (the ini file is what HPX reads).

#pragma once

#include <hpx/config.hpp>
#include <hpx/tuning/host_fingerprint.hpp>
#include <hpx/tuning/statistics.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hpx::tuning {

    /// The text as a JSON string, in double quotes: a quote, a backslash and
    /// the control characters are escaped, all other bytes are kept as they
    /// are (the text is taken to be UTF-8).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string json_string(
        std::string_view text);

    /// The number as JSON: the shortest form that reads back as the same
    /// double ("3", "0.25", "1e-07"); null if it isn't finite, as JSON has
    /// no infinity and no NaN.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string json_number(double value);

    /// A JSON object from its members, in the given order: each a name and
    /// a value that is JSON text already.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string json_object(
        std::vector<std::pair<std::string, std::string>> const& members);

    /// A JSON array from its elements, each JSON text already.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string json_array(
        std::vector<std::string> const& elements);

    /// A summary as {"count": ..., "min": ..., "q1": ..., "median": ...,
    /// "q3": ..., "p90": ..., "max": ...}.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_json(summary const& s);

    /// A setting as {"key": ..., "value": ..., "reason": ...,
    /// "is_default": ...}.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_json(
        tuned_setting const& setting);

    /// A fingerprint as an object with one member per entry, in its order,
    /// each {"value": ..., "compared": ...}.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string to_json(
        host_fingerprint const& fingerprint);
}    // namespace hpx::tuning
