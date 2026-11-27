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

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
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

        int exit_code(int const status)
        {
            return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
        }

        int close_pipe(FILE* pipe)
        {
            int const status = pclose(pipe);
            return status == -1 ? -1 : exit_code(status);
        }
#endif

        // the lines of a child's standard output that start with the area's
        // prefix, without it
        std::vector<std::string> collect_lines(FILE* out,
            std::string const& area,
            std::function<void(std::string const&)> const& on_line)
        {
            std::string const prefix = child_line_prefix(area);
            std::vector<std::string> lines;
            std::string line;
            char buffer[4096];
            while (std::fgets(buffer, sizeof(buffer), out) != nullptr)
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
                    lines.push_back(line.substr(at + prefix.size()));
                    if (on_line)
                        on_line(lines.back());
                }
                line.clear();
            }
            return lines;
        }

        [[noreturn]] void cannot_start(
            std::string const& area, std::string const& program)
        {
            throw std::runtime_error("hpx_tune: can't start the child of the " +
                area + " area: " + program);
        }

#if defined(_WIN32)
        // One argument as the C runtime's command line parser reads it back
        // (CommandLineToArgvW's rules): in quotes if it is empty or has a
        // blank or a quote, a quote escaped with a backslash, and the
        // backslashes in front of a quote or of the closing quote doubled.
        std::string quote_argument(std::string const& arg)
        {
            if (!arg.empty() &&
                arg.find_first_of(" \t\n\v\"") == std::string::npos)
                return arg;

            std::string result = "\"";
            std::size_t backslashes = 0;
            for (char const c : arg)
            {
                if (c == '\\')
                {
                    ++backslashes;
                    continue;
                }
                if (c == '"')
                    result.append(2 * backslashes + 1, '\\');
                else
                    result.append(backslashes, '\\');
                backslashes = 0;
                result += c;
            }
            result.append(2 * backslashes, '\\');
            result += '"';
            return result;
        }

        child_output run_program_windows(std::vector<std::string> const& argv,
            std::string const& area,
            std::function<void(std::string const&)> const& on_line)
        {
            std::string command_line;
            for (std::string const& arg : argv)
            {
                if (!command_line.empty())
                    command_line += ' ';
                command_line += quote_argument(arg);
            }

            // the child writes into the pipe, the parent reads from it; only
            // the child's end is inherited
            SECURITY_ATTRIBUTES inherit{};
            inherit.nLength = sizeof(inherit);
            inherit.bInheritHandle = TRUE;
            HANDLE read_end = nullptr;
            HANDLE write_end = nullptr;
            if (!CreatePipe(&read_end, &write_end, &inherit, 0) ||
                !SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0))
            {
                cannot_start(area, argv.front());
            }

            STARTUPINFOA startup{};
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            startup.hStdOutput = write_end;
            startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

            PROCESS_INFORMATION process{};
            std::vector<char> writable(
                command_line.begin(), command_line.end());
            writable.push_back('\0');
            BOOL const started =
                CreateProcessA(nullptr, writable.data(), nullptr, nullptr, TRUE,
                    0, nullptr, nullptr, &startup, &process);
            CloseHandle(write_end);
            if (!started)
            {
                CloseHandle(read_end);
                cannot_start(area, argv.front());
            }
            CloseHandle(process.hThread);

            // the read end as a FILE*, closed with it
            int const fd = _open_osfhandle(
                reinterpret_cast<intptr_t>(read_end), _O_RDONLY | _O_TEXT);
            FILE* out = fd == -1 ? nullptr : _fdopen(fd, "r");
            child_output output;
            if (out != nullptr)
            {
                output.lines = collect_lines(out, area, on_line);
                std::fclose(out);
            }
            else
            {
                if (fd != -1)
                    _close(fd);
                else
                    CloseHandle(read_end);
            }

            WaitForSingleObject(process.hProcess, INFINITE);
            DWORD code = 0;
            GetExitCodeProcess(process.hProcess, &code);
            CloseHandle(process.hProcess);
            output.exit_code = static_cast<int>(code);
            return output;
        }
#else
        child_output run_program_posix(std::vector<std::string> const& argv,
            std::string const& area,
            std::function<void(std::string const&)> const& on_line)
        {
            int fds[2];
            if (pipe(fds) != 0)
                cannot_start(area, argv.front());

            // the child's standard output is the pipe's write end
            posix_spawn_file_actions_t actions;
            posix_spawn_file_actions_init(&actions);
            posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
            posix_spawn_file_actions_addclose(&actions, fds[0]);
            posix_spawn_file_actions_addclose(&actions, fds[1]);

            std::vector<char*> args;
            args.reserve(argv.size() + 1);
            for (std::string const& arg : argv)
                args.push_back(const_cast<char*>(arg.c_str()));
            args.push_back(nullptr);

            pid_t pid = 0;
            int const spawned = posix_spawnp(
                &pid, args[0], &actions, nullptr, args.data(), environ);
            posix_spawn_file_actions_destroy(&actions);
            close(fds[1]);
            if (spawned != 0)
            {
                close(fds[0]);
                cannot_start(area, argv.front());
            }

            child_output output;
            FILE* out = fdopen(fds[0], "r");
            if (out != nullptr)
            {
                output.lines = collect_lines(out, area, on_line);
                std::fclose(out);
            }
            else
            {
                close(fds[0]);
            }

            int status = 0;
            while (waitpid(pid, &status, 0) == -1)
            {
            }
            output.exit_code = exit_code(status);
            return output;
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
            cannot_start(area, command);

        child_output output;
        output.lines = collect_lines(pipe, area, on_line);
        output.exit_code = close_pipe(pipe);
        return output;
    }

    child_output run_program(std::vector<std::string> const& argv,
        std::string const& area,
        std::function<void(std::string const&)> const& on_line)
    {
        if (argv.empty())
            cannot_start(area, "(no program)");
#if defined(_WIN32)
        return run_program_windows(argv, area, on_line);
#else
        return run_program_posix(argv, area, on_line);
#endif
    }

    std::vector<std::string> child_arguments(
        std::string const& area, std::vector<std::string> const& args)
    {
        std::vector<std::string> argv = {
            hpx::util::get_executable_filename(), child_option + area};
        argv.insert(argv.end(), args.begin(), args.end());
        return argv;
    }

    std::string child_command(
        std::string const& area, std::vector<std::string> const& args)
    {
        std::vector<std::string> const argv = child_arguments(area, args);
        std::string command = in_quotes(argv[0]);
        for (std::size_t i = 1; i != argv.size(); ++i)
            command += " " + in_quotes(argv[i]);
        return command;
    }

    std::vector<std::string> run_child(std::string const& area,
        std::vector<std::string> const& args,
        std::function<void(std::string const&)> const& on_line)
    {
        child_output output =
            run_program(child_arguments(area, args), area, on_line);
        if (output.exit_code != 0)
        {
            throw std::runtime_error("hpx_tune: the child of the " + area +
                " area exited with " + std::to_string(output.exit_code));
        }
        return HPX_MOVE(output.lines);
    }
}    // namespace hpx_tune
