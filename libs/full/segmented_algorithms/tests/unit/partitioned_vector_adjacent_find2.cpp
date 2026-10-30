//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompadjacent_finding
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_adjacent_find.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define TEST_SIZE 64

struct pred
{
    template <typename T>
    bool operator()(T const& prev, T const& curr) const
    {
        return curr < prev;
    }
};

template <typename T>
void initialize(hpx::partitioned_vector<T>& xvalues)
{
    T init_array[TEST_SIZE] = {1, 2, 3, 4, 5, 1, 2, 3, 1, 5, 2, 3, 4, 2, 3, 2,
        1, 2, 3, 4, 5, 6, 5, 6, 1, 2, 3, 4, 2, 1, 2, 3, 3, 5, 4, 3, 2, 1, 1, 2,
        3, 4, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 7, 6, 5, 7, 5, 4, 2, 3, 4, 5, 2};
    typename hpx::partitioned_vector<T>::iterator it = xvalues.begin();
    for (int i = 0; i < TEST_SIZE; i++, it++)
    {
        *it = init_array[i];
    }
}

template <typename ExPolicy, typename T>
void test_adjacent_find(ExPolicy&& policy, hpx::partitioned_vector<T>& xvalues)
{
    auto result = hpx::adjacent_find(policy, xvalues.begin(), xvalues.end());
    HPX_TEST_EQ(std::distance(xvalues.begin(), result), 31);

    result = hpx::adjacent_find(policy, xvalues.begin(), xvalues.end(), pred());
    HPX_TEST_EQ(std::distance(xvalues.begin(), result), 4);
}

template <typename ExPolicy, typename T>
void test_adjacent_find_async(
    ExPolicy&& policy, hpx::partitioned_vector<T>& xvalues)
{
    auto result =
        hpx::adjacent_find(policy, xvalues.begin(), xvalues.end()).get();
    HPX_TEST_EQ(std::distance(xvalues.begin(), result), 31);

    result = hpx::adjacent_find(policy, xvalues.begin(), xvalues.end(), pred())
                 .get();
    HPX_TEST_EQ(std::distance(xvalues.begin(), result), 4);
}

template <typename T>
void adjacent_find_tests(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> xvalues(
        TEST_SIZE, T(0), hpx::container_layout(localities));
    initialize(xvalues);

    test_adjacent_find(hpx::execution::seq, xvalues);
    test_adjacent_find(hpx::execution::par, xvalues);
    test_adjacent_find_async(
        hpx::execution::seq(hpx::execution::task), xvalues);
    test_adjacent_find_async(
        hpx::execution::par(hpx::execution::task), xvalues);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    adjacent_find_tests<double>(localities);
    return hpx::util::report_errors();
}
#endif
