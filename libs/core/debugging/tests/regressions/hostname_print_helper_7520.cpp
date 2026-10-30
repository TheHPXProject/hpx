//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for issue #7520.
//
// hostname_print_helper::get_hostname() used to lazily fill a static buffer
// behind a plain bool flag with a trivial constant initializer, so the
// compiler's magic-static guard did not cover it. Two threads reaching the
// helper concurrently (e.g. from on_start_thread during worker startup)
// could both observe the flag as false and race on the flag and the buffer.
//
// This test releases many OS threads onto get_hostname() at the same time so
// that first use is genuinely contended, and checks that every thread
// observed the same fully-formed hostname string.

#include <hpx/modules/debugging.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(__FreeBSD__)
extern char** environ;
#endif

int main()
{
#if defined(__FreeBSD__)
    freebsd_environ = environ;
#endif

    constexpr int num_threads = 32;
    hpx::debug::detail::hostname_print_helper helper;

    // Written from the worker threads, read after join(). The mutex only
    // keeps ThreadSanitizer quiet about the vector element writes; the
    // equality checks below are what actually validates the race fix.
    std::shared_mutex results_mutex;
    std::vector<std::string> results(num_threads);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::atomic<int> ready{0};
    std::atomic<bool> go{false};

    for (int i = 0; i != num_threads; ++i)
    {
        threads.emplace_back(
            [i, &helper, &results, &results_mutex, &ready, &go]() {
                // Hold every thread at the gate so that the first calls into
                // get_hostname() are actually contended instead of being
                // serialized by thread creation.
                ready.fetch_add(1, std::memory_order_acq_rel);
                while (!go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }

                // The first call must return a pointer to the same, fully
                // written buffer for every thread.
                char const* name = helper.get_hostname();
                if (name != nullptr)
                {
                    std::unique_lock<std::shared_mutex> lock(results_mutex);
                    results[i] = name;
                }
            });
    }

    // Wait until all threads are parked at the gate, then release them.
    while (ready.load(std::memory_order_acquire) != num_threads)
    {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);

    for (auto& t : threads)
    {
        t.join();
    }

    // Every thread must have seen an identical hostname string; a race in
    // the lazy initialization could otherwise hand some threads a partially
    // written or truncated buffer.
    std::string const& first = results[0];
    for (int i = 1; i != num_threads; ++i)
    {
        HPX_TEST_EQ(results[i], first);
    }

    // The hostname itself must come out intact (not garbled by a torn
    // write). On FreeBSD the gethostname call is skipped, so the buffer may
    // legitimately be empty when no rank suffix is appended.
#if !defined(__FreeBSD__)
    HPX_TEST(!first.empty());
    HPX_TEST(
        first.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789()-_.") == std::string::npos);
#endif

    return hpx::util::report_errors();
}
