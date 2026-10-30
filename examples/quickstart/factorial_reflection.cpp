////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
// Factorial example using C++26 reflection for remote dispatch.
//
// This is the reflection-based equivalent of factorial.cpp.
// Compare the two files to see how C++26 reflection eliminates boilerplate:
//
// Before (factorial.cpp):
//   HPX_PLAIN_ACTION(factorial, factorial_action)  // define action type
//   hpx::async<factorial_action>(hpx::find_here(), n - 1);
//
// After (this file):
//   hpx::async<^^factorial>(hpx::find_here(), n - 1);  // that is it
//
// No action type. No registration macro. No boilerplate.
// The reflection operator ^^factorial extracts the function signature
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
// Forward declaration required before use of ^^factorial below.
std::uint64_t factorial(std::uint64_t n);

///////////////////////////////////////////////////////////////////////////////
std::uint64_t factorial(std::uint64_t n)
{
    if (n == 0)
        return 1;

    // C++26 reflection: dispatch factorial remotely with no action type.
    // ^^factorial reflects the function at compile time -- no
    // HPX_PLAIN_ACTION or HPX_REGISTER_ACTION needed.
    hpx::future<std::uint64_t> n1 =
        hpx::async<^^factorial>(hpx::find_here(), n - 1);
    return n * n1.get();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint64_t n = vm["n-value"].as<std::uint64_t>();
    if (n > 20)
    {
        std::cerr << "n-value must be <= 20 to avoid uint64_t overflow\n";
        return hpx::finalize();
    }
    {
        hpx::chrono::high_resolution_timer t;

        // Direct dispatch via reflection -- no action object needed.
        std::uint64_t r = hpx::async<^^factorial>(hpx::find_here(), n).get();
        double elapsed = t.elapsed();

        hpx::util::format_to(std::cout,
            "factorial({1}) == {2}\n"
            "elapsed time == {3} [s]\n",
            n, r, elapsed);
    }
    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    using hpx::program_options::options_description;
    using hpx::program_options::value;

    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("n-value",
        value<std::uint64_t>()->default_value(10),
        "n value for the factorial function");

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
