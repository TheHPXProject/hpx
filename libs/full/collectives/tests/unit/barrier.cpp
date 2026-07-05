//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void barrier_test(
    std::size_t num, std::size_t rank, std::atomic<std::size_t>& c)
{
    hpx::distributed::barrier b(
        hpx::util::format("local_barrier_test_{}", hpx::get_locality_id()), num,
        rank);
    ++c;

    // wait for all threads to enter the barrier
    b.wait();
}

///////////////////////////////////////////////////////////////////////////////
void local_tests(hpx::program_options::variables_map& vm)
{
    std::size_t threads = 0;
    if (vm.count("threads"))
        threads = vm["threads"].as<std::size_t>();

    std::size_t iterations = 0;
    if (vm.count("iterations"))
        iterations = vm["iterations"].as<std::size_t>();

    hpx::id_type here = hpx::find_here();
    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::distributed::barrier b(
            hpx::util::format("local_barrier_test_{}", hpx::get_locality_id()),
            threads + 1, 0);

        std::atomic<std::size_t> c(0);
        for (std::size_t j = 1; j <= threads; ++j)
        {
            hpx::async(&barrier_test, threads + 1, j, std::ref(c));
        }

        b.wait();    // wait for all threads to enter the barrier

        HPX_TEST_EQ(threads, c.load());
    }
}

///////////////////////////////////////////////////////////////////////////////
void remote_test_multiple(hpx::program_options::variables_map& vm)
{
    std::size_t iterations = 0;
    if (vm.count("iterations"))
        iterations = vm["iterations"].as<std::size_t>();

    char const* const barrier_test_name = "/test/barrier/multiple";

    hpx::distributed::barrier b(barrier_test_name);
    for (std::size_t i = 0; i != iterations; ++i)
        b.wait();
}

void remote_test_single(hpx::program_options::variables_map& vm)
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    if (localities.size() == 1)
        return;    // nothing to be done here

    std::size_t iterations = 0;
    if (vm.count("iterations"))
        iterations = vm["iterations"].as<std::size_t>();

    char const* const barrier_test_name_outer = "/test/barrier/single_outer";
    hpx::distributed::barrier outer(barrier_test_name_outer);

    char const* const barrier_test_name = "/test/barrier/single";
    for (std::size_t i = 0; i != iterations; ++i)
    {
        hpx::distributed::barrier b(barrier_test_name);
        b.wait();

        outer.wait();
    }
}

void test_release_from_non_hpx_thread(hpx::program_options::variables_map& vm)
{
    std::size_t iterations = 0;
    if (vm.count("iterations"))
        iterations = vm["iterations"].as<std::size_t>();

    for (std::size_t i = 0; i != iterations; ++i)
    {
        auto b = std::make_shared<hpx::distributed::barrier>(
            hpx::util::format(
                "/test/barrier/release_from_external_thread/{}/{}",
                hpx::get_locality_id(), i),
            1, 0);

        // Avoid joining a std::thread from hpx_main when running with a single
        // HPX worker thread. release() from the external thread internally uses
        // run_as_hpx_thread and needs the HPX scheduler to make progress.
        hpx::promise<void> release_finished;
        hpx::future<void> release_done = release_finished.get_future();

        std::thread t([b, p = HPX_MOVE(release_finished)]() mutable {
            try
            {
                b->release();
                p.set_value();
            }
            catch (...)
            {
                p.set_exception(std::current_exception());
            }
        });
        t.detach();

        // This suspends the current HPX thread and allows the scheduler to run
        // the HPX work queued by run_as_hpx_thread.
        release_done.get();
    }
}

void collectives_barrier_generation_tests()
{
    using namespace hpx::collectives;

    auto const default_generation_client = create_local_communicator(
        "/test/barrier/collectives/default_generation/", num_sites_arg(1),
        this_site_arg(0));
    hpx::collectives::barrier(
        default_generation_client, this_site_arg(0), generation_arg())
        .get();

    auto const zero_generation_client =
        create_local_communicator("/test/barrier/collectives/zero_generation/",
            num_sites_arg(1), this_site_arg(0));

    bool caught_exception = false;
    try
    {
        hpx::collectives::barrier(
            zero_generation_client, this_site_arg(0), generation_arg(0))
            .get();
    }
    catch (hpx::exception const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(caught_exception);

    hpx::collectives::barrier("/test/barrier/collectives/basename_async/",
        num_sites_arg(1), this_site_arg(0), generation_arg(1))
        .get();

    hpx::collectives::barrier(hpx::launch::sync,
        "/test/barrier/collectives/basename_sync/", num_sites_arg(1),
        this_site_arg(0), generation_arg(1));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    // Regression test for release() from non-HPX threads.
    test_release_from_non_hpx_thread(vm);

    collectives_barrier_generation_tests();

    local_tests(vm);

    remote_test_single(vm);
    remote_test_multiple(vm);

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    // Configure application-specific options
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("threads,T", value<std::size_t>()->default_value(64),
            "the number of threads to invoke")
        ("iterations", value<std::size_t>()->default_value(64),
            "the number of times to repeat the test")
        ;
    // clang-format on

    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {
        "hpx.os_threads=all", "hpx.run_hpx_main!=1"};

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
