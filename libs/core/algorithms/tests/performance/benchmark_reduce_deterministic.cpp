//  Copyright (c) 2024 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <cstddef>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

int seed = 1000;
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
template <typename PolicyT, typename IteratorT, typename InitVal, typename Op>
void bench_reduce_deterministic(PolicyT const& policy,
    IteratorT const& deterministic_shuffled, InitVal const& val_det,
    Op const& op)
{
    // check if different type for deterministic and nondeeterministic
    // and same result

    [[maybe_unused]] auto r1_shuffled = hpx::experimental::reduce_deterministic(
        policy, std::begin(deterministic_shuffled),
        std::end(deterministic_shuffled), val_det, op);
}

template <typename PolicyT, typename IteratorT, typename InitVal, typename Op>
void bench_reduce(PolicyT const& policy,
    IteratorT const& non_deterministic_shuffled, InitVal const& val_det,
    Op const& op)
{
    [[maybe_unused]] auto r =
        hpx::reduce(policy, (std::begin(non_deterministic_shuffled)),
            (std::end(non_deterministic_shuffled)), val_det, op);
}

//////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::srand(seed);

    auto test_count = vm["test_count"].as<int>();
    std::size_t vector_size = vm["vector-size"].as<std::size_t>();

    hpx::util::perftests_init(vm);

    // verify that input is within domain of program
    if (test_count == 0 || test_count < 0)
    {
        std::cerr << "test_count cannot be zero or negative...\n" << std::flush;
        hpx::local::finalize();
        return -1;
    }

    {
        {
            using FloatTypeDeterministic = float;
            std::size_t LEN = vector_size;

            constexpr FloatTypeDeterministic num_bounds_det =
                std::is_same_v<FloatTypeDeterministic, float> ? 1000.0 :
                                                                1000000.0;

            std::vector<FloatTypeDeterministic> deterministic(LEN);

            for (size_t i = 0; i < LEN; ++i)
            {
                deterministic[i] = get_rand<FloatTypeDeterministic>(
                    -num_bounds_det, num_bounds_det);
            }

            std::vector<FloatTypeDeterministic> deterministic_shuffled =
                deterministic;

            std::shuffle(deterministic_shuffled.begin(),
                deterministic_shuffled.end(), gen);

            FloatTypeDeterministic val_det(41.999);

            auto op = [](FloatTypeDeterministic v1, FloatTypeDeterministic v2) {
                return v1 + v2;
            };
            {
                hpx::util::perftests_report(
                    "fl reduce", "seq", test_count, [&]() {
                        bench_reduce(hpx::execution::seq,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "fl reduce", "par", test_count, [&]() {
                        bench_reduce(hpx::execution::par,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "fl reduce deterministic", "seq", test_count, [&]() {
                        bench_reduce_deterministic(hpx::execution::seq,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "fl reduce deterministic", "par", test_count, [&]() {
                        bench_reduce_deterministic(hpx::execution::par,
                            deterministic_shuffled, val_det, op);
                    });
            }
        }
        {
            using FloatTypeDeterministic = double;
            std::size_t LEN = vector_size;

            constexpr FloatTypeDeterministic num_bounds_det =
                std::is_same_v<FloatTypeDeterministic, float> ? 1000.0 :
                                                                1000000.0;

            std::vector<FloatTypeDeterministic> deterministic(LEN);

            for (size_t i = 0; i < LEN; ++i)
            {
                deterministic[i] = get_rand<FloatTypeDeterministic>(
                    -num_bounds_det, num_bounds_det);
            }

            std::vector<FloatTypeDeterministic> deterministic_shuffled =
                deterministic;

            std::shuffle(deterministic_shuffled.begin(),
                deterministic_shuffled.end(), gen);

            FloatTypeDeterministic val_det(41.999);

            auto op = [](FloatTypeDeterministic v1, FloatTypeDeterministic v2) {
                return v1 + v2;
            };
            {
                hpx::util::perftests_report(
                    "dbl reduce", "seq", test_count, [&]() {
                        bench_reduce(hpx::execution::seq,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "dbl reduce", "par", test_count, [&]() {
                        bench_reduce(hpx::execution::par,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "dbl reduce deterministic", "seq", test_count, [&]() {
                        bench_reduce_deterministic(hpx::execution::seq,
                            deterministic_shuffled, val_det, op);
                    });
            }
            {
                hpx::util::perftests_report(
                    "dbl reduce deterministic", "par", test_count, [&]() {
                        bench_reduce_deterministic(hpx::execution::par,
                            deterministic_shuffled, val_det, op);
                    });
            }
        }

        hpx::util::perftests_print_times();
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("test_count", value<int>()->default_value(100),
            "number of tests to be averaged")
        ("vector-size", value<std::size_t>()->default_value(1000000),
            "number of elements to be reduced")
        ;
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all"};

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#endif
