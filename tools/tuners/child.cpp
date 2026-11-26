//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/prefix.hpp>

#include "child.hpp"

#include <cstddef>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace hpx_tune {

    namespace {

        constexpr char child_option[] = "--child=";

#if defined(_WIN32)
        FILE* open_pipe(std::string const& command)
        {
            // cmd.exe drops the first and the last quote of a command that
            // starts with one
            std::string const quoted = "\"" + command + "\"";
            return _popen(quoted.c_str(), "r");
        }

        int close_pipe(FILE* pipe)
        {
            return _pclose(pipe);
        }
#else
        FILE* open_pipe(std::string const& command)
        {
            return popen(command.c_str(), "r");
        }

        int close_pipe(FILE* pipe)
        {
            int const status = pclose(pipe);
            if (status == -1)
                return -1;
            return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
        }
#endif
    }    // namespace

    std::string child_area(int argc, char* argv[])
    {
        constexpr std::size_t length = sizeof(child_option) - 1;
        for (int i = 1; i < argc; ++i)
        {
            std::string const arg = argv[i];
            if (arg.compare(0, length, child_option) == 0)
                return arg.substr(length, arg.find(':') - length);
        }
        return {};
    }

    std::string child_line_prefix(std::string const& area)
    {
        return "hpx_tune_" + area + ":";
    }

    std::string in_quotes(std::string const& text)
    {
        return "\"" + text + "\"";
    }

    std::string replace_all(
        std::string text, std::string const& what, std::string const& with)
    {
        for (std::size_t pos = text.find(what); pos != std::string::npos;
            pos = text.find(what, pos + with.size()))
        {
            text.replace(pos, what.size(), with);
        }
        return text;
    }

    child_output run_command(std::string const& command,
        std::string const& area,
        std::function<void(std::string const&)> const& on_line)
    {
        FILE* pipe = open_pipe(command);
        if (pipe == nullptr)
        {
            throw std::runtime_error("hpx_tune: can't start the child of the " +
                area + " area: " + command);
        }

        std::string const prefix = child_line_prefix(area);
        child_output output;
        std::string line;
        char buffer[4096];
        while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            line += buffer;
            if (line.empty() || line.back() != '\n')
                continue;
            line.pop_back();
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            std::string::size_type const at = line.find(prefix);
            if (at != std::string::npos)
            {
                output.lines.push_back(line.substr(at + prefix.size()));
                if (on_line)
                    on_line(output.lines.back());
            }
            line.clear();
        }
        output.exit_code = close_pipe(pipe);
        return output;
    }

    std::string child_command(
        std::string const& area, std::vector<std::string> const& args)
    {
        std::string command = in_quotes(hpx::util::get_executable_filename()) +
            " " + child_option + area;
        for (std::string const& arg : args)
            command += " " + in_quotes(arg);
        return command;
    }

    std::vector<std::string> run_child(std::string const& area,
        std::vector<std::string> const& args,
        std::function<void(std::string const&)> const& on_line)
    {
        child_output output =
            run_command(child_command(area, args), area, on_line);
        if (output.exit_code != 0)
        {
            throw std::runtime_error("hpx_tune: the child of the " + area +
                " area exited with " + std::to_string(output.exit_code));
        }
        return HPX_MOVE(output.lines);
    }
}    // namespace hpx_tune
