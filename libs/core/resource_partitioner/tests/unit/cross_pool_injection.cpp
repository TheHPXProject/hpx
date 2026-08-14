//  Copyright (c) 2019 John Biddiscombe
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test creates a random number of thread pools at startup
// and assigns a random number of threads to each (not exceeding the
// number of threads available in total). Tasks are created and assigned
// to random pools, with continuations assigned to another random pool
// using an executor per pool.
// The test is intended to stress test the scheduler and ensure that
// cross pool injection of tasks does not cause segfaults or other
// problems such as lockups.

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

std::size_t num_pools = 0;

// dummy function we will call using async
void dummy_task(std::size_t n)
{
    if (n != 0)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(n));
    }
}

inline std::size_t st_rand(std::size_t a, std::size_t b)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> dist(a, b);
    //
    return std::size_t(dist(gen));
}

int hpx_main()
{
    HPX_TEST_EQ(std::size_t(0), hpx::resource::get_pool_index("default"));
    HPX_TEST_EQ(std::size_t(0), hpx::resource::get_pool_index("pool-0"));

    // print partition characteristics
    hpx::threads::get_thread_manager().print_pools(std::cout);

    auto const sched = hpx::threads::get_self_id_data()->get_scheduler_base();
    if (std::string("core-shared_priority_queue_scheduler") ==
        sched->get_description())
    {
        auto const full_mask =
            hpx::resource::get_partitioner().get_pool_pus_mask(
                sched->get_parent_pool()->get_pool_name());
        sched->add_remove_scheduler_mode(
            // add these flags
            hpx::threads::policies::scheduler_mode::enable_stealing |
                hpx::threads::policies::scheduler_mode::enable_stealing_numa |
                hpx::threads::policies::scheduler_mode::
                    assign_work_thread_parent |
                hpx::threads::policies::scheduler_mode::steal_after_local,
            // remove these flags
            hpx::threads::policies::scheduler_mode::assign_work_round_robin |
                hpx::threads::policies::scheduler_mode::
                    steal_high_priority_first |
                hpx::threads::policies::scheduler_mode::enable_stealing |
                hpx::threads::policies::scheduler_mode::enable_stealing_numa,
            full_mask);
    }

    // setup executors for different task priorities on the pools
    std::vector<hpx::execution::parallel_executor> HP_executors;
    std::vector<hpx::execution::parallel_executor> NP_executors;
    for (std::size_t i = 0; i < num_pools; ++i)
    {
        std::string pool_name = "pool-" + std::to_string(i);
        HP_executors.emplace_back(&hpx::resource::get_thread_pool(pool_name),
            hpx::threads::thread_priority::high);
        NP_executors.emplace_back(&hpx::resource::get_thread_pool(pool_name),
            hpx::threads::thread_priority::default_);
    }

    // randomly create tasks that run on a random pool attach continuations to
    // them that run on different random pools
    int const loops = 1000;

    std::vector<hpx::future<void>> futures;
    futures.resize(loops);

    //
    std::cout << "1: Starting HP " << loops << std::endl;
    std::atomic<int> counter(loops);
    for (int i = 0; i < loops; ++i)
    {
        // high priority
        std::size_t random_pool_1 = st_rand(0, num_pools - 1);
        std::size_t random_pool_2 = st_rand(0, num_pools - 1);
        auto& exec_1 = HP_executors[random_pool_1];
        auto& exec_2 = HP_executors[random_pool_2];
        auto f1 = hpx::async(exec_1, &dummy_task, 0);
        futures[i] = f1.then(exec_2, [=, &counter](hpx::future<void>&&) {
            dummy_task(0);
            --counter;
        });
    }

    hpx::wait_all(futures);
    HPX_TEST(counter == 0);

    std::cout << "2: Starting NP " << loops << std::endl;
    counter = loops;
    for (int i = 0; i < loops; ++i)
    {
        // normal priority
        std::size_t random_pool_1 = st_rand(0, num_pools - 1);
        std::size_t random_pool_2 = st_rand(0, num_pools - 1);
        auto& exec_3 = NP_executors[random_pool_1];
        auto& exec_4 = NP_executors[random_pool_2];
        auto f3 = hpx::async(exec_3, &dummy_task, 0);
        futures[i] = f3.then(exec_4, [=, &counter](hpx::future<void>&&) {
            dummy_task(0);
            --counter;
        });
    }

    hpx::wait_all(futures);
    HPX_TEST(counter == 0);

    std::cout << "3: Starting HP->NP " << loops << std::endl;
    counter = loops;
    for (int i = 0; i < loops; ++i)
    {
        // mixed priority, HP->NP
        std::size_t random_pool_1 = st_rand(0, num_pools - 1);
        std::size_t random_pool_2 = st_rand(0, num_pools - 1);
        auto& exec_5 = HP_executors[random_pool_1];
        auto& exec_6 = NP_executors[random_pool_2];
        auto f5 = hpx::async(exec_5, &dummy_task, 0);
        futures[i] = f5.then(exec_6, [=, &counter](hpx::future<void>&&) {
            dummy_task(0);
            --counter;
        });
    }

    hpx::wait_all(futures);
    HPX_TEST(counter == 0);

    std::cout << "4: Starting NP->HP " << loops << std::endl;
    counter = loops;
    for (int i = 0; i < loops; ++i)
    {
        // mixed priority, NP->HP
        std::size_t random_pool_1 = st_rand(0, num_pools - 1);
        std::size_t random_pool_2 = st_rand(0, num_pools - 1);
        auto& exec_7 = NP_executors[random_pool_1];
        auto& exec_8 = HP_executors[random_pool_2];
        auto f7 = hpx::async(exec_7, &dummy_task, 0);
        futures[i] = f7.then(exec_8, [=, &counter](hpx::future<void>&&) {
            dummy_task(0);
            --counter;
        });
    }

    hpx::wait_all(futures);
    HPX_TEST(counter == 0);

    std::cout << "5: Starting suspending " << loops << std::endl;
    counter = loops;
    for (int i = 0; i < loops; ++i)
    {
        // tasks that depend on each other and need to suspend
        std::size_t random_pool_1 = st_rand(0, num_pools - 1);
        std::size_t random_pool_2 = st_rand(0, num_pools - 1);
        auto& exec_7 = NP_executors[random_pool_1];
        auto& exec_8 = HP_executors[random_pool_2];
        // random delay up to 5 milliseconds
        //std::size_t delay = st_rand(0, 5);
        auto f7 = hpx::async(exec_7, &dummy_task, 0);

        // we pass f7 as an argument to prevent direct execution from happening
        futures[i] = hpx::async(
            exec_8,
            [&counter](auto&& f7) mutable {
                // if f7 is not ready then f8 will suspend itself on get
                f7.get();
                dummy_task(0);
                --counter;
            },
            std::move(f7));
    }

    hpx::wait_all(futures);
    HPX_TEST(counter == 0);

    return hpx::local::finalize();
}

