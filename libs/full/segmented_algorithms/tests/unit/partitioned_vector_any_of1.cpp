//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Apple Clang and Clang V19.1.1 ICE's while compiling this file
#if !defined(HPX_COMPUTE_DEVICE_CODE) && !defined(HPX_APPLE_CLANG_VERSION) &&  \
    (!defined(HPX_CLANG_VERSION) || HPX_CLANG_VERSION != 190101)

#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_all_any_none_of.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Define the vector types to be used.
HPX_REGISTER_PARTITIONED_VECTOR(int)

///////////////////////////////////////////////////////////////////////////////
#define TEST_SIZE 64

template <typename T>
void initialize(hpx::partitioned_vector<T>& xvalues)
{
    T init_array[TEST_SIZE] = {1, 2, 3, 4, 5, 1, 2, 3, 3, 5, 5, 3, 4, 2, 3, 2,
        1, 2, 3, 4, 5, 6, 5, 6, 1, 2, 3, 4, 1, 1, 2, 3, 4, 5, 4, 3, 2, 1, 1, 2,
        3, 4, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 7, 6, 5, 7, 5, 4, 2, 3, 4, 5, 2};
    typename hpx::partitioned_vector<T>::iterator it = xvalues.begin();
    for (int i = 0; i < TEST_SIZE; i++, it++)
    {
        *it = init_array[i];
    }
}

struct op5
{
    template <typename T>
    bool operator()(T& value) const
    {
        return value > 5;
    }
};

struct op0
{
    template <typename T>
    bool operator()(T& value) const
    {
        return value > 0;
    }
};

struct op8
{
    template <typename T>
    bool operator()(T& value) const
    {
        return value > 8;
    }
};

template <typename ExPolicy, typename T, typename Func>
void test_any(ExPolicy&& policy, hpx::partitioned_vector<T>& xvalues, Func&& f,
    bool expected_result)
{
    bool result = hpx::any_of(policy, xvalues.begin(), xvalues.end(), f);
    HPX_TEST_EQ(result, expected_result);
}

template <typename ExPolicy, typename T, typename Func>
void test_any_async(ExPolicy&& policy, hpx::partitioned_vector<T>& xvalues,
    Func&& f, bool expected_result)
{
    bool result = hpx::any_of(policy, xvalues.begin(), xvalues.end(), f).get();
    HPX_TEST_EQ(result, expected_result);
}

template <typename T>
void any_of_tests(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> xvalues(
        TEST_SIZE, T(0), hpx::container_layout(localities));
    initialize(xvalues);

    test_any(hpx::execution::seq, xvalues, op5(), true);
    test_any(hpx::execution::par, xvalues, op5(), true);
    test_any_async(
        hpx::execution::seq(hpx::execution::task), xvalues, op5(), true);
    test_any_async(
        hpx::execution::par(hpx::execution::task), xvalues, op5(), true);

    test_any(hpx::execution::seq, xvalues, op8(), false);
    test_any(hpx::execution::par, xvalues, op8(), false);
    test_any_async(
        hpx::execution::seq(hpx::execution::task), xvalues, op8(), false);
    test_any_async(
        hpx::execution::par(hpx::execution::task), xvalues, op8(), false);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    any_of_tests<int>(localities);
    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
