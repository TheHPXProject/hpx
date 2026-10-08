//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/barrier.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

std::atomic<std::size_t> c1(0);
std::atomic<std::size_t> c2(0);

///////////////////////////////////////////////////////////////////////////////
void local_barrier_test_no_completion(std::shared_ptr<hpx::barrier<>> b)
{
    ++c1;

    // wait for all threads to enter the barrier
    b->arrive_and_wait();

    ++c2;
}

void test_barrier_empty_oncomplete()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        std::shared_ptr<hpx::barrier<>> b =
            std::make_shared<hpx::barrier<>>(threads + 1);
        c1 = 0;
        c2 = 0;

        // create the threads which will wait on the barrier
        std::vector<hpx::future<void>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.push_back(hpx::async(&local_barrier_test_no_completion, b));
        }

        b->arrive_and_wait();    // wait for all threads to enter the barrier
        HPX_TEST_EQ(threads, c1);

        hpx::wait_all(results);

        HPX_TEST_EQ(threads, c2);
    }
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::size_t> complete(0);

struct oncomplete
{
    void operator()() const
    {
        ++complete;
    }
};

void local_barrier_test(std::shared_ptr<hpx::barrier<oncomplete>> b)
{
    ++c1;

    // wait for all threads to enter the barrier
    b->arrive_and_wait();

    ++c2;
}

void test_barrier_oncomplete()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        std::shared_ptr<hpx::barrier<oncomplete>> b =
            std::make_shared<hpx::barrier<oncomplete>>(threads + 1);

        c1 = 0;
        c2 = 0;
        complete = 0;

        // create the threads which will wait on the barrier
        std::vector<hpx::future<void>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.push_back(hpx::async(&local_barrier_test, b));
        }

        b->arrive_and_wait();    // wait for all threads to enter the barrier
        HPX_TEST_EQ(threads, c1);

        hpx::wait_all(results);

        HPX_TEST_EQ(threads, c2);
        HPX_TEST_EQ(complete, std::size_t(1));
    }
}

///////////////////////////////////////////////////////////////////////////////
void local_barrier_test_no_completion_split(std::shared_ptr<hpx::barrier<>> b)
{
    // signal the barrier
    auto token = b->arrive();

    ++c1;

    // wait for all threads to enter the barrier
    b->wait(std::move(token));

    ++c2;
}

void test_barrier_empty_oncomplete_split()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        std::shared_ptr<hpx::barrier<>> b =
            std::make_shared<hpx::barrier<>>(threads + 1);
        c1 = 0;
        c2 = 0;

        // create the threads which will wait on the barrier
        std::vector<hpx::future<void>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.push_back(
                hpx::async(&local_barrier_test_no_completion_split, b));
        }

        b->arrive_and_wait();    // wait for all threads to enter the barrier
        HPX_TEST_EQ(threads, c1);

        hpx::wait_all(results);

        HPX_TEST_EQ(threads, c2);
    }
}

void local_barrier_test_split(std::shared_ptr<hpx::barrier<oncomplete>> b)
{
    // signal the barrier
    auto token = b->arrive();

    ++c1;

    // wait for all threads to enter the barrier
    b->wait(std::move(token));

    ++c2;
}

void test_barrier_oncomplete_split()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        std::shared_ptr<hpx::barrier<oncomplete>> b =
            std::make_shared<hpx::barrier<oncomplete>>(threads + 1);

        c1 = 0;
        c2 = 0;
        complete = 0;

        // create the threads which will wait on the barrier
        std::vector<hpx::future<void>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.push_back(hpx::async(&local_barrier_test_split, b));
        }

        b->arrive_and_wait();    // wait for all threads to enter the barrier
        HPX_TEST_EQ(threads, c1);

        hpx::wait_all(results);

        HPX_TEST_EQ(threads, c2);
        HPX_TEST_EQ(complete, std::size_t(1));
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_barrier_arrive_and_wait_single_expected()
{
    constexpr std::size_t tasks = 4;
    constexpr std::size_t iterations = 10000;

    hpx::barrier<> b(1);
    std::atomic<std::size_t> counter(0);

    std::vector<hpx::future<void>> results;
    results.reserve(tasks);
    for (std::size_t i = 0; i != tasks; ++i)
    {
        results.push_back(hpx::async([&b, &counter]() {
            for (std::size_t n = 0; n != iterations; ++n)
            {
                b.arrive_and_wait();
                ++counter;
            }
        }));
    }

    hpx::wait_all(results);

    for (std::size_t i = 0; i != tasks; ++i)
    {
        HPX_TEST(!results[i].has_exception());
    }

    HPX_TEST_EQ(counter.load(), std::size_t(tasks * iterations));
}

