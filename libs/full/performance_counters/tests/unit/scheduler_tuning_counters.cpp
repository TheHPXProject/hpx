//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The counters a scheduler with a tuner keeps (scheduler_tuning_counters) are
// available as performance counters /scheduler/tuning/<name>, for the
// locality, a pool and a worker thread, but only for the pools whose scheduler
// counts: here the default pool, not the pool "plain". The settings of such a
// scheduler (scheduler_tuning_parameters) are available as performance
// counters /scheduler/tuning/setting/<name>, for a pool and a worker thread.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace policies = hpx::threads::policies;

constexpr std::size_t num_tasks = 100;

std::string counter_name(std::string const& instance, std::string const& name)
{
    return "/scheduler{locality#0/" + instance + "}/tuning/" + name;
}

std::int64_t query(std::string const& instance, std::string const& name)
{
    hpx::performance_counters::performance_counter const counter(
        counter_name(instance, name));
    return counter.get_value<std::int64_t>(hpx::launch::sync);
}

// what the scheduler of this pool counted, all of its workers and the threads
// outside of it
std::int64_t total(policies::scheduler_tuning_counters const& counters,
    policies::scheduler_counter const c)
{
    std::int64_t value =
        counters.get(policies::scheduler_tuning_counters::external, c);
    for (std::size_t i = 0; i != counters.num_workers(); ++i)
    {
        value += counters.get(i, c);
    }
    return value;
}

void test_values(policies::scheduler_tuning_counters const& counters)
{
    // small threads are created only here, so their counts don't change
    // while the counters are queried
    hpx::latch done(static_cast<std::ptrdiff_t>(num_tasks + 1));
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        hpx::post(hpx::threads::small_task([&done] { done.count_down(1); }));
    }
    done.arrive_and_wait();

    auto const c = policies::scheduler_counter::created_small;
    std::int64_t const expected = total(counters, c);
    HPX_TEST_LTE(static_cast<std::int64_t>(num_tasks), expected);

    // the locality (only the default pool counts) and the default pool
    HPX_TEST_EQ(query("total", "created-small"), expected);
    HPX_TEST_EQ(query("pool#default", "created-small"), expected);

    // the workers, of the pool and of the default pool
    std::int64_t workers = 0;
    for (std::size_t i = 0; i != counters.num_workers(); ++i)
    {
        std::string const worker = "worker-thread#" + std::to_string(i);
        std::int64_t const value =
            query("pool#default/" + worker, "created-small");
        HPX_TEST_EQ(value, static_cast<std::int64_t>(counters.get(i, c)));
        HPX_TEST_EQ(query(worker, "created-small"), value);
        workers += value;
    }
    HPX_TEST_EQ(workers +
            counters.get(policies::scheduler_tuning_counters::external, c),
        expected);

    // every counter is available under its name
    for (std::size_t i = 0; i != policies::scheduler_tuning_counters::size; ++i)
    {
        std::string name = policies::get_scheduler_counter_name(
            static_cast<policies::scheduler_counter>(i));
        std::ranges::replace(name, '_', '-');
        HPX_TEST_LTE(0, query("total", name));
    }
}

