//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/process.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <signal.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace process = hpx::components::process;

namespace hpx::components::process::posix::initializers::detail {
    struct throw_on_error_test_access
    {
        template <typename PosixExecutor, typename Read>
        static void on_fork_success(throw_on_error const& handler,
            PosixExecutor& executor, Read&& read_some)
        {
            handler.on_fork_success_impl(
                executor, std::forward<Read>(read_some));
        }
    };
}    // namespace hpx::components::process::posix::initializers::detail

namespace {
    struct test_executor
    {
        enum class error_origin
        {
            none,
            setup,
            chdir,
            execve
        };

        pid_t child_pid = -1;
    };

    void reap_child(process::util::child const& child, int expected_status)
    {
        int status = 0;
        pid_t pid;
        do
        {
            pid = ::waitpid(child.pid, &status, 0);
        } while (pid == -1 && errno == EINTR);

        HPX_TEST_EQ(pid, child.pid);
        if (pid == child.pid)
        {
            HPX_TEST(WIFEXITED(status));
            if (WIFEXITED(status))
            {
                HPX_TEST_EQ(WEXITSTATUS(status), expected_status);
            }
        }
    }

    void check_no_children()
    {
        int status = 0;
        errno = 0;
        pid_t const pid = ::waitpid(-1, &status, WNOHANG);
        HPX_TEST_EQ(pid, -1);
        HPX_TEST_EQ(errno, ECHILD);

        // Avoid leaving a child behind if this regression is reintroduced.
        if (pid == 0)
        {
            int child_status = 0;
            do
            {
                errno = 0;
            } while (::waitpid(-1, &child_status, 0) == -1 && errno == EINTR);
        }
    }

    void check_child_reaped(pid_t pid)
    {
        int status = 0;
        errno = 0;
        pid_t const result = ::waitpid(pid, &status, WNOHANG);
        HPX_TEST_EQ(result, -1);
        HPX_TEST_EQ(errno, ECHILD);

        if (result == 0)
        {
            HPX_TEST_EQ(::kill(pid, SIGKILL), 0);
            do
            {
                errno = 0;
            } while (::waitpid(pid, &status, 0) == -1 && errno == EINTR);
        }
    }

    pid_t fork_test_child(bool delayed_exit)
    {
        pid_t const pid = ::fork();
        HPX_TEST(pid >= 0);
        if (pid == 0)
        {
            if (delayed_exit)
            {
                ::sleep(10);
            }
            ::_exit(EXIT_FAILURE);
        }
        return pid;
    }

    template <typename Read>
    void test_injected_read(Read&& read,
        std::initializer_list<std::string> expected_messages,
        bool delayed_exit = false)
    {
        process::throw_on_error handler;
        test_executor executor;
        handler.on_fork_setup(executor);
        executor.child_pid = fork_test_child(delayed_exit);
        if (executor.child_pid < 0)
        {
            return;
        }

        auto const start = std::chrono::steady_clock::now();
        bool caught = false;
        try
        {
            hpx::components::process::posix::initializers::detail::
                throw_on_error_test_access::on_fork_success(
                    handler, executor, std::forward<Read>(read));
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            std::string const message = e.what();
            for (std::string const& expected : expected_messages)
            {
                HPX_TEST(message.find(expected) != std::string::npos);
            }
        }
        HPX_TEST(caught);
        if (delayed_exit)
        {
            HPX_TEST(std::chrono::steady_clock::now() - start <
                std::chrono::seconds(5));
        }
        check_child_reaped(executor.child_pid);
    }

    template <typename... Initializers>
    void test_error(int code, char const* operation, bool handler_first,
        Initializers const&... init)
    {
        bool caught = false;
        try
        {
            if (handler_first)
            {
                process::util::execute(process::throw_on_error(), init...);
            }
            else
            {
                process::util::execute(init..., process::throw_on_error());
            }
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            HPX_TEST(e.get_error() == hpx::error::kernel_error);
            std::string const message = e.what();
            HPX_TEST(message.find(operation) != std::string::npos);
            HPX_TEST(message.find(std::generic_category().message(code)) !=
                std::string::npos);
        }
        HPX_TEST(caught);
        check_no_children();
    }

