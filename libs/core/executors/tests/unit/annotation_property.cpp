//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/properties.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <cstdlib>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::string annotation;

void test_post_f(int passed_through, hpx::latch& l)
{
    HPX_TEST_EQ(passed_through, 42);

    annotation =
        hpx::threads::get_thread_description(hpx::threads::get_self_id())
            .get_description();

    l.count_down(1);
}

void test_post()
{
    using executor = hpx::execution::parallel_executor;

    std::string desc("test_post");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        hpx::latch l(2);
        hpx::parallel::execution::post(exec, &test_post_f, 42, std::ref(l));
        l.arrive_and_wait();

        HPX_TEST_EQ(annotation, desc);
    }

    {
        annotation.clear();
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        hpx::latch l(2);
        hpx::parallel::execution::post(exec, &test_post_f, 42, std::ref(l));
        l.arrive_and_wait();

        HPX_TEST_EQ(annotation, desc);
    }
}

hpx::thread::id test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);

    annotation =
        hpx::threads::get_thread_description(hpx::threads::get_self_id())
            .get_description();

    return hpx::this_thread::get_id();
}

void test_sync()
{
    using executor = hpx::execution::parallel_executor;

    std::string desc("test_sync");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        HPX_TEST(hpx::parallel::execution::sync_execute(exec, &test, 42) ==
            hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }

    {
        annotation.clear();
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        HPX_TEST(hpx::parallel::execution::sync_execute(exec, &test, 42) ==
            hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }
}

void test_async()
{
    using executor = hpx::execution::parallel_executor;

    std::string desc("test_async");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        HPX_TEST(
            hpx::parallel::execution::async_execute(exec, &test, 42).get() !=
            hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }

    {
        annotation.clear();
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        HPX_TEST(
            hpx::parallel::execution::async_execute(exec, &test, 42).get() !=
            hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }
}

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id test_f(hpx::future<void> f, int passed_through)
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_EQ(passed_through, 42);

    annotation =
        hpx::threads::get_thread_description(hpx::threads::get_self_id())
            .get_description();

    return hpx::this_thread::get_id();
}

void test_then()
{
    using executor = hpx::execution::parallel_executor;

    std::string desc("test_then");
    {
        hpx::future<void> f = hpx::make_ready_future();
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        HPX_TEST(hpx::parallel::execution::then_execute(
                     exec, &test_f, std::move(f), 42)
                     .get() != hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }

    {
        hpx::future<void> f = hpx::make_ready_future();
        annotation.clear();
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        HPX_TEST(hpx::parallel::execution::then_execute(
                     exec, &test_f, std::move(f), 42)
                     .get() != hpx::this_thread::get_id());
        HPX_TEST_EQ(annotation, desc);
    }
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test(int seq, hpx::thread::id tid, int passed_through)    //-V813
{
    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        annotation =
            hpx::threads::get_thread_description(hpx::threads::get_self_id())
                .get_description();
    }
}

void test_bulk_sync()
{
    using executor_type = hpx::execution::parallel_executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    executor_type executor;
    auto hint = hpx::execution::experimental::get_hint(executor);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_share_function |
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto no_sharing_exec = hpx::execution::to_hierarchical_spawning(
        hpx::execution::experimental::with_hint(executor, hint));

    std::string desc("test_bulk_sync");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, no_sharing_exec,
            desc);

        hpx::parallel::execution::bulk_sync_execute(
            exec, hpx::bind(&bulk_test, _1, tid, _2), 107, 42);
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_sync_execute(
            exec, &bulk_test, 107, tid, 42);
        HPX_TEST_EQ(annotation, desc);
    }

    {
        auto exec = hpx::execution::experimental::with_annotation(
            no_sharing_exec, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_sync_execute(
            exec, hpx::bind(&bulk_test, _1, tid, _2), 107, 42);
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_sync_execute(
            exec, &bulk_test, 107, tid, 42);
        HPX_TEST_EQ(annotation, desc);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_bulk_async()
{
    using executor = hpx::execution::parallel_executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    std::string desc("test_bulk_async");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                          exec, hpx::bind(&bulk_test, _1, tid, _2), 107, 42))
            .get();
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                          exec, &bulk_test, 107, tid, 42))
            .get();
        HPX_TEST_EQ(annotation, desc);
    }

    {
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        annotation.clear();
        hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                          exec, hpx::bind(&bulk_test, _1, tid, _2), 107, 42))
            .get();
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::when_all(hpx::parallel::execution::bulk_async_execute(
                          exec, &bulk_test, 107, tid, 42))
            .get();
        HPX_TEST_EQ(annotation, desc);
    }
}

///////////////////////////////////////////////////////////////////////////////
void bulk_test_f(int seq, hpx::shared_future<void> f, hpx::thread::id tid,
    int passed_through)    //-V813
{
    HPX_TEST(f.is_ready());    // make sure, future is ready

    f.get();    // propagate exceptions

    HPX_TEST_NEQ(tid, hpx::this_thread::get_id());
    HPX_TEST_EQ(passed_through, 42);

    if (seq == 0)
    {
        annotation =
            hpx::threads::get_thread_description(hpx::threads::get_self_id())
                .get_description();
    }
}

void test_bulk_then()
{
    using executor = hpx::execution::parallel_executor;

    hpx::thread::id tid = hpx::this_thread::get_id();

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;

    hpx::shared_future<void> f = hpx::make_ready_future();

    std::string desc("test_bulk_then");
    {
        auto exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, executor{}, desc);

        hpx::parallel::execution::bulk_then_execute(
            exec, hpx::bind(&bulk_test_f, _1, _2, tid, _3), 107, f, 42)
            .get();
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_then_execute(
            exec, &bulk_test_f, 107, f, tid, 42)
            .get();
        HPX_TEST_EQ(annotation, desc);
    }

    {
        auto exec =
            hpx::execution::experimental::with_annotation(executor{}, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_then_execute(
            exec, hpx::bind(&bulk_test_f, _1, _2, tid, _3), 107, f, 42)
            .get();
        HPX_TEST_EQ(annotation, desc);

        annotation.clear();
        hpx::parallel::execution::bulk_then_execute(
            exec, &bulk_test_f, 107, f, tid, 42)
            .get();
        HPX_TEST_EQ(annotation, desc);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_post_policy_prefer()
{
    std::string desc("test_post_policy_prefer");
    auto policy =
        hpx::experimental::prefer(hpx::execution::experimental::with_annotation,
            hpx::execution::par, desc);

    hpx::latch l(2);
    hpx::parallel::execution::post(
        policy.executor(), &test_post_f, 42, std::ref(l));
    l.arrive_and_wait();

    HPX_TEST_EQ(annotation, desc);
}

void test_post_policy()
{
    std::string desc("test_post_policy");
    auto policy = hpx::execution::experimental::with_annotation(
        hpx::execution::par, desc);

    hpx::latch l(2);
    hpx::parallel::execution::post(
        policy.executor(), &test_post_f, 42, std::ref(l));
    l.arrive_and_wait();

    HPX_TEST_EQ(annotation, desc);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_post();

    test_sync();
    test_async();
    test_then();

    test_bulk_sync();
    test_bulk_async();
    test_bulk_then();

    test_post_policy_prefer();
    test_post_policy();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif
