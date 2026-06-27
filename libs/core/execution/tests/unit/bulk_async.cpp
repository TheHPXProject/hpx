//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
int bulk_test(hpx::thread::id const& tid, int value, bool is_par,
    int passed_through)    //-V813
{
    HPX_TEST_EQ(is_par, (tid != hpx::this_thread::get_id()));
    HPX_TEST_EQ(passed_through, 42);
    return value;
}

template <typename Executor>
void test_bulk_sync(Executor&& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), 0);

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    std::vector<int> results =
        hpx::parallel::execution::bulk_sync_execute(HPX_FORWARD(Executor, exec),
            hpx::bind(&bulk_test, tid, _1, false, _2), v, 42);

    HPX_TEST(std::equal(std::begin(results), std::end(results), std::begin(v)));
}

template <typename Executor>
void test_bulk_async(Executor&& exec)
{
    hpx::thread::id tid = hpx::this_thread::get_id();

    std::vector<int> v(107);
    std::iota(std::begin(v), std::end(v), 0);

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;

    auto results = hpx::parallel::execution::bulk_async_execute(
        exec, hpx::bind(&bulk_test, tid, _1, true, _2), v, 42);

    // bulk_async_execute returns either a future<vector<result_type>> or a
    // vector<future<result_type>>
    if constexpr (hpx::traits::is_future_v<decltype(results)>)
    {
        auto r = results.get();
        HPX_TEST(std::equal(std::begin(r), std::end(r), std::begin(v)));
    }
    else
    {
        HPX_TEST(std::equal(std::begin(results), std::end(results),
            std::begin(v), [](hpx::future<int>& lhs, int const& rhs) {
                return lhs.get() == rhs;
            }));
    }
}

template <typename Executor>
decltype(auto) disable_run_as_child(Executor&& exec)
{
    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

    return hpx::experimental::prefer(hpx::execution::experimental::with_hint,
        HPX_FORWARD(Executor, exec), hint);
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    hpx::execution::sequenced_executor seq_exec;
    test_bulk_sync(disable_run_as_child(seq_exec));

    hpx::execution::parallel_executor par_exec;
    hpx::execution::parallel_executor par_fork_exec(hpx::launch::fork);
    test_bulk_async(disable_run_as_child(par_exec));
    test_bulk_async(disable_run_as_child(par_fork_exec));

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
