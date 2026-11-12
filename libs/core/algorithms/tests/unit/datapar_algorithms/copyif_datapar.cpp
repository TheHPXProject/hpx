//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "../algorithms/test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename ExPolicy, typename IteratorTag>
void test_copy_if_iter(ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, 0);
    std::fill(middle, std::end(c), -1);

    auto pred = [](auto const& i) { return !(i < 0); };

    auto dest_end = hpx::copy_if(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), pred);

    std::size_t count = std::distance(std::begin(d), dest_end);
    HPX_TEST_EQ(count, static_cast<std::size_t>(c.size() / 2));

    std::vector<int> expected(c.size());
    auto std_end = std::copy_if(std::begin(c), std::end(c),
        std::begin(expected), [](int i) { return !(i < 0); });
    std::size_t exp_count = std::distance(std::begin(expected), std_end);
    HPX_TEST_EQ(count, exp_count);
    HPX_TEST(std::equal(std::begin(d), dest_end, std::begin(expected)));
}

template <typename ExPolicy, typename IteratorTag>
void test_copy_if_iter_async(ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);
    std::iota(std::begin(c), middle, 0);
    std::fill(middle, std::end(c), -1);

    auto pred = [](auto const& i) { return !(i < 0); };

    auto f = hpx::copy_if(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), pred);
    f.wait();

    auto dest_end = f.get();
    std::size_t count = std::distance(std::begin(d), dest_end);
    HPX_TEST_EQ(count, static_cast<std::size_t>(c.size() / 2));

    std::vector<int> expected(c.size());
    auto std_end = std::copy_if(std::begin(c), std::end(c),
        std::begin(expected), [](int i) { return !(i < 0); });
    std::size_t exp_count = std::distance(std::begin(expected), std_end);
    HPX_TEST_EQ(count, exp_count);
    HPX_TEST(std::equal(std::begin(d), dest_end, std::begin(expected)));
}

template <typename IteratorTag>
void test_copy_if_tags()
{
    using namespace hpx::execution;

    test_copy_if_iter(simd, IteratorTag());
    test_copy_if_iter(par_simd, IteratorTag());
    test_copy_if_iter_async(simd(task), IteratorTag());
    test_copy_if_iter_async(par_simd(task), IteratorTag());
}

