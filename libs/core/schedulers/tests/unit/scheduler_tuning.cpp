//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The tunable settings of the rules of the local-priority-tuned-fifo
// scheduler are registered with the values they were configured with, a tuner
// selected with hpx.scheduler_tuner sees them before the workers start, and
// the values are clamped to their ranges.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

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
            parameters.set(0, id, 3.0f);
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

// the tested settings belong to the local-priority-tuned-fifo scheduler, make
// sure it is used regardless of --hpx:queuing
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
        HPX_TEST_EQ(parameters.get<float>(i, spin_time), 37.0f);
    }

    // sizes are given by their names (see get_task_size_name)
    std::size_t const max_size =
        parameters.find("hpx.scheduler_policy.wake_one_max_size");
    HPX_TEST_NEQ(max_size, params::npos);
    HPX_TEST_EQ(parameters.get<std::uint32_t>(max_size),
        static_cast<std::uint32_t>(hpx::threads::task_size::medium));

    std::size_t const spins =
        parameters.find("hpx.scheduler_policy.wake_if_nobody_spins");
    HPX_TEST_NEQ(spins, params::npos);
    HPX_TEST(!parameters.info(spins).per_worker);
    HPX_TEST_EQ(parameters.get<std::uint32_t>(spins), 0u);

    std::size_t const pauses =
        parameters.find("hpx.idle_backoff_spin_round_pauses");
    HPX_TEST_NEQ(pauses, params::npos);
    HPX_TEST_EQ(parameters.get<std::uint32_t>(0, pauses), 64u);

    // the default tuner keeps the configured values
    std::size_t const interval =
        parameters.find("hpx.idle_backoff_steal_check_interval");
    HPX_TEST_NEQ(interval, params::npos);
    HPX_TEST_EQ(parameters.get<float>(0, interval),
        static_cast<float>(HPX_IDLE_BACKOFF_STEAL_CHECK_INTERVAL));

    // values are clamped to the range of the setting
    parameters.set_all(spins, 5u);
    HPX_TEST_EQ(parameters.get<std::uint32_t>(spins), 1u);
    parameters.set_all(spins, 0u);

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
    HPX_TEST_EQ(parameters.get<float>(0, interval), 3.0f);
    if (parameters.num_workers() > 1)
    {
        HPX_TEST_EQ(parameters.get<float>(1, interval),
            static_cast<float>(HPX_IDLE_BACKOFF_STEAL_CHECK_INTERVAL));
    }
#endif
    return hpx::local::finalize();
}

// a setting keeps its value as its type, clamped to its range, per worker
void test_storage()
{
    using policies::tuning_parameter_type;

    policies::scheduler_tuning_parameters parameters(2);
    std::size_t const count = parameters.add("count", 0u, 100u, true, 7u);
    std::size_t const time = parameters.add("time", 0.5f, 100.0f, true, 1.5f);

    // the range gives the type, and is of that type
    HPX_TEST(parameters.info(count).type == tuning_parameter_type::integer);
    HPX_TEST(parameters.info(time).type == tuning_parameter_type::real);
    HPX_TEST_EQ(parameters.info(count).max_value.as<std::uint32_t>(), 100u);
    HPX_TEST_EQ(parameters.info(time).min_value.as<float>(), 0.5f);

    HPX_TEST_EQ(parameters.get<std::uint32_t>(0, count), 7u);
    HPX_TEST_EQ(parameters.get<float>(0, time), 1.5f);

    parameters.set(1, count, 1000u);
    HPX_TEST_EQ(parameters.get<std::uint32_t>(1, count), 100u);
    parameters.set(1, time, 0.25f);
    HPX_TEST_EQ(parameters.get<float>(1, time), 0.5f);
    HPX_TEST_EQ(parameters.get<float>(0, time), 1.5f);
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

void test_float_clamping()
{
    policies::scheduler_tuning_parameters parameters(2);
    std::size_t const time = parameters.add("time", 0.5f, 100.0f, true, 1.5f);

    // Below min_value
    parameters.set(0, time, -10.0f);
    HPX_TEST_EQ(parameters.get<float>(0, time), 0.5f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));

    // Above max_value
    parameters.set(0, time, 200.0f);
    HPX_TEST_EQ(parameters.get<float>(0, time), 100.0f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));

    // Negative infinity
    parameters.set(0, time, -std::numeric_limits<float>::infinity());
    HPX_TEST_EQ(parameters.get<float>(0, time), 0.5f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));

    // Positive infinity
    parameters.set(0, time, std::numeric_limits<float>::infinity());
    HPX_TEST_EQ(parameters.get<float>(0, time), 100.0f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));

    // Quiet NaN
    parameters.set(0, time, std::numeric_limits<float>::quiet_NaN());
    HPX_TEST_EQ(parameters.get<float>(0, time), 0.5f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));

    // Signaling NaN
    if (std::numeric_limits<float>::has_signaling_NaN)
    {
        parameters.set(0, time, std::numeric_limits<float>::signaling_NaN());
        HPX_TEST_EQ(parameters.get<float>(0, time), 0.5f);
        HPX_TEST(std::isfinite(parameters.get<float>(0, time)));
    }

    // set_all with NaN
    parameters.set_all(time, std::numeric_limits<float>::quiet_NaN());
    HPX_TEST_EQ(parameters.get<float>(0, time), 0.5f);
    HPX_TEST_EQ(parameters.get<float>(1, time), 0.5f);
    HPX_TEST(std::isfinite(parameters.get<float>(0, time)));
    HPX_TEST(std::isfinite(parameters.get<float>(1, time)));

    // add() initialized with NaN
    std::size_t const nan_init = parameters.add("nan_init", 1.0f, 10.0f, false,
        std::numeric_limits<float>::quiet_NaN());
    HPX_TEST_EQ(parameters.get<float>(nan_init), 1.0f);
    HPX_TEST(std::isfinite(parameters.get<float>(nan_init)));
}

