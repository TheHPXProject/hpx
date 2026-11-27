//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// What the areas of hpx_tune share with the driver (hpx_tune.cpp). Each area
// lives in files of its own (idle_area.hpp and .cpp, ...) and registers
// itself with HPX_TUNE_REGISTER_AREA in its .cpp: its name (the option
// --<name> that selects it), its options, the function that measures and
// returns an area_result, and, if it starts children (child.hpp), the
// function a child runs. The driver knows no area by name: it offers the
// registered areas, runs the selected ones in their order, and writes the
// results of all of them into one file, and their reports into one JSON file
// if asked to (--report). Adding an area touches only the area's own files
// (and the list of sources in CMakeLists.txt).

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include <string>
#include <vector>

namespace hpx_tune {

    // what an area adds to the ini file: its sections and its comment lines
    // (the measurements the reader of the file should see), and to the
    // report (--report): its measurements as one JSON object (built with
    // hpx/tuning/json.hpp), empty if it has none
    struct area_result
    {
        std::vector<hpx::tuning::ini_section> sections;
        std::vector<std::string> comments;
        std::string report;
    };

    // what the driver gives every area it runs
    struct area_context
    {
        // the parsed command line, with the options of all areas
        hpx::program_options::variables_map const& vm;
        // the machine (phase 1 of the design)
        hpx::tuning::machine_description const& machine;
        // the general measurement options (--repeats)
        hpx::tuning::microbenchmark_options const& options;
        // --verbose: print the measurements
        bool verbose;
        // this executable as it was started (argv[0])
        char const* executable;
        // the host's fingerprint, to which an area adds its own entries
        hpx::tuning::host_fingerprint& fingerprint;
    };

    // one area of the tool
    struct area
    {
        // the area's name: the option that selects it (--<name>), the prefix
        // of its children (--child=<name>) and its key in the report
        std::string name;
        // the help of the option --<name>
        std::string description;
        // the order in which the selected areas run, lowest first. This
        // process never starts the HPX runtime, so that the idle area (0)
        // measures a machine on which nothing of HPX runs; an area that
        // starts children or a runtime comes after it.
        int order = 0;
        // the area's own command line options, nullptr if it has none
        hpx::program_options::options_description (*options)() = nullptr;
        // measure and derive the area's settings; throws if it can't (the
        // tool then writes no file)
        area_result (*tune)(area_context const&) = nullptr;
        // what a child of the area runs (child.hpp), with the child's
        // command line; nullptr if the area starts no children
        int (*child_main)(int argc, char* argv[]) = nullptr;
    };

    // the registered areas, in their order
    std::vector<area> registered_areas();

    // registers an area; used by HPX_TUNE_REGISTER_AREA
    struct area_registrar
    {
        explicit area_registrar(area a);
    };
}    // namespace hpx_tune

// Register an area of the tool, in the area's .cpp, outside of any
// namespace: HPX_TUNE_REGISTER_AREA(idle, hpx_tune::area{"idle", "tune the
// scheduler's idle settings", 0, &idle_options, &tune_idle, nullptr});
#define HPX_TUNE_REGISTER_AREA(id, ...)                                        \
    namespace {                                                                \
        hpx_tune::area_registrar const hpx_tune_area_registrar_##id(           \
            __VA_ARGS__);                                                      \
    }                                                                          \
    /**/