void copy_if_test()
{
    test_copy_if_tags<std::random_access_iterator_tag>();
    test_copy_if_tags<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename In, typename Out, typename Pred>
auto copy_if_test_invoke(
    ExPolicy&& policy, In first, In last, Out dest, Pred&& pred)
{
    if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
    {
        return hpx::copy_if(policy, first, last, dest, HPX_FORWARD(Pred, pred))
            .get();
    }
    else
    {
        return hpx::copy_if(policy, first, last, dest, HPX_FORWARD(Pred, pred));
    }
}

template <typename ExPolicy, typename Rng, typename Out, typename Pred>
auto copy_if_ranges_test_invoke(
    ExPolicy&& policy, Rng&& rng, Out dest, Pred&& pred)
{
    if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
    {
        return hpx::ranges::copy_if(
            policy, HPX_FORWARD(Rng, rng), dest, HPX_FORWARD(Pred, pred))
            .get();
    }
    else
    {
        return hpx::ranges::copy_if(
            policy, HPX_FORWARD(Rng, rng), dest, HPX_FORWARD(Pred, pred));
    }
}

template <typename ExPolicy>
void test_copy_if_contiguous(ExPolicy policy)
{
    std::vector<std::size_t> const sizes = {0, 1, 2, 7, 16, 31, 64, 100, 10007};

    for (std::size_t sz : sizes)
    {
        // 1. All match
        {
            std::vector<int> c(sz, 5);
            std::vector<int> d(sz, 0);
            auto pred = [](auto const& x) { return x > 0; };

            auto res = copy_if_test_invoke(
                policy, c.begin(), c.end(), d.begin(), pred);
            HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), sz);
            HPX_TEST(std::equal(c.begin(), c.end(), d.begin()));

            std::vector<int> d_rng(sz, 0);
            auto res_rng =
                copy_if_ranges_test_invoke(policy, c, d_rng.begin(), pred);
            HPX_TEST(res_rng.in == c.end());
            HPX_TEST_EQ(
                static_cast<std::size_t>(res_rng.out - d_rng.begin()), sz);
            HPX_TEST(std::equal(c.begin(), c.end(), d_rng.begin()));
        }

        // 2. None match
        {
            std::vector<int> c(sz, -5);
            std::vector<int> d(sz, 0);
            auto pred = [](auto const& x) { return x > 0; };

            auto res = copy_if_test_invoke(
                policy, c.begin(), c.end(), d.begin(), pred);
            HPX_TEST_EQ(
                static_cast<std::size_t>(res - d.begin()), std::size_t(0));

            std::vector<int> d_rng(sz, 0);
            auto res_rng =
                copy_if_ranges_test_invoke(policy, c, d_rng.begin(), pred);
            HPX_TEST(res_rng.in == c.end());
            HPX_TEST_EQ(static_cast<std::size_t>(res_rng.out - d_rng.begin()),
                std::size_t(0));
        }

        // 3. Alternate match
        {
            std::vector<int> c(sz);
            for (std::size_t i = 0; i != sz; ++i)
                c[i] = (i % 2 == 0) ? static_cast<int>(i) : -1;

            std::vector<int> d(sz, 0);
            std::vector<int> expected(sz, 0);
            auto pred = [](auto const& x) { return x >= 0; };

            auto exp_end = std::copy_if(c.begin(), c.end(), expected.begin(),
                [](int x) { return x >= 0; });
            std::size_t exp_count = std::distance(expected.begin(), exp_end);

            auto res = copy_if_test_invoke(
                policy, c.begin(), c.end(), d.begin(), pred);
            HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), exp_count);
            HPX_TEST(std::equal(d.begin(), res, expected.begin()));

            std::vector<int> d_rng(sz, 0);
            auto res_rng =
                copy_if_ranges_test_invoke(policy, c, d_rng.begin(), pred);
            HPX_TEST(res_rng.in == c.end());
            HPX_TEST_EQ(static_cast<std::size_t>(res_rng.out - d_rng.begin()),
                exp_count);
            HPX_TEST(std::equal(d_rng.begin(), res_rng.out, expected.begin()));
        }

        // 4. Random data
        {
            std::uniform_int_distribution<> dis(-1000, 1000);
            std::vector<int> c(sz);
            std::generate(c.begin(), c.end(), [&]() { return dis(gen); });

            std::vector<int> d(sz, 0);
            std::vector<int> expected(sz, 0);
            auto pred = [](auto const& x) { return x > 0; };

            auto exp_end = std::copy_if(c.begin(), c.end(), expected.begin(),
                [](int x) { return x > 0; });
            std::size_t exp_count = std::distance(expected.begin(), exp_end);

            auto res = copy_if_test_invoke(
                policy, c.begin(), c.end(), d.begin(), pred);
            HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), exp_count);
            HPX_TEST(std::equal(d.begin(), res, expected.begin()));

            std::vector<int> d_rng(sz, 0);
            auto res_rng =
                copy_if_ranges_test_invoke(policy, c, d_rng.begin(), pred);
            HPX_TEST(res_rng.in == c.end());
            HPX_TEST_EQ(static_cast<std::size_t>(res_rng.out - d_rng.begin()),
                exp_count);
            HPX_TEST(std::equal(d_rng.begin(), res_rng.out, expected.begin()));
        }
    }
}

template <typename ExPolicy>
void test_copy_if_boundaries(ExPolicy policy)
{
    using policy_t = std::decay_t<ExPolicy>;
    constexpr std::size_t lanes =
        hpx::execution::policy_traits<policy_t>::num_lanes;
    using vector_type = hpx::parallel::traits::vector_pack_type_t<int, lanes>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<vector_type>;

    std::vector<std::size_t> const test_sizes = {0, 1, 2,
        pack_size > 1 ? pack_size - 1 : 1, pack_size, pack_size + 1,
        pack_size * 2 - 1, pack_size * 2, pack_size * 2 + 1, pack_size * 3 + 5,
        pack_size * 10 + 7};

    for (std::size_t sz : test_sizes)
    {
        std::uniform_int_distribution<> dis(-50, 50);
        std::vector<int> c(sz);
        std::generate(c.begin(), c.end(), [&]() { return dis(gen); });

        std::vector<int> d(sz, 0);
        std::vector<int> expected(sz, 0);
        auto pred = [](auto const& x) { return x >= 0; };

        auto exp_end = std::copy_if(
            c.begin(), c.end(), expected.begin(), [](int x) { return x >= 0; });
        std::size_t exp_count = std::distance(expected.begin(), exp_end);

        auto res =
            copy_if_test_invoke(policy, c.begin(), c.end(), d.begin(), pred);
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), exp_count);
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    }
}

