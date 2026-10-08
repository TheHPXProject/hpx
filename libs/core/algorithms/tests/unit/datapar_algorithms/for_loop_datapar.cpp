//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_for_loop_idx(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    hpx::experimental::for_loop(std::forward<ExPolicy>(policy), 0,
        static_cast<int>(c.size()), [&c](auto i) {
            for (std::size_t e = 0; e < hpx::parallel::traits::size(i); ++e)
                c[hpx::parallel::traits::get(i, e)] = 42;
        });

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy>
void test_for_loop_idx_async(ExPolicy&& p)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    auto f = hpx::experimental::for_loop(
        std::forward<ExPolicy>(p), 0, static_cast<int>(c.size()), [&c](auto i) {
            for (std::size_t e = 0; e < hpx::parallel::traits::size(i); ++e)
                c[hpx::parallel::traits::get(i, e)] = 42;
        });
    f.wait();

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

void for_loop_test_idx()
{
    using namespace hpx::execution;

    test_for_loop_idx(simd);
    test_for_loop_idx(par_simd);

    test_for_loop_idx_async(simd(task));
    test_for_loop_idx_async(par_simd(task));

    test_for_loop_idx(fixed_size_simd<4>);
    test_for_loop_idx(par_fixed_size_simd<4>);

    test_for_loop_idx_async(fixed_size_simd<4>(task));
    test_for_loop_idx_async(par_fixed_size_simd<4>(task));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.contains("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    for_loop_test_idx();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
