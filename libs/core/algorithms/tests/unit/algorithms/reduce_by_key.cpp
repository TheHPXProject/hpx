//  Copyright (c) 2015-2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <iomanip>
#include <random>
#include <utility>
#include <vector>
#ifdef EXTRA_DEBUG
#include <cstddef>
#include <iostream>
#include <string>
#endif
//
#define HPX_REDUCE_BY_KEY_TEST_SIZE (1 << 18)
//
#include "sort_tests.hpp"
//
#define EXTRA_DEBUG
//
namespace debug {
    template <typename T>
    void output(std::string const& name, std::vector<T> const& v)
    {
#ifdef EXTRA_DEBUG
        std::cout << name.c_str() << "\t : {" << v.size() << "} : ";
        std::copy(std::begin(v), std::end(v),
            std::ostream_iterator<T>(std::cout, ", "));
        std::cout << "\n";
#endif
    }

    template <typename Iter>
    void output(std::string const& name, Iter begin, Iter end)
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

struct almost_equal
{
    template <typename T>
    bool operator()(T lhs, T rhs) const
    {
        return lhs == rhs;
    }
    bool operator()(float lhs, float rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10f;
    }
    bool operator()(double lhs, double rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10;
    }
    template <typename T>
    bool operator()(T lhs, double rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10;
    }
    template <typename T>
    bool operator()(double lhs, T rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10;
    }
    template <typename T>
    bool operator()(T lhs, float rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10f;
    }
    template <typename T>
    bool operator()(float lhs, T rhs) const
    {
        return std::abs(lhs - rhs) < 1e-10;
    }
};

////////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename ExPolicy, typename Tkey, typename Tval, typename Op,
    typename HelperOp>
void test_reduce_by_key1(ExPolicy&& policy, Tkey, Tval, bool benchmark,
    Op const& op, HelperOp const& ho)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");
    msg(typeid(ExPolicy).name(), typeid(Tval).name(), typeid(Op).name(), sync);
    std::cout << "\n";

    Tval rnd_min = -256;
    Tval rnd_max = 256;

