//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Exercises the blockwise distribution engine directly with small block sizes
// and bucket counts, so that the boundary cases (partial head/tail blocks,
// blocks spilling into the next bucket, the overflow block, single-bucket
// inputs) are hit far more often than through hpx::radix_sort.

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using hpx::parallel::detail::ips_distribute;
using hpx::parallel::detail::ips_distribute_parallel;
using hpx::parallel::detail::ips_distribution_buffers;
using hpx::parallel::detail::ips_parallel_distribution_buffers;

namespace {

    struct item
    {
        std::size_t bucket;
        std::size_t id;
    };

    enum class distribution
    {
        uniform,
        skewed,
        single,
        last_only,
        two_ends,
    };

    std::vector<item> make_input(
        std::size_t n, std::size_t k, distribution d, std::mt19937_64& gen)
    {
        std::uniform_int_distribution<std::size_t> any(0, k - 1);
        std::uniform_int_distribution<int> percent(0, 99);

        std::vector<item> v(n);
        for (std::size_t i = 0; i != n; ++i)
        {
            std::size_t b = 0;
            switch (d)
            {
            case distribution::uniform:
                b = any(gen);
                break;
            case distribution::skewed:
                b = percent(gen) < 90 ? 0 : any(gen);
                break;
            case distribution::single:
                b = k / 2;
                break;
            case distribution::last_only:
                b = k - 1;
                break;
            case distribution::two_ends:
                b = percent(gen) < 50 ? 0 : k - 1;
                break;
            }
            v[i] = item{b, i};
        }
        return v;
    }

    // Runs every phase of the parallel engine on HPX threads.
    auto const hpx_run = [](std::size_t count, auto&& f) {
        hpx::parallel::execution::bulk_sync_execute(
            hpx::execution::par.executor(), f,
            hpx::util::iterator_range(
                hpx::util::counting_iterator<std::size_t>(0),
                hpx::util::counting_iterator<std::size_t>(count)));
    };

    // threads == 0 selects the sequential engine.
    void check_distribution(std::size_t n, std::size_t k, std::size_t block,
        distribution d, std::mt19937_64& gen, std::size_t threads = 0)
    {
        auto v = make_input(n, k, d, gen);

        std::vector<std::size_t> expected_sizes(k, 0);
        for (auto const& x : v)
            ++expected_sizes[x.bucket];

        auto classify = [](item const& x) { return x.bucket; };
        std::vector<std::size_t> bucket_start;
        bool empty = true;
        if (threads == 0)
        {
            ips_distribution_buffers<item> buf(k, block);
            ips_distribute(v.begin(), n, classify, buf);
            bucket_start = buf.bucket_start;

            empty = buf.overflow.empty();
            for (auto const& bucket : buf.buckets)
                empty = empty && bucket.empty();
        }
        else
        {
            ips_parallel_distribution_buffers<item> buf(threads, k, block);
            ips_distribute_parallel(v.begin(), n, classify, buf, hpx_run);
            bucket_start = buf.bucket_start;

            empty = buf.overflow.empty();
            for (auto const& s : buf.spill)
                empty = empty && s.empty();
            for (auto const& worker : buf.workers)
            {
                for (auto const& bucket : worker.buckets)
                    empty = empty && bucket.empty();
            }
        }

        // Boundaries match the bucket sizes.
        HPX_TEST_EQ(bucket_start[0], std::size_t(0));
        HPX_TEST_EQ(bucket_start[k], n);
        for (std::size_t b = 0; b != k; ++b)
        {
            HPX_TEST_EQ(
                bucket_start[b + 1] - bucket_start[b], expected_sizes[b]);
        }

        // Every element sits in its bucket's range.
        bool all_in_place = true;
        for (std::size_t b = 0; b != k; ++b)
        {
            for (std::size_t i = bucket_start[b]; i != bucket_start[b + 1]; ++i)
            {
                all_in_place = all_in_place && v[i].bucket == b;
            }
        }
        HPX_TEST_MSG(all_in_place, "element outside of its bucket");

        // The output is a permutation of the input.
        std::vector<std::size_t> ids;
        ids.reserve(n);
        for (auto const& x : v)
            ids.push_back(x.id);
        std::sort(ids.begin(), ids.end());
        bool is_permutation = true;
        for (std::size_t i = 0; i != n; ++i)
            is_permutation = is_permutation && ids[i] == i;
        HPX_TEST_MSG(is_permutation, "elements lost or duplicated");

        // The buffers are left empty for reuse.
        HPX_TEST(empty);
    }

    void test_small_parameters(std::mt19937_64& gen)
    {
        for (distribution d :
            {distribution::uniform, distribution::skewed, distribution::single,
                distribution::last_only, distribution::two_ends})
        {
            for (std::size_t k : {1, 2, 3, 4, 7, 16})
            {
                for (std::size_t block : {1, 2, 3, 4, 5, 8})
                {
                    for (std::size_t n = 0; n != 100; ++n)
                        check_distribution(n, k, block, d, gen);
                    check_distribution(1000, k, block, d, gen);
                    check_distribution(1013, k, block, d, gen);
                }
            }
        }
    }

    void test_parallel(std::mt19937_64& gen)
    {
        for (distribution d :
            {distribution::uniform, distribution::skewed, distribution::single,
                distribution::last_only, distribution::two_ends})
        {
            for (std::size_t threads : {1, 2, 3, 8})
            {
                for (std::size_t k : {1, 3, 16})
                {
                    for (std::size_t block : {1, 3, 8})
                    {
                        for (std::size_t n : {0, 1, 5, 17, 64, 1000, 4099})
                            check_distribution(n, k, block, d, gen, threads);
                    }
                }
                check_distribution(100003, 256, 128, d, gen, threads);
            }
        }
    }

    void test_realistic_parameters(std::mt19937_64& gen)
    {
        for (distribution d : {distribution::uniform, distribution::skewed,
                 distribution::two_ends})
        {
            check_distribution(100000, 256, 128, d, gen);
            check_distribution(100003, 256, 128, d, gen);
        }
    }

    // The engine only requires move construction and move assignment.
    void test_move_only()
    {
        std::size_t const n = 1000;
        std::size_t const k = 5;

        std::vector<std::unique_ptr<std::size_t>> v;
        v.reserve(n);
        for (std::size_t i = 0; i != n; ++i)
            v.push_back(std::make_unique<std::size_t>((i * 7919) % n));

        ips_distribution_buffers<std::unique_ptr<std::size_t>> buf(k, 3);
        ips_distribute(
            v.begin(), n,
            [](std::unique_ptr<std::size_t> const& p) { return *p % k; }, buf);

        bool ok = true;
        std::vector<std::size_t> values;
        for (std::size_t b = 0; b != k; ++b)
        {
            for (std::size_t i = buf.bucket_start[b];
                i != buf.bucket_start[b + 1]; ++i)
            {
                ok = ok && v[i] && *v[i] % k == b;
                if (v[i])
                    values.push_back(*v[i]);
            }
        }
        HPX_TEST(ok);

        std::sort(values.begin(), values.end());
        HPX_TEST_EQ(values.size(), n);
        for (std::size_t i = 0; i != values.size(); ++i)
            HPX_TEST_EQ(values[i], i);
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::mt19937_64 gen(seed);

    test_small_parameters(gen);
    test_realistic_parameters(gen);
    test_parallel(gen);
    test_move_only();

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
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
