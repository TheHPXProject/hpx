//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Compares hpx::radix_sort with hpx::sort for several key types and input
// distributions, sequentially and in parallel. Every timed run first restores
// the unsorted input; the "copy" entries measure that restore alone so it can
// be subtracted.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

namespace {

    struct record
    {
        std::uint64_t key;
        std::uint64_t payload;
    };

    template <typename T>
    std::uint64_t key_of(T const& v)
    {
        if constexpr (std::is_same_v<T, record>)
            return v.key;
        else
            return v;
    }

    template <typename T>
    T make_value(std::uint64_t key, std::uint64_t i)
    {
        if constexpr (std::is_same_v<T, record>)
            return record{key, i};
        else
            return static_cast<T>(key);
    }

    // uniform:  keys uniform over the full key range
    // narrow:   1000 distinct keys, so the leading digits are shared
    // skewed:   90% of the keys below 2^24, the rest uniform
    // sorted:   ascending keys
    template <typename T>
    std::vector<T> make_input(
        std::string const& distribution, std::size_t n, std::mt19937_64& gen)
    {
        std::vector<T> v;
        v.reserve(n);
        for (std::size_t i = 0; i != n; ++i)
        {
            std::uint64_t key = gen();
            if (distribution == "narrow")
                key %= 1000;
            else if (distribution == "skewed" && gen() % 10 != 0)
                key &= 0xff'ffff;
            else if (distribution == "sorted")
                key = i;
            v.push_back(make_value<T>(key, i));
        }
        return v;
    }

    template <typename T>
    void bench_type(std::string const& type_name, std::size_t n, int test_count,
        std::vector<std::string> const& distributions, std::mt19937_64& gen)
    {
        auto const proj = [](T const& v) { return key_of(v); };
        auto const comp = [](T const& a, T const& b) {
            return key_of(a) < key_of(b);
        };

        for (auto const& distribution : distributions)
        {
            std::vector<T> const input = make_input<T>(distribution, n, gen);
            std::vector<T> data = input;
            std::string const name = type_name + " " + distribution;

            hpx::util::perftests_report(
                name + " copy", "seq", test_count, [&] { data = input; });

            hpx::util::perftests_report(name + " sort", "seq", test_count, [&] {
                data = input;
                hpx::sort(hpx::execution::seq, data.begin(), data.end(), comp);
            });
            hpx::util::perftests_report(name + " sort", "par", test_count, [&] {
                data = input;
                hpx::sort(hpx::execution::par, data.begin(), data.end(), comp);
            });

            hpx::util::perftests_report(
                name + " radix_sort", "seq", test_count, [&] {
                    data = input;
                    hpx::radix_sort(
                        hpx::execution::seq, data.begin(), data.end(), proj);
                });
            hpx::util::perftests_report(
                name + " radix_sort", "par", test_count, [&] {
                    data = input;
                    hpx::radix_sort(
                        hpx::execution::par, data.begin(), data.end(), proj);
                });
        }
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    int const test_count = vm["test_count"].as<int>();
    std::size_t const n = vm["vector_size"].as<std::size_t>();
    std::string const distribution = vm["distribution"].as<std::string>();

    if (test_count <= 0)
    {
        std::cerr << "test_count must be positive\n";
        hpx::local::finalize();
        return -1;
    }

    std::vector<std::string> distributions;
    if (distribution == "all")
        distributions = {"uniform", "narrow", "skewed", "sorted"};
    else
        distributions = {distribution};

    hpx::util::perftests_init(vm);

    std::mt19937_64 gen(vm["seed"].as<unsigned int>());
    bench_type<std::uint32_t>("uint32", n, test_count, distributions, gen);
    bench_type<std::uint64_t>("uint64", n, test_count, distributions, gen);
    bench_type<record>("record", n, test_count, distributions, gen);

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("test_count", value<int>()->default_value(10),
            "number of runs to be averaged")
        ("vector_size", value<std::size_t>()->default_value(1000000),
            "number of elements to sort")
        ("distribution", value<std::string>()->default_value("all"),
            "key distribution: uniform, narrow, skewed, sorted or all")
        ("seed", value<unsigned int>()->default_value(42),
            "random number generator seed")
        ;
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all"};

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#endif
