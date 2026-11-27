//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/init_runtime_local.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"
#include "child.hpp"
#include "chunk_area.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace hpx_tune {

    namespace {

        // a value with about three significant digits: 2.50, 0.593, 15.6
        std::string number(double const value)
        {
            double const magnitude = value < 0 ? -value : value;
            int const decimals = magnitude >= 100.0 ? 0 :
                magnitude >= 10.0                   ? 1 :
                magnitude >= 1.0                    ? 2 :
                                                      3;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << value;
            return os.str();
        }

        // the options a child of the chunk area understands besides HPX's
        po::options_description child_options()
        {
            po::options_description desc(
                "Options of a child of the chunk area");
            // clang-format off
            desc.add_options()
                ("child", po::value<std::string>(),
                    "run as a child of the given area")
                ("chunk-regions", po::value<std::size_t>()->default_value(2000),
                    "the regions timed together for one sample")
                ("repeats", po::value<std::size_t>()->default_value(21),
                    "the samples of each executor and number of indices");
            // clang-format on
            return desc;
        }

        // the chunk area's part of the report: what it measured for, every
        // region time, the fitted costs and the settings with their reasons
        std::string report(std::vector<std::size_t> const& pool_sizes,
            std::size_t const regions, std::size_t const runs,
            tuning::chunk_rule const rule, double const default_us,
            std::size_t const chunks_per_core,
            std::vector<tuning::region_time> const& measured,
            std::vector<std::size_t> const& run_of,
            std::vector<tuning::region_time> const& times,
            std::vector<tuning::chunk_cost> const& costs,
            std::vector<tuning::ini_section> const& sections)
        {
            std::vector<std::string> sizes;
            for (std::size_t const size : pool_sizes)
                sizes.push_back(std::to_string(size));

            auto const region_time = [](tuning::region_time const& t,
                                         std::string const* run) {
                std::vector<std::pair<std::string, std::string>> members;
                if (run != nullptr)
                    members.emplace_back("run", *run);
                members.emplace_back(
                    "executor", tuning::json_string(t.executor));
                members.emplace_back("pool_size", std::to_string(t.pool_size));
                members.emplace_back("indices", std::to_string(t.indices));
                members.emplace_back("time_us", tuning::to_json(t.time_us));
                return tuning::json_object(members);
            };

            std::vector<std::string> measured_times;
            for (std::size_t i = 0; i != measured.size(); ++i)
            {
                std::string const run = std::to_string(run_of[i]);
                measured_times.push_back(region_time(measured[i], &run));
            }
            std::vector<std::string> region_times;
            for (tuning::region_time const& t : times)
                region_times.push_back(region_time(t, nullptr));

            std::vector<std::string> fitted;
            for (tuning::chunk_cost const& c : costs)
            {
                fitted.push_back(tuning::json_object({
                    {"executor", tuning::json_string(c.executor)},
                    {"pool_size", std::to_string(c.pool_size)},
                    {"region_us", tuning::json_number(c.region_us)},
                    {"chunk_us", tuning::json_number(c.chunk_us)},
                    {"residual_us", tuning::json_number(c.residual_us)},
                    {"time_at_pool_size_us",
                        tuning::json_number(c.time_at_pool_size_us)},
                    {"spread", tuning::json_number(c.spread)},
                }));
            }

            std::vector<std::string> chosen;
            for (tuning::ini_section const& s : sections)
            {
                for (tuning::tuned_setting const& setting : s.settings)
                    chosen.push_back(tuning::to_json(setting));
            }

            return tuning::json_object({
                {"pool_sizes", tuning::json_array(sizes)},
                {"regions", std::to_string(regions)},
                {"runs", std::to_string(runs)},
                {"rule", tuning::json_string(tuning::to_string(rule))},
                {"default_min_chunk_time_us", tuning::json_number(default_us)},
                {"chunks_per_core", std::to_string(chunks_per_core)},
                {"measured_region_times", tuning::json_array(measured_times)},
                {"region_times", tuning::json_array(region_times)},
                {"costs", tuning::json_array(fitted)},
                {"settings", tuning::json_array(chosen)},
            });
        }

        // The pool sizes timed without --chunk-pool-size: the machine's
        // whole domains (machine_description) and half a NUMA domain, as
        // pools smaller than a domain have regions of their own costs. The
        // smallest whole domain is a NUMA domain (or the machine, if it has
        // only one).
        std::vector<std::size_t> default_pool_sizes(
            tuning::machine_description const& machine)
        {
            std::vector<std::size_t> sizes = machine.pool_sizes;
            if (!sizes.empty() && sizes.front() / 2 != 0)
                sizes.insert(sizes.begin(), sizes.front() / 2);
            return sizes;
        }
    }    // namespace

    po::options_description chunk_options()
    {
        po::options_description desc("Options of the chunk area (--chunk)");
        // clang-format off
        desc.add_options()
            ("chunk-pool-size", po::value<std::vector<std::size_t>>(),
                "a pool size in workers (one per core) to time the regions "
                "for; may be given more than once (default: half a NUMA "
                "domain, the whole NUMA domains, sockets and the machine)")
            ("chunk-rule", po::value<std::string>()->default_value("fitted"),
                "how the minimal run time of a chunk follows from the region "
                "times: 'fitted' (from the fitted region and chunk costs) or "
                "'ratio' (from the region times for as many indices as "
                "workers)")
            ("chunk-regions", po::value<std::size_t>()->default_value(2000),
                "the regions timed together for one sample")
            ("chunk-runs", po::value<std::size_t>()->default_value(3),
                "how many children time the regions of each pool size, one "
                "after the other; the median of their times is used");
        // clang-format on
        return desc;
    }

    area_result tune_chunk(area_context const& context)
    {
        po::variables_map const& vm = context.vm;
        tuning::machine_description const& machine = context.machine;
        std::size_t const repeats = context.options.repeats;
        bool const verbose = context.verbose;

        std::vector<std::size_t> pool_sizes = default_pool_sizes(machine);
        if (vm.count("chunk-pool-size"))
        {
            pool_sizes = vm["chunk-pool-size"].as<std::vector<std::size_t>>();
        }

        std::string const rule_name = vm["chunk-rule"].as<std::string>();
        std::optional<tuning::chunk_rule> const rule =
            tuning::chunk_rule_from_string(rule_name);
        if (!rule)
        {
            throw std::invalid_argument(
                "--chunk-rule: unknown rule '" + rule_name + "'");
        }

        // one worker per core: every pus / cores-th PU
        std::size_t const pu_step = machine.cores == 0 ?
            1 :
            (std::max) (std::size_t(1), machine.pus / machine.cores);
        std::size_t const regions = vm["chunk-regions"].as<std::size_t>();
        std::size_t const runs = vm["chunk-runs"].as<std::size_t>();
        if (runs == 0)
            throw std::invalid_argument("--chunk-runs: must be at least 1");

        // The runs differ more than the samples of one run do (a child's
        // workers, the other processes on the machine), so each pool size
        // is timed by several children, one after the other, and the
        // median of their times is used.
        std::vector<tuning::region_time> measured;
        std::vector<std::size_t> run_of;
        for (std::size_t const size : pool_sizes)
        {
            for (std::size_t run = 0; run != runs; ++run)
            {
                std::vector<std::string> const lines = run_child("chunk",
                    {"--hpx:threads=" + std::to_string(size),
                        "--hpx:pu-step=" + std::to_string(pu_step),
                        "--chunk-regions=" + std::to_string(regions),
                        "--repeats=" + std::to_string(repeats)});
                for (std::string const& line : lines)
                {
                    if (auto const time = tuning::from_line(line))
                    {
                        measured.push_back(*time);
                        run_of.push_back(run);
                    }
                }
            }
        }
        std::vector<tuning::region_time> const times =
            tuning::median_of_runs(measured);

        std::vector<tuning::chunk_cost> const costs =
            tuning::fit_chunk_costs(times);
        // the minimal run time and the chunks per core of task_size_chunk_size
        // as HPX ships them (this process doesn't start the runtime, so no
        // configuration changes them); the values are relative to these
        auto const defaults = hpx::execution::experimental::detail::
            get_task_size_chunk_size_defaults();
        std::size_t const chunks_per_core = defaults.chunks_per_core;
        double const default_us =
            std::chrono::duration<double, std::micro>(defaults.min_chunk_time)
                .count();
        std::vector<tuning::ini_section> sections =
            tuning::chunk_sections(costs, default_us, *rule, chunks_per_core);

        std::vector<std::string> comments;
        for (tuning::chunk_cost const& c : costs)
        {
            comments.push_back("chunk: " + c.executor + " at " +
                std::to_string(c.pool_size) + " workers: region " +
                number(c.region_us) + " us, chunk " + number(c.chunk_us) +
                " us (residual " + number(c.residual_us) + " us)");
        }

        if (verbose)
        {
            for (std::size_t i = 0; i != measured.size(); ++i)
            {
                tuning::region_time const& t = measured[i];
                std::cout << "chunk: run " << run_of[i] << ", " << t.executor
                          << ", " << t.pool_size << " workers, " << t.indices
                          << " indices: " << number(t.time_us.median)
                          << " us\n";
            }
            for (tuning::region_time const& t : times)
            {
                std::cout << "chunk: " << t.executor << ", " << t.pool_size
                          << " workers, " << t.indices
                          << " indices: " << number(t.time_us.median)
                          << " us (quartiles " << number(t.time_us.q1) << " to "
                          << number(t.time_us.q3) << ")\n";
            }
        }

        std::cout << "chunk: " << sections.size() << " sections ("
                  << pool_sizes.size() << " pool sizes, " << runs
                  << " runs each, rule " << tuning::to_string(*rule) << ")\n";
        for (tuning::ini_section const& s : sections)
        {
            for (tuning::tuned_setting const& setting : s.settings)
            {
                std::cout << "  " << setting.key << " = " << setting.value
                          << " (" << setting.reason
                          << (setting.is_default ? ", HPX's default" : "")
                          << ")\n";
            }
        }

        std::string json = report(pool_sizes, regions, runs, *rule, default_us,
            chunks_per_core, measured, run_of, times, costs, sections);
        return area_result{
            HPX_MOVE(sections), HPX_MOVE(comments), HPX_MOVE(json)};
    }

    int chunk_child_main(int argc, char* argv[])
    {
        po::options_description const desc = child_options();
        hpx::local::init_params params;
        params.desc_cmdline = desc;

        return hpx::local::init(
            [](po::variables_map& vm) {
                tuning::chunk_cost_options options;
                options.regions = vm["chunk-regions"].as<std::size_t>();
                options.repeats = vm["repeats"].as<std::size_t>();

                std::string const prefix = child_line_prefix("chunk");
                for (tuning::region_time const& t :
                    tuning::measure_region_times(options))
                {
                    std::cout << prefix << tuning::to_line(t) << "\n";
                }
                std::cout << std::flush;
                return hpx::local::finalize();
            },
            argc, argv, params);
    }
}    // namespace hpx_tune

// after the idle area (0), as its children start the HPX runtime
HPX_TUNE_REGISTER_AREA(chunk,
    hpx_tune::area{"chunk",
        "tune the minimal run time of a chunk of each executor", 10,
        &hpx_tune::chunk_options, &hpx_tune::tune_chunk,
        &hpx_tune::chunk_child_main})
