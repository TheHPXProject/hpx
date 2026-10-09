//  Copyright (C) 2020 ETH Zurich
//  Copyright (C) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/barrier.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/optional.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

unsigned int seed = std::random_device{}();

void test_basic_default()
{
    // A default constructed queue should be empty.
    hpx::concurrency::detail::non_contiguous_index_queue<> q;

    HPX_TEST(q.empty());
    HPX_TEST(!q.pop_left());
    HPX_TEST(!q.pop_right());
}

void test_basic(std::uint32_t placement_step)
{
    {
        // Popping from the left should give us the expected indices.
        std::uint32_t first = 3;
        std::uint32_t last = 11;
        std::uint32_t step = placement_step;
        hpx::concurrency::detail::non_contiguous_index_queue<> q{
            first, last, step};

        std::uint32_t count = 0;
        for (std::uint32_t curr_expected = first; curr_expected < last;
            curr_expected += step, ++count)
        {
            hpx::optional<std::uint32_t> curr = q.pop_left();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }
        HPX_TEST_EQ(count, (11u - 3u) / placement_step);

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());
    }

    {
        // Popping from the right should give us the expected indices.
        std::uint32_t first = 3;
        std::uint32_t last = 11;
        std::uint32_t step = placement_step;
        hpx::concurrency::detail::non_contiguous_index_queue<> q{
            first, last, step};

        std::uint32_t count = 0;
        for (std::uint32_t curr_expected = last - step; curr_expected >= first;
            curr_expected -= step, ++count)
        {
            hpx::optional<std::uint32_t> curr = q.pop_right();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }
        HPX_TEST_EQ(count, (11u - 3u) / placement_step);

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());

        // Resetting a queue should give us the same behaviour as a fresh
        // queue.
        q.reset(first, last, step);

        count = 0;
        for (std::uint32_t curr_expected = last - step; curr_expected >= first;
            curr_expected -= step, ++count)
        {
            hpx::optional<std::uint32_t> curr = q.pop_right();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }
        HPX_TEST_EQ(count, (11u - 3u) / placement_step);

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());
    }
}

enum class pop_mode
{
    left,
    right,
    random
};

void test_concurrent_worker(pop_mode m, std::size_t thread_index,
    std::shared_ptr<hpx::barrier<>> b,
    hpx::concurrency::detail::non_contiguous_index_queue<>& q,
    std::vector<std::uint32_t>& popped_indices)
{
    std::mt19937 r(static_cast<unsigned int>(seed + thread_index));
    std::uniform_int_distribution<> d(0, 1);

    auto pop = [&]() -> hpx::optional<std::uint32_t> {
        switch (m)
        {
        case pop_mode::left:
            return q.pop_left();
        case pop_mode::right:
            return q.pop_right();
        case pop_mode::random:
            return d(r) == 0 ? q.pop_left() : q.pop_right();
        default:
            HPX_TEST(false);
            return {};
        }
    };

    b->arrive_and_wait();

    // Let every task pop an item before any task can drain the queue. A start
    // barrier alone does not guarantee that the other tasks resume in time.
    hpx::optional<std::uint32_t> curr = pop();
    HPX_TEST(curr);
    if (curr)
    {
        popped_indices.push_back(curr.value());
    }
    b->arrive_and_wait();

    while ((curr = pop()))
    {
        popped_indices.push_back(curr.value());
    }
}

void test_concurrent(pop_mode m)
{
    std::uint32_t first = 33;
    std::uint32_t last = 731813;
    std::uint32_t step = 7;
    hpx::concurrency::detail::non_contiguous_index_queue<> q{first, last, step};

    std::size_t const num_threads = 2 * hpx::get_num_worker_threads();

    // This test should be run on at least two worker threads.
    HPX_TEST_LTE(std::size_t(4), num_threads);

    std::vector<hpx::future<void>> fs;
    std::vector<std::vector<std::uint32_t>> popped_indices(num_threads);
    fs.reserve(num_threads);
    std::shared_ptr<hpx::barrier<>> b =
        std::make_shared<hpx::barrier<>>(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
    {
        fs.push_back(hpx::async(test_concurrent_worker, m, i, b, std::ref(q),
            std::ref(popped_indices[i])));
    }

    hpx::wait_all(fs);

    // There should be no indices left in the queue at this point.
    HPX_TEST(q.empty());
    HPX_TEST(!q.pop_left());
    HPX_TEST(!q.pop_right());

    std::size_t num_indices_expected = (last - first) / step;
    std::vector<std::uint32_t> collected_popped_indices;
    collected_popped_indices.reserve(num_indices_expected);
    std::size_t num_nonzero_indices_popped = 0;
    for (auto const& p : popped_indices)
    {
        std::copy(
            p.begin(), p.end(), std::back_inserter(collected_popped_indices));
        if (!p.empty())
        {
            ++num_nonzero_indices_popped;
        }
    }

    // All the original indices should have been popped exactly once.
    HPX_TEST_EQ(collected_popped_indices.size(), num_indices_expected);
    std::sort(collected_popped_indices.begin(), collected_popped_indices.end());
    std::uint32_t curr_expected = first;
    for (auto const i : collected_popped_indices)
    {
        HPX_TEST_EQ(i, curr_expected);
        curr_expected += step;
    }

    // Every task must have participated before the remaining queue was drained.
    HPX_TEST_EQ(num_nonzero_indices_popped, num_threads);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
    }
    std::cout << "Using seed: " << seed << '\n';

    test_basic_default();

    for (std::uint32_t step = 1; step != 3; ++step)
    {
        test_basic(step);
    }

    test_concurrent(pop_mode::left);
    test_concurrent(pop_mode::right);
    test_concurrent(pop_mode::random);

    return hpx::local::finalize();
}

int main(int argc, char** argv)
{
    hpx::local::init_params i;
    hpx::program_options::options_description desc_cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");
    desc_cmdline.add_options()("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");
    i.desc_cmdline = desc_cmdline;
    hpx::local::init(hpx_main, argc, argv, i);
    return hpx::util::report_errors();
}
