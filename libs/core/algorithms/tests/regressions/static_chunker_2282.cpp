//  Copyright (c) 2016 Marcin Copik
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

// An algorithm that is called while the HPX runtime is not running has to
// throw, also when the executor parameters ask the executor for the number of
// cores (which is where the exception comes from).
template <typename Policy>
void test_fill(Policy&& policy, float* a, int const size)
{
    bool caught_exception = false;
    try
    {
        // this should throw as the HPX runtime has not been initialized
        hpx::fill(policy, a, a + size, 1.0f);

        // fill should have thrown
        HPX_TEST(false);
    }
    catch (hpx::exception const&)
    {
        caught_exception = true;
    }

    HPX_TEST(caught_exception);
}

int main()
{
    int const size = 1000000;
    float* a = new float[size];

    test_fill(hpx::execution::par, a, size);
    test_fill(
        hpx::execution::par.with(hpx::execution::experimental::num_cores(2)), a,
        size);
    test_fill(hpx::execution::par.with(
                  hpx::execution::experimental::task_size_chunk_size()),
        a, size);

    delete[] a;

    return 0;
}
