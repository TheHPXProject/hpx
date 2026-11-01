//  Copyright (c) 2026 GREYNINJA9

//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for issue #7905: hpx::make_heap with the par / par_unseq
// execution policies produced an invalid heap for ranges whose size is a
// power of two (2^k) or one more than a power of two (2^k + 1).

// The bug is a race: it only manifests when the chunks of the first parallel
// batch actually run concurrently, so a single run per size is not enough to
// reliably expose it. Each size is therefore repeated many times with fresh
// random data (200 runs per small size failed consistently without the fix).

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

///////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_make_heap_pow2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // sizes 2^k and 2^k + 1 for k = 3..17, plus a couple of large ones
    std::vector<std::size_t> sizes;
    for (std::size_t k = 3; k <= 17; ++k)
    {
        sizes.push_back(std::size_t(1) << k);
        sizes.push_back((std::size_t(1) << k) + 1);
    }
    sizes.push_back(std::size_t(1) << 20);
    sizes.push_back((std::size_t(1) << 20) + 1);

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
            for (auto& x : c)
                x = dist(gen);

            hpx::make_heap(policy, iterator(hpx::util::begin(c)),
                iterator(hpx::util::end(c)));

            HPX_TEST_EQ(
                std::is_heap(hpx::util::begin(c), hpx::util::end(c)), true);
        }
    }
}

template <typename IteratorTag>
void test_make_heap_pow2()
{
    using namespace hpx::execution;

    test_make_heap_pow2(par, IteratorTag());
    test_make_heap_pow2(par_unseq, IteratorTag());
}

void make_heap_pow2_test()
{
    test_make_heap_pow2<std::random_access_iterator_tag>();
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
