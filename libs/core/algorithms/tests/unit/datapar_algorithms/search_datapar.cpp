//  Copyright (c) 2026 Arivoli Ramamoorthy
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
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "../algorithms/test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<int> dis(3, 102);

template <typename ExPolicy, typename IteratorTag>
void test_search1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search1_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[0] = 1;
    c[1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search2_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[0] = 1;
    c[1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search3(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search3_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_search1()
{
    using namespace hpx::execution;

    test_search1(simd, IteratorTag());
    test_search1(par_simd, IteratorTag());

    test_search1_async(simd(task), IteratorTag());
    test_search1_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search2()
{
    using namespace hpx::execution;

    test_search2(simd, IteratorTag());
    test_search2(par_simd, IteratorTag());

    test_search2_async(simd(task), IteratorTag());
    test_search2_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search3()
{
    using namespace hpx::execution;

    test_search3(simd, IteratorTag());
    test_search3(par_simd, IteratorTag());

    test_search3_async(simd(task), IteratorTag());
    test_search3_async(par_simd(task), IteratorTag());
}

void search_test1()
{
    test_search1<std::random_access_iterator_tag>();
    test_search1<std::forward_iterator_tag>();
}

void search_test2()
{
    test_search2<std::random_access_iterator_tag>();
    test_search2<std::forward_iterator_tag>();
}

void search_test3()
{
    test_search3<std::random_access_iterator_tag>();
    test_search3<std::forward_iterator_tag>();
}

template <typename ExPolicy, typename Pred = std::equal_to<>>
void check_search_result(ExPolicy policy, std::vector<int> const& haystack,
    std::vector<int> const& needle, Pred pred = Pred{})
{
    auto const result = hpx::search(policy, haystack.begin(), haystack.end(),
        needle.begin(), needle.end(), pred);
    auto const expected = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end(), pred);

    HPX_TEST(result == expected);
}

template <typename ExPolicy>
void test_search_boundaries(ExPolicy policy)
{
    using pack_type = hpx::parallel::traits::vector_pack_type_t<int>;
    constexpr std::size_t pack_size =
        hpx::parallel::traits::vector_pack_size_v<pack_type>;

    // A single-element needle exercises the diff == 1 path.
    {
        std::vector<int> haystack(pack_size * 3 + 1, 9);
        std::vector<int> const needle{1};
        haystack[pack_size + 1] = needle[0];
        check_search_result(policy, haystack, needle);
    }

    // The match starts at the last valid position (part_size - 1 for the
    // single-partition SIMD policy).
    {
        std::vector<int> haystack(pack_size * 3 + 1, 9);
        std::vector<int> const needle{1, 2, 3};
        auto const needle_size =
            static_cast<std::vector<int>::difference_type>(needle.size());
        std::copy(needle.begin(), needle.end(), haystack.end() - needle_size);
        check_search_result(policy, haystack, needle);
    }

    // The candidate begins in the final lane and crosses a pack boundary.
    {
        std::vector<int> haystack(pack_size * 3 + 1, 9);
        std::vector<int> const needle{1, 2, 3};
        std::copy(needle.begin(), needle.end(),
            haystack.begin() + static_cast<std::ptrdiff_t>(pack_size - 1));
        check_search_result(policy, haystack, needle);
    }

    // Every other element is a false first-element candidate.
    {
        std::vector<int> haystack(pack_size * 3 + 1, 9);
        std::vector<int> const needle{1, 2};
        for (std::size_t i = 0; i < haystack.size(); i += 2)
            haystack[i] = needle[0];
        check_search_result(policy, haystack, needle);
    }

    // Non-transparent predicates are valid for search. They must fall back
    // to scalar comparisons if they cannot accept SIMD packs.
    {
        std::vector<int> haystack(pack_size * 3 + 1, 9);
        std::vector<int> const needle{1, 2};
        haystack[pack_size] = needle[0];
        haystack[pack_size + 1] = needle[1];
        check_search_result(policy, haystack, needle, std::equal_to<int>{});
    }
}

// Contiguous arithmetic ranges must actually take the SIMD path.
void test_search_uses_simd_path()
{
    std::atomic<int> pack_calls(0);
    auto pred = [&pack_calls](auto const& a, auto const& b) {
        if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(a)>>)
            ++pack_calls;
        return a == b;
    };

    std::vector<int> haystack(1000, 9);
    std::vector<int> const needle{1, 2};
    haystack[700] = needle[0];
    haystack[701] = needle[1];
    check_search_result(hpx::execution::par_simd, haystack, needle, pred);

    HPX_TEST(pack_calls > 0);
}

struct non_copyable
{
    explicit non_copyable(int v)
      : value(v)
    {
    }

    non_copyable(non_copyable const&) = delete;
    non_copyable& operator=(non_copyable const&) = delete;

    int value;
};

// The projected needle element is used in place and never copied.
void test_search_non_copyable_projection()
{
    static non_copyable const table[] = {
        non_copyable(0), non_copyable(1), non_copyable(2), non_copyable(3)};

    auto pred = [](auto const& a, non_copyable const& b) {
        return a == b.value;
    };
    auto proj2 = [](int i) -> non_copyable const& { return table[i]; };

    std::vector<int> haystack(1000, 0);
    std::vector<int> const needle{1, 2, 3};
    std::copy(needle.begin(), needle.end(), haystack.begin() + 600);

    auto const result = hpx::ranges::search(hpx::execution::par_simd,
        haystack.begin(), haystack.end(), needle.begin(), needle.end(), pred,
        hpx::identity{}, proj2);

    HPX_TEST(result == haystack.begin() + 600);
}

// Random-access but non-contiguous ranges must not take the SIMD path.
void test_search_non_contiguous()
{
    std::atomic<int> pack_calls(0);
    auto pred = [&pack_calls](auto const& a, auto const& b) {
        if constexpr (!std::is_arithmetic_v<std::decay_t<decltype(a)>>)
            ++pack_calls;
        return a == b;
    };

    std::deque<int> haystack(10007, 9);
    std::vector<int> const needle{1, 2, 3};
    for (std::size_t pos :
        {std::size_t(127), std::size_t(5000), haystack.size() - needle.size()})
    {
        std::fill(haystack.begin(), haystack.end(), 9);
        std::copy(needle.begin(), needle.end(),
            haystack.begin() + static_cast<std::ptrdiff_t>(pos));

        auto const result =
            hpx::search(hpx::execution::par_simd, haystack.begin(),
                haystack.end(), needle.begin(), needle.end(), pred);
        auto const expected = std::search(
            haystack.begin(), haystack.end(), needle.begin(), needle.end());

        HPX_TEST(result == expected);
    }

    HPX_TEST_EQ(pack_calls.load(), 0);
}

void search_boundary_tests()
{
    using namespace hpx::execution;

    test_search_boundaries(simd);
    test_search_boundaries(par_simd);
    test_search_uses_simd_path();
    test_search_non_copyable_projection();
    test_search_non_contiguous();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    search_test1();
    search_test2();
    search_test3();
    search_boundary_tests();
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
