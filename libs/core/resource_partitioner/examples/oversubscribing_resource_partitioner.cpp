//  Copyright (c) 2017 Shoshana Jakobovits
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// make available M_PI
#if !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>
//

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
//
#include "system_characteristics.hpp"

// NB
// this test needs to be updated as it no longer does what it is supposed to do

namespace resource::pools {
    enum ids
    {
        DEFAULT = 0,
        MPI = 1,
        GPU = 2,
        MATRIX = 3,
    };
}    // namespace resource::pools

static bool use_pools = false;
static int pool_threads = 1;
static std::string const pool_name = "mpi";

// this is our custom scheduler type
using namespace hpx::threads::policies;
using hpx::threads::policies::scheduler_mode;

// dummy function we will call using async
void do_stuff(std::size_t n, bool printout)
{
    if (printout)
        std::cout << "[do stuff] " << n << "\n";

    double convert = 2.0 * M_PI / static_cast<double>(n);
    for (std::size_t i(0); i < n; ++i)
    {
        double f = std::sin(convert * static_cast<double>(i));
        if (printout)
            std::cout << "sin(" << i << ") = " << f << ", ";
    }
    if (printout)
        std::cout << "\n";
}

// this is called on an hpx thread after the runtime starts up
int hpx_main(/*hpx::program_options::variables_map& vm*/)
{
    std::size_t num_threads = hpx::get_num_worker_threads();
    std::cout << "HPX using threads = " << num_threads << std::endl;

    if (num_threads == 1)
    {
        HPX_THROW_EXCEPTION(hpx::error::commandline_option_error, "hpx_main",
            "the oversubscribing_resource_partitioner example requires at "
            "least 2 worker threads (1 given)");
    }

    std::size_t loop_count = num_threads * 1;
    std::size_t async_count = num_threads * 1;

    // create an executor with high priority for important tasks
    hpx::execution::parallel_executor high_priority_executor(
        hpx::this_thread::get_pool(), hpx::threads::thread_priority::critical);
    hpx::execution::parallel_executor normal_priority_executor;

    hpx::execution::parallel_executor mpi_executor;
    // create an executor on the mpi pool
    if (use_pools)
    {
        // get executors
        mpi_executor = hpx::execution::parallel_executor(
            &hpx::resource::get_thread_pool(pool_name));
        std::cout << "\n[hpx_main] got mpi executor " << std::endl;
    }
    else
    {
        mpi_executor = high_priority_executor;
    }

    // print partition characteristics
    std::cout << "\n\n[hpx_main] print resource_partitioner characteristics : "
              << "\n";
    hpx::resource::get_partitioner().print_init_pool_data(std::cout);

    // print partition characteristics
    std::cout << "\n\n[hpx_main] print thread-manager pools : "
              << "\n";
    hpx::threads::get_thread_manager().print_pools(std::cout);

    // print system characteristics
    print_system_characteristics();

    // use executor to schedule work on custom pool
    hpx::future<void> future_1 = hpx::async(mpi_executor, &do_stuff, 5, true);

    hpx::future<void> future_2 = future_1.then(
        mpi_executor, [](hpx::future<void>&&) { do_stuff(5, true); });

    hpx::future<void> future_3 = future_2.then(mpi_executor,
        [mpi_executor, high_priority_executor, async_count](
            hpx::future<void>&&) mutable {
            hpx::future<void> future_4, future_5;
            for (std::size_t i = 0; i < async_count; i++)
            {
                if (i % 2 == 0)
                {
                    future_4 =
                        hpx::async(mpi_executor, &do_stuff, async_count, false);
                }
                else
                {
                    future_5 = hpx::async(
                        high_priority_executor, &do_stuff, async_count, false);
                }
            }
            // the last futures we made are stored in here
            future_4.get();
            future_5.get();
        });

    future_3.get();

    hpx::mutex m;
    std::set<std::thread::id> thread_set;

    // test a parallel algorithm on custom pool with high priority
    hpx::execution::experimental::static_chunk_size fixed(1);
    hpx::experimental::for_loop_strided(
        hpx::execution::par.with(fixed).on(high_priority_executor), 0,
        loop_count, 1, [&](std::size_t i) {
            std::lock_guard<hpx::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second)
            {
                std::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " high priority i " << std::dec << i << std::endl;
            }
        });
    std::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    // test a parallel algorithm on custom pool with normal priority
    hpx::experimental::for_loop_strided(
        hpx::execution::par.with(fixed).on(normal_priority_executor), 0,
        loop_count, 1, [&](std::size_t i) {
            std::lock_guard<hpx::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second)
            {
                std::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " normal priority i " << std::dec << i
                          << std::endl;
            }
        });
    std::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    // test a parallel algorithm on mpi_executor
    hpx::experimental::for_loop_strided(
        hpx::execution::par.with(fixed).on(mpi_executor), 0, loop_count, 1,
        [&](std::size_t i) {
            std::lock_guard<hpx::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second)
            {
                std::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " mpi pool i " << std::dec << i << std::endl;
            }
        });
    std::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    //     auto high_priority_async_policy =
    //         hpx::launch::async_policy(hpx::threads::thread_priority::critical);
    //     auto normal_priority_async_policy = hpx::launch::async_policy();

    // test a parallel algorithm on custom pool with high priority
    hpx::experimental::for_loop_strided(
        hpx::execution::par.with(fixed /*, high_priority_async_policy*/)
            .on(mpi_executor),
        0, loop_count, 1, [&](std::size_t i) {
            std::lock_guard<hpx::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second)
            {
                std::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " high priority mpi i " << std::dec << i
                          << std::endl;
            }
        });
    std::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    return hpx::local::finalize();
}

