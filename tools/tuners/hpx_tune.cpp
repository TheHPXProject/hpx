//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx_tune: measures the machine it runs on and writes HPX settings for it as
// an ini file, to be passed to an HPX application with --hpx:config. The
// settings come in areas, chosen on the command line; without an area option
// every area the build supports is tuned:
//
//   --idle  the scheduler's idle settings, one section per scheduler, tuner
//           and pool size (hpx.idle_tuning.<scheduler>.<tuner>.<pool size>)
//
// The measurements and the rules are those of the tuning module
// (hpx/modules/tuning.hpp); this program only calls them and writes the file.
//
// The idle area doesn't start the HPX runtime: idle workers would disturb its
// measurements. Run the tool on an otherwise idle machine.

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/version.hpp>

#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace {

    // what an area adds to the ini file
    struct area_result
    {
        std::vector<tuning::ini_section> sections;
        std::vector<std::string> comments;
    };

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

    std::tm local_time()
    {
        std::time_t const now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        return local;
    }

    std::string current_date()
    {
        std::tm const local = local_time();
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local);
        return buffer;
    }

    // a measurement with about three significant digits: 15626, 55.2, 1.31
    std::string number(double const value)
    {
        double const magnitude = value < 0 ? -value : value;
        int const decimals = magnitude >= 100.0 ? 0 : magnitude >= 10.0 ? 1 : 2;
        std::ostringstream os;
        os << std::fixed << std::setprecision(decimals) << value;
        return os.str();
    }

    std::string describe(tuning::summary const& s, char const* unit)
    {
        return number(s.median) + " " + unit + " (quartiles " + number(s.q1) +
            " to " + number(s.q3) + ")";
    }

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
                "more than once (default: the whole NUMA domains, sockets "
                "and the machine)");
        // clang-format on
        return desc;
    }

    // The idle area: phases 1, 2 and 4 of the design, without the runtime.
    // The settings measured so far don't depend on the scheduler or the pool
    // size, so every row gets the same ones.
    area_result tune_idle(po::variables_map const& vm,
        tuning::machine_description const& machine,
        tuning::microbenchmark_options const& options, bool const verbose)
    {
        std::vector<std::size_t> pool_sizes = machine.pool_sizes;
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

        std::cout << "idle: " << rows.size() << " sections ("
                  << configurations.size() << " configurations, "
                  << pool_sizes.size() << " pool sizes)\n";
        for (tuning::tuned_setting const& s : settings)
        {
            std::cout << "  " << s.key << " = " << s.value << " (" << s.reason
                      << (s.is_default ? ", HPX's default" : "") << ")\n";
        }

        return area_result{tuning::to_sections(rows),
            {"idle: clock read " + describe(clock, "ns"),
                "idle: spin rate " + describe(spin, "rounds/us")}};
    }
}    // namespace

int main(int argc, char* argv[])
{
    po::options_description general(
        "usage: hpx_tune [areas] [options]\n\n"
        "Measures this machine and writes HPX settings for it as an ini file\n"
        "to be passed to an HPX application with --hpx:config. Without an\n"
        "area option every area this build supports is tuned.\n\n"
        "Areas and general options");
    // clang-format off
    general.add_options()
        ("help,h", "print this help")
        ("idle", "tune the scheduler's idle settings")
        ("output,o", po::value<std::string>()->default_value("hpx-tuned.ini"),
            "the ini file to write")
        ("repeats,r", po::value<std::size_t>()->default_value(21),
            "how many times each measurement is repeated")
        ("verbose,v", "print the measurements");
    // clang-format on

    po::options_description desc;
    desc.add(general).add(idle_options());

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (std::exception const& e)
    {
        std::cerr << "hpx_tune: " << e.what() << "\n" << desc << "\n";
        return 1;
    }
    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    // without an area option, every area this build supports
    bool const all_areas = vm.count("idle") == 0;
    bool const idle = all_areas || vm.count("idle") != 0;

    bool const verbose = vm.count("verbose") != 0;
    tuning::microbenchmark_options options;
    options.repeats = vm["repeats"].as<std::size_t>();

    // phase 1: the machine, for every area
    tuning::machine_description const machine =
        tuning::describe_machine(hpx::threads::get_topology());

    // what the settings were measured on; each area adds its own entries
    tuning::host_fingerprint const fingerprint =
        tuning::describe_host(hpx::threads::get_topology());

    std::vector<std::string> comments = {
        "written by hpx_tune on " + current_date(),
        "HPX " + hpx::full_version_as_string() + ", " + hpx::build_string(),
        "pass to an HPX application with --hpx:config=<this file>",
    };
    std::vector<std::string> const fingerprint_comments =
        tuning::to_comments(fingerprint);
    comments.insert(comments.end(), fingerprint_comments.begin(),
        fingerprint_comments.end());
    std::vector<tuning::ini_section> sections = {
        tuning::to_section(fingerprint)};

    auto const add = [&](area_result&& result) {
        comments.insert(
            comments.end(), result.comments.begin(), result.comments.end());
        sections.insert(
            sections.end(), result.sections.begin(), result.sections.end());
    };

    if (idle)
        add(tune_idle(vm, machine, options, verbose));

    std::string const ini =
        tuning::to_ini(sections, comments, local_time().tm_year + 1900);
    std::string const output = vm["output"].as<std::string>();
    std::ofstream out(output);
    if (!out)
    {
        std::cerr << "hpx_tune: can't write " << output << "\n";
        return 1;
    }
    out << ini;

    std::cout << "wrote " << output << ": " << sections.size() << " sections\n";
    return 0;
}
