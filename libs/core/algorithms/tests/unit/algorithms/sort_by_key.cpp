//  Copyright (c) 2015-2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>
//
#if defined(HPX_DEBUG)
#define HPX_SORT_BY_KEY_TEST_SIZE (1 << 8)
#else
#define HPX_SORT_BY_KEY_TEST_SIZE (1 << 18)
#endif
//
#include "sort_tests.hpp"
//
#define EXTRA_DEBUG
//
namespace debug {
    template <typename T>
    void output([[maybe_unused]] std::string const& name,
        [[maybe_unused]] std::vector<T> const& v)
    {
#ifdef EXTRA_DEBUG7
        std::cout << name.c_str() << "\t : {" << v.size() << "} : ";
        std::copy(std::begin(v), std::end(v),
            std::ostream_iterator<T>(std::cout, ", "));
        std::cout << "\n";
#endif
    }

    template <typename Iter>
    void output([[maybe_unused]] std::string const& name,
        [[maybe_unused]] Iter begin, [[maybe_unused]] Iter end)
    {
#ifdef EXTRA_DEBUG
        std::cout << name.c_str() << "\t : {" << std::distance(begin, end)
                  << "} : ";
        std::copy(begin, end,
            std::ostream_iterator<
                typename std::iterator_traits<Iter>::value_type>(
                std::cout, ", "));
        std::cout << "\n";
#endif
    }

#if defined(EXTRA_DEBUG)
#define debug_msg(a) std::cout << a
#else
#define debug_msg(a)
#endif
}    // namespace debug

#undef msg
#define msg(a, b, c, d)                                                        \
    std::cout << std::setw(60) << a << std::setw(12) << b << std::setw(40)     \
              << c << std::setw(30) << std::setw(8) << #d << "\t";

