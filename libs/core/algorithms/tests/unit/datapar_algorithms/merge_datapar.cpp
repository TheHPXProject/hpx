//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

// sorted input of the given size with values in [low, low + range)
template <typename T, typename Comp>
std::vector<T> make_input(
    std::size_t size, int low, int range, Comp const& comp)
{
    std::uniform_int_distribution<int> dist(low, low + range - 1);
    std::vector<T> v(size);
    for (auto& x : v)
        x = static_cast<T>(dist(gen));
    std::sort(v.begin(), v.end(), comp);
    return v;
}

template <typename ExPolicy, typename F>
decltype(auto) get_result(ExPolicy const&, F&& f)
{
    if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
        return f().get();
    else
        return f();
}

///////////////////////////////////////////////////////////////////////////////
// merge two inputs with hpx::merge and hpx::ranges::merge and compare against
// std::merge; for integral types equal elements can't be told apart, so the
// results have to be identical
template <typename ExPolicy, typename T, typename Comp>
void test_merge_inputs(ExPolicy const& policy, std::vector<T> const& v1,
    std::vector<T> const& v2, Comp comp)
{
    std::size_t const size = v1.size() + v2.size();

    std::vector<T> expected(size);
    std::merge(
        v1.begin(), v1.end(), v2.begin(), v2.end(), expected.begin(), comp);

    {
        std::vector<T> dest(size);
        auto result = get_result(policy, [&] {
            return hpx::merge(policy, v1.begin(), v1.end(), v2.begin(),
                v2.end(), dest.begin(), comp);
        });
        HPX_TEST(result == dest.end());
        HPX_TEST(dest == expected);
    }

    {
        std::vector<T> dest(size);
        auto result = get_result(policy, [&] {
            return hpx::ranges::merge(policy, v1, v2, dest.begin(), comp);
        });
        HPX_TEST(result.in1 == v1.end());
        HPX_TEST(result.in2 == v2.end());
        HPX_TEST(result.out == dest.end());
        HPX_TEST(dest == expected);
    }
}

template <typename ExPolicy, typename T, typename Comp>
void test_merge_sizes(ExPolicy const& policy, Comp comp)
{
    // sizes around the pack widths of all targets (2 to 64 lanes)
    std::size_t const sizes1[] = {0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32,
        33, 63, 64, 65, 67, 100, 1000, 100003};
    std::size_t const sizes2[] = {0, 1, 5, 16, 33, 1000};

    for (std::size_t size1 : sizes1)
    {
        for (std::size_t size2 : sizes2)
        {
            // interleaved, with many equal keys
            test_merge_inputs(policy, make_input<T>(size1, 0, 4, comp),
                make_input<T>(size2, 0, 4, comp), comp);

            // interleaved, few equal keys
            test_merge_inputs(policy, make_input<T>(size1, 0, 100, comp),
                make_input<T>(size2, 0, 100, comp), comp);

            // partly overlapping, in both orders
            test_merge_inputs(policy, make_input<T>(size1, 0, 80, comp),
                make_input<T>(size2, 40, 80, comp), comp);
            test_merge_inputs(policy, make_input<T>(size1, 40, 80, comp),
                make_input<T>(size2, 0, 80, comp), comp);
        }
    }
}

template <typename ExPolicy, typename T>
void test_merge_comparators(ExPolicy const& policy)
{
    test_merge_sizes<ExPolicy, T>(policy, std::less<>());
    test_merge_sizes<ExPolicy, T>(policy, std::less<T>());
    test_merge_sizes<ExPolicy, T>(policy, std::greater<>());
    test_merge_sizes<ExPolicy, T>(policy, std::ranges::less());
    test_merge_sizes<ExPolicy, T>(policy, std::ranges::greater());
    test_merge_sizes<ExPolicy, T>(policy, hpx::ranges::less());
}

template <typename ExPolicy>
void test_merge_types(ExPolicy const& policy)
{
    test_merge_comparators<ExPolicy, std::int8_t>(policy);
    test_merge_comparators<ExPolicy, std::int16_t>(policy);
    test_merge_comparators<ExPolicy, std::int32_t>(policy);
    test_merge_comparators<ExPolicy, std::int64_t>(policy);
    test_merge_comparators<ExPolicy, std::uint32_t>(policy);
}

///////////////////////////////////////////////////////////////////////////////
// the default comparators of hpx::merge and hpx::ranges::merge
template <typename ExPolicy>
void test_merge_default_comparator(ExPolicy const& policy)
{
    auto const v1 = make_input<int>(10007, 0, 1000, std::less<>());
    auto const v2 = make_input<int>(3001, 0, 1000, std::less<>());

    std::vector<int> expected(v1.size() + v2.size());
    std::merge(v1.begin(), v1.end(), v2.begin(), v2.end(), expected.begin());

    std::vector<int> dest(expected.size());
    get_result(policy, [&] {
        return hpx::merge(
            policy, v1.begin(), v1.end(), v2.begin(), v2.end(), dest.begin());
    });
    HPX_TEST(dest == expected);

    std::vector<int> dest_rng(expected.size());
    get_result(policy,
        [&] { return hpx::ranges::merge(policy, v1, v2, dest_rng.begin()); });
    HPX_TEST(dest_rng == expected);
}

