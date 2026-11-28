//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A setting given with --hpx:ini wins over the same setting in a file given
// with --hpx:config, for the local runtime (hpx::local::init) in every build.
// HPX built with the distributed runtime used to leave --hpx:ini out of the
// local runtime's final configuration, so the file, read again when the
// configuration was reconfigured, won.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

    std::string const file_name = "ini_options_over_config_file.ini";

    int hpx_main()
    {
        // the setting given on the command line, the one the file has only,
        // and the one the command line has only
        HPX_TEST_EQ(hpx::get_config_entry("hpx.ini_order_test.both", ""),
            std::string("command_line"));
        HPX_TEST_EQ(hpx::get_config_entry("hpx.ini_order_test.file_only", ""),
            std::string("file"));
        HPX_TEST_EQ(
            hpx::get_config_entry("hpx.ini_order_test.command_line_only", ""),
            std::string("command_line"));
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    {
        std::ofstream file(file_name);
        file << "[hpx.ini_order_test]\n"
             << "both = file\n"
             << "file_only = file\n";
    }

    std::vector<std::string> arguments(argv, argv + argc);
    arguments.emplace_back("--hpx:config=" + file_name);
    arguments.emplace_back("--hpx:ini=hpx.ini_order_test.both=command_line");
    arguments.emplace_back(
        "--hpx:ini=hpx.ini_order_test.command_line_only!=command_line");

    std::vector<char*> args;
    for (std::string& a : arguments)
        args.push_back(a.data());
    args.push_back(nullptr);

    HPX_TEST_EQ(hpx::local::init(
                    hpx_main, static_cast<int>(args.size() - 1), args.data()),
        0);

    std::remove(file_name.c_str());
    return hpx::util::report_errors();
}
