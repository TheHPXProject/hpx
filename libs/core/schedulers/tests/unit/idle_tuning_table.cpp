//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle settings of a pool from the idle tuning table: a pool takes the
// entries of hpx.idle_tuning.<scheduler>.<tuner>.<number of workers>, else
// those of hpx.idle_tuning.<scheduler>.<tuner>, else the plain
// hpx.min_idle_backoff_time and hpx.max_idle_backoff_time, setting by
// setting; an invalid entry counts as missing. Two pools of the tuned
// scheduler (whose settings can be read back), of two and of one worker, see
// different rows of the same table.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <string>

namespace policies = hpx::threads::policies;

using tuned_scheduler = policies::local_priority_tuned_scheduler<>;

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
namespace {

    // a setting of the first worker of the named pool
    float pool_setting(std::string const& pool, char const* key)
    {
        auto* scheduler = dynamic_cast<tuned_scheduler*>(
            hpx::resource::get_thread_pool(pool).get_scheduler());
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
}    // namespace

int test_table()
{
    // two workers: the longest step from the row of its size; the spin there
    // is invalid, so the one of the row for any size
    HPX_TEST_EQ(pool_setting("default", "hpx.max_idle_backoff_time"), 700.0f);
    HPX_TEST_EQ(pool_setting("default", "hpx.min_idle_backoff_time"), 20.0f);

    // one worker: no row of its size, the row for any size; that row has no
    // longest step, so the plain value
    HPX_TEST_EQ(pool_setting("single", "hpx.min_idle_backoff_time"), 20.0f);
    HPX_TEST_EQ(pool_setting("single", "hpx.max_idle_backoff_time"), 900.0f);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;
    init_args.cfg = {
        "hpx.os_threads=3",
        "hpx.scheduler_tuner=static",
        "hpx.min_idle_backoff_time=50",
        "hpx.max_idle_backoff_time=900",
        // for pools of any size
        "hpx.idle_tuning.local-priority-tuned-fifo.static."
        "min_idle_backoff_time!=20",
        // for pools of two workers, the spin is ignored
        "hpx.idle_tuning.local-priority-tuned-fifo.static.2."
        "max_idle_backoff_time!=700",
        "hpx.idle_tuning.local-priority-tuned-fifo.static.2."
        "min_idle_backoff_time!=-5",
        // another scheduler's and another tuner's rows don't apply
        "hpx.idle_tuning.local-priority-fifo.none.min_idle_backoff_time!=7",
        "hpx.idle_tuning.local-priority-tuned-fifo.none."
        "min_idle_backoff_time!=8",
    };
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo);
        rp.create_thread_pool("single",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo);

        // the first processing unit for "single", the next two for "default"
        std::size_t added = 0;
        for (hpx::resource::numa_domain const& d : rp.numa_domains())
        {
            for (hpx::resource::core const& c : d.cores())
            {
                for (hpx::resource::pu const& p : c.pus())
                {
                    if (added < 3)
                        rp.add_resource(p, added == 0 ? "single" : "default");
                    ++added;
                }
            }
        }
    };

    HPX_TEST_EQ(hpx::local::init(test_table, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
#else
int main()
{
    return hpx::util::report_errors();
}
#endif