///////////////////////////////////////////////////////////////////////////////
// cases that take the scalar path; it has to be stable
template <typename ExPolicy>
void test_merge_fallbacks(ExPolicy const& policy)
{
    // floating point: -0.0 and +0.0 are equal, but distinguishable
    {
        std::vector<double> v1(1000, -0.0);
        std::vector<double> v2(1000, 0.0);
        std::vector<double> dest(v1.size() + v2.size());
        get_result(policy, [&] {
            return hpx::merge(policy, v1.begin(), v1.end(), v2.begin(),
                v2.end(), dest.begin(), std::less<>());
        });
        for (std::size_t i = 0; i != dest.size(); ++i)
        {
            HPX_TEST_EQ(std::signbit(dest[i]), i < v1.size());
        }
    }

    // a comparator the vector path doesn't know
    {
        auto comp = [](int lhs, int rhs) { return lhs < rhs; };
        test_merge_inputs(policy, make_input<int>(1000, 0, 100, comp),
            make_input<int>(777, 0, 100, comp), comp);
    }

    // a projection: equal keys of different elements keep their order
    {
        auto proj = [](int x) { return x < 0 ? -x : x; };
        auto comp = [&](int lhs, int rhs) { return proj(lhs) < proj(rhs); };

        auto v1 = make_input<int>(1000, -20, 41, comp);
        auto v2 = make_input<int>(777, -20, 41, comp);

        std::vector<int> expected(v1.size() + v2.size());
        std::ranges::merge(
            v1, v2, expected.begin(), std::ranges::less(), proj, proj);

        std::vector<int> dest(expected.size());
        get_result(policy, [&] {
            return hpx::ranges::merge(
                policy, v1, v2, dest.begin(), std::ranges::less(), proj, proj);
        });
        HPX_TEST(dest == expected);
    }

    // iterators that are not contiguous
    {
        auto const v1 = make_input<int>(1000, 0, 100, std::less<>());
        auto const v2 = make_input<int>(777, 0, 100, std::less<>());
        std::deque<int> const d1(v1.begin(), v1.end());
        std::deque<int> const d2(v2.begin(), v2.end());

        std::vector<int> expected(v1.size() + v2.size());
        std::merge(
            v1.begin(), v1.end(), v2.begin(), v2.end(), expected.begin());

        std::deque<int> dest(expected.size());
        get_result(policy, [&] {
            return hpx::merge(policy, d1.begin(), d1.end(), d2.begin(),
                d2.end(), dest.begin(), std::less<>());
        });
        HPX_TEST(std::equal(dest.begin(), dest.end(), expected.begin()));
    }
}

///////////////////////////////////////////////////////////////////////////////
// which calls take the vector kernel; the backends std::experimental::simd,
// the emulation and EVE provide it, Vc and SVE don't
template <typename ExPolicy, typename T, typename Comp = std::less<>,
    typename Proj = hpx::identity,
    typename Iter = typename std::vector<T>::iterator>
constexpr bool vectorized =
    hpx::parallel::detail::datapar_merge_applies<ExPolicy, Iter, Iter, Iter,
        Comp, Proj, Proj>();

using simd_policy = hpx::execution::simd_policy;
using par_simd_policy = hpx::execution::par_simd_policy;

#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD) ||                         \
    defined(HPX_HAVE_DATAPAR_EMULATION) || defined(HPX_HAVE_DATAPAR_EVE)
static_assert(vectorized<simd_policy, int>);
static_assert(vectorized<par_simd_policy, std::int8_t>);
static_assert(vectorized<simd_policy, std::uint32_t, std::greater<>>);
static_assert(vectorized<simd_policy, std::int64_t, hpx::ranges::less>);
static_assert(vectorized<simd_policy, int, std::ranges::less, std::identity>);
static_assert(vectorized<
    std::decay_t<decltype(hpx::execution::simd(hpx::execution::task))>, int>);
#else
static_assert(!vectorized<simd_policy, int>);
static_assert(!vectorized<par_simd_policy, int>);
#endif

static_assert(!vectorized<simd_policy, double>);
static_assert(!vectorized<simd_policy, bool>);
static_assert(!vectorized<simd_policy, int, std::less_equal<>>);
static_assert(!vectorized<simd_policy, int, std::less<>, int (*)(int)>);
static_assert(!vectorized<simd_policy, int, std::less<>, hpx::identity,
    std::deque<int>::iterator>);

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_merge(ExPolicy const& policy)
{
    test_merge_types(policy);
    test_merge_default_comparator(policy);
    test_merge_fallbacks(policy);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        gen.seed(seed);
    }
    std::cout << "using seed: " << seed << std::endl;

    using namespace hpx::execution;

    test_merge(simd);
    test_merge(par_simd);
    test_merge(simd(task));
    test_merge(par_simd(task));

    test_merge(fixed_size_simd<1>);
    test_merge(fixed_size_simd<4>);
    test_merge(par_fixed_size_simd<8>);
    test_merge(fixed_size_simd<16>(task));

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