////////////////////////////////////////////////////////////////////////////////
void sort_by_key_benchmark()
{
    try
    {
        int const bench_size = HPX_SORT_BY_KEY_TEST_SIZE * 256;
        // vector of values, and keys
        std::vector<double> values, o_values;
        std::vector<int64_t> keys, o_keys;
        //
        values.assign(bench_size, 0);
        keys.assign(bench_size, 0);

        // generate a sequence as the values
        std::iota(values.begin(), values.end(), 0);
        // generate a sequence as the keys
        std::iota(keys.begin(), keys.end(), 0);

        // shuffle the keys up,
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(keys.begin(), keys.end(), g);

        // make copies of initial states
        o_keys = keys;
        o_values = values;

        hpx::chrono::high_resolution_timer t;
        hpx::experimental::sort_by_key(
            hpx::execution::par, keys.begin(), keys.end(), values.begin());
        auto elapsed = static_cast<std::uint64_t>(t.elapsed_nanoseconds());

        // after sorting by key, the values should be equal to the original keys
        bool is_equal = std::equal(keys.begin(), keys.end(), o_values.begin());
        HPX_TEST(is_equal);
        if (is_equal)
        {
            // CDash graph plotting
            hpx::util::print_cdash_timing("SortByKeyTime", elapsed);
        }
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Tkey, typename Tval, typename Op,
    typename HelperOp>
void test_sort_by_key1(
    ExPolicy&& policy, Tkey, Tval, Op const&, HelperOp const&)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");
    msg(typeid(ExPolicy).name(), typeid(Tval).name(), typeid(Op).name(), sync);
    std::cout << "\n";

    // vector of values, and keys
    std::vector<Tval> values, o_values;
    std::vector<Tkey> keys, o_keys;
    //
    values.assign(HPX_SORT_BY_KEY_TEST_SIZE, 0);
    keys.assign(HPX_SORT_BY_KEY_TEST_SIZE, 0);

    // generate a sequence as the values
    std::iota(values.begin(), values.end(), 0);
    // generate a sequence as the keys
    std::iota(keys.begin(), keys.end(), 0);

    // shuffle the keys up,
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    // make copies of initial states
    o_keys = keys;
    o_values = values;

    // sort_by_key, blocking when seq, par, par_vec
    hpx::experimental::sort_by_key(std::forward<ExPolicy>(policy), keys.begin(),
        keys.end(), values.begin());

    // after sorting by key, the values should be equal to the original keys
    bool is_equal = std::equal(keys.begin(), keys.end(), o_values.begin());
    if (is_equal)
    {
        //std::cout << "Test Passed\n";
    }
    else
    {
        debug::output("keys     ", o_keys);
        debug::output("values   ", o_values);
        debug::output("key range", keys);
        debug::output("val range", values);
        throw std::string("Problem");
    }
    HPX_TEST(is_equal);
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Tkey, typename Tval, typename Op,
    typename HelperOp>
void test_sort_by_key_async(
    ExPolicy&& policy, Tkey, Tval, Op const&, HelperOp const&)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");
    msg(typeid(ExPolicy).name(), typeid(Tval).name(), typeid(Op).name(), async);
    std::cout << "\n";

    // vector of values, and keys
    std::vector<Tval> values, o_values;
    std::vector<Tkey> keys, o_keys;
    //
    values.assign(HPX_SORT_BY_KEY_TEST_SIZE, 0);
    keys.assign(HPX_SORT_BY_KEY_TEST_SIZE, 0);

    // generate a sequence as the values
    std::iota(values.begin(), values.end(), 0);
    // generate a sequence as the keys
    std::iota(keys.begin(), keys.end(), 0);

    // shuffle the keys up,
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    // make copies of initial states
    o_keys = keys;
    o_values = values;

    // sort_by_key, blocking when seq, par, par_vec
    auto fresult =
        hpx::experimental::sort_by_key(std::forward<ExPolicy>(policy),
            keys.begin(), keys.end(), values.begin());
    fresult.get();

    // after sorting by key, the values should be equal to the original keys
    bool is_equal = std::equal(keys.begin(), keys.end(), o_values.begin());
    if (is_equal)
    {
        //std::cout << "Test Passed\n";
    }
    else
    {
        debug::output("keys     ", o_keys);
        debug::output("values   ", o_values);
        debug::output("key range", keys);
        debug::output("val range", values);
        throw std::string("Problem");
    }
    HPX_TEST(is_equal);
}

////////////////////////////////////////////////////////////////////////////////
void test_sort_by_key1()
{
    using namespace hpx::execution;
    //
    // run many tests in a loop for N seconds just to play safe
    //
    int const seconds = 1;
    //
    hpx::chrono::high_resolution_timer t;
    do
    {
        //
        test_sort_by_key1(seq, int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        test_sort_by_key1(par, int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        test_sort_by_key1(par_unseq, int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        //
        test_sort_by_key1(seq, int(), double(), std::equal_to<double>(),
            [](int key) { return key; });
        test_sort_by_key1(par, int(), double(), std::equal_to<double>(),
            [](int key) { return key; });
        test_sort_by_key1(par_unseq, int(), double(), std::equal_to<double>(),
            [](int key) { return key; });
        // custom compare
        test_sort_by_key1(
            seq, double(), double(),
            [](double a, double b) {
                return std::floor(a) == std::floor(b);
            },    //-V550
            [](double a) { return std::floor(a); });
        test_sort_by_key1(
            par, double(), double(),
            [](double a, double b) {
                return std::floor(a) == std::floor(b);
            },    //-V550
            [](double a) { return std::floor(a); });
        test_sort_by_key1(
            par_unseq, double(), double(),
            [](double a, double b) {
                return std::floor(a) == std::floor(b);
            },    //-V550
            [](double a) { return std::floor(a); });
    } while (t.elapsed() < seconds);
    //
    hpx::chrono::high_resolution_timer t2;
    do
    {
        test_sort_by_key_async(seq(task), int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        test_sort_by_key_async(par(task), int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        //
        test_sort_by_key_async(seq(task), int(), double(),
            std::equal_to<double>(), [](int key) { return key; });
        test_sort_by_key_async(par(task), int(), double(),
            std::equal_to<double>(), [](int key) { return key; });
    } while (t2.elapsed() < seconds);
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_sort_by_key1();
    sort_by_key_benchmark();

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

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
