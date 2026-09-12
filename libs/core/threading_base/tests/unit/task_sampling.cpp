//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit test for the 1-in-N task-sampling countdown. The production entry
// point (should_sample_next()) bakes the rate in at build time, so the
// test drives sample_next<N> directly with per-subtest local counters.
// The pattern-locking cases (rate 3, rate 10) would catch a regression
// in the counter arithmetic; the rate-1 case guards the compile-time
// collapse path.

#include <hpx/modules/testing.hpp>
#include <hpx/threading_base/detail/task_sampling.hpp>

int main()
{
    using hpx::threads::detail::sample_next;

    {
        int c = 0;
        for (int i = 0; i != 100; ++i)
            HPX_TEST(sample_next<1>(c));
    }

    {
        int c = 0;
        HPX_TEST(sample_next<3>(c));     // 1
        HPX_TEST(!sample_next<3>(c));    // 2
        HPX_TEST(!sample_next<3>(c));    // 3
        HPX_TEST(sample_next<3>(c));     // 4
        HPX_TEST(!sample_next<3>(c));    // 5
        HPX_TEST(!sample_next<3>(c));    // 6
        HPX_TEST(sample_next<3>(c));     // 7
    }

    {
        int c = 0;
        int emitted = 0;
        for (int i = 0; i != 10000; ++i)
            if (sample_next<10>(c))
                ++emitted;
        HPX_TEST_EQ(emitted, 1000);
    }

    {
        int c = 0;
        int emitted = 0;
        for (int i = 0; i != 100000; ++i)
            if (sample_next<100>(c))
                ++emitted;
        HPX_TEST_EQ(emitted, 1000);
    }

    return hpx::util::report_errors();
}
