////////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
////////////////////////////////////////////////////////////////////////////////

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
#include <utility>
#include <vector>

unsigned int seed = std::random_device{}();

void test_basic()
{
    {
        // A default constructed queue should be empty.
        hpx::concurrency::detail::contiguous_index_queue<> q;

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());
    }

    {
        // Popping from the left should give us the expected indices.
        std::uint32_t first = 3;
        std::uint32_t last = 7;
        hpx::concurrency::detail::contiguous_index_queue<> q{first, last};

        for (std::uint32_t curr_expected = first; curr_expected < last;
            ++curr_expected)
        {
            hpx::optional<std::uint32_t> curr = q.pop_left();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());
    }

    {
        // Popping from the right should give us the expected indices.
        std::uint32_t first = 3;
        std::uint32_t last = 7;
        hpx::concurrency::detail::contiguous_index_queue<> q{first, last};

        for (std::uint32_t curr_expected = last - 1; curr_expected >= first;
            --curr_expected)
        {
            hpx::optional<std::uint32_t> curr = q.pop_right();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());

        // Resetting a queue should give us the same behaviour as a fresh
        // queue.
        q.reset(first, last);

        for (std::uint32_t curr_expected = last - 1; curr_expected >= first;
            --curr_expected)
        {
            hpx::optional<std::uint32_t> curr = q.pop_right();
            HPX_TEST(curr);
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            HPX_TEST_EQ(curr.value(), curr_expected);
        }

        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left());
        HPX_TEST(!q.pop_right());
    }
}

void test_chunked()
{
    using queue_type = hpx::concurrency::detail::contiguous_index_queue<>;
    using chunk_type = std::pair<std::uint32_t, std::uint32_t>;

    {
        // A default constructed queue gives no chunks.
        queue_type q;

        HPX_TEST(!q.pop_left_chunk(1));
        HPX_TEST(!q.pop_right_chunk(1));
        HPX_TEST(!q.pop_left_chunk(4));
        HPX_TEST(!q.pop_right_chunk(4));
    }

    {
        // A divisor of one takes all items at once.
        queue_type q{3, 7};
        hpx::optional<chunk_type> c = q.pop_left_chunk(1);
        HPX_TEST(c);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        HPX_TEST(c.value() == chunk_type(3, 7));
        HPX_TEST(q.empty());
        HPX_TEST(!q.pop_left_chunk(1));
        HPX_TEST(!q.pop_right_chunk(1));

        q.reset(3, 7);
        c = q.pop_right_chunk(1);
        HPX_TEST(c);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        HPX_TEST(c.value() == chunk_type(3, 7));
        HPX_TEST(q.empty());
    }

    {
        // Chunks from the left are 1/divisor of the items left, at least
        // one, and follow each other without gaps.
        std::uint32_t const first = 10;
        std::uint32_t const last = 110;
        std::uint32_t const divisor = 4;
        queue_type q{first, last};

        std::uint32_t expected_first = first;
        hpx::optional<chunk_type> c;
        while ((c = q.pop_left_chunk(divisor)))
        {
            std::uint32_t const left = last - expected_first;
            std::uint32_t const expected_size =
                (std::max) (left / divisor, std::uint32_t(1));
            HPX_TEST_EQ(c->first, expected_first);
            HPX_TEST_EQ(c->second, expected_first + expected_size);
            expected_first = c->second;
        }
        HPX_TEST_EQ(expected_first, last);
        HPX_TEST(q.empty());
    }

    {
        // Chunks from the right, the same from the other end.
        std::uint32_t const first = 10;
        std::uint32_t const last = 110;
        std::uint32_t const divisor = 3;
        queue_type q{first, last};

        std::uint32_t expected_last = last;
        hpx::optional<chunk_type> c;
        while ((c = q.pop_right_chunk(divisor)))
        {
            std::uint32_t const left = expected_last - first;
            std::uint32_t const expected_size =
                (std::max) (left / divisor, std::uint32_t(1));
            HPX_TEST_EQ(c->second, expected_last);
            HPX_TEST_EQ(c->first, expected_last - expected_size);
            expected_last = c->first;
        }
        HPX_TEST_EQ(expected_last, first);
        HPX_TEST(q.empty());
    }

    {
        // Chunked and single pops from both ends together take every item
        // once.
        std::uint32_t const last = 1000;
        queue_type q{0, last};

        std::vector<int> seen(last, 0);
        for (std::size_t round = 0; !q.empty(); ++round)
        {
            hpx::optional<chunk_type> c;
            switch (round % 4)
            {
            case 0:
                c = q.pop_left_chunk(8);
                break;
            case 1:
                c = q.pop_right_chunk(5);
                break;
            case 2:
                if (auto const i = q.pop_left())
                {
                    c = chunk_type(*i, *i + 1);
                }
                break;
            default:
                if (auto const i = q.pop_right())
                {
                    c = chunk_type(*i, *i + 1);
                }
                break;
            }

            HPX_TEST(c);
            if (c)
            {
                HPX_TEST_LT(c->first, c->second);
                for (std::uint32_t i = c->first; i != c->second; ++i)
                {
                    ++seen[i];
                }
            }
        }

        HPX_TEST(std::all_of(
            seen.begin(), seen.end(), [](int const n) { return n == 1; }));
    }
}

