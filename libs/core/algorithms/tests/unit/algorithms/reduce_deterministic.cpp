//  Copyright (c) 2024 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename T>
T get_rand(T LO = (std::numeric_limits<T>::min)(),
    T HI = (std::numeric_limits<T>::max)())
{
    return LO +
        static_cast<T>(std::rand()) /
        (static_cast<T>(static_cast<T>((RAND_MAX)) / (HI - LO)));
}

///////////////////////////////////////////////////////////////////////////////

template <typename IteratorTag, typename FloatTypeDeterministic,
    typename FloatTypeNonDeterministic, size_t LEN = 10007>
void test_reduce1(IteratorTag)
{
    // check if different type for deterministic and nondeeterministic
    // and same result i.e. correct computation
    using base_iterator_det =
        typename std::vector<FloatTypeDeterministic>::iterator;
    using iterator_det = test::test_iterator<base_iterator_det, IteratorTag>;

    using base_iterator_ndet =
        typename std::vector<FloatTypeNonDeterministic>::iterator;
    using iterator_ndet = test::test_iterator<base_iterator_ndet, IteratorTag>;

    std::vector<FloatTypeDeterministic> deterministic(LEN);
    std::vector<FloatTypeNonDeterministic> nondeterministic(LEN);

    std::iota(
        deterministic.begin(), deterministic.end(), FloatTypeDeterministic(0));

    std::iota(nondeterministic.begin(), nondeterministic.end(),
        FloatTypeNonDeterministic(0));

    FloatTypeDeterministic val_det(0);
    FloatTypeNonDeterministic val_non_det(0);
    auto op = [](FloatTypeNonDeterministic v1, FloatTypeNonDeterministic v2) {
        return v1 + v2;
    };

    FloatTypeDeterministic r1 = hpx::experimental::reduce_deterministic(
        iterator_det(std::begin(deterministic)),
        iterator_det(std::end(deterministic)), val_det, op);

    // verify values
    FloatTypeNonDeterministic r2 = hpx::reduce(hpx::execution::seq,
        iterator_ndet(std::begin(nondeterministic)),
        iterator_ndet(std::end(nondeterministic)), val_non_det, op);

    FloatTypeNonDeterministic r3 = std::accumulate(
        nondeterministic.begin(), nondeterministic.end(), val_non_det);

    HPX_TEST_EQ(static_cast<FloatTypeNonDeterministic>(r1), r3);
    HPX_TEST_EQ(static_cast<FloatTypeNonDeterministic>(r2), r3);
}

template <typename IteratorTag, typename FloatTypeDeterministic,
    typename FloatTypeNonDeterministic, size_t LEN = 10007>
void test_reduce_parallel1(IteratorTag)
{
    // check if different type for deterministic and nondeeterministic
    // and same result i.e. correct computation
    using base_iterator_det =
        typename std::vector<FloatTypeDeterministic>::iterator;
    using iterator_det = test::test_iterator<base_iterator_det, IteratorTag>;

    using base_iterator_ndet =
        typename std::vector<FloatTypeNonDeterministic>::iterator;
    using iterator_ndet = test::test_iterator<base_iterator_ndet, IteratorTag>;

    std::vector<FloatTypeDeterministic> deterministic(LEN);
    std::vector<FloatTypeNonDeterministic> nondeterministic(LEN);

    std::iota(
        deterministic.begin(), deterministic.end(), FloatTypeDeterministic(0));

    std::iota(nondeterministic.begin(), nondeterministic.end(),
        FloatTypeNonDeterministic(0));

    FloatTypeDeterministic val_det(0);
    FloatTypeNonDeterministic val_non_det(0);
    auto op = [](FloatTypeNonDeterministic v1, FloatTypeNonDeterministic v2) {
        return v1 + v2;
    };

    FloatTypeDeterministic r1 = hpx::experimental::reduce_deterministic(
        hpx::execution::par, iterator_det(std::begin(deterministic)),
        iterator_det(std::end(deterministic)), val_det, op);

    // verify values
    FloatTypeNonDeterministic r2 = hpx::reduce(hpx::execution::par,
        iterator_ndet(std::begin(nondeterministic)),
        iterator_ndet(std::end(nondeterministic)), val_non_det, op);

    FloatTypeNonDeterministic r3 = std::accumulate(
        nondeterministic.begin(), nondeterministic.end(), val_non_det);

    HPX_TEST_EQ(static_cast<FloatTypeNonDeterministic>(r1), r3);
    HPX_TEST_EQ(static_cast<FloatTypeNonDeterministic>(r2), r3);
}

template <typename IteratorTag, typename FloatTypeDeterministic,
    size_t LEN = 10007>
