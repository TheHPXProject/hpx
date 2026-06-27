//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

template <typename Executor>
decltype(auto) disable_run_as_child(Executor&& exec)
{
    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

    return hpx::experimental::prefer(hpx::execution::experimental::with_hint,
        HPX_FORWARD(Executor, exec), hint);
}

hpx::threads::thread_stacksize get_stacksize()
{
    return hpx::threads::get_self_stacksize_enum();
}

void test_stacksize()
{
    using executor = hpx::execution::parallel_executor;

    executor exec;

    for (auto stacksize : {hpx::threads::thread_stacksize::small_,
             hpx::threads::thread_stacksize::medium,
             hpx::threads::thread_stacksize::large,
             hpx::threads::thread_stacksize::huge})
    {
        HPX_TEST(hpx::execution::experimental::get_stacksize(exec) ==
            hpx::threads::thread_stacksize::default_);

        auto newexec = disable_run_as_child(
            hpx::execution::experimental::with_stacksize(exec, stacksize));

        HPX_TEST(hpx::execution::experimental::get_stacksize(exec) ==
            hpx::threads::thread_stacksize::default_);
        HPX_TEST(
            hpx::execution::experimental::get_stacksize(newexec) == stacksize);
        HPX_TEST(
            hpx::parallel::execution::async_execute(newexec, &get_stacksize)
                .get() == stacksize);
    }
}

hpx::threads::thread_priority get_priority()
{
    return hpx::this_thread::get_priority();
}

void test_priority()
{
    using executor = hpx::execution::parallel_executor;

    executor exec;

    for (auto priority : {hpx::threads::thread_priority::low,
             hpx::threads::thread_priority::normal,
             hpx::threads::thread_priority::high})
    {
        HPX_TEST(hpx::execution::experimental::get_priority(exec) ==
            hpx::threads::thread_priority::default_);

        auto newexec = disable_run_as_child(
            hpx::execution::experimental::with_priority(exec, priority));

        HPX_TEST(hpx::execution::experimental::get_priority(exec) ==
            hpx::threads::thread_priority::default_);
        HPX_TEST(
            hpx::execution::experimental::get_priority(newexec) == priority);
        HPX_TEST(hpx::parallel::execution::async_execute(newexec, &get_priority)
                     .get() == priority);
    }
}

void test_hint()
{
    using executor = hpx::execution::parallel_executor;

    executor exec;

    auto orghint = hpx::execution::experimental::get_hint(exec);
    HPX_TEST(orghint.mode == hpx::threads::thread_schedule_hint_mode::none);
    HPX_TEST(orghint.hint == static_cast<std::int16_t>(-1));

    for (auto const mode : {hpx::threads::thread_schedule_hint_mode::none,
             hpx::threads::thread_schedule_hint_mode::thread,
             hpx::threads::thread_schedule_hint_mode::numa})
    {
        for (auto const hint : {0, 1})
        {
            hpx::threads::thread_schedule_hint newhint(
                mode, static_cast<std::int16_t>(hint));
            auto newexec = disable_run_as_child(
                hpx::execution::experimental::with_hint(exec, newhint));

            orghint = hpx::execution::experimental::get_hint(exec);
            HPX_TEST(
                orghint.mode == hpx::threads::thread_schedule_hint_mode::none);
            HPX_TEST(orghint.hint == static_cast<std::int16_t>(-1));

            newhint = hpx::execution::experimental::get_hint(newexec);
            HPX_TEST(newhint.mode == mode);
            HPX_TEST(newhint.hint == hint);
        }
    }
}

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
std::string get_annotation()
{
    return hpx::threads::get_thread_description(hpx::threads::get_self_id())
        .get_description();
}

void test_annotation()
{
    using executor = hpx::execution::parallel_executor;

    executor exec;

    std::string desc("test_async");
    {
        auto newexec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, exec, desc);

        auto newdesc =
            hpx::parallel::execution::async_execute(newexec, &get_annotation)
                .get();
        HPX_TEST_EQ(newdesc, desc);

        HPX_TEST_EQ(newdesc,
            std::string(hpx::execution::experimental::get_annotation(newexec)));
    }

    {
        auto newexec =
            hpx::execution::experimental::with_annotation(exec, desc);

        auto newdesc =
            hpx::parallel::execution::async_execute(newexec, &get_annotation)
                .get();
        HPX_TEST_EQ(newdesc, desc);

        HPX_TEST_EQ(newdesc,
            std::string(hpx::execution::experimental::get_annotation(newexec)));
    }
}
#endif

void test_num_cores()
{
    using executor = hpx::execution::parallel_executor;

    executor exec;
    auto const num_cores =
        hpx::execution::experimental::processing_units_count(exec);

    auto newexec =
        hpx::execution::experimental::with_processing_units_count(exec, 2);

    HPX_TEST(num_cores ==
        hpx::execution::experimental::processing_units_count(exec));
    HPX_TEST(static_cast<std::size_t>(2) ==
        hpx::execution::experimental::processing_units_count(newexec));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_stacksize();
    test_priority();
    test_hint();
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    test_annotation();
#endif
    test_num_cores();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
