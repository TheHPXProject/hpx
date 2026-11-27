//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The idle area end to end (tuner chapter 13, level 1): hpx_tune --check
// starts children with a file and compares each pool's idle settings with
// the row the runtime's lookup should take. Files the tool wrote and files
// written by hand have to hold; a child given another value than the file
// says (--check-ini) has to fail, so that the check is known to be able to
// fail. The check compares with the file, not with the machine, so the test
// holds on any machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/modules/testing.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

    std::string const directory = HPX_TUNE_TEST_DIRECTORY;

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

    std::string in_quotes(std::string const& text)
    {
        return "\"" + text + "\"";
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

    int check(std::string const& file, std::string const& more = "")
    {
        return run_tool("--idle --check " + in_quotes(file) + more);
    }
}    // namespace

int main()
{
    // 1: the tool's own file, two configurations, rows for pools of any size
    std::string const any_size = directory + "/idle_e2e_any_size.ini";
    HPX_TEST_EQ(run_tool("--idle --repeats 3 --configuration "
                         "local-priority-fifo --configuration "
                         "local-priority-tuned-fifo:static --output " +
                    in_quotes(any_size)),
        0);
    HPX_TEST_EQ(check(any_size), 0);

    // 2: the tool's file with rows of one pool size only: the pool of that
    // size gets its row, the other the plain settings
    std::string const by_size = directory + "/idle_e2e_by_size.ini";
    HPX_TEST_EQ(run_tool("--idle --repeats 3 --pool-size 1 --output " +
                    in_quotes(by_size)),
        0);
    HPX_TEST_EQ(check(by_size), 0);

    // 3: written by hand, setting by setting: the longest step only in the
    // row for any size, the spin only in the row of one worker
    std::string const partial = directory + "/idle_e2e_partial.ini";
    write_file(partial,
        "[hpx.idle_tuning.local-priority-fifo.none]\n"
        "max_idle_backoff_time = 3000\n"
        "\n"
        "[hpx.idle_tuning.local-priority-fifo.none.1]\n"
        "min_idle_backoff_time = 70\n");
    HPX_TEST_EQ(check(partial), 0);

    // 4: written by hand, invalid values: ignored by the runtime and by the
    // check alike
    std::string const invalid = directory + "/idle_e2e_invalid.ini";
    write_file(invalid,
        "[hpx.idle_tuning.local-priority-fifo.none]\n"
        "max_idle_backoff_time = -5\n"
        "min_idle_backoff_time = many\n");
    HPX_TEST_EQ(check(invalid), 0);

    // 5: the children run with another longest step than the file's row
    // says: the check has to fail, on that value (not because a child
    // couldn't start)
    std::string const report = directory + "/idle_e2e_must_fail.json";
    HPX_TEST_NEQ(check(any_size,
                     " --check-ini=hpx.idle_tuning.local-priority-fifo.none."
                     "max_idle_backoff_time=12345 --report " +
                         in_quotes(report)),
        0);
    HPX_TEST(read_file(report).find("max_idle_backoff_time 12345, the row "
                                    "for any size says") != std::string::npos);

    return hpx::util::report_errors();
}