void test_add_setting_errors()
{
    policies::scheduler_tuning_parameters parameters(1);
    parameters.add("param", 0u, 10u, false, 0u);

    // Registering duplicate name throws bad_parameter
    bool caught_dup = false;
    try
    {
        parameters.add("param", 0u, 10u, false, 0u);
    }
    catch (hpx::exception const& e)
    {
        caught_dup = (e.get_error() == hpx::error::bad_parameter);
    }
    HPX_TEST(caught_dup);

    // Registering more than max_parameters (16) settings throws bad_parameter
    policies::scheduler_tuning_parameters p(1);
    static constexpr char const* names[] = {"s0", "s1", "s2", "s3", "s4", "s5",
        "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15"};
    for (std::size_t i = 0;
        i != policies::scheduler_tuning_parameters::max_parameters; ++i)
    {
        p.add(names[i], 0u, 100u, false, 0u);
    }
    HPX_TEST_EQ(
        p.size(), policies::scheduler_tuning_parameters::max_parameters);

    bool caught_overflow = false;
    try
    {
        p.add("setting_overflow", 0u, 100u, false, 0u);
    }
    catch (hpx::exception const& e)
    {
        caught_overflow = (e.get_error() == hpx::error::bad_parameter);
    }
    HPX_TEST(caught_overflow);
}

void test_concurrent_set_all_get()
{
    policies::scheduler_tuning_parameters parameters(4);
    std::size_t const p_int = parameters.add("conc_int", 0u, 1000u, false, 0u);
    std::size_t const p_real =
        parameters.add("conc_real", 0.0f, 100.0f, false, 0.0f);

    std::atomic<bool> stop{false};
    std::vector<std::thread> readers;
    readers.reserve(4);

    for (std::size_t w = 0; w != 4; ++w)
    {
        readers.emplace_back([&parameters, w, p_int, p_real, &stop] {
            while (!stop.load(std::memory_order_relaxed))
            {
                auto const vi = parameters.get<std::uint32_t>(w, p_int);
                auto const vr = parameters.get<float>(w, p_real);
                HPX_TEST_LTE(vi, 1000u);
                HPX_TEST_LTE(0.0f, vr);
                HPX_TEST_LTE(vr, 100.0f);
                HPX_TEST(std::isfinite(vr));

                auto const vi_pool = parameters.get<std::uint32_t>(p_int);
                auto const vr_pool = parameters.get<float>(p_real);
                HPX_TEST_LTE(vi_pool, 1000u);
                HPX_TEST_LTE(0.0f, vr_pool);
                HPX_TEST_LTE(vr_pool, 100.0f);
                HPX_TEST(std::isfinite(vr_pool));
            }
        });
    }

    std::thread writer([&parameters, p_int, p_real, &stop] {
        for (std::size_t i = 0; i != 1000; ++i)
        {
            parameters.set_all(p_int, static_cast<std::uint32_t>(i % 1001));
            parameters.set_all(p_real, static_cast<float>(i % 101));
        }
        stop.store(true, std::memory_order_relaxed);
    });

    writer.join();
    for (auto& r : readers)
    {
        r.join();
    }
}

int main(int argc, char* argv[])
{
    test_storage();
    test_registry();
    test_float_clamping();
    test_add_setting_errors();
    test_concurrent_set_all_get();

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
