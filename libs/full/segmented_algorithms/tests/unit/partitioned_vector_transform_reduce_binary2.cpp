//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct multiply
{
    template <typename T>
    typename std::decay<T>::type operator()(hpx::tuple<T, T> const& r) const
    {
        using hpx::get;
        return get<0>(r) * get<1>(r);
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename T>
T test_transform_reduce_binary(ExPolicy&& policy,
    hpx::partitioned_vector<T> const& xvalues,
    hpx::partitioned_vector<T> const& yvalues)
{
    return hpx::transform_reduce(policy, xvalues.begin(), xvalues.end(),
        yvalues.begin(), T(1), std::plus<T>(), std::multiplies<T>());
}

template <typename ExPolicy, typename T>
hpx::future<T> test_transform_reduce_binary_async(ExPolicy&& policy,
    hpx::partitioned_vector<T>& xvalues, hpx::partitioned_vector<T>& yvalues)
{
    return hpx::transform_reduce(policy, xvalues.begin(), xvalues.end(),
        yvalues.begin(), T(1), std::plus<T>(), std::multiplies<T>());
}

template <typename T>
void transform_reduce_binary_tests(std::size_t num,
    hpx::partitioned_vector<T>& xvalues, hpx::partitioned_vector<T>& yvalues)
{
    HPX_TEST_EQ(
        test_transform_reduce_binary(hpx::execution::seq, xvalues, yvalues),
        T(num + 1));
    HPX_TEST_EQ(
        test_transform_reduce_binary(hpx::execution::par, xvalues, yvalues),
        T(num + 1));

    HPX_TEST_EQ(test_transform_reduce_binary_async(
                    hpx::execution::seq(hpx::execution::task), xvalues, yvalues)
                    .get(),
        T(num + 1));
    HPX_TEST_EQ(test_transform_reduce_binary_async(
                    hpx::execution::par(hpx::execution::task), xvalues, yvalues)
                    .get(),
        T(num + 1));
}

template <typename T>
void transform_reduce_binary_tests(std::vector<hpx::id_type>& localities)
{
    std::size_t const num = 10007;
    {
        hpx::partitioned_vector<T> xvalues(num, T(1));
        hpx::partitioned_vector<T> yvalues(num, T(1));
        transform_reduce_binary_tests(num, xvalues, yvalues);
    }

    {
        hpx::partitioned_vector<T> xvalues(
            num, T(1), hpx::container_layout(localities));
        hpx::partitioned_vector<T> yvalues(
            num, T(1), hpx::container_layout(localities));
        transform_reduce_binary_tests(num, xvalues, yvalues);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    transform_reduce_binary_tests<double>(localities);
    return hpx::util::report_errors();
}
#endif
