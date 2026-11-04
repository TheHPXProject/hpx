//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

namespace {

    std::mt19937_64 gen;

    // Large enough for the parallel path to split the work.
    constexpr std::size_t test_size = std::size_t(1) << 18;

    template <typename T>
    std::vector<T> random_vec(std::size_t n)
    {
        std::uniform_int_distribution<std::uint64_t> dist;
        std::vector<T> v(n);
        for (auto& x : v)
            x = static_cast<T>(dist(gen));
        return v;
    }

    // False for an empty pack, i.e. the overloads without a policy.
    template <typename... Policy>
    constexpr bool is_async_v =
        (hpx::is_async_execution_policy_v<std::decay_t<Policy>> || ...);

    template <typename T>
    std::vector<T> sorted_copy(std::vector<T> v)
    {
        std::sort(v.begin(), v.end());
        return v;
    }

    // Iterator and sentinel: the sentinel stops at the first element equal to
    // the maximum value, which is placed at the end of the input.
    template <typename T, typename... Policy>
    void test_sentinel(Policy&&... policy)
    {
        T const max_value = (std::numeric_limits<T>::max)();
        auto v = random_vec<T>(test_size);
        for (auto& x : v)
        {
            if (x == max_value)
                x = 0;
        }
        v.back() = max_value;

        auto expected = v;
        std::sort(expected.begin(), expected.end() - 1);

        auto result = hpx::ranges::radix_sort(
            policy..., v.begin(), sentinel<T>{max_value});
        if constexpr (is_async_v<Policy...>)
        {
            HPX_TEST(result.get() == v.end() - 1);
        }
        else
        {
            HPX_TEST(result == v.end() - 1);
        }
        HPX_TEST(v == expected);
    }

    template <typename T>
    void test_sentinel_all()
    {
        test_sentinel<T>();
        test_sentinel<T>(hpx::execution::seq);
        test_sentinel<T>(hpx::execution::par);
        test_sentinel<T>(hpx::execution::par(hpx::execution::task));
    }

    template <typename T, typename... Policy>
    void test_range(Policy&&... policy)
    {
        auto v = random_vec<T>(test_size);
        auto const expected = sorted_copy(v);

        auto result = hpx::ranges::radix_sort(policy..., v);
        if constexpr (is_async_v<Policy...>)
        {
            HPX_TEST(result.get() == v.end());
        }
        else
        {
            HPX_TEST(result == v.end());
        }
        HPX_TEST(v == expected);
    }

    template <typename T>
    void test_range_all()
    {
        test_range<T>();
        test_range<T>(hpx::execution::seq);
        test_range<T>(hpx::execution::par);
        test_range<T>(hpx::execution::par(hpx::execution::task));
    }

    void test_projection()
    {
        struct item
        {
            std::int64_t key;
            std::size_t index;
        };

        std::vector<item> v(test_size);
        for (std::size_t i = 0; i != v.size(); ++i)
            v[i] = item{static_cast<std::int64_t>(gen()), i};

        auto const key = [](item const& x) { return x.key; };
        hpx::ranges::radix_sort(hpx::execution::par, v, key);

        bool sorted = true;
        for (std::size_t i = 1; i < v.size(); ++i)
            sorted = sorted && v[i - 1].key <= v[i].key;
        HPX_TEST(sorted);

        std::vector<std::size_t> indices(v.size());
        for (std::size_t i = 0; i != v.size(); ++i)
            indices[i] = v[i].index;
        std::sort(indices.begin(), indices.end());
        bool complete = true;
        for (std::size_t i = 0; i != indices.size(); ++i)
            complete = complete && indices[i] == i;
        HPX_TEST(complete);
    }

    void test_empty()
    {
        std::vector<int> v;
        HPX_TEST(hpx::ranges::radix_sort(v) == v.end());
        HPX_TEST(hpx::ranges::radix_sort(hpx::execution::par, v) == v.end());
    }
}    // namespace

// Only integral, non-bool keys are accepted.
static_assert(std::is_invocable_v<hpx::ranges::radix_sort_t,
    std::vector<std::uint32_t>&>);
static_assert(
    !std::is_invocable_v<hpx::ranges::radix_sort_t, std::vector<bool>&>);
static_assert(
    !std::is_invocable_v<hpx::ranges::radix_sort_t, std::vector<double>&>);

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    test_sentinel_all<std::uint32_t>();
    test_sentinel_all<std::int64_t>();
    test_range_all<std::uint32_t>();
    test_range_all<std::int16_t>();
    test_range_all<std::uint64_t>();
    test_projection();
    test_empty();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
