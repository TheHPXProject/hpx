//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <string>
#include <vector>

namespace tuning = hpx::tuning;

void test_section_name()
{
    tuning::tuning_row row;
    row.scheduler = "local-priority-fifo";
    row.pool_size = 20;
    HPX_TEST_EQ(tuning::section_name(row),
        std::string("hpx.idle_tuning.local-priority-fifo.none.20"));

    row.scheduler = "local-priority-tuned-fifo";
    row.tuner = "adaptive-spin";
    row.pool_size = 40;
    HPX_TEST_EQ(tuning::section_name(row),
        std::string(
            "hpx.idle_tuning.local-priority-tuned-fifo.adaptive-spin.40"));
}

void test_to_ini()
{
    tuning::tuning_row row;
    row.scheduler = "local-priority-fifo";
    row.pool_size = 20;
    row.settings.push_back(tuning::tuned_setting{
        "hpx.min_idle_backoff_time", "55", "shortest sleep 55.2 us", false});
    row.settings.push_back(tuning::tuned_setting{
        "hpx.max_idle_backoff_time", "1000", "shortest sleep 55.2 us", true});

    std::string const ini = tuning::to_ini({row}, {"written by a test"}, 2026);

    auto const has = [&](char const* text) {
        bool const found = ini.find(text) != std::string::npos;
        HPX_TEST_MSG(found, text);
    };
    has("# Copyright (c) 2026 TheHPXProject\n");
    has("# SPDX-License-Identifier: BSL-1.0\n");
    has("# Distributed under the Boost Software License, Version 1.0.");
    has("# written by a test\n");
    has("\n[hpx.idle_tuning.local-priority-fifo.none.20]\n");
    has("# shortest sleep 55.2 us\nmin_idle_backoff_time = 55\n");
    has("# shortest sleep 55.2 us (HPX's default)\n"
        "max_idle_backoff_time = 1000\n");

    // the license header comes first
    HPX_TEST_EQ(ini.find("# Copyright"), std::string::size_type(0));
    // keys are written without "hpx."
    HPX_TEST_EQ(ini.find("hpx.min_idle_backoff_time"), std::string::npos);
}

int main()
{
    test_section_name();
    test_to_ini();
    return hpx::util::report_errors();
}
