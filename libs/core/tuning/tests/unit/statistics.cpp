//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <cstddef>
#include <vector>

void test_empty()
{
    hpx::tuning::summary const s = hpx::tuning::summarize({});
    HPX_TEST_EQ(s.count, std::size_t(0));
    HPX_TEST_EQ(s.median, 0.0);
    HPX_TEST_EQ(s.max, 0.0);
}

void test_single()
{
    hpx::tuning::summary const s = hpx::tuning::summarize({3.0});
    HPX_TEST_EQ(s.count, std::size_t(1));
    HPX_TEST_EQ(s.min, 3.0);
    HPX_TEST_EQ(s.q1, 3.0);
    HPX_TEST_EQ(s.median, 3.0);
    HPX_TEST_EQ(s.p90, 3.0);
    HPX_TEST_EQ(s.max, 3.0);
}

void test_quantiles()
{
    // unsorted on purpose; 1 to 5: quartiles 2 and 4, median 3, p90 4.6
    hpx::tuning::summary const s = hpx::tuning::summarize({5, 1, 4, 2, 3});
    HPX_TEST_EQ(s.count, std::size_t(5));
    HPX_TEST_EQ(s.min, 1.0);
    HPX_TEST_EQ(s.q1, 2.0);
    HPX_TEST_EQ(s.median, 3.0);
    HPX_TEST_EQ(s.q3, 4.0);
    HPX_TEST_LT(s.p90 - 4.6, 1e-9);
    HPX_TEST_LT(4.6 - s.p90, 1e-9);
    HPX_TEST_EQ(s.max, 5.0);

    // an even count interpolates the median
    HPX_TEST_EQ(hpx::tuning::summarize({1, 2, 3, 4}).median, 2.5);
}

int main()
{
    test_empty();
    test_single();
    test_quantiles();
    return hpx::util::report_errors();
}
