//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Benchmark: continues_on (P2300) vs Native HPX Scheduling
//
// Part 1 -- Microbenchmark: measures raw task dispatch overhead by launching
//          N empty tasks via native hpx::async vs P2300 continues_on pipeline.
//
// Part 2 -- Macrobenchmark: measures a real-world DAXPY computation
//          (y[i] = alpha * x[i] + y[i]) over a large vector using both
//          native HPX async and P2300 sender pipelines.
//
// Usage:  ./benchmark_continues_on_test --hpx:threads=4

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>

#include <hpx/modules/executors.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;

// ==========================================================================
// Helpers
// ==========================================================================

struct timer
{
    hpx::chrono::high_resolution_timer t;

    double elapsed_ms() const
    {
        return t.elapsed() * 1e3;
    }
};

void print_header(char const* title)
{
    std::cout << "\n"
              << std::string(65, '=') << "\n"
              << "  " << title << "\n"
              << std::string(65, '=') << "\n\n";
}

void print_result(
    char const* label, double ms, std::size_t ops, char const* unit = "tasks")
{
    double const per_sec = static_cast<double>(ops) / (ms / 1e3);
    std::cout << std::left << std::setw(42) << label << std::right
              << std::setw(10) << std::fixed << std::setprecision(2) << ms
              << " ms   (" << std::setprecision(0) << per_sec << " " << unit
              << "/s)\n";
}

// ==========================================================================
// Part 1: Microbenchmark -- Raw task dispatch overhead
// ==========================================================================

