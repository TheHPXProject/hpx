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
#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
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
void test_lexicographical_compare1(ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    // d is lexicographically less than c
    std::vector<std::size_t> d(10006);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare1_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10006);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();
    HPX_TEST(!f.get());
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare2(ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare2_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();
    HPX_TEST(!f.get());
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare3(ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);
    std::uniform_int_distribution<> dis(1, 5000);
    c[dis(gen)] = 0;

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare3_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);
    std::uniform_int_distribution<> dis(1, 10006);
    c[dis(gen)] = 0;

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();
    HPX_TEST(f.get());
}

template <typename IteratorTag>
void test_lexicographical_compare()
{
    using namespace hpx::execution;

    test_lexicographical_compare1(simd, IteratorTag());
    test_lexicographical_compare1(par_simd, IteratorTag());
    test_lexicographical_compare1_async(simd(task), IteratorTag());
    test_lexicographical_compare1_async(par_simd(task), IteratorTag());

    test_lexicographical_compare2(simd, IteratorTag());
    test_lexicographical_compare2(par_simd, IteratorTag());
    test_lexicographical_compare2_async(simd(task), IteratorTag());
    test_lexicographical_compare2_async(par_simd(task), IteratorTag());

    test_lexicographical_compare3(simd, IteratorTag());
    test_lexicographical_compare3(par_simd, IteratorTag());
    test_lexicographical_compare3_async(simd(task), IteratorTag());
    test_lexicographical_compare3_async(par_simd(task), IteratorTag());
}

