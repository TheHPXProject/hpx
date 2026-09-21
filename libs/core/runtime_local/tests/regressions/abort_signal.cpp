//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <csignal>
#include <cstdlib>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

int hpx_main()
{
    std::abort();
}

int main(int argc, char* argv[])
{
    // Start the runtime only in the child, after fork, and check the signal
    // in the parent. A recursive abort handler can otherwise end in SIGSEGV.
    pid_t const child = fork();
    HPX_TEST(child >= 0);
    if (child == 0)
    {
        rlimit const limit{0, 0};
        if (setrlimit(RLIMIT_CORE, &limit) != 0)
            std::_Exit(EXIT_FAILURE);

        hpx::local::init_params const params;
        std::_Exit(hpx::local::init(hpx_main, argc, argv, params));
    }
    if (child > 0)
    {
        int status = 0;
        HPX_TEST_EQ(waitpid(child, &status, 0), child);
        HPX_TEST(WIFSIGNALED(status));
        if (WIFSIGNALED(status))
            HPX_TEST_EQ(WTERMSIG(status), SIGABRT);
    }
    return hpx::util::report_errors();
}