// -------------------------------------------------------------------------
void init_resource_partitioner_handler(hpx::resource::partitioner& rp,
    hpx::program_options::variables_map const& vm)
{
    use_pools = vm.count("use-pools") != 0;
    pool_threads = vm["pool-threads"].as<int>();

    std::cout << "[hpx_main] starting ..."
              << "use_pools " << use_pools << " "
              << "pool-threads " << pool_threads << "\n";

    if (pool_threads > 0)
    {
        // we use unspecified as the scheduler type and it will be set according to
        // the --hpx:queuing=xxx option or default.
        hpx::threads::policies::scheduler_mode deft =
            hpx::threads::policies::scheduler_mode::default_;
        rp.create_thread_pool(
            pool_name, hpx::resource::scheduling_policy::shared_priority, deft);
        // add N pus to network pool
        int count = 0;
        for (hpx::resource::numa_domain const& d : rp.numa_domains())
        {
            for (hpx::resource::core const& c : d.cores())
            {
                for (hpx::resource::pu const& p : c.pus())
                {
                    if (count < pool_threads)
                    {
                        std::cout << "Added pu " << count++ << " to pool \""
                                  << pool_name << "\"\n";
                        rp.add_resource(p, pool_name);
                    }
                }
            }
        }

        rp.create_thread_pool(
            "default", hpx::resource::scheduling_policy::unspecified, deft);
    }
}

// the normal int main function that is called at startup and runs on an OS
// thread the user must call hpx::local::init to start the hpx runtime which
// will execute hpx_main on an hpx thread
int main(int argc, char* argv[])
{
    // clang-format off
    hpx::program_options::options_description desc_cmdline("Test options");
    desc_cmdline.add_options()
        ("use-pools,u", "Enable advanced HPX thread pools and executors")
        ("use-scheduler,s", "Enable custom priority scheduler")
        ("pool-threads,m", hpx::program_options::value<int>()->default_value(1),
            "Number of threads to assign to custom pool");
    // clang-format on

    hpx::local::init_params iparams;

    iparams.desc_cmdline = desc_cmdline;
    iparams.rp_mode = hpx::resource::partitioner_mode::allow_oversubscription;
    iparams.rp_callback = init_resource_partitioner_handler;

    return hpx::local::init(hpx_main, argc, argv, iparams);
}

#endif
