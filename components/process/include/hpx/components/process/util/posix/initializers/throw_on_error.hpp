// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_WINDOWS)
#include <hpx/components/process/util/posix/initializers/initializer_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstddef>
#include <string>
#include <system_error>

namespace hpx { namespace components { namespace process { namespace posix {

    namespace initializers {

        class throw_on_error : public initializer_base
        {
            using error_report = std::array<int, 2>;
            static constexpr auto error_report_size = 2 * sizeof(int);

            static std::string extract_error_string(int code)
            {
                return std::generic_category().message(code);
            }

            static void wait_for_child(pid_t pid) noexcept
            {
                while (::waitpid(pid, nullptr, 0) == -1 && errno == EINTR)
                {
                }
            }

            static auto read_error_report(
                int fd, error_report& report, std::size_t offset) noexcept
            {
                auto* const data = reinterpret_cast<char*>(report.data());

                // The destination and requested size stay within the array.
                // flawfinder: ignore
                return ::read(fd, data + offset, error_report_size - offset);
            }

            template <class PosixExecutor>
            static char const* error_operation(int origin) noexcept
            {
                using error_origin = typename PosixExecutor::error_origin;
                switch (static_cast<error_origin>(origin))
                {
                case error_origin::chdir:
                    return "chdir(2)";
                case error_origin::execve:
                    return "execve(2)";
                case error_origin::none:
                case error_origin::setup:
                    return "child process setup";
                }
                return "child process setup";
            }

        public:
            template <class PosixExecutor>
            void on_fork_setup(PosixExecutor&) const
            {
                if (::pipe(fds_) == -1)
                {
                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_setup", "pipe(2) failed: {}",
                        extract_error_string(errno));
                }
                if (::fcntl(fds_[1], F_SETFD, FD_CLOEXEC) == -1)
                {
                    ::close(fds_[0]);
                    ::close(fds_[1]);

                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_setup", "fcntl(2) failed: {}",
                        extract_error_string(errno));
                }
            }

            template <class PosixExecutor>
            void on_fork_error(PosixExecutor&) const
            {
                ::close(fds_[0]);
                ::close(fds_[1]);

                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "throw_on_error::on_fork_error", "fork(2) failed: {}",
                    extract_error_string(errno));
            }

            template <class PosixExecutor>
            void on_fork_success(PosixExecutor& e) const
            {
                ::close(fds_[1]);
                error_report report{};
                std::size_t bytes_read = 0;
                int read_error = 0;
                while (bytes_read != error_report_size)
                {
                    auto const count =
                        read_error_report(fds_[0], report, bytes_read);
                    if (count > 0)
                    {
                        bytes_read += static_cast<std::size_t>(count);
                    }
                    else if (count == 0)
                    {
                        break;
                    }
                    else if (errno != EINTR)
                    {
                        read_error = errno;
                        break;
                    }
                }
                ::close(fds_[0]);

                if (bytes_read == error_report_size)
                {
                    wait_for_child(e.child_pid);
                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_success", "{} failed: {}",
                        error_operation<PosixExecutor>(report[1]),
                        extract_error_string(report[0]));
                }
                else if (read_error != 0)
                {
                    wait_for_child(e.child_pid);
                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_success", "read(2) failed: {}",
                        extract_error_string(read_error));
                }
                else if (bytes_read != 0)
                {
                    wait_for_child(e.child_pid);
                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_success",
                        "incomplete child error report");
                }
            }

            template <class PosixExecutor>
            void on_exec_setup(PosixExecutor&) const
            {
                ::close(fds_[0]);
            }

            template <class PosixExecutor>
            void on_exec_error(PosixExecutor& e) const
            {
                error_report const report{
                    e.exec_error, static_cast<int>(e.exec_error_origin)};
                while (
                    ::write(fds_[1], report.data(), error_report_size) == -1 &&
                    errno == EINTR)
                    ;
                ::close(fds_[1]);
            }

        private:
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive&, unsigned const)
            {
            }

            mutable int fds_[2];
        };

}}}}}    // namespace hpx::components::process::posix::initializers

#endif