void run_microbenchmark()
{
    print_header("Part 1: Microbenchmark - Empty Task Dispatch Overhead");

    constexpr std::size_t N = 100'000;
    constexpr int warmup_rounds = 3;
    constexpr int bench_rounds = 5;

    // ------------------------------------------------------------------
    // 1a. Native hpx::async (fan-out / wait-all pattern)
    // ------------------------------------------------------------------
    auto native_bench = [&]() -> double {
        timer t;
        std::vector<hpx::future<void>> futs;
        futs.reserve(N);
        for (std::size_t i = 0; i < N; ++i)
        {
            futs.push_back(hpx::async([] {}));
        }
        hpx::wait_all(futs);
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        native_bench();

    double native_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        native_total += native_bench();
    double const native_avg = native_total / bench_rounds;

    print_result("hpx::async fan-out + wait_all", native_avg, N);

    // ------------------------------------------------------------------
    // 1b. P2300: sync_wait(just() | continues_on(sched) | then(noop))
    //     Sequential dispatch -- measures per-task overhead.
    // ------------------------------------------------------------------
    ex::thread_pool_scheduler sched{};

    auto p2300_seq_bench = [&]() -> double {
        timer t;
        for (std::size_t i = 0; i < N; ++i)
        {
            ex::sync_wait(                   //
                ex::just()                   //
                | ex::continues_on(sched)    //
                | ex::then([] {})            //
            );
        }
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        p2300_seq_bench();

    double p2300_seq_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        p2300_seq_total += p2300_seq_bench();
    double const p2300_seq_avg = p2300_seq_total / bench_rounds;

    print_result("P2300 continues_on (sequential loop)", p2300_seq_avg, N);

    // ------------------------------------------------------------------
    // 1c. P2300: Fan-out via hpx::async wrapping sync_wait
    // ------------------------------------------------------------------
    constexpr std::size_t fan_out_N = 10'000;

    auto p2300_fanout_bench = [&]() -> double {
        timer t;

        constexpr std::size_t batch = 1000;
        for (std::size_t b = 0; b < fan_out_N; b += batch)
        {
            std::size_t const count =
                (std::min) (batch, static_cast<std::size_t>(fan_out_N - b));

            std::vector<hpx::future<void>> futs;
            futs.reserve(count);
            for (std::size_t i = 0; i < count; ++i)
            {
                futs.push_back(hpx::async([&] {
                    ex::sync_wait(                   //
                        ex::just()                   //
                        | ex::continues_on(sched)    //
                        | ex::then([] {})            //
                    );
                }));
            }
            hpx::wait_all(futs);
        }
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        p2300_fanout_bench();

    double p2300_fanout_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        p2300_fanout_total += p2300_fanout_bench();
    double const p2300_fanout_avg = p2300_fanout_total / bench_rounds;

    print_result("P2300 continues_on (fan-out)", p2300_fanout_avg, fan_out_N);

    // Summary
    std::cout << "\n  Overhead ratio (seq P2300 / native fan-out): "
              << std::fixed << std::setprecision(2)
              << (p2300_seq_avg / native_avg) << "x\n";
}

// ==========================================================================
// Part 2: Macrobenchmark -- DAXPY Pipeline
// ==========================================================================

// y[i] = alpha * x[i] + y[i]
void daxpy_kernel(
    double alpha, double const* x, double* y, std::size_t n) noexcept
{
    for (std::size_t i = 0; i < n; ++i)
    {
        y[i] = alpha * x[i] + y[i];
    }
}

void run_macrobenchmark()
{
    print_header("Part 2: Macrobenchmark - DAXPY Pipeline (N=10M doubles)");

    constexpr std::size_t N = 10'000'000;
    constexpr double alpha = 2.0;
    constexpr int warmup_rounds = 2;
    constexpr int bench_rounds = 5;

    std::vector<double> x(N);
    std::vector<double> y_native(N);
    std::vector<double> y_p2300(N);
    std::vector<double> y_bulk(N);

    std::iota(x.begin(), x.end(), 1.0);

    auto const num_threads =
        static_cast<std::size_t>(hpx::get_num_worker_threads());

    // ------------------------------------------------------------------
    // 2a. Native hpx::async -- manually chunked DAXPY
    // ------------------------------------------------------------------
    auto native_daxpy = [&]() -> double {
        std::fill(y_native.begin(), y_native.end(), 0.5);

        timer t;
        std::vector<hpx::future<void>> futs;
        futs.reserve(num_threads);

        std::size_t const chunk = (N + num_threads - 1) / num_threads;
        for (std::size_t tid = 0; tid < num_threads; ++tid)
        {
            std::size_t const begin = tid * chunk;
            std::size_t const end = (std::min) (begin + chunk, N);
            if (begin >= N)
                break;

            futs.push_back(hpx::async([&, begin, end] {
                daxpy_kernel(alpha, x.data() + begin, y_native.data() + begin,
                    end - begin);
            }));
        }
        hpx::wait_all(futs);
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        native_daxpy();

    double native_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        native_total += native_daxpy();
    double const native_avg = native_total / bench_rounds;

    print_result("Native hpx::async (chunked)", native_avg, N, "elements");

    // ------------------------------------------------------------------
    // 2b. P2300 Pipeline: continues_on per chunk
    // ------------------------------------------------------------------
    ex::thread_pool_scheduler sched{};

    auto p2300_daxpy = [&]() -> double {
        std::fill(y_p2300.begin(), y_p2300.end(), 0.5);

        timer t;
        std::vector<hpx::future<void>> futs;
        futs.reserve(num_threads);

        std::size_t const chunk = (N + num_threads - 1) / num_threads;
        for (std::size_t tid = 0; tid < num_threads; ++tid)
        {
            std::size_t const begin = tid * chunk;
            std::size_t const end = (std::min) (begin + chunk, N);
            if (begin >= N)
                break;

            futs.push_back(hpx::async([&, begin, end] {
                ex::sync_wait(                   //
                    ex::just()                   //
                    | ex::continues_on(sched)    //
                    | ex::then([&, begin, end] {
                          daxpy_kernel(alpha, x.data() + begin,
                              y_p2300.data() + begin, end - begin);
                      })    //
                );
            }));
        }
        hpx::wait_all(futs);
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        p2300_daxpy();

    double p2300_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        p2300_total += p2300_daxpy();
    double const p2300_avg = p2300_total / bench_rounds;

    print_result("P2300 continues_on (chunked)", p2300_avg, N, "elements");

    // ------------------------------------------------------------------
    // 2c. P2300 Bulk: starts_on(sched, just() | bulk(N, daxpy_element))
    //     Uses thread_pool_domain's optimized bulk transform.
    // ------------------------------------------------------------------
    auto p2300_bulk_daxpy = [&]() -> double {
        std::fill(y_bulk.begin(), y_bulk.end(), 0.5);

        timer t;
        auto bulk_n = static_cast<int>(N);
        ex::sync_wait(                    //
            ex::starts_on(sched,          //
                ex::just()                //
                    | ex::bulk(bulk_n,    //
                          [&](int i) {
                              auto const idx = static_cast<std::size_t>(i);
                              y_bulk[idx] = alpha * x[idx] + y_bulk[idx];
                          })    //
                )               //
        );
        return t.elapsed_ms();
    };

    for (int r = 0; r < warmup_rounds; ++r)
        p2300_bulk_daxpy();

    double p2300_bulk_total = 0.0;
    for (int r = 0; r < bench_rounds; ++r)
        p2300_bulk_total += p2300_bulk_daxpy();
    double const p2300_bulk_avg = p2300_bulk_total / bench_rounds;

    print_result(
        "P2300 bulk (domain-optimized)", p2300_bulk_avg, N, "elements");

    // ------------------------------------------------------------------
    // Correctness check
    // ------------------------------------------------------------------
    bool correct = true;
    for (std::size_t i = 0; i < 10 && i < N; ++i)
    {
        double const expected = alpha * x[i] + 0.5;
        if (std::abs(y_native[i] - expected) > 1e-10 ||
            std::abs(y_p2300[i] - expected) > 1e-10 ||
            std::abs(y_bulk[i] - expected) > 1e-10)
        {
            correct = false;
            break;
        }
    }
    std::cout << "\n  Correctness: " << (correct ? "PASS" : "FAIL") << "\n";

    // Summary ratios
    std::cout << "\n  Overhead ratio (P2300 continues_on / native): "
              << std::fixed << std::setprecision(2) << (p2300_avg / native_avg)
              << "x\n";
    std::cout << "  Speedup ratio (P2300 bulk / native):          "
              << std::fixed << std::setprecision(2)
              << (native_avg / p2300_bulk_avg) << "x\n";
}

// ==========================================================================
// Entry point
// ==========================================================================

int hpx_main(int, char*[])
{
    std::cout << "HPX P2300 Scheduling Benchmark\n";
    std::cout << "Worker threads: " << hpx::get_num_worker_threads() << "\n";

    run_microbenchmark();
    run_macrobenchmark();

    std::cout << "\n" << std::string(65, '=') << "\n";
    std::cout << "  Benchmark complete.\n";
    std::cout << std::string(65, '=') << "\n\n";

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(hpx_main, argc, argv);
}
