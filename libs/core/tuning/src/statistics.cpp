//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/tuning/statistics.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace hpx::tuning {

    namespace {

        // the value at quantile p of sorted values, interpolated linearly
        double quantile(std::vector<double> const& sorted, double const p)
        {
            double const pos = p * static_cast<double>(sorted.size() - 1);
            auto const lower = static_cast<std::size_t>(std::floor(pos));
            std::size_t const upper = (std::min) (lower + 1, sorted.size() - 1);
            double const frac = pos - static_cast<double>(lower);
            return sorted[lower] + frac * (sorted[upper] - sorted[lower]);
        }
    }    // namespace

    summary summarize(std::vector<double> values)
    {
        summary s;
        if (values.empty())
            return s;

        std::sort(values.begin(), values.end());
        s.count = values.size();
        s.min = values.front();
        s.q1 = quantile(values, 0.25);
        s.median = quantile(values, 0.5);
        s.q3 = quantile(values, 0.75);
        s.p90 = quantile(values, 0.9);
        s.max = values.back();
        return s;
    }
}    // namespace hpx::tuning