template <typename ExPolicy>
void test_copy_if_uses_simd_path(ExPolicy policy)
{
    using policy_t = std::decay_t<ExPolicy>;
    constexpr std::size_t lanes =
        hpx::execution::policy_traits<policy_t>::num_lanes;
    using vector_type = hpx::parallel::traits::vector_pack_type_t<int, lanes>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<vector_type>;

    if constexpr (pack_size > 1)
    {
        std::atomic<int> pack_calls(0);
        auto pred = [&pack_calls](auto const& x) {
            if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(x)>>)
            {
                ++pack_calls;
            }
            return x > 0;
        };

        std::size_t sz = (std::max) (std::size_t(1000), pack_size * 100 + 3);
        std::vector<int> c(sz);
        for (std::size_t i = 0; i != sz; ++i)
            c[i] = (i % 3 == 0) ? 10 : -10;

        std::vector<int> d(sz, 0);
        std::vector<int> expected(sz, 0);

        auto exp_end = std::copy_if(
            c.begin(), c.end(), expected.begin(), [](int x) { return x > 0; });
        std::size_t exp_count = std::distance(expected.begin(), exp_end);

        auto res =
            copy_if_test_invoke(policy, c.begin(), c.end(), d.begin(), pred);
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), exp_count);
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
        HPX_TEST(pack_calls > 0);

        std::atomic<int> ranges_pack_calls(0);
        auto ranges_pred = [&ranges_pack_calls](auto const& x) {
            if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(x)>>)
            {
                ++ranges_pack_calls;
            }
            return x > 0;
        };

        std::vector<int> d_rng(sz, 0);
        auto res_rng =
            copy_if_ranges_test_invoke(policy, c, d_rng.begin(), ranges_pred);
        HPX_TEST_EQ(
            static_cast<std::size_t>(res_rng.out - d_rng.begin()), exp_count);
        HPX_TEST(std::equal(d_rng.begin(), res_rng.out, expected.begin()));
        HPX_TEST(ranges_pack_calls > 0);
    }
}

void test_copy_if_non_contiguous()
{
    using namespace hpx::execution;

    std::deque<int> c = {1, -2, 3, 4, -5, 6, -7, 8};
    std::vector<int> expected = {1, 3, 4, 6, 8};

    auto run_test = [&](auto&& pol) {
        std::vector<int> d(c.size(), 0);
        auto res = copy_if_test_invoke(pol, c.begin(), c.end(), d.begin(),
            [](auto const& x) { return x > 0; });
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), expected.size());
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    };

    run_test(simd);
    run_test(par_simd);
    run_test(simd(task));
    run_test(par_simd(task));
}

void test_copy_if_bool_returning_predicate()
{
    using namespace hpx::execution;

    std::vector<int> c = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> expected = {2, 4, 6, 8, 10};

    auto run_test = [&](auto&& pol) {
        std::vector<int> d(c.size(), 0);
        auto res = copy_if_test_invoke(pol, c.begin(), c.end(), d.begin(),
            [](int x) -> bool { return x % 2 == 0; });
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), expected.size());
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    };

    run_test(simd);
    run_test(par_simd);
    run_test(simd(task));
    run_test(par_simd(task));
}

void test_copy_if_bool_convertible_predicate()
{
    using namespace hpx::execution;

    struct convertible_to_bool
    {
        bool val;
        constexpr explicit operator bool() const noexcept
        {
            return val;
        }
    };

    std::vector<int> c = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> expected = {2, 4, 6, 8, 10};

    auto run_test = [&](auto&& pol) {
        std::vector<int> d(c.size(), 0);
        auto res = copy_if_test_invoke(pol, c.begin(), c.end(), d.begin(),
            [](int x) -> convertible_to_bool { return {x % 2 == 0}; });
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), expected.size());
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    };

    run_test(simd);
    run_test(par_simd);
    run_test(simd(task));
    run_test(par_simd(task));
}

