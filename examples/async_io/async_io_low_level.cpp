//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to demonstrate how it is possible to
// schedule an IO task onto one of the IO-threads in HPX (which are OS-threads)
// and how to synchronize the result of this IO task with a waiting HPX thread.

#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/util.hpp>
#include <hpx/iostream.hpp>

#include <hpx/modules/io_service.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

#include <iostream>
#include <memory>

// this function will be executed by a dedicated OS thread
void do_async_io(
    char const* string_to_write, std::shared_ptr<hpx::promise<int>> p)
{
    // This IO operation will possibly block the IO thread in the
    // kernel.
    std::cout << "OS-thread: " << string_to_write << std::endl;

    p->set_value(0);    // notify the waiting HPX thread and return a value
}

// This function will be executed by an HPX thread
hpx::future<int> async_io(char const* string_to_write)
{
    std::shared_ptr<hpx::promise<int>> p =
        std::make_shared<hpx::promise<int>>();

    // Get a reference to one of the IO specific HPX io_service objects ...
    hpx::util::io_service_pool* pool =
        hpx::get_runtime().get_thread_pool("io_pool");

    // ... and schedule the handler to run on one of its OS-threads.
#if ASIO_VERSION >= 103400
    ::asio::post(
        pool->get_io_service(), hpx::bind(&do_async_io, string_to_write, p));
#else
    pool->get_io_service().post(hpx::bind(&do_async_io, string_to_write, p));
#endif

    return p->get_future();
}

int hpx_main()
{
    {
        // Initiate an asynchronous IO operation wait for it to complete without
        // blocking any of the HPX thread-manager threads.
        hpx::future<int> f = async_io("Write this string to std::cout");

        // This will suspend the current HPX thread until the IO operation is
        // finished.
        int result = f.get();

        // Print the returned result.
        hpx::cout << "HPX-thread: The asynchronous IO operation returned: "
                  << result << "\n"
                  << std::flush;
    }

    return hpx::finalize();    // Initiate shutdown of the runtime system.
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);    // Initialize and run HPX.
}