    // vector of values, and keys
    std::vector<Tval> values, o_values;
    std::vector<Tkey> keys, o_keys;
    values.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);
    keys.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);

    std::vector<Tval> check_values;

    // use the default random engine and an uniform distribution for values
    std::mt19937 eng(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distr(rnd_min, rnd_max);

    // use the default random engine and an uniform distribution for keys
    std::mt19937 engk(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distrk(0, 256);

    // generate test data
    int keysize = 0;
    Tkey key = 0, helperkey = 0, lastkey = 0;
    for (/* */; keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;)
    {
        do
        {
            key = static_cast<Tkey>(distrk(engk));
        } while (ho(key) == lastkey);
        helperkey = ho(key);
        lastkey = helperkey;
        //
        int numkeys = static_cast<int>(distrk(engk)) + 1;
        //
        Tval sum = 0;
        for (int i = 0; i < numkeys && keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;
            ++i)
        {
            Tval value = static_cast<Tval>(distr(eng));
            keys.push_back(key);
            values.push_back(value);
            sum += value;
            keysize++;
        }
        check_values.push_back(sum);
    }
    o_values = values;
    o_keys = keys;

    hpx::chrono::high_resolution_timer t;
    // reduce_by_key, blocking when seq, par, par_vec
    auto result = hpx::experimental::reduce_by_key(
        std::forward<ExPolicy>(policy), keys.begin(), keys.end(),
        values.begin(), keys.begin(), values.begin(), op);
    double elapsed = t.elapsed();

    bool is_equal = std::equal(
        values.begin(), result.out, check_values.begin(), almost_equal());
    HPX_TEST(is_equal);
    if (is_equal)
    {
        if (benchmark)
        {
            // CDash graph plotting
            hpx::util::print_cdash_timing("ReduceByKeyTime", elapsed);
        }
    }
    else
    {
//         debug::output("keys     ", o_keys);
//         debug::output("values   ", o_values);
//         debug::output("key range", keys.begin(), result.in);
//         debug::output("val range", values.begin(), result.out);
//         debug::output("expected ", check_values);
//         throw std::string("Problem");
#if defined(EXTRA_DEBUG)
        for (std::size_t i = 0; i != check_values.size(); ++i)
        {
            if (!almost_equal()(values[i], check_values[i]))
            {
                std::cout << i << ": " << values[i] << " != " << check_values[i]
                          << "\n";
            }
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Tkey, typename Tval, typename Op,
    typename HelperOp>
void test_reduce_by_key_const(ExPolicy&& policy, Tkey, Tval, bool benchmark,
    Op const& op, HelperOp const& ho)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");
    msg(typeid(ExPolicy).name(), typeid(Tval).name(), typeid(Op).name(), sync);
    std::cout << "\n";

    Tval rnd_min = -256;
    Tval rnd_max = 256;

    // vector of values, and keys
    std::vector<Tval> values, o_values;
    std::vector<Tkey> keys, o_keys;
    values.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);
    keys.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);

    std::vector<Tval> check_values;

    // use the default random engine and an uniform distribution for values
    std::mt19937 eng(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distr(rnd_min, rnd_max);

    // use the default random engine and an uniform distribution for keys
    std::mt19937 engk(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distrk(0, 256);

    // generate test data
    int keysize = 0;
    Tkey key = 0, helperkey = 0, lastkey = 0;
    for (/* */; keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;)
    {
        do
        {
            key = static_cast<Tkey>(distrk(engk));
        } while (ho(key) == lastkey);
        helperkey = ho(key);
        lastkey = helperkey;
        //
        int numkeys = static_cast<int>(distrk(engk)) + 1;
        //
        Tval sum = 0;
        for (int i = 0; i < numkeys && keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;
            ++i)
        {
            Tval value = static_cast<Tval>(distr(eng));
            keys.push_back(key);
            values.push_back(value);
            sum += value;
            keysize++;
        }
        check_values.push_back(sum);
    }
    o_values = values;
    o_keys = keys;

    std::vector<Tkey> const const_keys(keys.begin(), keys.end());
    std::vector<Tval> const const_values(values.begin(), values.end());

    hpx::chrono::high_resolution_timer t;
    // reduce_by_key, blocking when seq, par, par_vec
    auto result = hpx::experimental::reduce_by_key(
        std::forward<ExPolicy>(policy), const_keys.begin(), const_keys.end(),
        const_values.begin(), keys.begin(), values.begin(), op);
    double elapsed = t.elapsed();

    bool is_equal = std::equal(
        values.begin(), result.out, check_values.begin(), almost_equal());
    HPX_TEST(is_equal);
    if (is_equal)
    {
        if (benchmark)
        {
            // CDash graph plotting
            hpx::util::print_cdash_timing("ReduceByKeyTime", elapsed);
        }
    }
    else
    {
//         debug::output("keys     ", o_keys);
//         debug::output("values   ", o_values);
//         debug::output("key range", keys.begin(), result.in);
//         debug::output("val range", values.begin(), result.out);
//         debug::output("expected ", check_values);
//         throw std::string("Problem");
#if defined(EXTRA_DEBUG)
        for (std::size_t i = 0; i != check_values.size(); ++i)
        {
            if (values[i] != check_values[i])
            {
                std::cout << i << ": " << values[i] << " != " << check_values[i]
                          << "\n";
            }
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Tkey, typename Tval, typename Op,
    typename HelperOp>
void test_reduce_by_key_async(
    ExPolicy&& policy, Tkey, Tval, Op const& op, HelperOp const& ho)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");
    msg(typeid(ExPolicy).name(), typeid(Tval).name(), typeid(Op).name(), async);
    std::cout << "\n";

    Tval rnd_min = -256;
    Tval rnd_max = 256;

    // vector of values, and keys
    std::vector<Tval> values, o_values;
    std::vector<Tkey> keys, o_keys;
    values.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);
    keys.reserve(HPX_REDUCE_BY_KEY_TEST_SIZE);

    std::vector<Tval> check_values;

    // use the default random engine and an uniform distribution for values
    std::mt19937 eng(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distr(rnd_min, rnd_max);

    // use the default random engine and an uniform distribution for keys
    std::mt19937 engk(static_cast<unsigned int>(gen()));
    std::uniform_real_distribution<double> distrk(0, 256);

    // generate test data
    int keysize = 0;
    Tkey key = 0, helperkey = 0, lastkey = 0;
    for (/* */; keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;)
    {
        do
        {
            key = static_cast<Tkey>(distrk(engk));
        } while (ho(key) == lastkey);
        helperkey = ho(key);
        lastkey = helperkey;
        //
        int numkeys = static_cast<Tkey>(distrk(engk)) + 1;
        //
        Tval sum = 0;
        for (int i = 0; i < numkeys && keysize < HPX_REDUCE_BY_KEY_TEST_SIZE;
            ++i)
        {
            Tval value = static_cast<Tval>(distr(eng));
            keys.push_back(key);
            values.push_back(value);
            sum += value;
            keysize++;
        }
        check_values.push_back(sum);
    }
    o_values = values;
    o_keys = keys;

    // reduce_by_key, blocking when seq, par, par_vec
    hpx::chrono::high_resolution_timer t;
    auto fresult = hpx::experimental::reduce_by_key(
        std::forward<ExPolicy>(policy), keys.begin(), keys.end(),
        values.begin(), keys.begin(), values.begin(), op);
    double async_seconds = t.elapsed();
    auto result = fresult.get();
    double sync_seconds = t.elapsed();

    std::cout << "Async time " << async_seconds << " Sync time " << sync_seconds
              << "\n";
    bool is_equal = std::equal(
        values.begin(), result.out, check_values.begin(), almost_equal());
    HPX_TEST(is_equal);
    if (is_equal)
    {
        //std::cout << "Test Passed\n";
    }
    else
    {
//         debug::output("keys     ", o_keys);
//         debug::output("values   ", o_values);
//         debug::output("key range", keys.begin(), result.in);
//         debug::output("val range", values.begin(), result.out);
//         debug::output("expected ", check_values);
//         throw std::string("Problem");
#if defined(EXTRA_DEBUG)
        for (std::size_t i = 0; i != check_values.size(); ++i)
        {
            if (values[i] != check_values[i])
            {
                std::cout << i << ": " << values[i] << " != " << check_values[i]
                          << "\n";
            }
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////
void test_reduce_by_key1()
{
    using namespace hpx::execution;
    //
    hpx::chrono::high_resolution_timer t;
    do
    {
        test_reduce_by_key1(seq, int(), int(), false, std::equal_to<int>(),
            [](int key) { return key; });
        test_reduce_by_key1(par, int(), int(), false, std::equal_to<int>(),
            [](int key) { return key; });
        test_reduce_by_key1(par_unseq, int(), int(), false,
            std::equal_to<int>(), [](int key) { return key; });
        //
        // default comparison operator (std::equal_to)
        test_reduce_by_key1(seq, int(), double(), false, almost_equal(),
            [](int key) { return key; });
        test_reduce_by_key1(par, int(), double(), false, almost_equal(),
            [](int key) { return key; });
        test_reduce_by_key1(par_unseq, int(), double(), false, almost_equal(),
            [](int key) { return key; });
        //
        //
        test_reduce_by_key1(
            seq, double(), double(), false,
            [](double a, double b) {
                return std::abs(std::floor(a) - std::floor(b)) < 1e-15;
            },    //-V550
            [](double a) { return std::floor(a); });
        test_reduce_by_key1(
            par, double(), double(), false,
            [](double a, double b) {
                return std::abs(std::floor(a) - std::floor(b)) < 1e-15;
            },    //-V550
            [](double a) { return std::floor(a); });
        test_reduce_by_key1(
            par_unseq, double(), double(), false,
            [](double a, double b) {
                return std::abs(std::floor(a) - std::floor(b)) < 1e-15;
            },    //-V550
            [](double a) { return std::floor(a); });
    } while (t.elapsed() < 2);
    //
    hpx::chrono::high_resolution_timer t3;
    do
    {
        test_reduce_by_key_const(seq, int(), int(), false, std::equal_to<int>(),
            [](int key) { return key; });
        //
        // default comparison operator (std::equal_to)
        test_reduce_by_key_const(seq, int(), double(), false, almost_equal(),
            [](int key) { return key; });
        //
        test_reduce_by_key_const(
            seq, double(), double(), false,
            [](double a, double b) {
                return std::abs(std::floor(a) - std::floor(b)) < 1e-15;
            },    //-V550
            [](double a) { return std::floor(a); });
    } while (t3.elapsed() < 0.5);
    //
    hpx::chrono::high_resolution_timer t2;
    do
    {
        test_reduce_by_key_async(seq(task), int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        test_reduce_by_key_async(par(task), int(), int(), std::equal_to<int>(),
            [](int key) { return key; });
        //
        test_reduce_by_key_async(seq(task), int(), double(), almost_equal(),
            [](int key) { return key; });
        test_reduce_by_key_async(par(task), int(), double(), almost_equal(),
            [](int key) { return key; });
    } while (t2.elapsed() < 2);

    // one last test with timing output enabled
    test_reduce_by_key1(
        par, double(), double(), true,
        [](double a, double b) {
            return std::abs(std::floor(a) - std::floor(b)) < 1e-15;
        },    //-V550
        [](double a) { return std::floor(a); });
}

////////////////////////////////////////////////////////////////////////////////
// Edge-case tests for hpx::experimental::reduce_by_key
//
// The main test suite only exercises large (1<<18) random inputs.
// The following tests cover boundary conditions that stress the internal
// merge / scan stages when key groups are degenerate.

// Edge 1: empty input - result iterators must equal the begin of output
void test_reduce_by_key_empty()
{
    using namespace hpx::execution;

    std::vector<int> keys, values;
    std::vector<int> out_keys(1, -1), out_vals(1, -1);

    // seq
    {
        auto r = hpx::experimental::reduce_by_key(seq, keys.begin(), keys.end(),
            values.begin(), out_keys.begin(), out_vals.begin());
        // Nothing written - result iterators must equal the output begin
        HPX_TEST(r.in == out_keys.begin());
        HPX_TEST(r.out == out_vals.begin());
    }

    // par
    {
        auto r = hpx::experimental::reduce_by_key(par, keys.begin(), keys.end(),
            values.begin(), out_keys.begin(), out_vals.begin());
        HPX_TEST(r.in == out_keys.begin());
        HPX_TEST(r.out == out_vals.begin());
    }
}

// Edge 2: all keys identical - single output group, value == sum of all inputs
void test_reduce_by_key_single_group()
{
    using namespace hpx::execution;

    std::vector<int> keys = {5, 5, 5, 5, 5};
    std::vector<int> values = {10, 20, 30, 40, 50};    // sum = 150
    std::vector<int> out_k(5, 0), out_v(5, 0);

    // Run under par and verify correctness
    auto r = hpx::experimental::reduce_by_key(par, keys.begin(), keys.end(),
        values.begin(), out_k.begin(), out_v.begin(), std::equal_to<int>());

    // Exactly one output group
    auto n_groups = std::distance(out_v.begin(), r.out);
    HPX_TEST_EQ(n_groups, std::ptrdiff_t(1));
    HPX_TEST_EQ(out_k[0], 5);
    HPX_TEST_EQ(out_v[0], 150);

    // Verify seq produces the same result
    std::vector<int> out_k2(5, 0), out_v2(5, 0);
    auto r2 = hpx::experimental::reduce_by_key(seq, keys.begin(), keys.end(),
        values.begin(), out_k2.begin(), out_v2.begin(), std::equal_to<int>());
    HPX_TEST_EQ(std::distance(out_v2.begin(), r2.out), n_groups);
    HPX_TEST_EQ(out_k2[0], out_k[0]);
    HPX_TEST_EQ(out_v2[0], out_v[0]);
}

// Edge 3: all keys unique - N output groups, each value equals its input
void test_reduce_by_key_all_unique()
{
    using namespace hpx::execution;

    std::vector<int> keys = {1, 2, 3, 4, 5};
    std::vector<int> values = {10, 20, 30, 40, 50};
    std::vector<int> out_k(5, 0), out_v(5, 0);

    auto r = hpx::experimental::reduce_by_key(par, keys.begin(), keys.end(),
        values.begin(), out_k.begin(), out_v.begin(), std::equal_to<int>());

    // All 5 groups must be emitted
    auto n_groups = std::distance(out_v.begin(), r.out);
    HPX_TEST_EQ(n_groups, std::ptrdiff_t(5));
    // Each output value must equal the corresponding input value
    bool vals_match =
        std::equal(values.begin(), values.end(), out_v.begin(), almost_equal());
    HPX_TEST(vals_match);
    bool keys_match = std::equal(keys.begin(), keys.end(), out_k.begin());
    HPX_TEST(keys_match);
}

void reduce_by_key_edge_case_test()
{
    test_reduce_by_key_empty();
    test_reduce_by_key_single_group();
    test_reduce_by_key_all_unique();
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    test_reduce_by_key1();
    reduce_by_key_edge_case_test();
    //    test_reduce_by_key2();
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
