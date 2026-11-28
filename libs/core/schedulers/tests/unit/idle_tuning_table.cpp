//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle settings of a pool from the idle tuning table: a pool takes the
// entries of hpx.idle_tuning.<scheduler>.<tuner>.<number of workers>, else
// those of hpx.idle_tuning.<scheduler>.<tuner>, else the plain
// hpx.min_idle_backoff_time, hpx.max_idle_backoff_time and
// hpx.fast_idle_cap_time, setting by setting; an invalid entry counts as
// missing. The runtime starts twice with the same table, its default pool (of
// the tuned scheduler, whose settings can be read back) once of two workers
// and once of one; the other worker threads, if there are more, go to a
// second pool. It needs two worker threads.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <string>

namespace policies = hpx::threads::policies;

using tuned_scheduler = policies::local_priority_tuned_scheduler<>;

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
namespace {

    // a setting of the first worker of the default pool
    float pool_setting(char const* key)
    {
        auto* scheduler = dynamic_cast<tuned_scheduler*>(
            hpx::resource::get_thread_pool("default").get_scheduler());
        HPX_TEST(scheduler != nullptr);
        if (scheduler == nullptr)
            return -1.0f;

        auto& parameters = scheduler->get_tuning_parameters();
        std::size_t const id = parameters.find(key);
        HPX_TEST_NEQ(id, policies::scheduler_tuning_parameters::npos);
        if (id == policies::scheduler_tuning_parameters::npos)
            return -1.0f;
        return parameters.get<float>(0, id);
    }

    int test_two_workers()
    {
        HPX_TEST_EQ(hpx::resource::get_num_threads("default"), std::size_t(2));

        // the longest step from the row of its size; the spin there is
        // invalid, so the one of the row for any size
        HPX_TEST_EQ(pool_setting("hpx.max_idle_backoff_time"), 700.0f);
        HPX_TEST_EQ(pool_setting("hpx.min_idle_backoff_time"), 20.0f);
        // the cap of the fast idle rounds from the row of its size
        HPX_TEST_EQ(
            hpx::resource::get_thread_pool("default").get_fast_idle_cap_time(),
            1000.0);
        return hpx::local::finalize();
    }

    int test_one_worker()
    {
        HPX_TEST_EQ(hpx::resource::get_num_threads("default"), std::size_t(1));

        // no row of its size: the row for any size; that row has no longest
        // step, so the plain value
        HPX_TEST_EQ(pool_setting("hpx.min_idle_backoff_time"), 20.0f);
        HPX_TEST_EQ(pool_setting("hpx.max_idle_backoff_time"), 900.0f);
        // no cap in the row for any size: the plain value
        HPX_TEST_EQ(
            hpx::resource::get_thread_pool("default").get_fast_idle_cap_time(),
            1500.0);
        return hpx::local::finalize();
    }

    // the table, and a default pool of the given size; the other worker
    // threads go to a pool of their own
    hpx::local::init_params configuration(std::size_t const pool_size)
    {
        std::string const table = "hpx.idle_tuning.";
        std::string const row = table + "local-priority-tuned-fifo.static";

        hpx::local::init_params init_args;
        init_args.cfg = {
            "hpx.scheduler_tuner=static",
            "hpx.min_idle_backoff_time=50",
            "hpx.max_idle_backoff_time=900",
            "hpx.fast_idle_cap_time=1500",
            // for pools of any size
            row + ".min_idle_backoff_time!=20",
            // for pools of two workers, the spin is ignored
            row + ".2.max_idle_backoff_time!=700",
            row + ".2.min_idle_backoff_time!=-5",
            row + ".2.fast_idle_cap_time!=1000",
            // another scheduler's and another tuner's rows don't apply
            table + "local-priority-fifo.none.min_idle_backoff_time!=7",
            table + "local-priority-tuned-fifo.none.min_idle_backoff_time!=8",
        };
        init_args
            .rp_callback = [pool_size](hpx::resource::partitioner& rp,
                               hpx::program_options::variables_map const&) {
            std::size_t const threads = rp.get_number_requested_threads();
            HPX_TEST_LTE(pool_size, threads);

            rp.create_thread_pool("default",
                hpx::resource::scheduling_policy::local_priority_tuned_fifo);
            if (threads > pool_size)
            {
                rp.create_thread_pool("rest",
                    hpx::resource::scheduling_policy::
                        local_priority_tuned_fifo);
            }

            std::size_t added = 0;
            for (hpx::resource::numa_domain const& d : rp.numa_domains())
            {
                for (hpx::resource::core const& c : d.cores())
                {
                    for (hpx::resource::pu const& p : c.pus())
                    {
                        if (added < threads)
                        {
                            rp.add_resource(
                                p, added < pool_size ? "default" : "rest");
                        }
                        ++added;
                    }
                }
            }
        };
        return init_args;
    }
}    // namespace

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(
        hpx::local::init(test_two_workers, argc, argv, configuration(2)), 0);
    HPX_TEST_EQ(
        hpx::local::init(test_one_worker, argc, argv, configuration(1)), 0);
    return hpx::util::report_errors();
}
#else
int main()
{
    return hpx::util::report_errors();
}
#endif
