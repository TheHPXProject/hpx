//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A smoke test of hpx_tune's memory area: run the tool as a process of its
// own, with one run and few samples, and check what it wrote. The file has
// to hold a rate for data in memory (and copy_bytes_per_ns with it), each
// between 1 and 4096 bytes per ns; the report has to hold the area's part
// with a level for memory; hpx_tune --check has to hold for the file and
// fail for one whose rate the runtime doesn't take (above 4096). The test
// checks the form of the files and that HPX takes their values, not whether
// the rates are good for the machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/modules/ini.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

    std::string const memory_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_memory.ini";
    std::string const report_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_memory.json";
    std::string const check_report_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_memory_check.json";
    std::string const wrong_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_memory_wrong.ini";

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

    void check_rate(hpx::util::section const& file, std::string const& key)
    {
        std::string const value = file.get_entry(key, "");
        HPX_TEST_MSG(!value.empty(), key.c_str());
        if (!value.empty())
        {
            unsigned long const rate = std::stoul(value);
            HPX_TEST_MSG(rate >= 1 && rate <= 4096, key.c_str());
        }
    }
}    // namespace

int main()
{
    HPX_TEST_EQ(run_tool("--memory --memory-runs 1 --repeats 3 --output \"" +
                    memory_file + "\" --report \"" + report_file + "\""),
        0);

    {
        hpx::util::section file;
        file.read(memory_file);
        check_rate(file, "hpx.task_size_chunk_size.copy_bytes_per_ns_memory");
        check_rate(file, "hpx.task_size_chunk_size.copy_bytes_per_ns");
    }
    {
        std::string const json = read_file(report_file);
        for (char const* member : {"\"areas\": {\"memory\": {", "\"runs\": 1",
                 "\"level\": \"memory\"", "\"medians_between_runs\": "})
        {
            HPX_TEST_MSG(json.find(member) != std::string::npos, member);
        }
    }

    // the file checked: a child started with it reads back its rates
    HPX_TEST_EQ(run_tool("--memory --check \"" + memory_file +
                    "\" --report \"" + check_report_file + "\""),
        0);
    {
        std::string const json = read_file(check_report_file);
        for (char const* member : {"\"check\": {\"memory\": {\"checked\": true",
                 "\"failures\": []", "\"holds\": true"})
        {
            HPX_TEST_MSG(json.find(member) != std::string::npos, member);
        }
    }

    // a rate the runtime doesn't take (above 4096): it keeps the default,
    // the check fails
    {
        std::ofstream out(wrong_file);
        out << "[hpx.task_size_chunk_size]\n"
            << "copy_bytes_per_ns_memory = 5000\n";
    }
    HPX_TEST_NEQ(run_tool("--memory --check \"" + wrong_file + "\""), 0);

    return hpx::util::report_errors();
}
