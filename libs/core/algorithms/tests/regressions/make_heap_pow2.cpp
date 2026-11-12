//  Copyright (c) 2026 GREYNINJA9
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Regression test for issue #7905: hpx::make_heap with the par / par_unseq
// execution policies produced an invalid heap for ranges whose size is a
// power of two (2^k) or one more than a power of two (2^k + 1).
//
// The bug is a race: it only manifests when the chunks of the first parallel
// batch actually run concurrently, so a single run per size is not enough to
// reliably expose it. Small sizes are therefore repeated many times with
// fresh random data (200 runs per small size failed consistently without the
// fix).

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

///////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

///////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_make_heap_pow2(ExPolicy&& policy)
{
    // sizes 2^k and 2^k + 1 for k = 3..17
    std::vector<std::size_t> sizes;
    for (std::size_t k = 3; k <= 17; ++k)
    {
        sizes.push_back(std::size_t(1) << k);
        sizes.push_back((std::size_t(1) << k) + 1);
    }

    // Small sizes need many repetitions to make the race reliable; large
    // sizes are exercised a few times to keep the test fast.
    constexpr int small_reps = 200;
    constexpr int large_reps = 5;

    std::uniform_int_distribution<std::size_t> dist(
        0, (std::numeric_limits<std::size_t>::max)());

    for (std::size_t len : sizes)
    {
        int const reps = len <= 1025 ? small_reps : large_reps;
        for (int rep = 0; rep != reps; ++rep)
        {
            std::vector<std::size_t> c(len);
            std::generate(c.begin(), c.end(), [&]() { return dist(gen); });

            hpx::make_heap(policy, c.begin(), c.end());

            HPX_TEST_EQ(std::is_heap(c.begin(), c.end()), true);
        }
    }
}

void make_heap_pow2_test()
{
    using namespace hpx::execution;

    test_make_heap_pow2(par);
    test_make_heap_pow2(par_unseq);
}

///////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    make_heap_pow2_test();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with a non-zero status");

    return hpx::util::report_errors();
}
