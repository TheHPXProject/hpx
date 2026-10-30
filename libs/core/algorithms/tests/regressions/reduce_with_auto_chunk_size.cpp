//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Algorithms that combine per-chunk results failed to compile with executor
// parameters that invoke a testing function (auto_chunk_size,
// persistent_auto_chunk_size) if the executor's bulk_async_execute returns a
// single future holding all chunk results (like the parallel_executor does).

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

constexpr std::size_t num_elements = 100007;

template <typename ExPolicy>
void test_reduce(ExPolicy&& policy, std::vector<std::uint64_t> const& v)
{
    std::uint64_t const expected =
        std::accumulate(v.begin(), v.end(), std::uint64_t(0));

    HPX_TEST_EQ(
        hpx::transform_reduce(policy, v.begin(), v.end(), std::uint64_t(0),
            std::plus<>(), [](std::uint64_t x) { return x; }),
        expected);

    HPX_TEST_EQ(
        hpx::reduce(policy, v.begin(), v.end(), std::uint64_t(0)), expected);

    HPX_TEST_EQ(hpx::count_if(policy, v.begin(), v.end(),
                    [](std::uint64_t x) { return x % 2 == 0; }),
        static_cast<std::ptrdiff_t>((num_elements + 1) / 2));

    HPX_TEST(hpx::all_of(policy, v.begin(), v.end(),
        [](std::uint64_t x) { return x < num_elements; }));
    HPX_TEST(!hpx::all_of(policy, v.begin(), v.end(),
        [](std::uint64_t x) { return x + 1 < num_elements; }));

    HPX_TEST(hpx::is_sorted(policy, v.begin(), v.end()));
    HPX_TEST(hpx::equal(policy, v.begin(), v.end(), v.begin()));

    auto const [min, max] = hpx::minmax_element(policy, v.begin(), v.end());
    HPX_TEST(min == v.begin());
    HPX_TEST(max == v.end() - 1);
}

template <typename ExPolicy>
void test_reduce_async(ExPolicy&& policy, std::vector<std::uint64_t> const& v)
{
    std::uint64_t const expected =
        std::accumulate(v.begin(), v.end(), std::uint64_t(0));

    HPX_TEST_EQ(
        hpx::transform_reduce(policy, v.begin(), v.end(), std::uint64_t(0),
            std::plus<>(), [](std::uint64_t x) { return x; })
            .get(),
        expected);

    HPX_TEST_EQ(hpx::reduce(policy, v.begin(), v.end(), std::uint64_t(0)).get(),
        expected);

    HPX_TEST_EQ(hpx::count_if(policy, v.begin(), v.end(),
                    [](std::uint64_t x) { return x % 2 == 0; })
                    .get(),
        static_cast<std::ptrdiff_t>((num_elements + 1) / 2));

    HPX_TEST(hpx::all_of(policy, v.begin(), v.end(), [](std::uint64_t x) {
        return x < num_elements;
    }).get());

    HPX_TEST(hpx::is_sorted(policy, v.begin(), v.end()).get());
    HPX_TEST(hpx::equal(policy, v.begin(), v.end(), v.begin()).get());

    auto const [min, max] =
        hpx::minmax_element(policy, v.begin(), v.end()).get();
    HPX_TEST(min == v.begin());
    HPX_TEST(max == v.end() - 1);
}

// the last element is processed by the chunks that run after measuring
template <typename ExPolicy>
void test_reduce_exception(
    ExPolicy&& policy, std::vector<std::uint64_t> const& v)
{
    bool caught_exception = false;
    try
    {
        auto result = hpx::transform_reduce(policy, v.begin(), v.end(),
            std::uint64_t(0), std::plus<>(), [](std::uint64_t x) {
                if (x + 1 == num_elements)
                    throw std::runtime_error("test");
                return x;
            });

        if constexpr (hpx::traits::is_future_v<decltype(result)>)
        {
            result.get();
        }
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(e.size(), static_cast<std::size_t>(1));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(caught_exception);
}

int hpx_main()
{
    std::vector<std::uint64_t> v(num_elements);
    std::iota(v.begin(), v.end(), std::uint64_t(0));

    using hpx::execution::par;
    using hpx::execution::task;
    using hpx::execution::experimental::auto_chunk_size;
    using hpx::execution::experimental::persistent_auto_chunk_size;

    // use a short measuring period to make sure that there are chunks left
    // to execute after measuring
    std::chrono::microseconds const period(1);

    test_reduce(par.with(auto_chunk_size()), v);
    test_reduce(par.with(auto_chunk_size(period)), v);
    test_reduce(par.with(persistent_auto_chunk_size()), v);

    test_reduce_async(par(task).with(auto_chunk_size()), v);
    test_reduce_async(par(task).with(auto_chunk_size(period)), v);
    test_reduce_async(par(task).with(persistent_auto_chunk_size()), v);

    test_reduce_exception(par.with(auto_chunk_size(period)), v);
    test_reduce_exception(par(task).with(auto_chunk_size(period)), v);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
