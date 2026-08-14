//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example demonstrates several things:
//
// - How to initialize (and terminate) the HPX runtime from a global object
//   (see the type `manage_global_runtime' below)
// - How to register and unregister any (kernel) thread with the HPX runtime
// - How to launch an HPX thread from any (registered) kernel thread and
//   how to wait for the HPX thread to run to completion before continuing.
//   Any value returned from the HPX thread will be marshaled back to the
//   calling (kernel) thread.
//
// This scheme is generally useful if HPX should be initialized from a shared
// library and the main executable might not even be aware of this.

#include <hpx/condition_variable.hpp>
#include <hpx/functional.hpp>
#include <hpx/manage_runtime.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <mutex>
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

// This class demonstrates how to initialize a console instance of HPX
// (locality 0). In order to create an HPX instance which connects to a running
// HPX application two changes have to be made:
//
//  - replace hpx::runtime_mode::default_ with hpx::runtime_mode::connect
//  - replace hpx::finalize() with hpx::disconnect()
//
// Note that the mode runtime_mode::default_ corresponds to runtime_mode::console
// if the distributed runtime is enabled, and runtime_mode::local otherwise.
//
// The separation of the implementation into manage_global_runtime and
// manage_global_runtime_impl ensures that the manage_global_runtime_impl
// destructor is called before the thread_local default agent is destructed,
// allowing the former to call yield if necessary while waiting for the runtime
// to shut down. This is done by accessing the default agent before accessing
// the manage_global_runtime object. Although the latter is thread_local, only
// one instance will be created. It is thread_local only to ensure the correct
// sequencing of destructors.
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
            init_args.cfg = {
                // make sure hpx_main is always executed
                "hpx.run_hpx_main!=1",
                // allow for unknown command line options
                "hpx.commandline.allow_unknown!=1",
                // disable HPX' short options
                "hpx.commandline.aliasing!=0",
            };
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
            (void) rts.stop();
        }
    };

    hpx::manage_runtime& get()
    {
        static thread_local init m;
        return m.rts;
    }

    hpx::execution_base::agent_base& agent =
        hpx::execution_base::detail::get_default_agent();
    hpx::manage_runtime& m = get();

public:
    void register_thread(char const* name)
    {
        hpx::register_thread(m.get_runtime_ptr(), name);
    }

    void unregister_thread()
    {
        hpx::unregister_thread(m.get_runtime_ptr());
    }
};

// This global object will initialize HPX in its constructor and make sure HPX
// stops running in its destructor.
manage_global_runtime init;

///////////////////////////////////////////////////////////////////////////////
struct thread_registration_wrapper
{
    thread_registration_wrapper(char const* name)
    {
        // Register this thread with HPX, this should be done once for
        // each external OS-thread intended to invoke HPX functionality.
        // Calling this function more than once will silently fail (will
        // return false).
        init.register_thread(name);
    }
    ~thread_registration_wrapper()
    {
        // Unregister the thread from HPX, this should be done once in the
        // end before the external thread exists.
        init.unregister_thread();
    }
};

///////////////////////////////////////////////////////////////////////////////
// These functions will be executed as HPX threads.
void hpx_thread_func1()
{
    // All of the HPX functionality is available here, including hpx::async,
    // hpx::future, and friends.

    // As an example, just sleep for one second.
    hpx::this_thread::sleep_for(std::chrono::seconds(1));
}

int hpx_thread_func2(int arg)
{
    // All of the HPX functionality is available here, including hpx::async,
    // hpx::future, and friends.

    // As an example, just sleep for one second.
    hpx::this_thread::sleep_for(std::chrono::seconds(1));

    // Simply return the argument.
    return arg;
}

///////////////////////////////////////////////////////////////////////////////
// This code will be executed by a system thread.
void thread_func()
{
    // Register this (kernel) thread with the HPX runtime (unregister at exit).
    // Use a unique name for each of the created threads (could be derived from
    // std::this_thread::get_id()).
    thread_registration_wrapper register_thread("thread_func");

    // Now, a limited number of HPX API functions can be called.

    // Create an HPX thread (returning an int) and wait for it to run to
    // completion.
    int result = hpx::run_as_hpx_thread(&hpx_thread_func2, 42);

    // Create an HPX thread (returning void) and wait for it to run to
    // completion.
    if (result == 42)
        hpx::run_as_hpx_thread(&hpx_thread_func1);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    // Start a new (kernel) thread to demonstrate thread registration with HPX.
    std::thread t(&thread_func);

    // The main thread was automatically registered with the HPX runtime,
    // no explicit registration for this thread is necessary.
    hpx::run_as_hpx_thread(&hpx_thread_func1);

    // wait for the (kernel) thread to run to completion
    t.join();

    return 0;
}