void test_copy_if_bool_contiguous()
{
    using namespace hpx::execution;

    std::array<bool, 6> c = {{true, false, true, true, false, true}};
    std::vector<bool> expected = {true, true, true, true};

    auto run_test = [&](auto&& pol) {
        std::vector<bool> d(c.size());
        auto res = copy_if_test_invoke(
            pol, c.begin(), c.end(), d.begin(), [](bool x) { return x; });
        std::size_t count = std::distance(d.begin(), res);
        HPX_TEST_EQ(count, expected.size());
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    };

    run_test(simd);
    run_test(par_simd);
    run_test(simd(task));
    run_test(par_simd(task));
}

///////////////////////////////////////////////////////////////////////////////
struct user_payload
{
    int val;

    bool operator==(user_payload const& other) const noexcept
    {
        return val == other.val;
    }
};

void test_copy_if_user_type()
{
    using namespace hpx::execution;

    std::vector<user_payload> c = {{1}, {-2}, {3}, {4}, {-5}, {6}, {-7}, {8}};
    std::vector<user_payload> expected = {{1}, {3}, {4}, {6}, {8}};

    auto run_test = [&](auto&& pol) {
        std::vector<user_payload> d(c.size());
        auto res = copy_if_test_invoke(pol, c.begin(), c.end(), d.begin(),
            [](user_payload const& x) { return x.val > 0; });
        HPX_TEST_EQ(static_cast<std::size_t>(res - d.begin()), expected.size());
        HPX_TEST(std::equal(d.begin(), res, expected.begin()));
    };

    run_test(simd);
    run_test(par_simd);
    run_test(simd(task));
    run_test(par_simd(task));
}

template <typename ExPolicy>
void test_copy_if_projections(ExPolicy policy)
{
    using policy_t = std::decay_t<ExPolicy>;
    constexpr std::size_t lanes =
        hpx::execution::policy_traits<policy_t>::num_lanes;
    using vector_type = hpx::parallel::traits::vector_pack_type_t<int, lanes>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<vector_type>;

    std::size_t sz = (std::max) (std::size_t(1000), pack_size * 100 + 7);
    std::vector<int> c(sz);
    for (std::size_t i = 0; i != sz; ++i)
        c[i] = (i % 2 == 0) ? -static_cast<int>(i) : static_cast<int>(i);

    // Projection negates the value: proj(x) = -x.
    // pred checks -x > 0 (meaning original x < 0).
    std::atomic<int> pack_calls(0);
    auto proj = [&pack_calls](auto const& x) {
        if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(x)>>)
        {
            ++pack_calls;
        }
        return -x;
    };
    auto pred = [](auto const& x) { return x > 0; };

    std::vector<int> d(sz, 0);
    auto res = hpx::ranges::copy_if(policy, c, d.begin(), pred, proj);
    if constexpr (hpx::is_async_execution_policy_v<policy_t>)
    {
        auto actual = res.get();
        HPX_TEST(actual.in == c.end());
    }
    else
    {
        HPX_TEST(res.in == c.end());
    }

    if constexpr (pack_size > 1)
    {
        HPX_TEST(pack_calls > 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    using namespace hpx::execution;

    copy_if_test();

    test_copy_if_contiguous(simd);
    test_copy_if_contiguous(par_simd);
    test_copy_if_contiguous(simd(task));
    test_copy_if_contiguous(par_simd(task));

    test_copy_if_boundaries(simd);
    test_copy_if_boundaries(par_simd);
    test_copy_if_boundaries(simd(task));
    test_copy_if_boundaries(par_simd(task));

    test_copy_if_uses_simd_path(simd);
    test_copy_if_uses_simd_path(par_simd);
    test_copy_if_uses_simd_path(simd(task));
    test_copy_if_uses_simd_path(par_simd(task));

    test_copy_if_non_contiguous();
    test_copy_if_bool_returning_predicate();
    test_copy_if_bool_convertible_predicate();
    test_copy_if_bool_contiguous();
    test_copy_if_user_type();

    test_copy_if_projections(simd);
    test_copy_if_projections(par_simd);
    test_copy_if_projections(simd(task));
    test_copy_if_projections(par_simd(task));

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
