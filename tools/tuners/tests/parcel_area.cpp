//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// A smoke test of hpx_tune's parcel area: run the tool as a process of its
// own for the TCP parcelport on this node (the localities started by
// hpxrun.py itself, which needs no launcher), with a few small sizes, and
// check the file it wrote with HPX's ini reader: the host's fingerprint with
// the parcel area's entries, and a report of the rules for TCP. The test
// checks the form of the file, not whether the settings are good for the
// machine; whether the rules write settings at all depends on HPX (host
// chunks through the device conduit) and on the machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/config.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

    std::string const parcel_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_parcel.ini";

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

    bool contains(std::string const& text, std::string const& what)
    {
        return text.find(what) != std::string::npos;
    }
}    // namespace

int main()
{
    // the TCP parcelport on this node, without and (where it is built) with
    // the UCX device conduit; hpxrun.py starts the two localities itself
    HPX_TEST_EQ(run_tool("--parcel --placement same-node "
                         "--parcel-configurations tcp --sizes 8K:64K "
                         "--rounds 3 --child-threads 2 --output \"" +
                    parcel_file + "\""),
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

    return hpx::util::report_errors();
}
