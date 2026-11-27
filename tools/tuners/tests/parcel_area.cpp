//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// A smoke test of hpx_tune's parcel area: run the tool as a process of its
// own for the TCP parcelport on this node (the localities started by
// hpxrun.py itself, which needs no launcher), with a few small sizes, and
// check the file it wrote: with HPX's ini reader (the host's fingerprint with
// the parcel area's entries, a report of the rules for TCP) and by starting
// the HPX runtime with it (--hpx:config), so that it is known to be accepted;
// and the JSON report (--report: two passes, both configurations). First an
// option the area can't use (--sizes bogus): the tool fails and writes no
// file. The test checks the form of the file, not whether the settings are
// good for the machine; whether the rules write settings at all depends on
// HPX (host chunks through the device conduit) and on the machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

    std::string const parcel_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel.ini";
    std::string const report_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel.json";

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

    void write_file(std::string const& name, std::string const& text)
    {
        std::ofstream out(name);
        out << text;
    }

    // hpx_tune --check=<file> for the parcel area alone, TCP on this node
    int check_file(std::string const& file, std::string const& report)
    {
        return run_tool("--check=\"" + file +
            "\" --parcel --placement same-node --parcel-configurations tcp "
            "--child-threads 2 --report \"" +
            report + "\"");
    }

    bool contains(std::string const& text, std::string const& what)
    {
        return text.find(what) != std::string::npos;
    }

    // the file through the runtime: accepted with --hpx:config, the parcel
    // area's fingerprint entries in the runtime's configuration
    int check_with_runtime()
    {
        std::string const parcelports = hpx::get_config_entry(
            "hpx.tuning.fingerprint.parcel_parcelports", "");
        HPX_TEST_MSG(contains(parcelports, "tcp"), parcelports.c_str());
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    // an option the parcel area can't use: the tool says so, fails and
    // writes no file (std::system's result isn't the exit code everywhere,
    // only 0 means success)
    {
        std::string const bad_file =
            std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel_bad.ini";
        std::remove(bad_file.c_str());
        HPX_TEST_NEQ(
            run_tool("--parcel --sizes bogus --output \"" + bad_file + "\""),
            0);
        HPX_TEST(!std::ifstream(bad_file).good());
    }

    // the TCP parcelport on this node, without and (where it is built) with
    // the UCX device conduit; hpxrun.py starts the two localities itself
    HPX_TEST_EQ(run_tool("--parcel --placement same-node "
                         "--parcel-configurations tcp --sizes 8K:64K "
                         "--rounds 3 --passes 2 --child-threads 2 --output \"" +
                    parcel_file + "\" --report \"" + report_file + "\""),
        0);

    std::string const text = read_file(parcel_file);
    HPX_TEST_EQ(text.find("# Copyright (c) "), std::string::size_type(0));
    HPX_TEST(contains(text, "\n[hpx.tuning.fingerprint]\n"));

    // the rules' report: rule 2 for TCP either wrote its setting or said why
    // not, as a comment of the file or a setting's reason
    HPX_TEST(contains(text, "rule 2 (tcp)") ||
        contains(text, "conduit_host_chunks"));
    // a child that failed would be reported in the file
    HPX_TEST(!contains(text, "the child failed"));

    // the parcel area's entries in the fingerprint, read with HPX's ini reader
    hpx::util::section file;
    file.read(parcel_file);
    std::string const parcelports =
        file.get_entry("hpx.tuning.fingerprint.parcel_parcelports", "");
    HPX_TEST_MSG(contains(parcelports, "tcp"), parcelports.c_str());
    std::string const measured =
        file.get_entry("hpx.tuning.fingerprint.parcel_measured", "");
    HPX_TEST_MSG(contains(measured, "same-node:tcp:none"), measured.c_str());
    HPX_TEST(
        !file.get_entry("hpx.tuning.fingerprint.parcel_conduits", "").empty());

    // the JSON report: two passes, the placement, the configuration without
    // the conduit
    std::string const report = read_file(report_file);
    HPX_TEST(contains(report, "\"areas\": {\"parcel\": {"));
    HPX_TEST(contains(report, "\"fingerprint\": {"));
    HPX_TEST(contains(report, "\"passes\": 2"));
    HPX_TEST(contains(report, "\"placement\": \"same-node\""));
    HPX_TEST(contains(report, "\"conduit\": \"none\""));
    HPX_TEST(contains(report, "\"combined\": ["));

    // the file through the runtime
    std::string const config = "--hpx:config=" + parcel_file;
    std::vector<char*> args(argv, argv + argc);
    args.push_back(const_cast<char*>(config.c_str()));
    args.push_back(nullptr);
    HPX_TEST_EQ(hpx::local::init(check_with_runtime,
                    static_cast<int>(args.size() - 1), args.data()),
        0);

    // the file checked: an application started with it moves its data the
    // way the file says (tuner chapter 10, level 1)
    std::string const check_report =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel_check.json";
    HPX_TEST_EQ(check_file(parcel_file, check_report), 0);
    {
        std::string const report = read_file(check_report);
        HPX_TEST(contains(report, "\"parcel\": {"));
        HPX_TEST(contains(report, "\"holds\": true"));
        HPX_TEST(!contains(report, "\"holds\": false"));
    }

    // values the tool may not write here: host chunks off for TCP, a
    // threshold of 256 KiB (with or without host chunks in this HPX)
    std::string const tcp_off =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel_tcp_off.ini";
    write_file(tcp_off,
        "[hpx.parcel]\nconduit_host_threshold = "
        "262144\n"
        "[hpx.parcel.tcp]\nconduit_host_chunks = 0\n");
    HPX_TEST_EQ(check_file(tcp_off, check_report), 0);

    // a parcelport this build may not have: never a failure
    std::string const other_parcelport = std::string(HPX_TUNE_TEST_DIRECTORY) +
        "/hpx_tune_parcel_other_parcelport.ini";
    write_file(other_parcelport,
        "[hpx.parcel.lcw]\nconduit_host_chunks = 1\n"
        "zero_copy_serialization_threshold = 4096\n");
    HPX_TEST_EQ(check_file(other_parcelport, check_report), 0);

    // host chunks through a conduit that isn't there: the file can't do
    // what it says, the check fails
    std::string const no_conduit = std::string(HPX_TUNE_TEST_DIRECTORY) +
        "/hpx_tune_parcel_no_conduit.ini";
    write_file(no_conduit,
        "[hpx.parcel]\ndevice_conduit = none\n"
        "[hpx.parcel.tcp]\nconduit_host_chunks = 1\n");
    HPX_TEST_NEQ(check_file(no_conduit, check_report), 0);
    HPX_TEST(contains(read_file(check_report), "\"holds\": false"));

    return hpx::util::report_errors();
}
