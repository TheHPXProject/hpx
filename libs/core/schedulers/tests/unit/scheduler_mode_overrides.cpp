//  Copyright (c) 2026 Sai Charan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// add/remove/add_remove_scheduler_mode must honor the scheduler specific
// mode adjustments, just like set_scheduler_mode does (see #7745).

#include <hpx/init.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <string>
#include <vector>

using hpx::threads::policies::scheduler_mode;

constexpr auto stealing =
    scheduler_mode::enable_stealing | scheduler_mode::enable_stealing_numa;

bool stealing_forced_on = false;

void check_stealing(hpx::threads::policies::scheduler_base const* scheduler)
{
    for (std::size_t i = 0; i != hpx::get_os_thread_count(); ++i)
    {
        HPX_TEST_EQ(
            scheduler->has_scheduler_mode(scheduler_mode::enable_stealing, i),
            stealing_forced_on);
        HPX_TEST_EQ(scheduler->has_scheduler_mode(
                        scheduler_mode::enable_stealing_numa, i),
            stealing_forced_on);
    }
}

int hpx_main()
{
    auto const* scheduler =
        hpx::threads::get_self_id_data()->get_scheduler_base();

    check_stealing(scheduler);

    hpx::threads::add_scheduler_mode(stealing);
    check_stealing(scheduler);

    hpx::threads::remove_scheduler_mode(stealing);
    check_stealing(scheduler);

    hpx::threads::add_remove_scheduler_mode(
        stealing, scheduler_mode::nothing_special);
    check_stealing(scheduler);

    hpx::threads::add_remove_scheduler_mode(
        scheduler_mode::nothing_special, stealing);
    check_stealing(scheduler);

    return hpx::local::finalize();
}

void test_scheduler(char* argv0, std::string const& name, bool forced_on)
{
    stealing_forced_on = forced_on;

    std::string const queuing = "--hpx:queuing=" + name;
    std::vector<char*> av = {argv0, const_cast<char*>(queuing.c_str())};

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=2"};

    HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(av.size()),
                    av.data(), init_args),
        0);
}

int main(int, char* argv[])
{
    test_scheduler(argv[0], "static", false);
    test_scheduler(argv[0], "static-priority", false);
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
    test_scheduler(argv[0], "local-workrequesting-fifo", true);
#endif

    return hpx::util::report_errors();
}
