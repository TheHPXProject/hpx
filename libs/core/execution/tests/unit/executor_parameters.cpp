//  Copyright (c) 2015-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "foreach_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename... Parameters>
void parameters_test_impl(Parameters&&... params)
{
    static_assert(
        hpx::util::all_of<
            hpx::traits::is_executor_parameters<Parameters>...>::value,
        "hpx::traits::is_executor_parameters<Parameters>::value");

    typedef std::random_access_iterator_tag iterator_tag;
    test_for_each(hpx::execution::seq.with(params...), iterator_tag());
    test_for_each(hpx::execution::par.with(params...), iterator_tag());
    test_for_each_async(
        hpx::execution::seq(hpx::execution::task).with(params...),
        iterator_tag());
    test_for_each_async(
        hpx::execution::par(hpx::execution::task).with(params...),
        iterator_tag());

    hpx::execution::sequenced_executor seq_exec;
    test_for_each(
        hpx::execution::seq.on(seq_exec).with(params...), iterator_tag());
    test_for_each_async(
        hpx::execution::seq(hpx::execution::task).on(seq_exec).with(params...),
        iterator_tag());

    hpx::execution::parallel_executor par_exec;
    test_for_each(
        hpx::execution::par.on(par_exec).with(params...), iterator_tag());
    test_for_each_async(
        hpx::execution::par(hpx::execution::task).on(par_exec).with(params...),
        iterator_tag());
}

template <typename... Parameters>
void parameters_test(Parameters&&... params)
{
    parameters_test_impl(std::ref(params)...);
    parameters_test_impl(std::forward<Parameters>(params)...);
}

void test_dynamic_chunk_size()
{
    {
        hpx::execution::experimental::dynamic_chunk_size dcs;
        parameters_test(dcs);
    }

    {
        hpx::execution::experimental::dynamic_chunk_size dcs(100);
        parameters_test(dcs);
    }
}

void test_static_chunk_size()
{
    {
        hpx::execution::experimental::static_chunk_size scs;
        parameters_test(scs);
    }

    {
        hpx::execution::experimental::static_chunk_size scs(100);
        parameters_test(scs);
    }
}

void test_adaptive_static_chunk_size()
{
    {
        hpx::execution::experimental::adaptive_static_chunk_size asc;
        parameters_test(asc);
    }
}

void test_guided_chunk_size()
{
    {
        hpx::execution::experimental::guided_chunk_size gcs;
        parameters_test(gcs);
    }

    {
        hpx::execution::experimental::guided_chunk_size gcs(100);
        parameters_test(gcs);
    }
}

void test_auto_chunk_size()
{
    {
        hpx::execution::experimental::auto_chunk_size acs;
        parameters_test(acs);
    }

    {
        hpx::execution::experimental::auto_chunk_size acs(
            std::chrono::milliseconds(1));
        parameters_test(acs);
    }
}

void test_persistent_auto_chunk_size()
{
    {
        hpx::execution::experimental::persistent_auto_chunk_size pacs;
        parameters_test(pacs);
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size pacs(
            std::chrono::milliseconds(0), std::chrono::milliseconds(1));
        parameters_test(pacs);
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size pacs(
            std::chrono::milliseconds(0));
        parameters_test(pacs);
    }
}

void test_num_cores()
{
    {
        hpx::execution::experimental::num_cores nc;
        parameters_test(nc);
    }

    {
        hpx::execution::experimental::num_cores nc(2);
        parameters_test(nc);
    }
}

void test_collect_execution_parameters()
{
    hpx::execution::experimental::chunking_parameters ep;
    hpx::execution::experimental::collect_chunking_parameters cep(ep);
    parameters_test(cep);
}

///////////////////////////////////////////////////////////////////////////////
struct timer_hooks_parameters
{
    explicit timer_hooks_parameters(char const* name)
      : name_(name)
    {
    }

    template <typename Executor>
    void mark_begin_execution(Executor&&) const
    {
    }

    template <typename Executor>
    void mark_end_of_scheduling(Executor&&) const
    {
    }

    template <typename Executor>
    void mark_end_execution(Executor&&) const
    {
    }

    std::string name_;
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    timer_hooks_parameters> : std::true_type
{
};

void test_combined_hooks()
{
    timer_hooks_parameters pacs("time_hooks");
    hpx::execution::experimental::auto_chunk_size acs;

    parameters_test(acs, pacs);
    parameters_test(pacs, acs);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_dynamic_chunk_size();
    test_static_chunk_size();
    test_adaptive_static_chunk_size();
    test_guided_chunk_size();
    test_auto_chunk_size();
    test_persistent_auto_chunk_size();
    test_num_cores();

    test_combined_hooks();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
