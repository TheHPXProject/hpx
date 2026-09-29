//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/process.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <cerrno>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace process = hpx::components::process;

namespace {
    void reap_child(int expected_status)
    {
        int status = 0;
        pid_t pid;
        do
        {
            pid = ::waitpid(-1, &status, 0);
        } while (pid == -1 && errno == EINTR);

        HPX_TEST(pid > 0);
        if (pid > 0)
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
            reap_child(EXIT_FAILURE);
        }
    }

    template <typename... Initializers>
    void test_error(int code, bool handler_first, Initializers const&... init)
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
            HPX_TEST(std::string(e.what()).find(std::generic_category().message(
                         code)) != std::string::npos);
        }
        HPX_TEST(caught);
        check_no_children();
    }
}    // namespace

int main()
{
    auto const exe = process::run_exe("/bin/sh");
    auto const env = process::inherit_env();
    auto const args = process::set_args(
        std::vector<std::string>{"sh", "-c", "test \"$(pwd -P)\" = /"});

    process::util::execute(
        exe, args, env, process::start_in_dir("/"), process::throw_on_error());
    reap_child(EXIT_SUCCESS);

    for (bool handler_first : {false, true})
    {
        // Neither execve nor subsequent setup callbacks may run after chdir
        // fails, and error callbacks must see the original errno.
        auto const must_not_run =
            process::on_exec_setup([](auto&) { ::_exit(99); });
        auto const change_errno =
            process::on_exec_error([](auto&) { errno = EACCES; });
        test_error(ENOENT, handler_first, exe, env, process::start_in_dir(""),
            must_not_run, change_errno);
        test_error(ENOTDIR, handler_first, exe, env,
            process::start_in_dir("/dev/null"), must_not_run, change_errno);
        test_error(
            ENOENT, handler_first, process::run_exe(""), env, change_errno);

        // Exercise the portable fallback for an unknown errno value too.
        int const unknown_error = (std::numeric_limits<int>::max)();
        test_error(unknown_error, handler_first,
            process::on_exec_setup(
                [=](auto& e) { e.exec_error = unknown_error; }));
    }

    // Without throw_on_error the caller still observes an unsuccessful child.
    process::util::execute(exe, env, process::start_in_dir(""));
    reap_child(EXIT_FAILURE);

    return hpx::util::report_errors();
}