enum class pop_mode
{
    left,
    right,
    random,
    left_chunk,
    right_chunk,
    random_chunk
};

void test_concurrent_worker(pop_mode m, std::size_t thread_index,
    std::shared_ptr<hpx::barrier<>> b,
    hpx::concurrency::detail::contiguous_index_queue<>& q,
    std::vector<std::uint32_t>& popped_indices)
{
    std::mt19937 r(static_cast<unsigned int>(seed + thread_index));
    std::uniform_int_distribution<> d(0, 1);

    using chunk_type = std::pair<std::uint32_t, std::uint32_t>;

    // Chunked pops take a fraction of the items left, use a few different
    // ones.
    auto const divisor = static_cast<std::uint32_t>(2 + thread_index % 7);

    auto single = [](hpx::optional<std::uint32_t> const& i) {
        return i ? hpx::optional<chunk_type>(chunk_type(*i, *i + 1)) :
                   hpx::optional<chunk_type>();
    };

    auto pop = [&]() -> hpx::optional<chunk_type> {
        switch (m)
        {
        case pop_mode::left:
            return single(q.pop_left());
        case pop_mode::right:
            return single(q.pop_right());
        case pop_mode::random:
            return single(d(r) == 0 ? q.pop_left() : q.pop_right());
        case pop_mode::left_chunk:
            return q.pop_left_chunk(divisor);
        case pop_mode::right_chunk:
            return q.pop_right_chunk(divisor);
        case pop_mode::random_chunk:
            return d(r) == 0 ? q.pop_left_chunk(divisor) :
                               q.pop_right_chunk(divisor);
        default:
            HPX_TEST(false);
            return {};
        }
    };

    auto collect = [&](chunk_type const& c) {
        HPX_TEST_LT(c.first, c.second);
        for (std::uint32_t i = c.first; i != c.second; ++i)
        {
            popped_indices.push_back(i);
        }
    };

    // Every thread pops one index before any thread continues (a single one:
    // a chunk might not leave enough items for the others). This makes sure
    // that all threads take part, even if some of them start late (for
    // instance on an oversubscribed machine).
    hpx::optional<chunk_type> curr = single(q.pop_left());
    HPX_TEST(curr);
    if (curr)
    {
        collect(*curr);
    }

    // Make sure all threads continue roughly at the same time.
    b->arrive_and_wait();

    while ((curr = pop()))
    {
        collect(*curr);
    }
}

void test_concurrent(pop_mode m)
{
    std::uint32_t first = 33;
    std::uint32_t last = 732100;
    hpx::concurrency::detail::contiguous_index_queue<> q{first, last};

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

    std::size_t num_indices_expected = last - first;
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
        ++curr_expected;
    }

    // Every thread has popped at least one index before the barrier.
    HPX_TEST_EQ(num_nonzero_indices_popped, num_threads);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
    }
    std::cout << "Using seed: " << seed << '\n';

    test_basic();
    test_concurrent(pop_mode::left);
    test_concurrent(pop_mode::right);
    test_concurrent(pop_mode::random);

    test_chunked();
    test_concurrent(pop_mode::left_chunk);
    test_concurrent(pop_mode::right_chunk);
    test_concurrent(pop_mode::random_chunk);

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
