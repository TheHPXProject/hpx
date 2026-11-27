//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"
#include "idle_area.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace hpx_tune {

    namespace {

        // a configuration given as <scheduler>[:<tuner>]
        struct configuration
        {
            std::string scheduler;
            std::string tuner;
        };

        configuration parse_configuration(std::string const& text)
        {
            auto const colon = text.find(':');
            if (colon == std::string::npos)
                return configuration{text, ""};
            return configuration{text.substr(0, colon), text.substr(colon + 1)};
        }

        // a measurement with about three significant digits: 15626, 55.2,
        // 1.31
        std::string number(double const value)
        {
            double const magnitude = value < 0 ? -value : value;
            int const decimals = magnitude >= 100.0 ? 0 :
                magnitude >= 10.0                   ? 1 :
                                                      2;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << value;
            return os.str();
        }

        std::string describe(tuning::summary const& s, char const* unit)
        {
            return number(s.median) + " " + unit + " (quartiles " +
                number(s.q1) + " to " + number(s.q3) + ")";
        }

        // the idle area's part of the report: what it tuned for, every
        // measurement and the settings with their reasons
        std::string report(std::vector<configuration> const& configurations,
            std::vector<std::size_t> const& pool_sizes,
            tuning::summary const& clock, tuning::summary const& spin,
            tuning::idle_measurements const& measured,
            std::vector<tuning::tuned_setting> const& settings)
        {
            std::vector<std::string> tuned_for;
            for (configuration const& c : configurations)
            {
                tuned_for.push_back(tuning::json_object({
                    {"scheduler", tuning::json_string(c.scheduler)},
                    {"tuner", tuning::json_string(c.tuner)},
                }));
            }
            // the pool sizes asked for, empty for pools of any size
            std::vector<std::string> sizes;
            for (std::size_t const size : pool_sizes)
            {
                if (size != tuning::any_pool_size)
                    sizes.push_back(std::to_string(size));
            }
            std::vector<std::string> timed_waits;
            for (tuning::timed_wait const& w : measured.timed_waits)
            {
                timed_waits.push_back(tuning::json_object({
                    {"requested_us", tuning::json_number(w.requested_us)},
                    {"actual_us", tuning::to_json(w.actual_us)},
                }));
            }
            std::vector<std::string> chosen;
            for (tuning::tuned_setting const& s : settings)
                chosen.push_back(tuning::to_json(s));

            return tuning::json_object({
                {"configurations", tuning::json_array(tuned_for)},
                {"pool_sizes", tuning::json_array(sizes)},
                {"clock_read_ns", tuning::to_json(clock)},
                {"spin_rate_rounds_per_us", tuning::to_json(spin)},
                {"timed_waits", tuning::json_array(timed_waits)},
                {"wake_up",
                    tuning::json_object({
                        {"asleep_us", tuning::to_json(measured.wake.asleep_us)},
                        {"spinning_us",
                            tuning::to_json(measured.wake.spinning_us)},
                    })},
                {"settings", tuning::json_array(chosen)},
            });
        }
    }    // namespace

    po::options_description idle_options()
    {
        po::options_description desc("Options of the idle area (--idle)");
        // clang-format off
        desc.add_options()
            ("configuration,c", po::value<std::vector<std::string>>(),
                "a scheduler and its tuner as <scheduler>[:<tuner>], as "
                "--hpx:queuing and hpx.scheduler_tuner name them; may be "
                "given more than once (default: local-priority-fifo)")
            ("pool-size,p", po::value<std::vector<std::size_t>>(),
                "a pool size in workers to write settings for; may be given "
                "more than once (default: one section for pools of any "
                "size)");
        // clang-format on
        return desc;
    }

    // The settings measured so far don't depend on the scheduler or the pool
    // size, so every row gets the same ones, and without --pool-size one row
    // per configuration holds for pools of any size (a pool of half a NUMA
    // domain gets them as well as a whole one).
    area_result tune_idle(po::variables_map const& vm,
        tuning::machine_description const& /* machine */,
        tuning::microbenchmark_options const& options, bool const verbose)
    {
        std::vector<std::size_t> pool_sizes = {tuning::any_pool_size};
        if (vm.count("pool-size"))
            pool_sizes = vm["pool-size"].as<std::vector<std::size_t>>();

        std::vector<configuration> configurations;
        if (vm.count("configuration"))
        {
            for (auto const& c :
                vm["configuration"].as<std::vector<std::string>>())
            {
                configurations.push_back(parse_configuration(c));
            }
        }
        else
        {
            configurations.push_back(configuration{"local-priority-fifo", ""});
        }

        // phase 2: the measurements without a runtime
        tuning::summary const clock = tuning::measure_clock_read(options);
        tuning::summary const spin = tuning::measure_spin_rate(options);
        tuning::idle_measurements measured;
        measured.timed_waits = tuning::measure_timed_waits(1024.0, options);
        measured.wake = tuning::measure_wake_cost(options);

        if (verbose)
        {
            std::cout << "clock read: " << describe(clock, "ns") << "\n"
                      << "spin rate: " << describe(spin, "rounds/us") << "\n";
            for (tuning::timed_wait const& w : measured.timed_waits)
            {
                std::cout << "timed wait of " << number(w.requested_us)
                          << " us: " << describe(w.actual_us, "us") << "\n";
            }
            std::cout << "wake up, asleep: "
                      << describe(measured.wake.asleep_us, "us")
                      << "\nwake up, spinning: "
                      << describe(measured.wake.spinning_us, "us") << "\n";
        }

        // phase 4: the rules
        std::vector<tuning::tuned_setting> const settings =
            tuning::derive_idle_settings(measured);

        std::vector<tuning::tuning_row> rows;
        for (configuration const& c : configurations)
        {
            for (std::size_t const size : pool_sizes)
            {
                rows.push_back(
                    tuning::tuning_row{c.scheduler, c.tuner, size, settings});
            }
        }

        bool const any_size = pool_sizes.size() == 1 &&
            pool_sizes.front() == tuning::any_pool_size;
        std::cout << "idle: " << rows.size() << " sections ("
                  << configurations.size() << " configurations, "
                  << (any_size ?
                             std::string("any pool size") :
                             std::to_string(pool_sizes.size()) + " pool sizes")
                  << ")\n";
        for (tuning::tuned_setting const& s : settings)
        {
            std::cout << "  " << s.key << " = " << s.value << " (" << s.reason
                      << (s.is_default ? ", HPX's default" : "") << ")\n";
        }

        return area_result{tuning::to_sections(rows),
            {"idle: clock read " + describe(clock, "ns"),
                "idle: spin rate " + describe(spin, "rounds/us")},
            report(
                configurations, pool_sizes, clock, spin, measured, settings)};
    }
}    // namespace hpx_tune
