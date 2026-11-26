//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file statistics.hpp
/// \brief The summary of repeated measurements that the tuning routines
///        report (median, quartiles and extremes).

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <vector>

namespace hpx::tuning {

    /// The summary of a series of repeated measurements of one quantity. A
    /// rule compares medians and treats two values whose interquartile
    /// ranges overlap as equal.
    HPX_CXX_CORE_EXPORT struct summary
    {
        std::size_t count = 0;
        double min = 0.0;
        double q1 = 0.0;
        double median = 0.0;
        double q3 = 0.0;
        double p90 = 0.0;
        double max = 0.0;
    };

    /// Summarize the given values (in any order); all fields are 0 for an
    /// empty series.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT summary summarize(
        std::vector<double> values);
}    // namespace hpx::tuning
