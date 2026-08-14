//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/manage_runtime.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Store the command line arguments in global variables to make them available
// to the startup code.

#if defined(linux) || defined(__linux) || defined(__linux__)

int __argc = 0;
char** __argv = nullptr;

void set_argc_argv(int argc, char* argv[], char*[])
{
    __argc = argc;
    __argv = argv;
}

__attribute__((section(".preinit_array"))) void (*set_global_argc_argv)(
    int, char*[], char*[]) = &set_argc_argv;

#elif defined(__APPLE__)

#include <crt_externs.h>

inline int get_arraylen(char** argv)
{
    int count = 0;
    if (nullptr != argv)
    {
        while (nullptr != argv[count])
            ++count;
    }
    return count;
}

int __argc = get_arraylen(*_NSGetArgv());
char** __argv = *_NSGetArgv();

#endif

///////////////////////////////////////////////////////////////////////////////
class manage_global_runtime
{
    struct init
    {
        hpx::manage_runtime rts;

        init()
        {
#if defined(HPX_WINDOWS) && defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            hpx::detail::init_winsocket();
#endif

            hpx::init_params init_args;
            init_args.cfg = {// make sure hpx_main is always executed
                "hpx.run_hpx_main!=1",
                // allow for unknown command line options
                "hpx.commandline.allow_unknown!=1",
                // disable HPX' short options
                "hpx.commandline.aliasing!=0",
                // run on two threads
                "hpx.os_threads=2"};
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            init_args.mode = hpx::runtime_mode::default_;
#endif

            if (!rts.start(__argc, __argv, init_args))
            {
                // Something went wrong while initializing the runtime.
                // This early we can't generate any output, just bail out.
                std::abort();
            }
        }

        ~init()
        {
            // Something went wrong while stopping the runtime. Ignore.
            [[maybe_unused]] auto result = rts.stop();
        }
    };

    static hpx::manage_runtime& get()
    {
        thread_local init m;
        return m.rts;
    }

    hpx::manage_runtime& m = get();
};

// This global object will initialize HPX in its constructor and make sure HPX
// stops running in its destructor.
manage_global_runtime init;

int main()
{
    constexpr int num_iterations = 65536;
    std::atomic<int> invoked(0);
    std::vector<int> vs(num_iterations);

    {
        hpx::run_as_hpx_thread([&]() {
            invoked.store(0);

            hpx::execution::experimental::fork_join_executor exec;
            hpx::for_each(hpx::execution::par.on(exec), vs.begin(), vs.end(),
                [&](int) { ++invoked; });

            HPX_TEST_EQ(invoked.load(), num_iterations);
        });
    }

    {
        // The thread created by run_as_hpx_thread needs to have at least the
        // same priority as the threads created by the fork-join executor.
        constexpr hpx::launch::async_policy policy(
            hpx::threads::thread_priority::bound);

        hpx::execution::experimental::fork_join_executor exec;
        hpx::run_as_hpx_thread(policy, [&]() {
            invoked.store(0);

            hpx::for_each(hpx::execution::par.on(exec), vs.begin(), vs.end(),
                [&](int) { ++invoked; });

            HPX_TEST_EQ(invoked.load(), num_iterations);
        });
    }

    {
        invoked.store(0);

        hpx::execution::experimental::fork_join_executor exec;
        hpx::for_each(hpx::execution::par.on(exec), vs.begin(), vs.end(),
            [&](int) { ++invoked; });

        HPX_TEST_EQ(invoked.load(), num_iterations);
    }

    {
        invoked.store(0);

        hpx::for_each(
            hpx::execution::par, vs.begin(), vs.end(), [&](int) { ++invoked; });

        HPX_TEST_EQ(invoked.load(), num_iterations);
    }

    return hpx::util::report_errors();
}
