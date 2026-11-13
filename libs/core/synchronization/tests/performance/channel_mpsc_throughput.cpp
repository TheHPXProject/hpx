//  Copyright (c) 2019 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is inspired by https://github.com/aprell/tasking-2.0

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct data
{
    data() = default;

    explicit data(int d)
    {
        data_[0] = d;
    }

    int data_[8];
};

#if HPX_DEBUG
constexpr int NUM_TESTS = 1000000;
#else
constexpr int NUM_TESTS = 100000000;
#endif

///////////////////////////////////////////////////////////////////////////////
template <hpx::lcos::local::channel_mode Mode>
inline data channel_get(hpx::lcos::local::channel_mpsc<data, Mode> const& c)
{
    data result;
    while (!c.get(&result))
    {
        hpx::this_thread::yield();
    }
    return result;
}

template <hpx::lcos::local::channel_mode Mode>
inline void channel_set(
    hpx::lcos::local::channel_mpsc<data, Mode>& c, data&& val)
{
    while (!c.set(std::move(val)))    // NOLINT
    {
        hpx::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Produce
template <hpx::lcos::local::channel_mode Mode>
double thread_func_0(hpx::lcos::local::channel_mpsc<data, Mode>& c)
{
    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i != NUM_TESTS; ++i)
    {
        channel_set(c, data{i});
    }

    std::uint64_t end = hpx::chrono::high_resolution_clock::now();

    return static_cast<double>(end - start) / 1e9;
}

// Consume
template <hpx::lcos::local::channel_mode Mode>
double thread_func_1(hpx::lcos::local::channel_mpsc<data, Mode>& c)
{
    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i != NUM_TESTS; ++i)
    {
        data d = channel_get(c);
        if (d.data_[0] != i)
        {
            std::cout << "Error!\n";
        }
    }

    std::uint64_t end = hpx::chrono::high_resolution_clock::now();

    return static_cast<double>(end - start) / 1e9;
}

template <hpx::lcos::local::channel_mode Mode>
double thread_func_multi_producer(
    hpx::lcos::local::channel_mpsc<data, Mode>& c, int num_items)
{
    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i != num_items; ++i)
    {
        channel_set(c, data{i});
    }

    std::uint64_t end = hpx::chrono::high_resolution_clock::now();

    return static_cast<double>(end - start) / 1e9;
}

template <hpx::lcos::local::channel_mode Mode>
double thread_func_multi_consumer(
    hpx::lcos::local::channel_mpsc<data, Mode>& c, int total_items)
{
    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i != total_items; ++i)
    {
        data d = channel_get(c);
        HPX_UNUSED(d);
    }

    std::uint64_t end = hpx::chrono::high_resolution_clock::now();

    return static_cast<double>(end - start) / 1e9;
}

template <hpx::lcos::local::channel_mode Mode>
void run_benchmark(char const* mode_name)
{
    std::cout << "=== Benchmarking " << mode_name << " (1P1C) ===\n";
    hpx::lcos::local::channel_mpsc<data, Mode> c(10000);

    hpx::future<double> producer = hpx::async(thread_func_0<Mode>, std::ref(c));
    hpx::future<double> consumer = hpx::async(thread_func_1<Mode>, std::ref(c));

    auto producer_time = producer.get();
    std::cout << "Producer throughput: " << (NUM_TESTS / producer_time)
              << " [op/s] (" << (producer_time / NUM_TESTS) * 1e9
              << " [ns/op])\n";

    auto consumer_time = consumer.get();
    std::cout << "Consumer throughput: " << (NUM_TESTS / consumer_time)
              << " [op/s] (" << (consumer_time / NUM_TESTS) * 1e9
              << " [ns/op])\n\n";
}

template <hpx::lcos::local::channel_mode Mode>
void run_multi_producer_benchmark(char const* mode_name, int num_producers)
{
    std::cout << "=== Benchmarking " << mode_name << " (" << num_producers
              << "P1C) ===\n";
    hpx::lcos::local::channel_mpsc<data, Mode> c(10000);

    int items_per_producer = NUM_TESTS / num_producers;
    int total_items = items_per_producer * num_producers;

    std::vector<hpx::future<double>> producers;
    producers.reserve(num_producers);

    hpx::future<double> consumer =
        hpx::async(thread_func_multi_consumer<Mode>, std::ref(c), total_items);

    for (int p = 0; p < num_producers; ++p)
    {
        producers.push_back(hpx::async(
            thread_func_multi_producer<Mode>, std::ref(c), items_per_producer));
    }

    double max_producer_time = 0.0;
    for (auto& f : producers)
    {
        max_producer_time = (std::max) (max_producer_time, f.get());
    }
    auto consumer_time = consumer.get();

    double elapsed = (std::max) (max_producer_time, consumer_time);
    std::cout << "Total throughput:    " << (total_items / elapsed)
              << " [op/s] (" << (elapsed / total_items) * 1e9 << " [ns/op])\n"
              << "Producer max time:   " << max_producer_time << " [s]\n"
              << "Consumer time:       " << consumer_time << " [s]\n\n";
}

int hpx_main()
{
    using hpx::lcos::local::channel_mode;

    run_benchmark<channel_mode::normal>("channel_mode::normal (mutex-based)");
    run_benchmark<channel_mode::dont_support_close>(
        "channel_mode::dont_support_close (lock-free Vyukov ring buffer)");

    run_multi_producer_benchmark<channel_mode::normal>(
        "channel_mode::normal (mutex-based)", 2);
    run_multi_producer_benchmark<channel_mode::dont_support_close>(
        "channel_mode::dont_support_close (lock-free Vyukov ring buffer)", 2);

    run_multi_producer_benchmark<channel_mode::normal>(
        "channel_mode::normal (mutex-based)", 4);
    run_multi_producer_benchmark<channel_mode::dont_support_close>(
        "channel_mode::dont_support_close (lock-free Vyukov ring buffer)", 4);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
