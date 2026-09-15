//  Copyright (c) 2026 The HPX Project
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Demonstrating #4627: Allow HPX counters to be requested before they are
// registered.
//
// A counter type may be registered only after query_counters::start() has
// already run its initial discovery, for instance because the counter's
// provider (such as APEX) does not register a counter with HPX until the
// counter is sampled for the first time. Since the requested counter is
// specified as a wild-card pattern (e.g. /apex/*), it should still be
// picked up by the time counters are printed at shutdown, even though it
// did not exist when the runtime started.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/performance_counters/query_counters.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace {

    std::int64_t early_counter_value(bool)
    {
        return 42;
    }

    std::int64_t late_counter_value(bool)
    {
        return 43;
    }

    void register_early_counter_type()
    {
        hpx::performance_counters::install_counter_type(
            "/late_registration_4627/early", &early_counter_value,
            "counter registered before query_counters::start()", "",
            hpx::performance_counters::counter_type::raw);
    }
}    // namespace

int hpx_main()
{
    std::vector<std::string> const names{"/late_registration_4627/*"};
    std::vector<std::string> const reset_names;

    // Use a long interval so the periodic timer does not interfere with the
    // two explicit, forced evaluations performed below.
    hpx::util::query_counters qc(names, reset_names, /* interval = */ 3600000,
        "none", "normal", std::vector<std::string>{}, /* csv_header = */ false,
        /* print_counters_locally = */ false, /* counter_types = */ false);

    qc.start();

    // At this point only the counter type registered before start() was
    // called can be found.
    HPX_TEST_EQ(qc.size(), std::size_t(1));

    hpx::error_code ec(hpx::throwmode::lightweight);
    bool const found_before = qc.evaluate_counters(false, nullptr, true, ec);
    HPX_TEST(!ec);
    HPX_TEST(found_before);

    // Simulate a counter, such as an APEX counter, that only becomes known
    // to HPX well after query_counters::start() has already run, e.g.
    // because it is registered the first time it is sampled.
    hpx::performance_counters::install_counter_type(
        "/late_registration_4627/late", &late_counter_value,
        "counter registered after query_counters::start()", "",
        hpx::performance_counters::counter_type::raw);

    // Before the fix for #4627 this second, forced evaluation (mirroring
    // what happens when counters are printed at shutdown) would still miss
    // the counter that was registered after start() was called, and
    // qc.size() would remain 1. With the fix, query_counters re-discovers
    // matching counters before a forced evaluation, so the late counter is
    // picked up as well.
    hpx::error_code ec2(hpx::throwmode::lightweight);
    bool const found_after = qc.evaluate_counters(false, nullptr, true, ec2);
    HPX_TEST(!ec2);
    HPX_TEST(found_after);
    HPX_TEST_EQ(qc.size(), std::size_t(2));

    qc.stop_evaluating_counters(true);

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    hpx::register_startup_function(&register_early_counter_type);
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
