//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/runtime.hpp>

#include <cstddef>
#include <string>

namespace {

    std::size_t expected_numa_sensitivity = 0;

    void set_expected_numa_sensitivity(int argc, char* argv[])
    {
        std::string const option = "--hpx:numa-sensitive";

        for (int i = 1; i != argc; ++i)
        {
            std::string const argument = argv[i];
            if (argument == option)
            {
                expected_numa_sensitivity = 1;
            }
            else if (argument.starts_with(option + "="))
            {
                expected_numa_sensitivity = hpx::util::from_string<std::size_t>(
                    argument.substr(option.size() + 1));
            }
        }
    }
}    // namespace

int hpx_main()
{
    std::size_t const numa_sensitivity = hpx::util::from_string<std::size_t>(
        hpx::get_config_entry("hpx.numa_sensitive", "0"));

    HPX_TEST_EQ(numa_sensitivity, expected_numa_sensitivity);
    HPX_TEST_EQ(
        hpx::is_scheduler_numa_sensitive(), expected_numa_sensitivity != 0);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    bool caught_exception = false;
    try
    {
        (void) hpx::is_scheduler_numa_sensitive();
    }
    catch (hpx::exception const& e)
    {
        caught_exception = e.get_error() == hpx::error::invalid_status;
    }
    HPX_TEST(caught_exception);

    set_expected_numa_sensitivity(argc, argv);
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);

    return hpx::util::report_errors();
}
