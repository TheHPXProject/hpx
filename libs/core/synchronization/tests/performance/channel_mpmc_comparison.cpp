//  Copyright (c) 2026 Arpit Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/synchronization.hpp>

// Orbit can be found at https://github.com/austin-hill/orbit.
#include <orbit/mpmc_queue.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

    using data = hpx::util::cache_line_data<std::uint64_t>;

    struct consumer_result
    {
        std::uint64_t count_ = 0;
        std::uint64_t checksum_ = 0;
    };

    struct benchmark_result
    {
        std::string_view name_;
        double average_seconds_ = 0.0;
        double best_seconds_ = 0.0;
        double average_ops_per_second_ = 0.0;
        double best_ops_per_second_ = 0.0;
    };

    class hpx_channel_mpmc_adapter
    {
    public:
        explicit hpx_channel_mpmc_adapter(std::size_t capacity)
          : queue_(capacity)
        {
        }

        bool try_push(data value)
        {
            return queue_.set(std::move(value));
        }

        bool try_pop(data& value)
        {
            return queue_.get(&value);
        }

    private:
        hpx::lcos::local::channel_mpmc<data> queue_;
    };

    template <std::size_t Capacity, bool MinimiseLatency = true>
    class orbit_queue_adapter
    {
    public:
        explicit orbit_queue_adapter(std::size_t) {}

        bool try_push(data value)
        {
            return queue_.try_push(std::move(value));
        }

        bool try_pop(data& value)
        {
            return queue_.try_pop(value);
        }

    private:
        orbit::mpmc_queue<data, Capacity, MinimiseLatency> queue_;
    };

    template <typename Queue>
    void push_value(Queue& queue, data value)
    {
        while (!queue.try_push(std::move(value)))
        {
            std::this_thread::yield();
        }
    }

    template <typename Queue>
    consumer_result consume_values(Queue& queue)
    {
        consumer_result result;
        data value;

        while (true)
        {
            if (!queue.try_pop(value))
            {
                std::this_thread::yield();
                continue;
            }

            if (value.data_ == 0)
            {
                return result;
            }

            ++result.count_;
            result.checksum_ += value.data_;
        }
    }

    template <typename Queue>
    double run_once(std::size_t queue_capacity, std::size_t producers,
        std::size_t consumers, std::uint64_t total_values)
    {
        Queue queue(queue_capacity);

        std::vector<std::thread> producer_threads;
        std::vector<std::thread> consumer_threads;
        std::vector<consumer_result> consumer_results(consumers);

        producer_threads.reserve(producers);
        consumer_threads.reserve(consumers);

        auto const start = std::chrono::steady_clock::now();

        for (std::size_t i = 0; i != consumers; ++i)
        {
            consumer_threads.emplace_back([&queue, &consumer_results, i]() {
                consumer_results[i] = consume_values(queue);
            });
        }

        std::uint64_t next_value = 1;
        std::uint64_t const base_chunk = total_values / producers;
        std::uint64_t const remainder = total_values % producers;

        for (std::size_t i = 0; i != producers; ++i)
        {
            std::uint64_t const chunk_size =
                base_chunk + (i < remainder ? 1 : 0);
            std::uint64_t const first_value = next_value;
            next_value += chunk_size;

            producer_threads.emplace_back([&queue, chunk_size, first_value]() {
                for (std::uint64_t j = 0; j != chunk_size; ++j)
                {
                    push_value(queue, data(first_value + j));
                }
            });
        }

        for (auto& producer_thread : producer_threads)
        {
            producer_thread.join();
        }

        for (std::size_t i = 0; i != consumers; ++i)
        {
            push_value(queue, data{});
        }

        for (auto& consumer_thread : consumer_threads)
        {
            consumer_thread.join();
        }

        auto const stop = std::chrono::steady_clock::now();

        std::uint64_t const observed_count = std::accumulate(
            consumer_results.begin(), consumer_results.end(), std::uint64_t(0),
            [](std::uint64_t sum, consumer_result const& result) {
                return sum + result.count_;
            });
        std::uint64_t const observed_checksum = std::accumulate(
            consumer_results.begin(), consumer_results.end(), std::uint64_t(0),
            [](std::uint64_t sum, consumer_result const& result) {
                return sum + result.checksum_;
            });

        std::uint64_t const expected_checksum =
            (total_values * (total_values + 1)) / 2;

        if (observed_count != total_values)
        {
            throw std::runtime_error("benchmark lost or duplicated values");
        }

        if (observed_checksum != expected_checksum)
        {
            throw std::runtime_error("benchmark checksum mismatch");
        }

        std::chrono::duration<double> const elapsed = stop - start;
        return elapsed.count();
    }

    template <typename Queue>
    benchmark_result run_benchmark(std::string_view name,
        std::size_t queue_capacity, std::size_t producers,
        std::size_t consumers, std::uint64_t total_values,
        std::size_t repetitions)
    {
        std::vector<double> elapsed_seconds;
        elapsed_seconds.reserve(repetitions);

        for (std::size_t i = 0; i != repetitions; ++i)
        {
            elapsed_seconds.push_back(run_once<Queue>(
                queue_capacity, producers, consumers, total_values));
        }

        double const total_seconds = std::accumulate(
            elapsed_seconds.begin(), elapsed_seconds.end(), 0.0);
        double const average_seconds = total_seconds / repetitions;
        double const best_seconds =
            *std::min_element(elapsed_seconds.begin(), elapsed_seconds.end());

        return benchmark_result{name, average_seconds, best_seconds,
            total_values / average_seconds, total_values / best_seconds};
    }

    void print_result(benchmark_result const& result)
    {
        std::cout << result.name_ << ": avg=" << result.average_seconds_
                  << " s, best=" << result.best_seconds_
                  << " s, avg throughput=" << result.average_ops_per_second_
                  << " op/s, best throughput=" << result.best_ops_per_second_
                  << " op/s\n";
    }

    void print_csv_result(benchmark_result const& result, std::size_t capacity,
        std::size_t producers, std::size_t consumers,
        std::uint64_t total_values, std::size_t repetitions)
    {
        std::cout << result.name_ << ',' << capacity << ',' << producers << ','
                  << consumers << ',' << total_values << ',' << repetitions
                  << ',' << result.average_seconds_ << ','
                  << result.best_seconds_ << ','
                  << result.average_ops_per_second_ << ','
                  << result.best_ops_per_second_ << '\n';
    }

    template <std::size_t Capacity>
    int run_for_capacity(hpx::program_options::variables_map& vm)
    {
        std::size_t const producers = vm["producers"].as<std::size_t>();
        std::size_t const consumers = vm["consumers"].as<std::size_t>();
        std::uint64_t const total_values = vm["values"].as<std::uint64_t>();
        std::size_t const repetitions = vm["repetitions"].as<std::size_t>();
        bool const csv = vm["csv"].as<bool>();

        std::vector<benchmark_result> results;
        results.reserve(3);

        results.push_back(
            run_benchmark<hpx_channel_mpmc_adapter>("hpx_channel_mpmc",
                Capacity, producers, consumers, total_values, repetitions));
        results.push_back(
            run_benchmark<orbit_queue_adapter<Capacity, true>>("orbit_default",
                Capacity, producers, consumers, total_values, repetitions));
        results.push_back(run_benchmark<orbit_queue_adapter<Capacity, false>>(
            "orbit_throughput", Capacity, producers, consumers, total_values,
            repetitions));

        if (csv)
        {
            std::cout
                << "queue,capacity,producers,consumers,values,repetitions,"
                   "average_seconds,best_seconds,average_ops_per_second,"
                   "best_ops_per_second\n";
            for (benchmark_result const& result : results)
            {
                print_csv_result(result, Capacity, producers, consumers,
                    total_values, repetitions);
            }
        }
        else
        {
            std::cout << "capacity=" << Capacity << ", producers=" << producers
                      << ", consumers=" << consumers
                      << ", values=" << total_values
                      << ", repetitions=" << repetitions << '\n';
            for (benchmark_result const& result : results)
            {
                print_result(result);
            }
        }

        return 0;
    }

    int run_dispatch(hpx::program_options::variables_map& vm)
    {
        std::size_t const capacity = vm["capacity"].as<std::size_t>();

        switch (capacity)
        {
        case 64:
            return run_for_capacity<64>(vm);
        case 128:
            return run_for_capacity<128>(vm);
        case 256:
            return run_for_capacity<256>(vm);
        case 512:
            return run_for_capacity<512>(vm);
        case 1024:
            return run_for_capacity<1024>(vm);
        case 2048:
            return run_for_capacity<2048>(vm);
        case 4096:
            return run_for_capacity<4096>(vm);
        case 8192:
            return run_for_capacity<8192>(vm);
        case 16384:
            return run_for_capacity<16384>(vm);
        default:
            std::cerr
                << "Unsupported capacity " << capacity
                << ". Supported capacities are 64, 128, 256, 512, 1024, 2048, "
                   "4096, 8192, and 16384.\n";
            return 1;
        }
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const producers = vm["producers"].as<std::size_t>();
    std::size_t const consumers = vm["consumers"].as<std::size_t>();
    std::uint64_t const total_values = vm["values"].as<std::uint64_t>();
    std::size_t const repetitions = vm["repetitions"].as<std::size_t>();

    if (producers == 0 || consumers == 0 || total_values == 0 ||
        repetitions == 0)
    {
        std::cerr
            << "producers, consumers, values, and repetitions must all be "
               "greater than zero\n";
        return hpx::local::finalize();
    }

    int const result = run_dispatch(vm);
    hpx::local::finalize();
    return result;
}

int main(int argc, char* argv[])
{
    using hpx::program_options::bool_switch;
    using hpx::program_options::options_description;
    using hpx::program_options::value;

    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("capacity",
        value<std::size_t>()->default_value(2048),
        "Queue capacity. Must match one of the compiled-in power-of-two "
        "capacities.")("producers", value<std::size_t>()->default_value(2),
        "Number of producer threads.")("consumers",
        value<std::size_t>()->default_value(2), "Number of consumer threads.")(
        "values", value<std::uint64_t>()->default_value(1000000),
        "Total number of values produced across all producers.")("repetitions",
        value<std::size_t>()->default_value(5),
        "Number of times to repeat each benchmark.")("csv",
        bool_switch()->default_value(false),
        "Emit results as CSV instead of human-readable text.");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
