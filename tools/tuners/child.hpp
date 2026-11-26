//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Children of hpx_tune's areas. The tool's own process never starts the HPX
// runtime (the idle area measures a machine on which nothing of HPX runs); an
// area that needs a running runtime starts hpx_tune again as a child, with
// --child=<area>[:<configuration>], directly or through a launcher such as
// hpxrun.py. The child starts the runtime, measures, and writes its results
// to its standard output as lines starting with the area's prefix
// (child_line_prefix), which the parent collects; everything else the child
// or a launcher prints is ignored.

#pragma once

#include <hpx/config.hpp>

#include <functional>
#include <string>
#include <vector>

namespace hpx_tune {

    // The area this process is a child of, from --child=<area> or
    // --child=<area>:<configuration>; empty if it isn't a child. Looked at
    // before the command line is parsed, as a child's command line has HPX's
    // options too.
    std::string child_area(int argc, char* argv[]);

    // The prefix of the lines a child of the given area writes for its
    // parent: "hpx_tune_<area>:".
    std::string child_line_prefix(std::string const& area);

    // The text in double quotes, for a command line.
    std::string in_quotes(std::string const& text);

    // The text with every occurrence of what replaced by with (for launcher
    // templates).
    std::string replace_all(
        std::string text, std::string const& what, std::string const& with);

    // what a command run for an area returned
    struct child_output
    {
        // the lines that started with the area's prefix, without it
        std::vector<std::string> lines;
        // the command's exit code
        int exit_code = 0;
    };

    // Run a command line (a child of the given area, started directly or
    // through a launcher) and collect the lines of its standard output that
    // start with the area's prefix; on_line sees each of them as it arrives.
    // Throws std::runtime_error if the command can't be started.
    child_output run_command(std::string const& command,
        std::string const& area,
        std::function<void(std::string const&)> const& on_line = {});

    // The command line that starts this executable as a child of the given
    // area, with the given arguments: "<this executable>" --child=<area>
    // <arguments>, each argument in quotes.
    std::string child_command(
        std::string const& area, std::vector<std::string> const& args);

    // Start this executable directly as a child of the given area (one
    // locality) and return its lines for the parent. Throws
    // std::runtime_error if it can't be started or exits with an error.
    std::vector<std::string> run_child(std::string const& area,
        std::vector<std::string> const& args,
        std::function<void(std::string const&)> const& on_line = {});
}    // namespace hpx_tune
