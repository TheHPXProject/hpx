///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/threads.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <vector>

#if !defined(__APPLE__)
#include <hwloc.h>
#endif

std::size_t thread_affinity_worker(std::size_t desired)
{
    // Returns the OS-thread number of the worker that is running this
    // PX-thread.
    std::size_t current = hpx::get_worker_thread_num();
    if (current == desired)
    {
#if !defined(__APPLE__)
        // extract the desired affinity mask
        hpx::runtime& rt = hpx::get_runtime();
        hpx::threads::topology const& t = rt.get_topology();
        hpx::threads::mask_type desired_mask = t.get_thread_affinity_mask(
            hpx::resource::get_partitioner().get_pu_num(current));

        std::size_t logical_idx = hpx::threads::find_first(desired_mask);

        std::size_t idx = 0;

        hwloc_topology_t topo;
        hwloc_topology_init(&topo);
        hwloc_topology_load(topo);

        int const pu_depth = hwloc_get_type_or_below_depth(topo, HWLOC_OBJ_PU);
        hwloc_obj_t const pu_obj = hwloc_get_obj_by_depth(
            topo, pu_depth, static_cast<unsigned>(logical_idx));
        idx = pu_obj->os_index;

        // retrieve the current affinity mask
        hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(cpuset);
        if (0 == hwloc_get_cpubind(topo, cpuset, HWLOC_CPUBIND_THREAD))
        {
            // sadly get_cpubind is not implemented for Windows based systems
            hwloc_cpuset_t cpuset_cmp = hwloc_bitmap_alloc();
            hwloc_bitmap_zero(cpuset_cmp);
            hwloc_bitmap_only(cpuset_cmp, unsigned(idx));
            HPX_TEST_EQ(hwloc_bitmap_compare(cpuset, cpuset_cmp), 0);
            hwloc_bitmap_free(cpuset_cmp);
        }
        else
        {
            HPX_TEST(false && "hwloc_get_cpubind(topo, cpuset, \
                        HWLOC_CPUBIND_THREAD) failed!");
        }

        hwloc_bitmap_free(cpuset);
        hwloc_topology_destroy(topo);
#endif
        return desired;
    }

    // This PX-thread has been run by the wrong OS-thread, make the foreman
    // try again by rescheduling it.
    return std::size_t(-1);
}

HPX_PLAIN_ACTION(thread_affinity_worker, thread_affinity_worker_action)

void check_in(std::set<std::size_t>& attendance, std::size_t t)
{
    if (std::size_t(-1) != t)
        attendance.erase(t);
}

void thread_affinity_foreman()
{
    // Get the number of worker OS-threads in use by this locality.
    std::size_t const os_threads = hpx::get_os_thread_count();

    // Find the global name of the current locality.
    hpx::id_type const here = hpx::find_here();

    // Populate a set with the OS-thread numbers of all OS-threads on this
    // locality. When the hello world message has been printed on a particular
    // OS-thread, we will remove it from the set.
    std::set<std::size_t> attendance;
    for (std::size_t os_thread = 0; os_thread < os_threads; ++os_thread)
        attendance.insert(os_thread);

    // As long as there are still elements in the set, we must keep scheduling
    // PX-threads. Because HPX features work-stealing task schedulers, we have
    // no way of enforcing which worker OS-thread will actually execute
    // each PX-thread.
    while (!attendance.empty())
    {
        // Each iteration, we create a task for each element in the set of
        // OS-threads that have not said "Hello world". Each of these tasks
        // is encapsulated in a future.
        std::vector<hpx::future<std::size_t>> futures;
        futures.reserve(attendance.size());

        for (std::size_t worker : attendance)
        {
            // Asynchronously start a new task. The task is encapsulated in a
            // future, which we can query to determine if the task has
            // completed.
            typedef thread_affinity_worker_action action_type;
            futures.push_back(hpx::async<action_type>(here, worker));
        }

        // Wait for all of the futures to finish. The callback version of the
        // hpx::wait_each function takes two arguments: a vector of futures,
        // and a binary callback.  The callback takes two arguments; the first
        // is the index of the future in the vector, and the second is the
        // return value of the future. hpx::wait_each doesn't return until
        // all the futures in the vector have returned.
        using hpx::placeholders::_1;
        hpx::wait_each(
            hpx::unwrapping(hpx::bind(&check_in, std::ref(attendance), _1)),
            futures);
    }
}

HPX_PLAIN_ACTION(thread_affinity_foreman, thread_affinity_foreman_action)

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& /*vm*/)
{
    {
        // Get a list of all available localities.
        std::vector<hpx::id_type> localities = hpx::find_all_localities();

        // Reserve storage space for futures, one for each locality.
        std::vector<hpx::future<void>> futures;
        futures.reserve(localities.size());

        for (hpx::id_type const& node : localities)
        {
            // Asynchronously start a new task. The task is encapsulated in a
            // future, which we can query to determine if the task has
            // completed.
            typedef thread_affinity_foreman_action action_type;
            futures.push_back(hpx::async<action_type>(node));
        }

        // The non-callback version of hpx::lcos::wait takes a single parameter,
        // a future of vectors to wait on. hpx::lcos::wait only returns when
        // all of the futures have finished.
        hpx::wait_all(futures);
    }

    // Initiate shutdown of the runtime system.
    hpx::finalize();
    return hpx::util::report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    hpx::program_options::options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}
#endif
