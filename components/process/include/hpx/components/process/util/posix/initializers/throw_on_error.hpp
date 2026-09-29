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

#include <string>
#include <system_error>

namespace hpx { namespace components { namespace process { namespace posix {

    namespace initializers {

        class throw_on_error : public initializer_base
        {
            static std::string extract_error_string(int code)
            {
                return std::generic_category().message(code);
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
                int code;
                if (::read(fds_[0], &code, sizeof(int)) > 0)
                {
                    ::close(fds_[0]);

                    while (::waitpid(e.child_pid, nullptr, 0) == -1 &&
                        errno == EINTR)
                    {
                    }

                    HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                        "throw_on_error::on_fork_success",
                        "process setup or execve(2) failed: {}",
                        extract_error_string(code));
                }
                ::close(fds_[0]);
            }

            template <class PosixExecutor>
            void on_exec_setup(PosixExecutor&) const
            {
                ::close(fds_[0]);
            }

            template <class PosixExecutor>
            void on_exec_error(PosixExecutor& e) const
            {
                int const code = e.exec_error;
                while (::write(fds_[1], &code, sizeof(int)) == -1 &&
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
