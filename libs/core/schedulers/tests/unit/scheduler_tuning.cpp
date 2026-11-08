//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The tunable settings of the rules of the local-priority-tuned-fifo scheduler are
// registered with the values they were configured with, a tuner selected with
// hpx.scheduler_tuner sees them before the workers start, and the values are
// clamped to their ranges.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

namespace policies = hpx::threads::policies;

using tuned_scheduler = policies::local_priority_tuned_scheduler<>;

std::atomic<int> attached{0};

struct test_tuner final : policies::scheduler_tuner
{
    void attach(policies::scheduler_tuning_parameters& parameters) override
    {
        ++attached;
        std::size_t const id =
            parameters.find("hpx.idle_backoff_steal_check_interval");
        if (id != policies::scheduler_tuning_parameters::npos)
        {
            parameters.set(0, id, 3.0);
        }
    }
};

policies::scheduler_tuning_parameters& get_parameters()
{
    auto* scheduler = dynamic_cast<tuned_scheduler*>(
        hpx::threads::get_self_id_data()->get_scheduler_base());
    HPX_TEST(scheduler != nullptr);
    return scheduler->get_tuning_parameters();
}

// the tested settings belong to the local-priority-tuned-fifo scheduler, make sure
// it is used regardless of --hpx:queuing
void use_tuned_scheduler(hpx::local::init_params& init_args)
{
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo);
    };
}

int test_configured_values()
{
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
    auto& parameters = get_parameters();
    using params = policies::scheduler_tuning_parameters;

    std::size_t const spin_time = parameters.find("hpx.min_idle_backoff_time");
    HPX_TEST_NEQ(spin_time, params::npos);
    HPX_TEST(parameters.info(spin_time).per_worker);
    for (std::size_t i = 0; i != parameters.num_workers(); ++i)
    {
        HPX_TEST_EQ(parameters.get(i, spin_time), 37.0);
    }

    // sizes are given by their names (see get_task_size_name)
    std::size_t const max_size =
        parameters.find("hpx.scheduler_policy.wake_one_max_size");
    HPX_TEST_NEQ(max_size, params::npos);
    HPX_TEST_EQ(parameters.get(max_size),
        static_cast<double>(hpx::threads::task_size::medium));

    std::size_t const spins =
        parameters.find("hpx.scheduler_policy.wake_if_nobody_spins");
    HPX_TEST_NEQ(spins, params::npos);
    HPX_TEST(!parameters.info(spins).per_worker);
    HPX_TEST_EQ(parameters.get(spins), 0.0);

    std::size_t const pauses =
        parameters.find("hpx.idle_backoff_spin_round_pauses");
    HPX_TEST_NEQ(pauses, params::npos);
    HPX_TEST_EQ(parameters.get(0, pauses), 64.0);

    // the default tuner keeps the configured values
    std::size_t const interval =
        parameters.find("hpx.idle_backoff_steal_check_interval");
    HPX_TEST_NEQ(interval, params::npos);
    HPX_TEST_EQ(parameters.get(0, interval),
        double(HPX_IDLE_BACKOFF_STEAL_CHECK_INTERVAL));

    // values are clamped to the range of the setting
    parameters.set_all(spins, 5.0);
    HPX_TEST_EQ(parameters.get(spins), 1.0);
    parameters.set_all(spins, 0.0);

    HPX_TEST_EQ(parameters.find("hpx.no_such_setting"), params::npos);
#endif
    return hpx::local::finalize();
}

int test_custom_tuner()
{
    HPX_TEST_EQ(attached.load(), 1);
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
    auto& parameters = get_parameters();
    std::size_t const interval =
        parameters.find("hpx.idle_backoff_steal_check_interval");
    HPX_TEST_EQ(parameters.get(0, interval), 3.0);
    if (parameters.num_workers() > 1)
    {
        HPX_TEST_EQ(parameters.get(1, interval),
            double(HPX_IDLE_BACKOFF_STEAL_CHECK_INTERVAL));
    }
#endif
    return hpx::local::finalize();
}

void test_registry()
{
    HPX_TEST(policies::create_scheduler_tuner("static") != nullptr);

    bool caught = false;
    try
    {
        [[maybe_unused]] auto tuner =
            policies::create_scheduler_tuner("no_such_tuner");
    }
    catch (hpx::exception const& e)
    {
        caught = e.get_error() == hpx::error::bad_parameter;
    }
    HPX_TEST(caught);
}

int main(int argc, char* argv[])
{
    test_registry();

    {
        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.os_threads=2", "hpx.min_idle_backoff_time=37",
            "hpx.scheduler_policy.wake_one_max_size=medium",
            "hpx.scheduler_policy.wake_if_nobody_spins=0",
            "hpx.idle_backoff_spin_round_pauses=64"};
        use_tuned_scheduler(init_args);
        HPX_TEST_EQ(
            hpx::local::init(test_configured_values, argc, argv, init_args), 0);
    }

    {
        policies::register_scheduler_tuner(
            "test", [] { return std::make_unique<test_tuner>(); });

        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.os_threads=2", "hpx.scheduler_tuner=test"};
        use_tuned_scheduler(init_args);
        HPX_TEST_EQ(
            hpx::local::init(test_custom_tuner, argc, argv, init_args), 0);
    }

    return hpx::util::report_errors();
}
