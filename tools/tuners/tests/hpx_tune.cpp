//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A smoke test of hpx_tune's idle area: run the tool as a process of its own,
// twice, and check the files it wrote: the first by starting the HPX runtime
// with it (--hpx:config) and reading the runtime's configuration, so that the
// file is known to be accepted, the second with HPX's ini reader alone (the
// runtime is started once per process). A file has to hold the host's
// fingerprint and one idle section per configuration and pool size, with
// valid backoff times. The test checks the form of the files, not whether
// the settings are good for the machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/init.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/version.hpp>

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

    // the two runs of the tool: its defaults, and an explicit configuration
    // and pool size
    std::string const default_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_default.ini";
    std::string const explicit_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_explicit.ini";

    // run hpx_tune with the given arguments, return its exit code
    int run_tool(std::string const& arguments)
    {
        std::string command =
            "\"" + std::string(HPX_TUNE_EXECUTABLE) + "\" " + arguments;
#if defined(_WIN32)
        // cmd.exe drops the first and the last quote of a command that
        // starts with one
        command = "\"" + command + "\"";
#endif
        std::cout << "running: " << command << std::endl;
        return std::system(command.c_str());
    }

    std::string read_file(std::string const& name)
    {
        std::ifstream in(name);
        return std::string(std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>());
    }

    // where the entries are read from: the runtime's configuration, or a file
    // read with HPX's ini reader
    hpx::util::section const* file_entries = nullptr;

    std::string entry(std::string const& key)
    {
        return file_entries != nullptr ? file_entries->get_entry(key, "") :
                                         hpx::get_config_entry(key, "");
    }

    // the value of an idle setting of a section, -1 if it is missing or not a
    // number
    double backoff_time(std::string const& section, char const* key)
    {
        std::string const value = entry(section + "." + key);
        if (value.empty())
            return -1.0;
        try
        {
            std::size_t pos = 0;
            double const result = std::stod(value, &pos);
            return pos == value.size() ? result : -1.0;
        }
        catch (...)
        {
            return -1.0;
        }
    }

    void check_idle_section(std::string const& section)
    {
        double const spin = backoff_time(section, "min_idle_backoff_time");
        double const longest = backoff_time(section, "max_idle_backoff_time");
        HPX_TEST_MSG(spin > 0.0, (section + ": min_idle_backoff_time").c_str());
        HPX_TEST_MSG(
            longest > 0.0, (section + ": max_idle_backoff_time").c_str());
        HPX_TEST_MSG(spin <= longest, (section + ": spin > longest").c_str());
    }

    void check_fingerprint()
    {
        hpx::tuning::machine_description const machine =
            hpx::tuning::describe_machine(hpx::threads::get_topology());

        // the fingerprint describes this machine and this build
        HPX_TEST_EQ(entry("hpx.tuning.fingerprint.cores"),
            std::to_string(machine.cores));
        HPX_TEST_EQ(
            entry("hpx.tuning.fingerprint.pus"), std::to_string(machine.pus));
        HPX_TEST_EQ(entry("hpx.tuning.fingerprint.hpx_version"),
            hpx::full_version_as_string());
    }

    // the defaults: one section of the default scheduler per whole-domain
    // pool size
    void check_default_file()
    {
        check_fingerprint();

        hpx::tuning::machine_description const machine =
            hpx::tuning::describe_machine(hpx::threads::get_topology());
        HPX_TEST(!machine.pool_sizes.empty());
        for (std::size_t const size : machine.pool_sizes)
        {
            check_idle_section("hpx.idle_tuning.local-priority-fifo.none." +
                std::to_string(size));
        }
    }

    // only the configuration and the pool size asked for
    void check_explicit_file()
    {
        check_fingerprint();

        check_idle_section("hpx.idle_tuning.local-priority-lifo.static.2");
        HPX_TEST_EQ(entry("hpx.idle_tuning.local-priority-fifo.none.2."
                          "min_idle_backoff_time"),
            std::string());
    }

    int check_with_runtime()
    {
        check_default_file();
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    // the idle area with its defaults: the default scheduler, the machine's
    // whole-domain pool sizes (the parcel area, which starts localities of
    // its own, has a test of its own)
    HPX_TEST_EQ(
        run_tool("--idle --repeats 3 --output \"" + default_file + "\""), 0);
    std::string const text = read_file(default_file);
    HPX_TEST_EQ(text.find("# Copyright (c) "), std::string::size_type(0));
    HPX_TEST(text.find("\n[hpx.tuning.fingerprint]\n") != std::string::npos);
    HPX_TEST(text.find("\n[hpx.idle_tuning.") != std::string::npos);

    // the idle area alone, with an explicit configuration and pool size
    HPX_TEST_EQ(
        run_tool("--idle --repeats 3 --configuration "
                 "local-priority-lifo:static --pool-size 2 --output \"" +
            explicit_file + "\""),
        0);
    {
        hpx::util::section file;
        file.read(explicit_file);
        file_entries = &file;
        check_explicit_file();
        file_entries = nullptr;
    }

    // the first file through the runtime: accepted with --hpx:config
    std::string const config = "--hpx:config=" + default_file;
    std::vector<char*> args(argv, argv + argc);
    args.push_back(const_cast<char*>(config.c_str()));
    args.push_back(nullptr);
    HPX_TEST_EQ(hpx::local::init(check_with_runtime,
                    static_cast<int>(args.size() - 1), args.data()),
        0);

    // a child of an area that starts none is refused (the dispatch of
    // child.hpp, before the command line is parsed)
    HPX_TEST_NEQ(run_tool("--child=no_such_area"), 0);

    return hpx::util::report_errors();
}
