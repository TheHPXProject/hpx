//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies the wiring between the --hpx:queuing command line option
// and the hpx::resource::scheduling_policy that ends up being used by the
// "default" pool, i.e. the prefix-matching logic implemented in
// partitioner::setup_schedulers(). It spawns the runtime once per queuing
// string (similar to cross_pool_injection.cpp) so that the actual command line
// parsing path is exercised, rather than only the explicit scheduling_policy
// enum API.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace {

    auto expected_policy = hpx::resource::scheduling_policy::unspecified;

    int hpx_main()
    {
        HPX_TEST_EQ(
            static_cast<int>(
                hpx::resource::get_partitioner().which_scheduler("default")),
            static_cast<int>(expected_policy));

        // make sure the pool actually constructed a working scheduler, not just
        // that the enum resolution is correct
        HPX_TEST_EQ(hpx::async([]() { return 42; }).get(), 42);

        return hpx::local::finalize();
    }

    // Spawn the runtime with the given --hpx:queuing value and verify that it
    // resolves to the expected scheduling policy.
    void test_queuing_string(int const argc, char* argv[],
        std::string const& queuing_value,
        hpx::resource::scheduling_policy const policy)
    {
        expected_policy = policy;

        hpx::local::init_params init_args;
        init_args.cfg = {"--hpx:queuing=" + queuing_value};

        HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    }

    // Spawn the runtime with a --hpx:queuing value that does not match (as a
    // prefix) any known scheduler name and verify that this is rejected rather
    // than silently mapped to some scheduler. This pins down the current
    // fallback behavior as a regression guard. hpx_main is never invoked in
    // this case, as the runtime fails to start.
    void test_invalid_queuing_string(
        int const argc, char* argv[], std::string const& queuing_value)
    {
        hpx::local::init_params init_args;
        init_args.cfg = {"--hpx:queuing=" + queuing_value};

        HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), -1);
    }

    // Construct the scheduler directly through
    // partitioner::create_thread_pool(), bypassing --hpx:queuing command line
    // parsing altogether. This isolates the
    // threadmanager::create_scheduler_local_priority_fifo_double dispatch from
    // the string-mapping logic exercised above.
    void test_explicit_policy(int const argc, char* argv[])
    {
        expected_policy =
            hpx::resource::scheduling_policy::local_priority_fifo_double;

        hpx::local::init_params init_args;
        init_args
            .rp_callback = [](hpx::resource::partitioner& rp,
                               hpx::program_options::variables_map const&) {
            rp.create_thread_pool("default",
                hpx::resource::scheduling_policy::local_priority_fifo_double);
        };

        HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    }
}    // namespace

int main(int argc, char* argv[])
{
    // Explicit scheduling_policy API, bypassing command line parsing.
    test_explicit_policy(argc, argv);

    // Some CIs invoke tests with --hpx:queuing=... which would cause duplicated
    // command line option exceptions below. Remove this option from argc/argv
    // if found.
    char** end = std::remove_if(&argv[0], &argv[argc], [](char const* arg) {
        return std::strstr(arg, "--hpx:queuing") != nullptr;
    });

    if (end != &argv[argc])
    {
        argc = static_cast<int>(end - &argv[0]);
        *end = nullptr;
    }

    // command line option exceptions below. Exact match: must resolve to
    // local_priority_fifo and must not be short-circuited into the
    // local_priority_fifo_double branch that is checked immediately afterward
    // in partitioner::setup_schedulers().
    test_queuing_string(argc, argv, "local-priority-fifo",
        hpx::resource::scheduling_policy::local_priority_fifo);

    // Must resolve to local_priority_fifo_double, and must not be
    // short-circuited by the shorter local-priority-fifo branch that is checked
    // earlier in the if/else-if chain.
    test_queuing_string(argc, argv, "local-priority-fifo-double",
        hpx::resource::scheduling_policy::local_priority_fifo_double);

    // A value that is not a prefix of any known scheduler name.
    test_invalid_queuing_string(argc, argv, "not-a-real-scheduler");

    return hpx::util::report_errors();
}
