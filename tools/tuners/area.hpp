//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// What the areas of hpx_tune share with the driver (hpx_tune.cpp). Each area
// lives in files of its own (idle_area.hpp and .cpp, ...) and registers
// itself with HPX_TUNE_REGISTER_AREA in its .cpp: its name (the option
// --<name> that selects it), its options, the function that measures and
// returns an area_result, if it starts children (child.hpp), the function a
// child runs, and the function that checks a written file (--check). The
// driver knows no area by name: it offers the registered areas, runs the
// selected ones in their order, and writes the results of all of them into
// one file, and their reports into one JSON file if asked to (--report);
// with --check=<file> it runs their checks of that file instead. Adding an
// area touches only the area's own files (and the list of sources in
// CMakeLists.txt).

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

    // what the driver gives an area's check (hpx_tune --check=<file>)
    struct verify_context
    {
        // as for tuning: the options, the machine, --verbose, this
        // executable, and this host's fingerprint, to which a check may add
        // the area's entries so that the driver compares them with the
        // profile's
        area_context const& context;
        // the file to check, as given with --check
        std::string const& file;
        // the file's fingerprint ([hpx.tuning.fingerprint]), empty if it has
        // none
        hpx::tuning::host_fingerprint const& profile;
    };

    // what an area's check found: one line per item checked (printed with
    // the area's name in front), the items that don't hold (empty: the file
    // does what it says for this area), and the area's part of the report
    // as one JSON object, empty if it has none
    struct verify_result
    {
        std::vector<std::string> lines;
        std::vector<std::string> failures;
        std::string report;
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
        // check that an HPX application started with a written file does
        // what the file says for the area's settings (hpx_tune
        // --check=<file>), in children, as for tuning; measures and writes
        // nothing; throws if it can't check. nullptr if the area has no
        // check yet (reported, not a failure)
        verify_result (*verify)(verify_context const&) = nullptr;
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
// scheduler's idle settings", 0, &idle_options, &tune_idle, nullptr,
// nullptr}); every member is given, as some compilers warn about missing
// initializers of an aggregate.
#define HPX_TUNE_REGISTER_AREA(id, ...)                                        \
    namespace {                                                                \
        hpx_tune::area_registrar const hpx_tune_area_registrar_##id(           \
            __VA_ARGS__);                                                      \
    }                                                                          \
    /**/