void test_pool_without_tuner()
{
    std::size_t const plain = hpx::resource::get_pool_index("plain");
    HPX_TEST_EQ(plain, std::size_t(1));

    // no counters for the pool whose scheduler doesn't count
    for (std::string const instance :
        {"pool#plain", "pool#plain/worker-thread#0"})
    {
        bool caught = false;
        try
        {
            query(instance, "created-small");
        }
        catch (hpx::exception const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
    }

    // the counters are discovered for the default pool only
    std::vector<hpx::performance_counters::counter_info> infos;
    hpx::performance_counters::discover_counter_type(
        "/scheduler/tuning/created-small", infos,
        hpx::performance_counters::discover_counters_mode::full);

    // (the worker threads are expanded when the counters are created)
    std::vector<std::string> expected = {counter_name("total", "created-small"),
        counter_name("pool#default", "created-small"),
        counter_name("pool#default/worker-thread#*", "created-small")};

    // the worker threads of the default pool
    hpx::performance_counters::performance_counter_set const workers(
        counter_name("pool#default/worker-thread#*", "created-small"));
    HPX_TEST_EQ(workers.size(), hpx::resource::get_num_threads("default"));

    std::vector<std::string> discovered;
    for (auto const& info : infos)
    {
        discovered.push_back(info.fullname_);
    }
    std::ranges::sort(expected);
    std::ranges::sort(discovered);
    HPX_TEST(discovered == expected);

    // a pool wildcard stands for the pools whose scheduler counts only
    for (std::string const instance : {"pool#*", "pool#*/worker-thread#*"})
    {
        std::vector<hpx::performance_counters::counter_info> pools;
        hpx::performance_counters::discover_counter_type(
            counter_name(instance, "created-small"), pools,
            hpx::performance_counters::discover_counters_mode::full);

        std::string const pool_instance = instance == "pool#*" ?
            "pool#default" :
            "pool#default/worker-thread#*";
        HPX_TEST_EQ(pools.size(), std::size_t(1));
        if (!pools.empty())
        {
            HPX_TEST_EQ(pools[0].fullname_,
                counter_name(pool_instance, "created-small"));
        }
        for (auto const& info : pools)
        {
            HPX_TEST(info.fullname_.find("pool#plain") == std::string::npos);
        }
    }
    if (discovered != expected)
    {
        for (auto const& name : discovered)
        {
            std::cerr << "discovered: " << name << "\n";
        }
        for (auto const& name : expected)
        {
            std::cerr << "expected: " << name << "\n";
        }
    }
}

// the name of the counter of a setting: its configuration entry without
// "hpx.", with '-' in place of '_' and '.'
std::string setting_counter_name(std::string entry)
{
    entry.erase(0, 4);
    std::ranges::replace(entry, '_', '-');
    std::ranges::replace(entry, '.', '-');
    return "setting/" + entry;
}

double setting_value(policies::scheduler_tuning_parameters const& parameters,
    std::size_t const worker, std::size_t const id)
{
    if (parameters.info(id).type == policies::tuning_parameter_type::real)
    {
        return static_cast<double>(parameters.get<float>(worker, id));
    }
    return parameters.get<std::uint32_t>(worker, id);
}

bool query_throws(std::string const& instance, std::string const& name)
{
    try
    {
        query(instance, name);
    }
    catch (hpx::exception const&)
    {
        return true;
    }
    return false;
}

void test_settings(policies::scheduler_tuning_parameters const& parameters)
{
    // the configured value (see main)
    std::size_t const spin = parameters.find("hpx.min_idle_backoff_time");
    HPX_TEST_NEQ(spin, policies::scheduler_tuning_parameters::npos);
    HPX_TEST_EQ(query("pool#default", "setting/min-idle-backoff-time"), 37);
    HPX_TEST_EQ(query("worker-thread#0", "setting/min-idle-backoff-time"), 37);

    for (std::size_t id = 0; id != parameters.size(); ++id)
    {
        auto const& info = parameters.info(id);
        HPX_TEST(std::string_view(info.name).starts_with("hpx."));
        std::string const name = setting_counter_name(info.name);

        // the workers, of the pool and of the default pool
        double sum = 0.0;
        for (std::size_t i = 0; i != parameters.num_workers(); ++i)
        {
            std::string const worker = "worker-thread#" + std::to_string(i);
            double const value = setting_value(parameters, i, id);
            HPX_TEST_EQ(query("pool#default/" + worker, name),
                static_cast<std::int64_t>(std::llround(value)));
            HPX_TEST_EQ(query(worker, name),
                static_cast<std::int64_t>(std::llround(value)));
            sum += value;
        }

        // the pool: the mean over the workers, or the value of the pool
        double const expected = info.per_worker ?
            sum / static_cast<double>(parameters.num_workers()) :
            setting_value(parameters, 0, id);
        HPX_TEST_EQ(query("pool#default", name),
            static_cast<std::int64_t>(std::llround(expected)));

        // no total, and nothing for the pool whose scheduler has no settings
        HPX_TEST(query_throws("total", name));
        HPX_TEST(query_throws("pool#plain", name));
        HPX_TEST(query_throws("pool#plain/worker-thread#0", name));
    }

    // the settings are discovered for the default pool only, without a total
    std::vector<hpx::performance_counters::counter_info> infos;
    hpx::performance_counters::discover_counter_type(
        "/scheduler/tuning/setting/min-idle-backoff-time", infos,
        hpx::performance_counters::discover_counters_mode::full);

    std::vector<std::string> expected = {
        counter_name("pool#default", "setting/min-idle-backoff-time"),
        counter_name(
            "pool#default/worker-thread#*", "setting/min-idle-backoff-time")};
    std::vector<std::string> discovered;
    for (auto const& info : infos)
    {
        discovered.push_back(info.fullname_);
    }
    std::ranges::sort(expected);
    std::ranges::sort(discovered);
    HPX_TEST(discovered == expected);
}

int hpx_main()
{
    auto const* counters = hpx::threads::get_self_id_data()
                               ->get_scheduler_base()
                               ->get_tuning_counters();
    HPX_TEST(counters != nullptr);
    if (counters != nullptr)
    {
        test_values(*counters);
        test_pool_without_tuner();
    }

    auto const* parameters = hpx::threads::get_self_id_data()
                                 ->get_scheduler_base()
                                 ->find_tuning_parameters();
    HPX_TEST(parameters != nullptr);
    if (parameters != nullptr)
    {
        test_settings(*parameters);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params init_args;
    init_args.cfg = {
        "hpx.force_min_os_threads!=2", "hpx.min_idle_backoff_time!=37"};
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo);

        // a pool whose scheduler doesn't count, on the first processing unit
        rp.create_thread_pool(
            "plain", hpx::resource::scheduling_policy::local_priority_fifo);
        rp.add_resource(rp.numa_domains()[0].cores()[0].pus()[0], "plain");
    };

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
#endif
