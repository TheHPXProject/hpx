//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include "../algorithms/test_utils.hpp"
#include "../algorithms/unique_copy_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_unique_copy_simd()
{
    using namespace hpx::execution;

    int rand_base = std::rand();

    // Test with generic predicate (exercising the SIMD path)
    test_unique_copy(
        simd, IteratorTag(), int(),
        [](auto const a, auto const b) { return a == b; }, rand_base);
    test_unique_copy(
        par_simd, IteratorTag(), int(),
        [](auto const a, auto const b) { return a == b; }, rand_base);

    test_unique_copy_async(
        simd(task), IteratorTag(), int(),
        [](auto const a, auto const b) { return a == b; }, rand_base);
    test_unique_copy_async(
        par_simd(task), IteratorTag(), int(),
        [](auto const a, auto const b) { return a == b; }, rand_base);

    // Test non-vectorizable predicate (exercising fallback to non-simd)
    test_unique_copy(
        simd, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique_copy(
        par_simd, IteratorTag(), int(),
        [](int const a, int const b) -> bool { return a == b; }, rand_base);
    test_unique_copy(
        simd, IteratorTag(), int(),
        [](auto const& a, auto const& b) -> bool { return a == b; }, rand_base);
    test_unique_copy(
        par_simd, IteratorTag(), int(),
        [](auto const& a, auto const& b) -> bool { return a == b; }, rand_base);

    test_unique_copy(
        simd, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy(
        par_simd, IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);

    test_unique_copy_async(
        simd(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
    test_unique_copy_async(
        par_simd(task), IteratorTag(), user_defined_type(),
        [](user_defined_type const& a, user_defined_type const& b) -> bool {
            return a == b;
        },
        rand_base);
}

template <typename ExPolicy>
void test_unique_copy_contiguous(ExPolicy policy)
{
    std::vector<int> c = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5, 6};
    std::vector<int> res(c.size());
    std::vector<int> expected(c.size());

    auto result = hpx::unique_copy(policy, c.begin(), c.end(), res.begin(),
        [](auto const a, auto const b) { return a == b; });
    res.erase(result, res.end());

    auto exp_end = std::unique_copy(c.begin(), c.end(), expected.begin(),
        [](int a, int b) { return a == b; });
    expected.erase(exp_end, expected.end());

    HPX_TEST(res == expected);
}

template <typename ExPolicy>
void test_unique_copy_deque(ExPolicy policy)
{
    std::deque<int> c = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5, 6};
    std::vector<int> res(c.size());
    std::vector<int> expected(c.size());

    auto result = hpx::unique_copy(policy, c.begin(), c.end(), res.begin(),
        [](auto const a, auto const b) { return a == b; });
    res.erase(result, res.end());

    auto exp_end = std::unique_copy(c.begin(), c.end(), expected.begin(),
        [](int a, int b) { return a == b; });
    expected.erase(exp_end, expected.end());

    HPX_TEST(res == expected);
}

template <typename ExPolicy>
void test_unique_copy_arithmetic_pred(ExPolicy policy)
{
    std::vector<int> c = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5, 6};
    std::vector<int> res(c.size());
    std::vector<int> expected(c.size());

    auto result = hpx::unique_copy(policy, c.begin(), c.end(), res.begin(),
        [](auto const a, auto const b) -> int { return a == b ? 1 : 0; });
    res.erase(result, res.end());

    auto exp_end = std::unique_copy(c.begin(), c.end(), expected.begin(),
        [](int a, int b) { return a == b; });
    expected.erase(exp_end, expected.end());

    HPX_TEST(res == expected);
}

void unique_copy_test()
{
    test_unique_copy_simd<std::random_access_iterator_tag>();
    test_unique_copy_simd<std::bidirectional_iterator_tag>();
    test_unique_copy_simd<std::forward_iterator_tag>();

    test_unique_copy_contiguous(hpx::execution::simd);
    test_unique_copy_contiguous(hpx::execution::par_simd);
    test_unique_copy_deque(hpx::execution::simd);
    test_unique_copy_deque(hpx::execution::par_simd);
    test_unique_copy_arithmetic_pred(hpx::execution::simd);
    test_unique_copy_arithmetic_pred(hpx::execution::par_simd);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    unique_copy_test();
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