    void test_error_report_reads()
    {
        std::array<int, 2> const fragmented_report{
            {ENOENT, static_cast<int>(test_executor::error_origin::execve)}};
        std::size_t fragments = 0;
        test_injected_read(
            [&](int, auto& destination, std::size_t offset) -> ssize_t {
                std::size_t const count =
                    fragments++ == 0 ? 1 : sizeof(fragmented_report) - offset;
                std::memcpy(
                    reinterpret_cast<char*>(destination.data()) + offset,
                    reinterpret_cast<char const*>(fragmented_report.data()) +
                        offset,
                    count);
                return static_cast<ssize_t>(count);
            },
            {"execve(2) failed", std::generic_category().message(ENOENT)});

        std::array<int, 2> const interrupted_report{
            {EACCES, static_cast<int>(test_executor::error_origin::chdir)}};
        bool interrupted = false;
        test_injected_read(
            [&](int, auto& destination, std::size_t offset) -> ssize_t {
                if (!interrupted)
                {
                    interrupted = true;
                    errno = EINTR;
                    return -1;
                }

                std::size_t const count = sizeof(interrupted_report) - offset;
                std::memcpy(
                    reinterpret_cast<char*>(destination.data()) + offset,
                    reinterpret_cast<char const*>(interrupted_report.data()) +
                        offset,
                    count);
                return static_cast<ssize_t>(count);
            },
            {"chdir(2) failed", std::generic_category().message(EACCES)});
        HPX_TEST(interrupted);

        bool sent_prefix = false;
        test_injected_read(
            [&](int, auto& destination, std::size_t) -> ssize_t {
                if (sent_prefix)
                {
                    return 0;
                }
                sent_prefix = true;
                *reinterpret_cast<char*>(destination.data()) = 0;
                return 1;
            },
            {"incomplete child error report"});

        test_injected_read(
            [](int, auto&, std::size_t) -> ssize_t {
                errno = EIO;
                return -1;
            },
            {"read(2) failed", std::generic_category().message(EIO)}, true);

        test_injected_read(
            [](int, auto&, std::size_t) -> ssize_t {
                return static_cast<ssize_t>(3 * sizeof(int));
            },
            {"read(2) failed", std::generic_category().message(EIO)}, true);
    }

    void test_successful_exec_does_not_wait(process::inherit_env const& env)
    {
        auto const exe = process::run_exe("/bin/sleep");
        auto const args =
            process::set_args(std::vector<std::string>{"sleep", "10"});
        auto const start = std::chrono::steady_clock::now();
        auto child =
            process::util::execute(exe, args, env, process::throw_on_error());
        HPX_TEST(
            std::chrono::steady_clock::now() - start < std::chrono::seconds(5));

        HPX_TEST_EQ(::kill(child.pid, SIGKILL), 0);
        int status = 0;
        pid_t result;
        do
        {
            result = ::waitpid(child.pid, &status, 0);
        } while (result == -1 && errno == EINTR);
        HPX_TEST_EQ(result, child.pid);
        if (result == child.pid)
        {
            HPX_TEST(WIFSIGNALED(status));
            if (WIFSIGNALED(status))
            {
                HPX_TEST_EQ(WTERMSIG(status), SIGKILL);
            }
        }
    }
}    // namespace

int main()
{
    auto const exe = process::run_exe("/bin/sh");
    auto const env = process::inherit_env();
    auto const args = process::set_args(
        std::vector<std::string>{"sh", "-c", "test \"$(pwd -P)\" = /"});

    auto child = process::util::execute(
        exe, args, env, process::start_in_dir("/"), process::throw_on_error());
    reap_child(child, EXIT_SUCCESS);

    for (bool handler_first : {false, true})
    {
        // Neither execve nor subsequent setup callbacks may run after chdir
        // fails, and error callbacks must see the original errno.
        auto const must_not_run =
            process::on_exec_setup([](auto&) { ::_exit(99); });
        auto const change_errno =
            process::on_exec_error([](auto&) { errno = EACCES; });
        test_error(ENOENT, "chdir(2) failed", handler_first, exe, env,
            process::start_in_dir(""), must_not_run, change_errno);
        test_error(ENOTDIR, "chdir(2) failed", handler_first, exe, env,
            process::start_in_dir("/dev/null"), must_not_run, change_errno);
        auto const missing_exe = process::run_exe(
            "/this/path/does/not/exist/hpx-posix-initializers-7646");
        auto const missing_args = process::set_args(std::vector<std::string>{
            "/this/path/does/not/exist/hpx-posix-initializers-7646"});
        test_error(ENOENT, "execve(2) failed", handler_first, missing_exe,
            missing_args, env, change_errno);

        // Exercise the portable fallback for an unknown errno value too.
        int const unknown_error = (std::numeric_limits<int>::max)();
        test_error(unknown_error, "child process setup failed", handler_first,
            process::on_exec_setup(
                [=](auto& e) { e.exec_error = unknown_error; }));
    }

    test_error_report_reads();
    test_successful_exec_does_not_wait(env);

    // Without throw_on_error the caller still observes an unsuccessful child.
    child = process::util::execute(exe, env, process::start_in_dir(""));
    reap_child(child, EXIT_FAILURE);

    return hpx::util::report_errors();
}
