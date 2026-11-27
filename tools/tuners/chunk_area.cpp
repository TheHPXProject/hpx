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
    }    // namespace

    po::options_description chunk_options()
    {
        po::options_description desc("Options of the chunk area (--chunk)");
        // clang-format off
        desc.add_options()
            ("chunk-pool-size", po::value<std::vector<std::size_t>>(),
                "a pool size in workers (one per core) to time the regions "
                "for; may be given more than once (default: the whole NUMA "
                "domains, sockets and the machine)")
            ("chunk-rule", po::value<std::string>()->default_value("fitted"),
                "how the minimal run time of a chunk follows from the region "
                "times: 'fitted' (from the fitted region and chunk costs) or "
                "'ratio' (from the region times for as many indices as "
                "workers)")
            ("chunk-regions", po::value<std::size_t>()->default_value(2000),
                "the regions timed together for one sample");
        // clang-format on
        return desc;
    }

    area_result tune_chunk(po::variables_map const& vm,
        tuning::machine_description const& machine, std::size_t const repeats,
        bool const verbose)
    {
        std::vector<std::size_t> pool_sizes = machine.pool_sizes;
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

        std::vector<tuning::region_time> times;
        for (std::size_t const size : pool_sizes)
        {
            std::vector<std::string> const lines = run_child("chunk",
                {"--hpx:threads=" + std::to_string(size),
                    "--hpx:pu-step=" + std::to_string(pu_step),
                    "--chunk-regions=" + std::to_string(regions),
                    "--repeats=" + std::to_string(repeats)});
            for (std::string const& line : lines)
            {
                if (auto const time = tuning::from_line(line))
                    times.push_back(*time);
            }
        }

        std::vector<tuning::chunk_cost> const costs =
            tuning::fit_chunk_costs(times);
        double const default_us = std::chrono::duration<double, std::micro>(
            hpx::execution::experimental::detail::
                get_task_size_chunk_size_defaults()
                    .min_chunk_time)
                                      .count();
        std::vector<tuning::ini_section> sections =
            tuning::chunk_sections(costs, default_us, *rule);

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
                  << pool_sizes.size() << " pool sizes, rule "
                  << tuning::to_string(*rule) << ")\n";
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

        return area_result{HPX_MOVE(sections), HPX_MOVE(comments), {}};
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