void test_barrier_oncomplete_single_expected()
{
    constexpr std::size_t tasks = 4;
    constexpr std::size_t iterations = 10000;

    complete = 0;
    hpx::barrier<oncomplete> b(1);
    std::atomic<std::size_t> counter(0);

    std::vector<hpx::future<void>> results;
    results.reserve(tasks);
    for (std::size_t i = 0; i != tasks; ++i)
    {
        results.push_back(hpx::async([&b, &counter]() {
            for (std::size_t n = 0; n != iterations; ++n)
            {
                b.arrive_and_wait();
                ++counter;
            }
        }));
    }

    hpx::wait_all(results);

    for (std::size_t i = 0; i != tasks; ++i)
    {
        HPX_TEST(!results[i].has_exception());
    }

    HPX_TEST_EQ(counter.load(), std::size_t(tasks * iterations));
    HPX_TEST_EQ(complete.load(), std::size_t(tasks * iterations));
}

void test_barrier_mixed_single_expected()
{
    constexpr std::size_t tasks = 4;
    constexpr std::size_t iterations = 10000;

    hpx::barrier<> b(1);
    std::atomic<std::size_t> counter(0);

    std::vector<hpx::future<void>> results;
    results.reserve(tasks);
    for (std::size_t i = 0; i != tasks; ++i)
    {
        results.push_back(hpx::async([&b, &counter, i]() {
            for (std::size_t n = 0; n != iterations; ++n)
            {
                if (i % 2 == 0)
                {
                    b.arrive_and_wait();
                }
                else
                {
                    auto token = b.arrive();
                    b.wait(std::move(token));
                }
                ++counter;
            }
        }));
    }

    hpx::wait_all(results);

    for (std::size_t i = 0; i != tasks; ++i)
    {
        HPX_TEST(!results[i].has_exception());
    }

    HPX_TEST_EQ(counter.load(), std::size_t(tasks * iterations));
}

void test_barrier_arrive_and_drop_single_expected()
{
    constexpr std::size_t initial_tasks = 4;
    constexpr std::size_t iterations = 10000;

    hpx::barrier<> b(initial_tasks);
    std::atomic<std::size_t> counter(0);

    for (std::size_t i = 0; i != initial_tasks - 1; ++i)
    {
        b.arrive_and_drop();
    }

    constexpr std::size_t runner_tasks = 4;
    std::vector<hpx::future<void>> results;
    results.reserve(runner_tasks);
    for (std::size_t i = 0; i != runner_tasks; ++i)
    {
        results.push_back(hpx::async([&b, &counter]() {
            for (std::size_t n = 0; n != iterations; ++n)
            {
                b.arrive_and_wait();
                ++counter;
            }
        }));
    }

    hpx::wait_all(results);

    for (std::size_t i = 0; i != runner_tasks; ++i)
    {
        HPX_TEST(!results[i].has_exception());
    }

    HPX_TEST_EQ(counter.load(), std::size_t(runner_tasks * iterations));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_barrier_empty_oncomplete();
    test_barrier_oncomplete();

    test_barrier_empty_oncomplete_split();
    test_barrier_oncomplete_split();

    test_barrier_arrive_and_wait_single_expected();
    test_barrier_oncomplete_single_expected();
    test_barrier_mixed_single_expected();
    test_barrier_arrive_and_drop_single_expected();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