void init_resource_partitioner_handler(hpx::resource::partitioner& rp,
    hpx::program_options::variables_map const&,
    hpx::resource::scheduling_policy policy)
{
    num_pools = 0;

    // before adding pools - set the default pool name to "pool-0"
    rp.set_default_pool_name("pool-0");

    auto seed = std::time(nullptr);
    std::srand((unsigned int) seed);
    std::cout << "Random seed " << seed << std::endl;

    // create N pools
    std::size_t max_threads = rp.get_number_requested_threads();
    std::string pool_name;
    std::size_t threads_remaining = max_threads;
    std::size_t threads_in_pool = 0;

    // create pools randomly and add a random number of PUs to each pool
    for (hpx::resource::numa_domain const& d : rp.numa_domains())
    {
        for (hpx::resource::core const& c : d.cores())
        {
            for (hpx::resource::pu const& p : c.pus())
            {
                if (threads_in_pool == 0)
                {
                    // pick a random number of threads less than the max
                    threads_in_pool = (std::max) (std::size_t(1),
                        st_rand(0, max_threads / 2));
                    pool_name = "pool-" + std::to_string(num_pools);
                    rp.create_thread_pool(pool_name, policy);
                    num_pools++;

                    std::cout << "Created pool: " << pool_name << "\n";
                    std::cout << "  threads_in_pool: " << threads_in_pool
                              << "\n";
                }
                std::cout << "Added pu " << p.id() << " to " << pool_name
                          << "\n";
                rp.add_resource(p, pool_name);
                threads_in_pool--;
                if (threads_remaining-- == 0)
                {
                    std::cerr << "This should not happen!\n";
                    std::cerr << "  max_threads: " << max_threads << "\n";
                    std::cerr << "  num_pools: " << num_pools << "\n";
                    std::cerr << "  threads_in_pool: " << threads_in_pool
                              << "\n";
                }
            }
        }
    }
}

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy scheduler)
{
    hpx::local::init_params init_args;
    init_args.rp_callback =
        hpx::bind_back(init_resource_partitioner_handler, scheduler);
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    std::vector<hpx::resource::scheduling_policy> schedulers = {
        hpx::resource::scheduling_policy::local,
        hpx::resource::scheduling_policy::local_priority_fifo,
        hpx::resource::scheduling_policy::local_priority_fifo_double,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::local_priority_lifo,
#endif
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::abp_priority_fifo,
        hpx::resource::scheduling_policy::abp_priority_lifo,
#endif
        hpx::resource::scheduling_policy::static_,
        hpx::resource::scheduling_policy::static_priority,
    // The shared_priority scheduler sometimes hangs in this test.
    //hpx::resource::scheduling_policy::shared_priority,

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        hpx::resource::scheduling_policy::local_workrequesting_fifo,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::local_workrequesting_lifo,
#endif
        hpx::resource::scheduling_policy::local_workrequesting_mc,
#endif
    };

    for (auto const scheduler : schedulers)
    {
        test_scheduler(argc, argv, scheduler);
    }

    return hpx::util::report_errors();
}
