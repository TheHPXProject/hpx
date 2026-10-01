//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/process.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <signal.h>
#include <string>
#include <system_error>
#include <thread>
#include <time.h>
#include <vector>

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

namespace process = hpx::components::process;

namespace {
    sig_atomic_t volatile signal_count = 0;

    extern "C" void handle_signal(int)
    {
        signal_count = 1;
    }

    void delay_signal_test()
    {
        timespec delay{0, 100000000};
        while (::nanosleep(&delay, &delay) == -1 && errno == EINTR)
        {
        }
    }

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

    template <typename... Initializers>
    void test_interrupted_read(Initializers const&... init)
    {
        struct sigaction action{};
        struct sigaction previous_action{};
        action.sa_handler = &handle_signal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        HPX_TEST_EQ(::sigaction(SIGUSR1, &action, &previous_action), 0);

        signal_count = 0;
        std::atomic<int> signal_error{0};
        std::jthread interrupter;
        auto const start_interrupting = process::on_fork_success([&](auto&) {
            pthread_t const reading_thread = ::pthread_self();
            interrupter = std::jthread([&, reading_thread](
                                           std::stop_token stop) {
                while (!stop.stop_requested())
                {
                    timespec delay{0, 10000000};
                    while (::nanosleep(&delay, &delay) == -1 && errno == EINTR)
                    {
                    }
                    int const error = ::pthread_kill(reading_thread, SIGUSR1);
                    if (error != 0)
                    {
                        signal_error = error;
                        return;
                    }
                }
            });
        });
        auto const delayed_error = process::on_exec_setup([](auto& e) {
            delay_signal_test();
            delay_signal_test();
            e.exec_error = ENOENT;
        });

        bool caught = false;
        try
        {
            process::util::execute(start_interrupting,
                process::throw_on_error(), init..., delayed_error);
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            std::string const message = e.what();
            HPX_TEST(message.find("child process setup failed") !=
                std::string::npos);
            HPX_TEST(message.find(std::generic_category().message(ENOENT)) !=
                std::string::npos);
        }
        interrupter.request_stop();
        if (interrupter.joinable())
        {
            interrupter.join();
        }
        HPX_TEST(caught);
        HPX_TEST_EQ(signal_count, 1);
        HPX_TEST_EQ(signal_error.load(), 0);
        check_no_children();

        HPX_TEST_EQ(::sigaction(SIGUSR1, &previous_action, nullptr), 0);
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

    test_interrupted_read(exe, env);

    // Without throw_on_error the caller still observes an unsuccessful child.
    child = process::util::execute(exe, env, process::start_in_dir(""));
    reap_child(child, EXIT_FAILURE);

    return hpx::util::report_errors();
}
