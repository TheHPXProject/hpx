//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

    template <typename T, typename Gen>
    std::vector<T> random_vec(std::size_t n, Gen& gen)
    {
        // uniform_int_distribution is not defined for char-sized types, so
        // draw 64-bit values and truncate.
        std::uniform_int_distribution<std::uint64_t> dist;
        std::vector<T> v(n);
        for (auto& x : v)
            x = static_cast<T>(dist(gen));
        return v;
    }

    template <typename T>
    void check_sorted_against_std(std::vector<T> v)
    {
        auto reference = v;
        std::sort(reference.begin(), reference.end());
        hpx::radix_sort(v.begin(), v.end());
        HPX_TEST(v == reference);
    }

    template <typename T, typename ExPolicy>
    void check_sorted_against_std(ExPolicy&& policy, std::vector<T> v)
    {
        auto reference = v;
        std::sort(reference.begin(), reference.end());
        hpx::radix_sort(std::forward<ExPolicy>(policy), v.begin(), v.end());
        HPX_TEST(v == reference);
    }

    template <typename T>
    void test_random(std::size_t n, std::mt19937_64& gen)
    {
        check_sorted_against_std<T>(random_vec<T>(n, gen));
        check_sorted_against_std<T>(hpx::execution::seq, random_vec<T>(n, gen));
        check_sorted_against_std<T>(hpx::execution::par, random_vec<T>(n, gen));
    }

    void test_edge_cases()
    {
        check_sorted_against_std<int>(std::vector<int>{});
        check_sorted_against_std<int>(std::vector<int>{42});
        check_sorted_against_std<int>(std::vector<int>{3, 1, 2});

        // Already sorted.
        std::vector<std::uint32_t> sorted_input(1024);
        for (std::size_t i = 0; i < sorted_input.size(); ++i)
            sorted_input[i] = static_cast<std::uint32_t>(i);
        check_sorted_against_std<std::uint32_t>(sorted_input);

        // Reverse-sorted.
        std::vector<std::uint32_t> reverse_input(1024);
        for (std::size_t i = 0; i < reverse_input.size(); ++i)
            reverse_input[i] =
                static_cast<std::uint32_t>(reverse_input.size() - 1 - i);
        check_sorted_against_std<std::uint32_t>(reverse_input);

        // All equal -- exercises the lazy pass-skipping path.
        check_sorted_against_std<std::uint32_t>(
            std::vector<std::uint32_t>(1024, 7u));

        // Signed inputs spanning both halves of the integer range.
        std::vector<std::int32_t> mixed = {-5, 100, 0, -1, 1,
            (std::numeric_limits<std::int32_t>::min)(),
            (std::numeric_limits<std::int32_t>::max)(), -100};
        check_sorted_against_std<std::int32_t>(mixed);
    }

    void test_projection()
    {
        struct item
        {
            std::int64_t key;
            int payload;
            bool operator==(item const&) const = default;
        };

        std::mt19937_64 gen{12345};
        std::uniform_int_distribution<std::int64_t> dist(-1000, 1000);

        std::vector<item> v;
        v.reserve(2048);
        for (int i = 0; i < 2048; ++i)
            v.push_back({dist(gen), i});

        auto reference = v;
        std::stable_sort(reference.begin(), reference.end(),
            [](item const& a, item const& b) { return a.key < b.key; });

        hpx::radix_sort(
            v.begin(), v.end(), [](item const& it) { return it.key; });

        // radix_sort is unstable, so compare only the key sequence and make
        // sure every payload is still present.
        for (std::size_t i = 0; i < v.size(); ++i)
            HPX_TEST_EQ(v[i].key, reference[i].key);

        std::vector<int> payloads;
        payloads.reserve(v.size());
        for (auto const& it : v)
            payloads.push_back(it.payload);
        std::sort(payloads.begin(), payloads.end());
        for (std::size_t i = 0; i < payloads.size(); ++i)
            HPX_TEST_EQ(payloads[i], static_cast<int>(i));
    }

    // Large enough to go through the parallel distribution engine and the
    // task-based recursion.
    template <typename T>
    void test_parallel_large(std::mt19937_64& gen)
    {
        using U = std::make_unsigned_t<T>;
        std::size_t const n = std::size_t(1) << 21;

        check_sorted_against_std<T>(hpx::execution::par, random_vec<T>(n, gen));

        // Most keys share the top digit, so one bucket holds ~90% of the
        // elements and recurses in parallel.
        auto skewed = random_vec<T>(n, gen);
        for (auto& x : skewed)
        {
            if (gen() % 10 != 0)
                x = static_cast<T>(static_cast<U>(x) & (U(~U(0)) >> 8));
        }
        check_sorted_against_std<T>(hpx::execution::par, skewed);

        // Few distinct keys: every level but the last is a single bucket.
        std::vector<T> few(n);
        for (auto& x : few)
            x = static_cast<T>(gen() % 4);
        check_sorted_against_std<T>(hpx::execution::par, few);

        check_sorted_against_std<T>(
            hpx::execution::par, std::vector<T>(n, T(5)));

        // Already sorted input returns after the key summary pass. The near
        // misses must still be sorted: one element out of place at the end,
        // and two sorted halves in the wrong order, where only the boundary
        // between the parallel summary chunks is unsorted.
        std::vector<T> sorted_input = random_vec<T>(n, gen);
        std::sort(sorted_input.begin(), sorted_input.end());
        check_sorted_against_std<T>(hpx::execution::seq, sorted_input);
        check_sorted_against_std<T>(hpx::execution::par, sorted_input);

        auto last_unsorted = sorted_input;
        last_unsorted.back() = (std::numeric_limits<T>::min)();
        check_sorted_against_std<T>(hpx::execution::seq, last_unsorted);
        check_sorted_against_std<T>(hpx::execution::par, last_unsorted);

        auto swapped_halves = sorted_input;
        std::rotate(swapped_halves.begin(),
            swapped_halves.begin() + static_cast<std::ptrdiff_t>(n / 2),
            swapped_halves.end());
        check_sorted_against_std<T>(hpx::execution::seq, swapped_halves);
        check_sorted_against_std<T>(hpx::execution::par, swapped_halves);

        // Narrow key range away from zero: the leading digits are skipped.
        std::vector<T> narrow(n);
        T const base = static_cast<T>((std::numeric_limits<T>::max)() / 3);
        for (auto& x : narrow)
            x = static_cast<T>(base + static_cast<T>(gen() % 1000));
        check_sorted_against_std<T>(hpx::execution::seq, narrow);
        check_sorted_against_std<T>(hpx::execution::par, narrow);

        // For signed keys, a small range around zero spans the sign flip of
        // the lifted key.
        if constexpr (std::is_signed_v<T>)
        {
            std::vector<T> around_zero(n);
            for (auto& x : around_zero)
                x = static_cast<T>(static_cast<int>(gen() % 1001) - 500);
            check_sorted_against_std<T>(hpx::execution::seq, around_zero);
            check_sorted_against_std<T>(hpx::execution::par, around_zero);
        }

        // Asynchronous execution.
        auto v = random_vec<T>(n, gen);
        auto reference = v;
        std::sort(reference.begin(), reference.end());
        auto f = hpx::radix_sort(
            hpx::execution::par(hpx::execution::task), v.begin(), v.end());
        f.get();
        HPX_TEST(v == reference);
    }

    void test_parallel_projection(std::mt19937_64& gen)
    {
        struct item
        {
            std::uint32_t key;
            std::uint32_t payload;
        };

        std::size_t const n = std::size_t(1) << 20;
        std::vector<item> v(n);
        for (std::size_t i = 0; i != n; ++i)
            v[i] = item{static_cast<std::uint32_t>(gen()),
                static_cast<std::uint32_t>(i)};

        hpx::radix_sort(hpx::execution::par, v.begin(), v.end(),
            [](item const& it) { return it.key; });

        bool sorted = true;
        for (std::size_t i = 1; i < n; ++i)
            sorted = sorted && v[i - 1].key <= v[i].key;
        HPX_TEST(sorted);

        std::vector<std::uint32_t> payloads(n);
        for (std::size_t i = 0; i != n; ++i)
            payloads[i] = v[i].payload;
        std::sort(payloads.begin(), payloads.end());
        bool complete = true;
        for (std::size_t i = 0; i != n; ++i)
            complete = complete && payloads[i] == i;
        HPX_TEST(complete);
    }

}    // namespace

// bool keys have no meaningful radix decomposition and must be rejected.
static_assert(!std::is_invocable_v<hpx::radix_sort_t, bool*, bool*>);
static_assert(std::is_invocable_v<hpx::radix_sort_t, int*, int*>);

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::mt19937_64 gen(seed);

    test_edge_cases();
    test_projection();

    for (std::size_t n : {std::size_t{0}, std::size_t{1}, std::size_t{17},
             std::size_t{4096}, std::size_t{200000}})
    {
        test_random<std::uint32_t>(n, gen);
        test_random<std::int32_t>(n, gen);
        test_random<std::uint64_t>(n, gen);
        test_random<std::int64_t>(n, gen);
        test_random<std::uint8_t>(n, gen);
        test_random<std::int8_t>(n, gen);
        test_random<std::int16_t>(n, gen);
    }

    test_parallel_large<std::uint32_t>(gen);
    test_parallel_large<std::int32_t>(gen);
    test_parallel_large<std::uint64_t>(gen);
    test_parallel_large<std::int64_t>(gen);
    test_parallel_large<std::uint16_t>(gen);
    test_parallel_projection(gen);

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
