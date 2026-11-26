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
//   --idle  the scheduler's idle settings (idle_area.hpp)
//
// This file is the driver: the command line, the order of the areas, the
// host's fingerprint and the file. Each area lives in files of its own and
// gives the driver its options and a function returning an area_result
// (area.hpp). The measurements and the rules are those of the tuning module
// (hpx/modules/tuning.hpp).

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/version.hpp>

#include "area.hpp"
#include "child.hpp"
#include "idle_area.hpp"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

using hpx_tune::area_result;

namespace {

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

    // A child of an area (child.hpp) measures for that area: it starts the
    // HPX runtime itself and writes its results for the parent. Each area
    // that starts children adds its entry here.
    int run_as_child_of(
        std::string const& area, int /* argc */, char* /* argv */[])
    {
        std::cerr << "hpx_tune: no area '" << area << "' starts children\n";
        return 1;
    }
}    // namespace

int main(int argc, char* argv[])
{
    // a child of an area: before the command line is parsed, as a child's
    // command line has HPX's options too
    if (std::string const area = hpx_tune::child_area(argc, argv);
        !area.empty())
    {
        return run_as_child_of(area, argc, argv);
    }

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
    desc.add(general).add(hpx_tune::idle_options());

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
    tuning::host_fingerprint fingerprint =
        tuning::describe_host(hpx::threads::get_topology());

    std::vector<std::string> comments = {
        "written by hpx_tune on " + current_date(),
        "HPX " + hpx::full_version_as_string() + ", " + hpx::build_string(),
        "pass to an HPX application with --hpx:config=<this file>",
    };
    // the areas' comments and sections; the fingerprint, to which the areas
    // add their entries, comes first once they are done
    std::vector<std::string> area_comments;
    std::vector<tuning::ini_section> area_sections;

    auto const add = [&](area_result&& result) {
        area_comments.insert(area_comments.end(), result.comments.begin(),
            result.comments.end());
        area_sections.insert(area_sections.end(), result.sections.begin(),
            result.sections.end());
    };

    // The order of the areas matters: this process never starts the HPX
    // runtime (no HPX main wrapping, no hpx::init), so that the idle area
    // measures a machine on which nothing of HPX runs. The idle area
    // therefore runs first; an area that starts processes or a runtime has
    // to come after it.
    if (idle)
        add(hpx_tune::tune_idle(vm, machine, options, verbose));

    std::vector<std::string> const fingerprint_comments =
        tuning::to_comments(fingerprint);
    comments.insert(comments.end(), fingerprint_comments.begin(),
        fingerprint_comments.end());
    comments.insert(comments.end(), area_comments.begin(), area_comments.end());
    std::vector<tuning::ini_section> sections = {
        tuning::to_section(fingerprint)};
    sections.insert(sections.end(), area_sections.begin(), area_sections.end());

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
