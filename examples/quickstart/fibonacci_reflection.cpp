////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
// Fibonacci example using C++26 reflection for remote dispatch.
//
// This is the reflection-based equivalent of fibonacci.cpp.
// Compare the two files to see how C++26 reflection eliminates boilerplate:
//
// Before (fibonacci.cpp):
//   HPX_PLAIN_ACTION(fibonacci, fibonacci_action)  // step 1: define action
//   fibonacci_action fib;
//   hpx::async(fib, locality_id, n - 1);          // step 2: dispatch
//
// After (this file):
//   hpx::async<^^fibonacci>(locality_id, n - 1);  // that is it
//
// No action type. No registration macro. No boilerplate.
// The reflection operator ^^fibonacci extracts the function signature
// at compile time and handles registration automatically.
#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/util.hpp>

#include <cstdint>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Forward declaration required before use of ^^fibonacci below.
std::uint64_t fibonacci(std::uint64_t n);

///////////////////////////////////////////////////////////////////////////////
std::uint64_t fibonacci(std::uint64_t n)
{
    if (n < 2)
        return n;

    hpx::id_type const locality_id = hpx::find_here();

    // C++26 reflection: dispatch fibonacci remotely with no action type.
    // ^^fibonacci reflects the function at compile time -- no
    // HPX_PLAIN_ACTION or HPX_REGISTER_ACTION needed.
    hpx::future<std::uint64_t> n1 = hpx::async<^^fibonacci>(locality_id, n - 1);
    hpx::future<std::uint64_t> n2 = hpx::async<^^fibonacci>(locality_id, n - 2);

    return n1.get() + n2.get();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint64_t n = vm["n-value"].as<std::uint64_t>();

    {
        hpx::chrono::high_resolution_timer t;

        // Direct dispatch via reflection -- no action object needed.
        hpx::future<std::uint64_t> f =
            hpx::async<^^fibonacci>(hpx::find_here(), n);
        std::uint64_t r = f.get();

        char const* fmt = "fibonacci({1}) == {2}\nelapsed time: {3} [s]\n";
        hpx::util::format_to(std::cout, fmt, n, r, t.elapsed());
    }

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("n-value",
        hpx::program_options::value<std::uint64_t>()->default_value(10),
        "n value for the Fibonacci function");

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}

#else

int main()
{
    return 0;
}

#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
