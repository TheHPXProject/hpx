//  Copyright (c) 2020-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// This test verifies that all parameters customization points dispatch through
// the executor before potentially being handled by the parameters object.

std::atomic<std::size_t> params_count(0);
std::atomic<std::size_t> exec_count(0);

///////////////////////////////////////////////////////////////////////////////
// get_chunks_size

struct test_executor_get_chunk_size : hpx::execution::parallel_executor
{
    test_executor_get_chunk_size() = default;

    template <typename Parameters>
    friend std::size_t tag_invoke(
        hpx::execution::experimental::get_chunk_size_t, Parameters&&,
        test_executor_get_chunk_size, hpx::chrono::steady_duration const&,
        std::size_t cores, std::size_t count)
    {
        ++exec_count;
        return (count + cores - 1) / cores;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_get_chunk_size> : std::true_type
{
};

struct test_chunk_size
{
    template <typename Executor>
    friend std::size_t tag_override_invoke(
        hpx::execution::experimental::get_chunk_size_t, test_chunk_size,
        Executor&&, hpx::chrono::steady_duration const&, std::size_t cores,
        std::size_t count)
    {
        ++params_count;
        return (count + cores - 1) / cores;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<test_chunk_size>
  : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_get_chunk_size()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::get_chunk_size(
            test_chunk_size{}, hpx::execution::par.executor(), 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::get_chunk_size(test_chunk_size{},
            hpx::execution::par.executor(), hpx::chrono::null_duration, 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::get_chunk_size(
            test_chunk_size{}, test_executor_get_chunk_size{}, 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::get_chunk_size(
            hpx::execution::par.parameters(), test_executor_get_chunk_size{},
            hpx::chrono::null_duration, 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::get_chunk_size(
            hpx::execution::par.parameters(), test_executor_get_chunk_size{}, 1,
            1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }
}

///////////////////////////////////////////////////////////////////////////////
// measure_iteration

struct test_executor_measure_iteration : hpx::execution::parallel_executor
{
    test_executor_measure_iteration() = default;

    template <typename Parameters, typename F>
    friend auto tag_invoke(hpx::execution::experimental::measure_iteration_t,
        Parameters&&, test_executor_measure_iteration, F&&, std::size_t)
    {
        ++exec_count;
        return hpx::chrono::null_duration;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_measure_iteration> : std::true_type
{
};

struct test_measure_iteration
{
    template <typename Executor, typename F>
    friend auto tag_override_invoke(
        hpx::execution::experimental::measure_iteration_t,
        test_measure_iteration, Executor&&, F&&, std::size_t)
    {
        ++params_count;
        return hpx::chrono::null_duration;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    test_measure_iteration> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_get_measure_iteration()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::measure_iteration(
            test_measure_iteration{}, hpx::execution::par.executor(),
            [](std::size_t) { return 0; }, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::measure_iteration(
            hpx::execution::par.parameters(), test_executor_measure_iteration{},
            [](std::size_t) { return 0; }, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::measure_iteration(
            test_measure_iteration{}, test_executor_measure_iteration{},
            [](std::size_t) { return 0; }, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

///////////////////////////////////////////////////////////////////////////////
// maximal_number_of_chunks

struct test_executor_maximal_number_of_chunks
  : hpx::execution::parallel_executor
{
    test_executor_maximal_number_of_chunks() = default;

    template <typename Parameters>
    friend std::size_t tag_invoke(
        hpx::execution::experimental::maximal_number_of_chunks_t, Parameters&&,
        test_executor_maximal_number_of_chunks, std::size_t,
        std::size_t num_tasks)
    {
        ++exec_count;
        return num_tasks;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_maximal_number_of_chunks> : std::true_type
{
};

struct test_number_of_chunks
{
    template <typename Executor>
    friend std::size_t tag_override_invoke(
        hpx::execution::experimental::maximal_number_of_chunks_t,
        test_number_of_chunks, Executor&&, std::size_t, std::size_t num_tasks)
    {
        ++params_count;
        return num_tasks;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    test_number_of_chunks> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_maximal_number_of_chunks()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::maximal_number_of_chunks(
            test_number_of_chunks{}, hpx::execution::par.executor(), 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::maximal_number_of_chunks(
            hpx::execution::par.parameters(),
            test_executor_maximal_number_of_chunks{}, 1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::maximal_number_of_chunks(
            test_number_of_chunks{}, test_executor_maximal_number_of_chunks{},
            1, 1);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

///////////////////////////////////////////////////////////////////////////////
// reset_thread_distribution

struct test_executor_reset_thread_distribution
  : hpx::execution::parallel_executor
{
    test_executor_reset_thread_distribution() = default;

    template <typename Parameters>
    friend void tag_invoke(
        hpx::execution::experimental::reset_thread_distribution_t, Parameters&&,
        test_executor_reset_thread_distribution)
    {
        ++exec_count;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_reset_thread_distribution> : std::true_type
{
};

struct test_thread_distribution
{
    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::reset_thread_distribution_t,
        test_thread_distribution, Executor&&)
    {
        ++params_count;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    test_thread_distribution> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_reset_thread_distribution()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::reset_thread_distribution(
            test_thread_distribution{}, hpx::execution::par.executor());

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::reset_thread_distribution(
            hpx::execution::par.parameters(),
            test_executor_reset_thread_distribution{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::reset_thread_distribution(
            test_thread_distribution{},
            test_executor_reset_thread_distribution{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

///////////////////////////////////////////////////////////////////////////////
// processing_units_count

struct test_executor_processing_units_count : hpx::execution::parallel_executor
{
    test_executor_processing_units_count() = default;

    template <typename Parameters>
    friend std::size_t tag_invoke(
        hpx::execution::experimental::processing_units_count_t, Parameters&&,
        test_executor_processing_units_count,
        hpx::chrono::steady_duration const&, std::size_t)
    {
        ++exec_count;
        return 1;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_processing_units_count> : std::true_type
{
};

struct test_processing_units
{
    template <typename Executor>
    friend std::size_t tag_override_invoke(
        hpx::execution::experimental::processing_units_count_t,
        test_processing_units, Executor&&,
        hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
        std::size_t = 0)
    {
        ++params_count;
        return 1;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    test_processing_units> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_processing_units_count()
{
    {
        params_count = 0;

        hpx::execution::experimental::processing_units_count(
            test_processing_units{}, hpx::execution::parallel_executor());

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::processing_units_count(
            test_processing_units{}, test_executor_processing_units_count{},
            hpx::chrono::null_duration, 0);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;

        auto p = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 2);

        std::size_t const num_cores =
            hpx::execution::experimental::processing_units_count(
                test_processing_units{}, p.executor());

        HPX_TEST_EQ(num_cores, static_cast<std::size_t>(1));
        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
    }

    {
        hpx::execution::experimental::num_cores nc(2);
        auto p = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, nc);

        std::size_t const num_cores =
            hpx::execution::experimental::processing_units_count(
                hpx::execution::experimental::null_parameters, p.executor(),
                hpx::chrono::null_duration, 0);

        HPX_TEST_EQ(num_cores, static_cast<std::size_t>(2));
    }

    {
        auto p = hpx::execution::experimental::with_processing_units_count(
            hpx::execution::par, 2);

        std::size_t const num_cores =
            hpx::execution::experimental::processing_units_count(p);

        HPX_TEST_EQ(num_cores, static_cast<std::size_t>(2));
    }
}

///////////////////////////////////////////////////////////////////////////////
// mark_begin_execution, mark_end_of_scheduling, mark_end_execution

struct test_executor_begin_end : hpx::execution::parallel_executor
{
    test_executor_begin_end() = default;

    template <typename Parameters>
    friend void tag_invoke(hpx::execution::experimental::mark_begin_execution_t,
        Parameters&&, test_executor_begin_end)
    {
        ++exec_count;
    }

    template <typename Parameters>
    friend void tag_invoke(
        hpx::execution::experimental::mark_end_of_scheduling_t, Parameters&&,
        test_executor_begin_end)
    {
        ++exec_count;
    }

    template <typename Parameters>
    friend void tag_invoke(hpx::execution::experimental::mark_end_execution_t,
        Parameters&&, test_executor_begin_end)
    {
        ++exec_count;
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_begin_end> : std::true_type
{
};

struct test_begin_end
{
    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::mark_begin_execution_t, test_begin_end,
        Executor&&)
    {
        ++params_count;
    }

    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::mark_end_of_scheduling_t, test_begin_end,
        Executor&&)
    {
        ++params_count;
    }

    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::mark_end_execution_t, test_begin_end,
        Executor&&)
    {
        ++params_count;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<test_begin_end>
  : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_mark_begin_execution()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_begin_execution(
            test_begin_end{}, hpx::execution::par.executor());

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_begin_execution(
            hpx::execution::par.parameters(), test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_begin_execution(
            test_begin_end{}, test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

void test_mark_end_of_scheduling()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_of_scheduling(
            test_begin_end{}, hpx::execution::par.executor());

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_of_scheduling(
            hpx::execution::par.parameters(), test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_of_scheduling(
            test_begin_end{}, test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

void test_mark_end_execution()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_execution(
            test_begin_end{}, hpx::execution::par.executor());

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_execution(
            hpx::execution::par.parameters(), test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::mark_end_execution(
            test_begin_end{}, test_executor_begin_end{});

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

///////////////////////////////////////////////////////////////////////////////
// adjust_chunk_size_and_max_chunks

struct test_executor_adjust_chunk_size : hpx::execution::parallel_executor
{
    test_executor_adjust_chunk_size() = default;

    template <typename Parameters>
    friend std::pair<std::size_t, std::size_t> tag_invoke(
        hpx::execution::experimental::adjust_chunk_size_and_max_chunks_t,
        Parameters&&, test_executor_adjust_chunk_size,
        std::size_t /*num_elements*/, std::size_t /*num_cores*/,
        std::size_t /*num_chunks*/, std::size_t /*chunk_size*/)
    {
        ++exec_count;
        return {1, 1};
    }
};

template <>
struct hpx::execution::experimental::is_two_way_executor<
    test_executor_adjust_chunk_size> : std::true_type
{
};

struct test_adjust_chunk_size
{
    template <typename Executor>
    friend std::pair<std::size_t, std::size_t> tag_override_invoke(
        hpx::execution::experimental::adjust_chunk_size_and_max_chunks_t,
        test_adjust_chunk_size, Executor&&, std::size_t /*num_elements*/,
        std::size_t /*num_cores*/, std::size_t /*num_chunks*/,
        std::size_t /*chunk_size*/)
    {
        ++params_count;
        return {1, 1};
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    test_adjust_chunk_size> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
void test_adjust_chunk_size_and_max_chunks()
{
    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::adjust_chunk_size_and_max_chunks(
            test_adjust_chunk_size{}, hpx::execution::par.executor(), 100, 4, 0,
            0);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::adjust_chunk_size_and_max_chunks(
            hpx::execution::par.parameters(), test_executor_adjust_chunk_size{},
            100, 4, 0, 0);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(0));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(1));
    }

    {
        params_count = 0;
        exec_count = 0;

        hpx::execution::experimental::adjust_chunk_size_and_max_chunks(
            test_adjust_chunk_size{}, test_executor_adjust_chunk_size{}, 100, 4,
            0, 0);

        HPX_TEST_EQ(params_count, static_cast<std::size_t>(1));
        HPX_TEST_EQ(exec_count, static_cast<std::size_t>(0));
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_get_chunk_size();
    test_get_measure_iteration();
    test_maximal_number_of_chunks();
    test_reset_thread_distribution();
    test_processing_units_count();
    test_mark_begin_execution();
    test_mark_end_of_scheduling();
    test_mark_end_execution();

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
