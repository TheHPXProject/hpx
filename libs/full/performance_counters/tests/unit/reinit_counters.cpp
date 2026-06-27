//  Copyright (c) 2018 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::atomic<unsigned int> value_count_;

class test_counter
  : public hpx::performance_counters::base_performance_counter<test_counter>
{
public:
    test_counter() = default;

    test_counter(hpx::performance_counters::counter_info const& info)
      : hpx::performance_counters::base_performance_counter<test_counter>(info)
      , base_counter_(0)
    {
        value_count_ = std::rand() % 100;
    }

    hpx::performance_counters::counter_values_array get_counter_values_array(
        bool reset) override
    {
        hpx::performance_counters::counter_values_array value;

        value.time_ = hpx::chrono::high_resolution_clock::now();
        value.status_ = hpx::performance_counters::counter_status::new_data;
        value.count_ = ++invocation_count_;

        std::vector<std::int64_t> result(value_count_.load());
        std::iota(result.begin(), result.end(), base_counter_.load());

        ++base_counter_;
        if (reset)
            base_counter_.store(0);

        value.values_ = std::move(result);

        return value;
    }

    void reinit(bool) override
    {
        value_count_ = std::rand() % 100;
    }

private:
    std::atomic<std::int64_t> base_counter_;
};

typedef hpx::components::component<test_counter> test_counter_type;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(
    test_counter_type, test_counter, "base_performance_counter")

///////////////////////////////////////////////////////////////////////////////
hpx::naming::gid_type test_counter_creator(
    hpx::performance_counters::counter_info const& info, hpx::error_code& ec)
{
    hpx::performance_counters::counter_path_elements paths;
    get_counter_path_elements(info.fullname_, paths, ec);
    if (ec)
        return hpx::naming::invalid_gid;

    if (paths.parentinstance_is_basename_)
    {
        HPX_THROWS_IF(ec, hpx::error::bad_parameter, "test_counter_creator",
            "invalid counter instance parent name: " +
                paths.parentinstancename_);
        return hpx::naming::invalid_gid;
    }

    // create individual counter
    if (paths.instancename_ == "total" && paths.instanceindex_ == -1)
    {
        // make sure parent instance name is set properly
        hpx::performance_counters::counter_info complemented_info = info;
        complement_counter_info(complemented_info, info, ec);
        if (ec)
            return hpx::naming::invalid_gid;

        // create the counter as requested
        hpx::naming::gid_type id;
        try
        {
            id = hpx::components::server::construct<test_counter_type>(
                complemented_info);
        }
        catch (hpx::exception const& e)
        {
            if (&ec == &hpx::throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
            return hpx::naming::invalid_gid;
        }

        if (&ec != &hpx::throws)
            ec = hpx::make_success_code();
        return id;
    }

    HPX_THROWS_IF(ec, hpx::error::bad_parameter, "test_counter_creator",
        "invalid counter instance name: " + paths.instancename_);
    return hpx::naming::invalid_gid;
}

///////////////////////////////////////////////////////////////////////////////
void register_counter_type()
{
    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type("/test/reinit-values",
        hpx::performance_counters::counter_type::raw_values,
        "returns an array of linearly increasing counter values, supports "
        "reinit",
        &test_counter_creator,
        &hpx::performance_counters::locality_counter_discoverer,
        hpx::performance_counters::HPX_PERFORMANCE_COUNTER_V1);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    for (int i = 0; i != 10; ++i)
    {
        hpx::performance_counters::performance_counter c("/test/reinit-values");

        c.reinit(hpx::launch::sync);

        auto values = c.get_counter_values_array(hpx::launch::sync, false);

        HPX_TEST_EQ(values.count_, static_cast<std::uint64_t>(i + 1));

        std::vector<std::int64_t> expected(value_count_.load());
        std::iota(expected.begin(), expected.end(), i);
        HPX_TEST(values.values_ == expected);

        std::string name = c.get_name(hpx::launch::sync);
        HPX_TEST_EQ(name, std::string("/test{locality#0/total}/reinit-values"));
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    hpx::register_startup_function(&register_counter_type);

    // Initialize and run HPX.
    std::vector<std::string> const cfg = {"hpx.os_threads=1"};
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);

    return hpx::util::report_errors();
}

#endif
