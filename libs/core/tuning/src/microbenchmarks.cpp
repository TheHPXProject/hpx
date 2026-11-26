//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/tuning/microbenchmarks.hpp>
#include <hpx/tuning/statistics.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

// These measurements describe the machine and the operating system, not
// HPX's runtime, so they use the threads and the condition variable of the
// standard library on purpose: the scheduler's timed waits are those of
// std::condition_variable, and a woken worker is a thread of the operating
// system.

namespace hpx::tuning {

    namespace {

        using clock = std::chrono::steady_clock;

        double microseconds(clock::duration const d)
        {
            return std::chrono::duration<double, std::micro>(d).count();
        }
    }    // namespace

    summary measure_clock_read(microbenchmark_options const& options)
    {
        constexpr int reads = 1000;
        std::vector<double> ns;
        ns.reserve(options.repeats);
        for (std::size_t r = 0; r != options.repeats; ++r)
        {
            auto const start = clock::now();
            clock::time_point last = start;
            for (int i = 0; i != reads; ++i)
            {
                last = clock::now();
            }
            ns.push_back(1000.0 * microseconds(last - start) / reads);
        }
        return summarize(HPX_MOVE(ns));
    }

    summary measure_spin_rate(microbenchmark_options const& options)
    {
        // the calibration of the scheduler's spin (scheduler_base), repeated
        constexpr int rounds = 1000;
        std::atomic<std::size_t> queue_length{0};
        std::vector<double> rates;
        rates.reserve(options.repeats);
        for (std::size_t r = 0; r != options.repeats; ++r)
        {
            auto const start = clock::now();
            for (int i = 0; i != rounds; ++i)
            {
                for (std::uint32_t p = 0; p !=
                    threads::policies::scheduling_policy::
                        calibration_pauses_per_round;
                    ++p)
                {
                    HPX_SMT_PAUSE;
                }
                if (queue_length.load(std::memory_order_relaxed) != 0)
                    break;
            }
            // HPX_SMT_PAUSE may be empty, keep the rate finite
            double const us = microseconds(clock::now() - start);
            rates.push_back(rounds / (us > 0.001 ? us : 0.001));
        }
        return summarize(HPX_MOVE(rates));
    }

    std::vector<timed_wait> measure_timed_waits(
        double const max_us, microbenchmark_options const& options)
    {
        std::mutex mtx;
        std::condition_variable cond;
        std::vector<timed_wait> waits;
        for (double requested = 1.0; requested <= max_us; requested *= 2.0)
        {
            auto const period = std::chrono::duration_cast<clock::duration>(
                std::chrono::duration<double, std::micro>(requested));
            std::vector<double> actual;
            actual.reserve(options.repeats);
            for (std::size_t r = 0; r != options.repeats; ++r)
            {
                std::unique_lock<std::mutex> l(mtx);
                auto const start = clock::now();
                // nobody notifies: the wait ends when its period runs out (or
                // spuriously, which shows up as a short outlier)
                cond.wait_for(l, period);
                actual.push_back(microseconds(clock::now() - start));
            }
            waits.push_back(timed_wait{requested, summarize(HPX_MOVE(actual))});
        }
        return waits;
    }

    wake_cost measure_wake_cost(microbenchmark_options const& options)
    {
        wake_cost result;

        // a thread asleep on a condition variable, woken by a notification
        {
            std::mutex mtx;
            std::condition_variable cond;
            bool ready = false;
            bool done = false;
            clock::time_point notified;
            std::vector<double> latency;
            latency.reserve(options.repeats);

            std::thread waiter([&] {
                std::unique_lock<std::mutex> l(mtx);
                for (std::size_t r = 0; r != options.repeats; ++r)
                {
                    cond.wait(l, [&] { return ready || done; });
                    if (done)
                        return;
                    latency.push_back(microseconds(clock::now() - notified));
                    ready = false;
                    cond.notify_all();
                }
            });
            for (std::size_t r = 0; r != options.repeats; ++r)
            {
                // give the waiter time to go to sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                {
                    std::lock_guard<std::mutex> l(mtx);
                    ready = true;
                    notified = clock::now();
                }
                cond.notify_all();
                std::unique_lock<std::mutex> l(mtx);
                cond.wait(l, [&] { return !ready; });
            }
            {
                std::lock_guard<std::mutex> l(mtx);
                done = true;
            }
            cond.notify_all();
            waiter.join();
            result.asleep_us = summarize(HPX_MOVE(latency));
        }

        // a thread spinning on a flag
        {
            std::atomic<std::int64_t> signal{0};    // nanoseconds since epoch
            std::atomic<bool> seen{false};
            std::atomic<bool> done{false};
            std::vector<double> latency;
            latency.reserve(options.repeats);

            std::thread spinner([&] {
                while (!done.load(std::memory_order_acquire))
                {
                    std::int64_t const s =
                        signal.load(std::memory_order_acquire);
                    if (s != 0)
                    {
                        auto const now = std::chrono::duration_cast<
                            std::chrono::nanoseconds>(
                            clock::now().time_since_epoch())
                                             .count();
                        latency.push_back(
                            static_cast<double>(now - s) / 1000.0);
                        signal.store(0, std::memory_order_relaxed);
                        seen.store(true, std::memory_order_release);
                    }
                    HPX_SMT_PAUSE;
                }
            });
            for (std::size_t r = 0; r != options.repeats; ++r)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                seen.store(false, std::memory_order_relaxed);
                signal.store(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        clock::now().time_since_epoch())
                        .count(),
                    std::memory_order_release);
                while (!seen.load(std::memory_order_acquire))
                    HPX_SMT_PAUSE;
            }
            done.store(true, std::memory_order_release);
            spinner.join();
            result.spinning_us = summarize(HPX_MOVE(latency));
        }
        return result;
    }
}    // namespace hpx::tuning
