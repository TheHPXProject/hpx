//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx_tune: measures the machine it runs on and writes HPX settings for it as
// an ini file, to be passed to an HPX application with --hpx:config. The
// settings come in areas, each selected with --<area> (--idle: the
// scheduler's idle settings, idle_area.hpp, ...); without an area option
// every area the build has is tuned.
//
// This file is the driver: the command line, the order of the areas, the
// host's fingerprint and the files. It knows no area by name: each area
// lives in files of its own and registers itself (area.hpp) with its
// options, the function that measures it, and the function its children run
// (child.hpp). The measurements and the rules are those of the tuning module
// (hpx/modules/tuning.hpp).

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/version.hpp>

#include "area.hpp"
#include "child.hpp"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

using hpx_tune::area;
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
    // HPX runtime itself and writes its results for the parent.
    int run_as_child_of(std::string const& name, int argc, char* argv[])
    {
        for (area const& a : hpx_tune::registered_areas())
        {
            if (a.name == name && a.child_main != nullptr)
                return a.child_main(argc, argv);
        }
        std::cerr << "hpx_tune: no area '" << name << "' starts children\n";
        return 1;
    }
}    // namespace

int main(int argc, char* argv[])
{
    // a child of an area: before the command line is parsed, as a child's
    // command line has HPX's options too
    if (std::string const child_of = hpx_tune::child_area(argc, argv);
        !child_of.empty())
    {
        return run_as_child_of(child_of, argc, argv);
    }

    std::vector<area> const areas = hpx_tune::registered_areas();

    po::options_description general(
        "usage: hpx_tune [areas] [options]\n\n"
        "Measures this machine and writes HPX settings for it as an ini file\n"
        "to be passed to an HPX application with --hpx:config. Writing it as\n"
        ".hpx.ini (--output=.hpx.ini) makes the settings the defaults of the\n"
        "HPX applications started in that directory, as HPX reads .hpx.ini\n"
        "from the current and the home directory on its own. Without an\n"
        "area option every area this build has is tuned.\n\n"
        "Areas and general options");
    general.add_options()("help,h", "print this help");
    for (area const& a : areas)
        general.add_options()(a.name.c_str(), a.description.c_str());
    // clang-format off
    general.add_options()
        ("output,o", po::value<std::string>()->default_value("hpx-tuned.ini"),
            "the ini file to write (.hpx.ini: the default settings of HPX "
            "applications started in its directory)")
        ("report", po::value<std::string>(),
            "also write every area's measurements to this JSON file")
        ("repeats,r", po::value<std::size_t>()->default_value(21),
            "how many times each measurement is repeated")
        ("verbose,v", "print the measurements");
    // clang-format on

    po::options_description desc;
    desc.add(general);
    for (area const& a : areas)
    {
        if (a.options != nullptr)
            desc.add(a.options());
    }

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

    // the areas selected on the command line, in their order; without an
    // area option, every area this build has
    std::vector<area> selected;
    for (area const& a : areas)
    {
        if (vm.count(a.name) != 0)
            selected.push_back(a);
    }
    if (selected.empty())
        selected = areas;

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
    // the areas' parts of the report, by area
    std::vector<std::pair<std::string, std::string>> area_reports;

    auto const add = [&](std::string const& area, area_result&& result) {
        area_comments.insert(area_comments.end(), result.comments.begin(),
            result.comments.end());
        area_sections.insert(area_sections.end(), result.sections.begin(),
            result.sections.end());
        if (!result.report.empty())
            area_reports.emplace_back(area, HPX_MOVE(result.report));
    };

    // The order of the areas matters: this process never starts the HPX
    // runtime (no HPX main wrapping, no hpx::init), so that the idle area
    // measures a machine on which nothing of HPX runs. The idle area
    // therefore runs first; an area that starts processes or a runtime comes
    // after it (area::order). An area that can't measure (an invalid option,
    // a child that fails) throws; the tool then writes no file.
    hpx_tune::area_context const context{
        vm, machine, options, verbose, argv[0], fingerprint};
    try
    {
        for (area const& a : selected)
            add(a.name, a.tune(context));
    }
    catch (std::exception const& e)
    {
        std::cerr << "hpx_tune: " << e.what() << "\n";
        return 1;
    }

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

    // the report: {"hpx_tune": <HPX's version>, "written": <date>,
    // "fingerprint": {...}, "areas": {"idle": {...}, ...}}
    if (vm.count("report"))
    {
        std::string const report = vm["report"].as<std::string>();
        std::ofstream json(report);
        if (!json)
        {
            std::cerr << "hpx_tune: can't write " << report << "\n";
            return 1;
        }
        json << tuning::json_object({
                    {"hpx_tune",
                        tuning::json_string(hpx::full_version_as_string())},
                    {"written", tuning::json_string(current_date())},
                    {"fingerprint", tuning::to_json(fingerprint)},
                    {"areas", tuning::json_object(area_reports)},
                })
             << "\n";
        std::cout << "wrote " << report << ": " << area_reports.size()
                  << " areas\n";
    }
    return 0;
}