void lexicographical_compare_test()
{
    test_lexicographical_compare<std::random_access_iterator_tag>();
    test_lexicographical_compare<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Iterator1, typename Sentinel1,
    typename Iterator2, typename Sentinel2, typename... Args>
bool compare_test(ExPolicy&& policy, Iterator1 first1, Sentinel1 last1,
    Iterator2 first2, Sentinel2 last2, Args&&... args)
{
    if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
    {
        return hpx::lexicographical_compare(HPX_FORWARD(ExPolicy, policy),
            first1, last1, first2, last2, HPX_FORWARD(Args, args)...)
            .get();
    }
    else
    {
        return hpx::lexicographical_compare(HPX_FORWARD(ExPolicy, policy),
            first1, last1, first2, last2, HPX_FORWARD(Args, args)...);
    }
}

template <typename ExPolicy, typename... Args>
bool compare_ranges_test(ExPolicy&& policy, Args&&... args)
{
    if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
    {
        return hpx::ranges::lexicographical_compare(
            HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...)
            .get();
    }
    else
    {
        return hpx::ranges::lexicographical_compare(
            HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_lexicographical_compare_contiguous(ExPolicy policy)
{
    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    std::uniform_int_distribution<int> dis(1, 100);

    std::vector<std::size_t> test_sizes = {0, 1, 2, pack_size, pack_size + 1,
        2 * pack_size, 3 * pack_size + 2, 100,
        (std::max) (std::size_t(500), pack_size * 50 + 7)};
    if constexpr (pack_size > 1)
    {
        test_sizes.push_back(pack_size - 1);
        test_sizes.push_back(2 * pack_size - 1);
    }

    for (std::size_t sz1 : test_sizes)
    {
        for (std::size_t sz2 : test_sizes)
        {
            std::size_t common_sz = (std::min) (sz1, sz2);

            std::vector<int> c(sz1);
            std::generate(c.begin(), c.end(), [&]() { return dis(gen); });

            // Case A: common prefix is identical, lengths may differ
            std::vector<int> d(sz2);
            std::copy(c.begin(), c.begin() + common_sz, d.begin());
            if (sz2 > sz1)
            {
                std::generate(
                    d.begin() + common_sz, d.end(), [&]() { return dis(gen); });
            }

            bool actual_common =
                compare_test(policy, c.begin(), c.end(), d.begin(), d.end());
            bool expected_common = std::lexicographical_compare(
                c.begin(), c.end(), d.begin(), d.end());
            HPX_TEST_EQ(actual_common, expected_common);

            if (sz1 > 0 && sz2 > 0)
            {
                bool actual_ptr = compare_test(policy, c.data(),
                    c.data() + c.size(), d.data(), d.data() + d.size());
                HPX_TEST_EQ(actual_ptr, expected_common);
            }

            // Case B: change one element at a random position in the common part
            if (common_sz > 0)
            {
                std::uniform_int_distribution<std::size_t> pos_dis(
                    0, common_sz - 1);
                std::size_t diff_pos = pos_dis(gen);
                int old_val = d[diff_pos];
                d[diff_pos] = (old_val == 100) ? 1 : old_val + 1;

                bool actual_diff = compare_test(
                    policy, c.begin(), c.end(), d.begin(), d.end());
                bool expected_diff = std::lexicographical_compare(
                    c.begin(), c.end(), d.begin(), d.end());
                HPX_TEST_EQ(actual_diff, expected_diff);

                bool actual_diff_ptr = compare_test(policy, c.data(),
                    c.data() + c.size(), d.data(), d.data() + d.size());
                HPX_TEST_EQ(actual_diff_ptr, expected_diff);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_lexicographical_compare_boundaries(ExPolicy policy)
{
    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    if constexpr (pack_size > 1)
    {
        for (std::size_t mult = 1; mult <= 4; ++mult)
        {
            std::size_t sz = mult * pack_size * 2 + 5;
            std::vector<int> c(sz, 10);
            std::vector<int> d(sz, 10);

            // Mismatch right at pack boundary
            std::size_t boundary = mult * pack_size;
            c[boundary] = 9;
            HPX_TEST(
                compare_test(policy, c.begin(), c.end(), d.begin(), d.end()));
            HPX_TEST(
                !compare_test(policy, d.begin(), d.end(), c.begin(), c.end()));

            // Mismatch one before pack boundary
            if (boundary > 0)
            {
                c[boundary] = 10;
                c[boundary - 1] = 9;
                HPX_TEST(compare_test(
                    policy, c.begin(), c.end(), d.begin(), d.end()));
                HPX_TEST(!compare_test(
                    policy, d.begin(), d.end(), c.begin(), c.end()));
            }

            // Mismatch one after pack boundary
            if (boundary + 1 < sz)
            {
                c[boundary - 1] = 10;
                c[boundary + 1] = 9;
                HPX_TEST(compare_test(
                    policy, c.begin(), c.end(), d.begin(), d.end()));
                HPX_TEST(!compare_test(
                    policy, d.begin(), d.end(), c.begin(), c.end()));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_lexicographical_compare_uses_simd_path(ExPolicy policy)
{
    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    if constexpr (pack_size > 1)
    {
        std::atomic<int> pack_calls(0);
        auto pred = [&pack_calls](auto const& a, auto const& b) {
            if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(a)>>)
            {
                ++pack_calls;
            }
            return a < b;
        };

        std::size_t sz = (std::max) (std::size_t(1000), pack_size * 100 + 3);
        std::vector<int> c(sz, 5);
        std::vector<int> d(sz, 5);
        // Differ at the last element
        c[sz - 1] = 4;

        bool actual =
            compare_test(policy, c.begin(), c.end(), d.begin(), d.end(), pred);
        bool expected = std::lexicographical_compare(
            c.begin(), c.end(), d.begin(), d.end());

        HPX_TEST_EQ(actual, expected);
        HPX_TEST(actual);
        HPX_TEST(pack_calls > 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_lexicographical_compare_non_contiguous()
{
    std::atomic<int> pack_calls(0);
    auto pred = [&pack_calls](auto const& a, auto const& b) {
        if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(a)>>)
        {
            ++pack_calls;
        }
        return a < b;
    };

    std::deque<int> c(500, 10);
    std::deque<int> d(500, 10);
    c[400] = 5;

    bool actual = hpx::lexicographical_compare(
        hpx::execution::par_simd, c.begin(), c.end(), d.begin(), d.end(), pred);
    bool expected =
        std::lexicographical_compare(c.begin(), c.end(), d.begin(), d.end());

    HPX_TEST_EQ(actual, expected);
    HPX_TEST_EQ(pack_calls.load(), 0);
}

///////////////////////////////////////////////////////////////////////////////
void test_lexicographical_compare_bool_returning_predicate()
{
    auto pred = [](auto const& a, auto const& b) -> bool { return a < b; };

    std::vector<int> c(500, 10);
    std::vector<int> d(500, 10);
    c[400] = 5;

    bool expected =
        std::lexicographical_compare(c.begin(), c.end(), d.begin(), d.end());

    HPX_TEST_EQ(hpx::lexicographical_compare(hpx::execution::simd, c.begin(),
                    c.end(), d.begin(), d.end(), pred),
        expected);
    HPX_TEST_EQ(hpx::lexicographical_compare(hpx::execution::par_simd,
                    c.begin(), c.end(), d.begin(), d.end(), pred),
        expected);
}

///////////////////////////////////////////////////////////////////////////////
struct custom_bool_convertible
{
    bool val;
    constexpr operator bool() const noexcept
    {
        return val;
    }
};

void test_lexicographical_compare_bool_convertible_predicate()
{
    auto pred = [](auto const& a, auto const& b) -> custom_bool_convertible {
        return custom_bool_convertible{a < b};
    };

    std::vector<int> c(500, 10);
    std::vector<int> d(500, 10);
    c[400] = 5;

    bool expected =
        std::lexicographical_compare(c.begin(), c.end(), d.begin(), d.end());

    HPX_TEST_EQ(hpx::lexicographical_compare(hpx::execution::simd, c.begin(),
                    c.end(), d.begin(), d.end(), pred),
        expected);
    HPX_TEST_EQ(hpx::lexicographical_compare(hpx::execution::par_simd,
                    c.begin(), c.end(), d.begin(), d.end(), pred),
        expected);
}

///////////////////////////////////////////////////////////////////////////////
struct user_payload
{
    int val;
    bool operator<(user_payload const& other) const noexcept
    {
        return val < other.val;
    }
};

void test_lexicographical_compare_user_type()
{
    std::vector<user_payload> c(100, user_payload{1});
    std::vector<user_payload> d(100, user_payload{1});
    c[50] = user_payload{0};

    bool actual_simd = hpx::lexicographical_compare(
        hpx::execution::simd, c.begin(), c.end(), d.begin(), d.end());
    bool actual_par_simd = hpx::lexicographical_compare(
        hpx::execution::par_simd, c.begin(), c.end(), d.begin(), d.end());

    HPX_TEST(actual_simd);
    HPX_TEST(actual_par_simd);
}

///////////////////////////////////////////////////////////////////////////////
void test_lexicographical_compare_projections()
{
    std::vector<int> c = {1, 2, 3, 4, 5};
    std::vector<int> d = {1, 2, 3, 4, 6};

    auto proj = [](int x) { return -x; };

    // With negation projection, -5 > -6 so c is NOT less than d
    bool actual = hpx::ranges::lexicographical_compare(
        hpx::execution::simd, c, d, std::less<>{}, proj, proj);
    bool expected = std::lexicographical_compare(c.begin(), c.end(), d.begin(),
        d.end(), [](int a, int b) { return -a < -b; });

    HPX_TEST_EQ(actual, expected);

    bool actual_par = hpx::ranges::lexicographical_compare(
        hpx::execution::par_simd, c, d, std::less<>{}, proj, proj);
    HPX_TEST_EQ(actual_par, expected);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_lexicographical_compare_generic_projections(ExPolicy policy)
{
    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    if constexpr (pack_size > 1)
    {
        std::atomic<int> pack_calls(0);
        auto proj = [&pack_calls](auto const& x) {
            if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(x)>>)
            {
                ++pack_calls;
            }
            return -x;
        };

        std::size_t sz = (std::max) (std::size_t(1000), pack_size * 100 + 3);
        std::vector<int> c(sz, 5);
        std::vector<int> d(sz, 5);
        // At the last element: c has 6, d has 5.
        // With negation projection, proj(c) has -6, proj(d) has -5.
        // -6 < -5, so c is lexicographically less than d.
        c[sz - 1] = 6;

        bool actual = compare_ranges_test(policy, c.begin(), c.end(), d.begin(),
            d.end(), std::less<>{}, proj, proj);
        bool expected = std::lexicographical_compare(c.begin(), c.end(),
            d.begin(), d.end(), [](int a, int b) { return -a < -b; });

        HPX_TEST_EQ(actual, expected);
        HPX_TEST(actual);
        HPX_TEST(pack_calls > 0);

        std::atomic<int> ranges_pack_calls(0);
        auto ranges_proj = [&ranges_pack_calls](auto const& x) {
            if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(x)>>)
            {
                ++ranges_pack_calls;
            }
            return -x;
        };

        bool actual_ranges = compare_ranges_test(
            policy, c, d, std::less<>{}, ranges_proj, ranges_proj);
        HPX_TEST_EQ(actual_ranges, expected);
        HPX_TEST(actual_ranges);
        HPX_TEST(ranges_pack_calls > 0);
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

    lexicographical_compare_test();

    test_lexicographical_compare_contiguous(simd);
    test_lexicographical_compare_contiguous(par_simd);
    test_lexicographical_compare_contiguous(simd(task));
    test_lexicographical_compare_contiguous(par_simd(task));

    test_lexicographical_compare_boundaries(simd);
    test_lexicographical_compare_boundaries(par_simd);
    test_lexicographical_compare_boundaries(simd(task));
    test_lexicographical_compare_boundaries(par_simd(task));

    test_lexicographical_compare_uses_simd_path(simd);
    test_lexicographical_compare_uses_simd_path(par_simd);
    test_lexicographical_compare_uses_simd_path(simd(task));
    test_lexicographical_compare_uses_simd_path(par_simd(task));

    test_lexicographical_compare_non_contiguous();
    test_lexicographical_compare_bool_returning_predicate();
    test_lexicographical_compare_bool_convertible_predicate();
    test_lexicographical_compare_user_type();
    test_lexicographical_compare_projections();

    test_lexicographical_compare_generic_projections(simd);
    test_lexicographical_compare_generic_projections(par_simd);
    test_lexicographical_compare_generic_projections(simd(task));
    test_lexicographical_compare_generic_projections(par_simd(task));

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
