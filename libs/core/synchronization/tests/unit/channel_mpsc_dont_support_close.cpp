//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using channel_type = hpx::lcos::local::channel_mpsc<int,
    hpx::lcos::local::channel_mode::dont_support_close>;

void test_basic_fifo()
{
    channel_type c(16);
    HPX_TEST(c.is_empty());
    HPX_TEST(!c.get(nullptr));

    for (int i = 0; i != 10; ++i)
    {
        HPX_TEST(c.set(int(i)));
    }
    HPX_TEST(!c.is_empty());

    // Calling get(nullptr) checks availability without consuming the item
    HPX_TEST(c.get(nullptr));
    HPX_TEST(!c.is_empty());

    for (int i = 0; i != 10; ++i)
    {
        HPX_TEST(c.get(nullptr));
        int val = -1;
        HPX_TEST(c.get(&val));
        HPX_TEST_EQ(val, i);
    }
    HPX_TEST(c.is_empty());
    HPX_TEST(!c.get(nullptr));
}

void test_capacity_and_saturation()
{
    // A requested size of 7 rounds up to capacity 8
    channel_type c(7);
    HPX_TEST(c.is_empty());

    std::size_t count = 0;
    while (c.set(int(count)))
    {
        ++count;
    }

    HPX_TEST_EQ(count, c.capacity());
    HPX_TEST(!c.is_empty());

    // Next set must fail because buffer is full
    HPX_TEST(!c.set(999));

    // After popping one item, set must succeed
    int val = -1;
    HPX_TEST(c.get(&val));
    HPX_TEST_EQ(val, 0);

    HPX_TEST(c.set(999));
    HPX_TEST(!c.set(1000));
}

void test_move_only()
{
    using move_channel_type =
        hpx::lcos::local::channel_mpsc<std::unique_ptr<int>,
            hpx::lcos::local::channel_mode::dont_support_close>;

    move_channel_type c(8);
    HPX_TEST(c.is_empty());

    for (int i = 0; i != 5; ++i)
    {
        HPX_TEST(c.set(std::make_unique<int>(i)));
    }

    for (int i = 0; i != 5; ++i)
    {
        std::unique_ptr<int> val;
        HPX_TEST(c.get(&val));
        HPX_TEST(val != nullptr);
        HPX_TEST_EQ(*val, i);
    }
    HPX_TEST(c.is_empty());
}

struct test_item
{
    std::size_t producer_id;
    std::size_t seq;
};

void test_concurrent_mpsc()
{
    constexpr std::size_t num_producers = 4;
    constexpr std::size_t items_per_producer = 2000;
    constexpr std::size_t total_items = num_producers * items_per_producer;

    using item_channel_type = hpx::lcos::local::channel_mpsc<test_item,
        hpx::lcos::local::channel_mode::dont_support_close>;

    item_channel_type c(64);

    std::vector<hpx::future<void>> producers;
    producers.reserve(num_producers);

    for (std::size_t p = 0; p != num_producers; ++p)
    {
        producers.push_back(hpx::async([&c, p]() {
            for (std::size_t i = 0; i != items_per_producer; ++i)
            {
                test_item item{p, i};
                while (!c.set(std::move(item)))
                {
                    hpx::this_thread::yield();
                }
            }
        }));
    }

    std::vector<std::size_t> next_expected_seq(num_producers, 0);
    std::size_t items_received = 0;

    while (items_received < total_items)
    {
        test_item item{};
        if (c.get(&item))
        {
            HPX_TEST_LT(item.producer_id, num_producers);
            HPX_TEST_EQ(item.seq, next_expected_seq[item.producer_id]);
            ++next_expected_seq[item.producer_id];
            ++items_received;
        }
        else
        {
            hpx::this_thread::yield();
        }
    }

    hpx::wait_all(producers);

    HPX_TEST_EQ(items_received, total_items);
    for (std::size_t p = 0; p != num_producers; ++p)
    {
        HPX_TEST_EQ(next_expected_seq[p], items_per_producer);
    }
    HPX_TEST(c.is_empty());
}

int hpx_main()
{
    test_basic_fifo();
    test_capacity_and_saturation();
    test_move_only();
    test_concurrent_mpsc();

    hpx::local::finalize();
    return hpx::util::report_errors();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