void test_reduce_determinism(IteratorTag)
{
    // check if different type for deterministic and nondeeterministic
    // and same result
    using base_iterator_det =
        typename std::vector<FloatTypeDeterministic>::iterator;
    using iterator_det = test::test_iterator<base_iterator_det, IteratorTag>;

    constexpr FloatTypeDeterministic num_bounds_det =
        std::is_same_v<FloatTypeDeterministic, float> ? 1000.0 : 1000000.0;

    std::vector<FloatTypeDeterministic> deterministic(LEN);

    for (size_t i = 0; i < LEN; ++i)
    {
        deterministic[i] =
            get_rand<FloatTypeDeterministic>(-num_bounds_det, num_bounds_det);
    }

    std::vector<FloatTypeDeterministic> deterministic_shuffled = deterministic;

    std::shuffle(
        deterministic_shuffled.begin(), deterministic_shuffled.end(), gen);

    FloatTypeDeterministic val_det(41.999);

    auto op = [](FloatTypeDeterministic v1, FloatTypeDeterministic v2) {
        return v1 + v2;
    };

    FloatTypeDeterministic r1 = hpx::experimental::reduce_deterministic(
        iterator_det(std::begin(deterministic)),
        iterator_det(std::end(deterministic)), val_det, op);

    FloatTypeDeterministic r1_shuffled =
        hpx::experimental::reduce_deterministic(
            iterator_det(std::begin(deterministic_shuffled)),
            iterator_det(std::end(deterministic_shuffled)), val_det, op);

    HPX_TEST_EQ(r1,
        r1_shuffled);    // Deterministically calculated, should always satisfy
}

/// This test function is never called because it is not guaranteed to pass
/// It serves an important purpose to demonstrate that floating point summation
/// is not always associative i.e. a+b+c != a+c+b
template <typename IteratorTag, typename FloatTypeNonDeterministic,
    size_t LEN = 10007>
void test_orig_reduce_determinism(IteratorTag)
{
    using base_iterator_ndet =
        typename std::vector<FloatTypeNonDeterministic>::iterator;
    using iterator_ndet = test::test_iterator<base_iterator_ndet, IteratorTag>;

    constexpr auto num_bounds_ndet =
        std::is_same_v<FloatTypeNonDeterministic, float> ? 1000.0f : 1000000.0f;

    std::vector<FloatTypeNonDeterministic> nondeterministic(LEN);
    for (size_t i = 0; i < LEN; ++i)
    {
        nondeterministic[i] = get_rand<FloatTypeNonDeterministic>(
            -num_bounds_ndet, num_bounds_ndet);
    }
    std::vector<FloatTypeNonDeterministic> nondeterministic_shuffled =
        nondeterministic;
    std::shuffle(nondeterministic_shuffled.begin(),
        nondeterministic_shuffled.end(), gen);

    FloatTypeNonDeterministic val_non_det(41.999);

    auto op = [](FloatTypeNonDeterministic v1, FloatTypeNonDeterministic v2) {
        return v1 + v2;
    };

    FloatTypeNonDeterministic r2 = hpx::reduce(hpx::execution::seq,
        iterator_ndet(std::begin(nondeterministic)),
        iterator_ndet(std::end(nondeterministic)), val_non_det, op);
    FloatTypeNonDeterministic r2_shuffled = hpx::reduce(hpx::execution::seq,
        iterator_ndet(std::begin(nondeterministic_shuffled)),
        iterator_ndet(std::end(nondeterministic_shuffled)), val_non_det, op);

    FloatTypeNonDeterministic r3 = std::accumulate(
        nondeterministic.begin(), nondeterministic.end(), val_non_det);
    FloatTypeNonDeterministic r3_shuffled =
        std::accumulate(nondeterministic_shuffled.begin(),
            nondeterministic_shuffled.end(), val_non_det);

    /// failed around 131 times out of 1000 on macOS arm
    /// Floating point addition is not necessarily associative,
    /// might fail on an architecture not yet known with much higher precision
    HPX_TEST_NEQ(r2, r2_shuffled);
    HPX_TEST_NEQ(r3, r3_shuffled);
}

template <typename IteratorTag>
void test_reduce1()
{
    using namespace hpx::execution;

    test_reduce1<IteratorTag, float, float, 1000>(IteratorTag());
    test_reduce1<IteratorTag, double, float, 1000>(IteratorTag());
    test_reduce1<IteratorTag, float, double, 1000>(IteratorTag());
    test_reduce1<IteratorTag, double, double, 1000>(IteratorTag());
    test_reduce_parallel1<IteratorTag, float, float, 1000>(IteratorTag());
    test_reduce_parallel1<IteratorTag, float, double, 1000>(IteratorTag());
    test_reduce_parallel1<IteratorTag, double, float, 1000>(IteratorTag());
    test_reduce_parallel1<IteratorTag, double, double, 1000>(IteratorTag());
}

template <typename IteratorTag>
void test_reduce2()
{
    using namespace hpx::execution;

    test_reduce_determinism<IteratorTag, float, 1000>(IteratorTag());
    test_reduce_determinism<IteratorTag, double, 1000>(IteratorTag());
}

void reduce_test1()
{
    test_reduce1<std::random_access_iterator_tag>();
    test_reduce2<std::random_access_iterator_tag>();
    test_reduce1<std::forward_iterator_tag>();
    test_reduce2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    bool seed_random = false;

    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        seed_random = true;
    }

    if (vm.count("seed-random"))
        seed_random = vm["seed-random"].as<bool>();

    if (seed_random)
    {
        std::cout << "using seed: " << seed << std::endl;
        std::cout << "** std::accumulate, hpx::reduce may fail due to "
                     "non-determinism of the floating summation"
                  << std::endl;
        gen.seed(seed);
        std::srand(seed);
    }
    else
    {
        gen.seed(223);
        std::srand(223);
    }

    reduce_test1();

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
    desc_commandline.add_options()("seed-random", value<bool>(),
        "switch for the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
