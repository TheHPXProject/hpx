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
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

static_assert(hpx::parallel::detail::transform_reduce_chunk_size<std::int8_t,
                  int, int>() == (std::numeric_limits<std::int8_t>::max)());
static_assert(hpx::parallel::detail::transform_reduce_chunk_size<std::int16_t,
                  int, int>() == (std::numeric_limits<std::int16_t>::max)());
static_assert(hpx::parallel::detail::transform_reduce_chunk_size<std::int64_t,
                  int, int>() == 32 * 1024 * 1024);

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

template <typename Iterator1, typename Iterator2>
void check_binary_subrange(
    Iterator1 first, Iterator1 last, Iterator2 second, int expected)
{
    using namespace hpx::execution;

    auto reduce = [&](auto policy) {
        return hpx::transform_reduce(policy, first, last, second, 7,
            std::plus<int>(), std::multiplies<int>());
    };

    HPX_TEST_EQ(hpx::transform_reduce(first, last, second, 7, std::plus<int>(),
                    std::multiplies<int>()),
        expected);
    HPX_TEST_EQ(reduce(seq), expected);
    HPX_TEST_EQ(reduce(par), expected);
    HPX_TEST_EQ(reduce(seq(task)).get(), expected);
    HPX_TEST_EQ(reduce(par(task)).get(), expected);
}

void test_binary_partition_layouts(std::vector<std::size_t> const& first_sizes,
    std::vector<std::size_t> const& second_sizes,
    hpx::id_type const& first_owner, hpx::id_type const& second_owner)
{
    hpx::partitioned_vector<int> first(6,
        hpx::explicit_container_layout(first_sizes,
            std::vector<hpx::id_type>(first_sizes.size(), first_owner)));
    hpx::partitioned_vector<int> second(6,
        hpx::explicit_container_layout(second_sizes,
            std::vector<hpx::id_type>(second_sizes.size(), second_owner)));

    for (std::size_t i = 0; i != 6; ++i)
    {
        first[i] = static_cast<int>(i + 1);
        second[i] = static_cast<int>(i + 2);
    }

    auto first_begin = first.begin();
    auto second_begin = second.begin();
    check_binary_subrange(first_begin, first.end(), second_begin, 119);
    check_binary_subrange(first_begin + 1, first_begin + 5, second_begin, 61);
    check_binary_subrange(
        first_begin + 2, first_begin + 5, second_begin + 1, 57);
    check_binary_subrange(first_begin, first_begin + 2, second_begin + 1, 18);
    check_binary_subrange(first_begin, first_begin + 1, second_begin, 9);
    check_binary_subrange(first_begin, first_begin, second.end(), 7);
}

void transform_reduce_binary_regressions(
    std::vector<hpx::id_type> const& localities)
{
    auto check_owners = [](hpx::id_type const& first,
                            hpx::id_type const& second) {
        test_binary_partition_layouts({2, 4}, {4, 2}, first, second);
        test_binary_partition_layouts({4, 2}, {2, 4}, first, second);
        test_binary_partition_layouts({2, 0, 4}, {4, 0, 2}, first, second);
        test_binary_partition_layouts({4, 0, 2}, {2, 0, 4}, first, second);
    };

    check_owners(hpx::find_here(), hpx::find_here());
    if (localities.size() > 1)
    {
        check_owners(localities[0], localities[1]);
        check_owners(localities[1], localities[0]);
    }
}

void transform_reduce_binary_multiple_exceptions()
{
    std::vector<hpx::id_type> owners(2, hpx::find_here());
    hpx::partitioned_vector<int> first(
        2, 1, hpx::explicit_container_layout({1, 1}, owners));
    hpx::partitioned_vector<int> second(
        2, 1, hpx::explicit_container_layout({1, 1}, owners));

    auto check = [&](auto policy) {
        try
        {
            auto operation = hpx::transform_reduce(policy, first.begin(),
                first.end(), second.begin(), 0, std::plus<int>(),
                [](int, int) -> int { throw std::runtime_error("failure"); });
            if constexpr (hpx::is_async_execution_policy_v<decltype(policy)>)
                operation.get();
            HPX_TEST(false);
        }
        catch (hpx::exception_list const& errors)
        {
            HPX_TEST_EQ(errors.size(), std::size_t(2));
        }
    };

    check(hpx::execution::par);
    check(hpx::execution::par(hpx::execution::task));
}

hpx::threads::thread_id_type expected_reduction_thread;

void transform_reduce_binary_sequential_thread()
{
    std::vector<hpx::id_type> owners(1, hpx::find_here());
    hpx::partitioned_vector<int> first(
        1, 2, hpx::explicit_container_layout({1}, owners));
    hpx::partitioned_vector<int> second(
        1, 3, hpx::explicit_container_layout({1}, owners));
    expected_reduction_thread = hpx::threads::get_self_id();
    auto reduce = [](int left, int right) {
        if (left == 1000)
            HPX_TEST(hpx::threads::get_self_id() == expected_reduction_thread);
        return left + right;
    };

    HPX_TEST_EQ(
        hpx::transform_reduce(hpx::execution::seq, first.begin(), first.end(),
            second.begin(), 1000, reduce, std::multiplies<int>()),
        1006);
    HPX_TEST_EQ(hpx::transform_reduce(first.begin(), first.end(),
                    second.begin(), 1000, reduce, std::multiplies<int>()),
        1006);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    transform_reduce_binary_tests<int>(localities);
    transform_reduce_binary_regressions(localities);
    transform_reduce_binary_multiple_exceptions();
    transform_reduce_binary_sequential_thread();
    return hpx::util::report_errors();
}
#endif
