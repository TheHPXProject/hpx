//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Output through hpx::cout after the runtime has stopped goes to the local
// stream (std::cout) instead of the console, which needs the runtime (#8068).

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

int hpx_main()
{
    hpx::cout << "while running" << hpx::endl;
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    // the runtime is stopped: what hpx::cout writes from now on goes to
    // std::cout, captured here (std::flush flushes through the stream's sink,
    // flush_type and endl_type through the synchronous operator)
    std::stringstream captured;
    std::streambuf* const original = std::cout.rdbuf(captured.rdbuf());

    hpx::cout << "flushed by std::flush;" << std::flush;
    hpx::cout << "flushed by flush_type;" << hpx::iostreams::flush_type();
    hpx::cout << "flushed repeatedly;" << std::flush
              << hpx::iostreams::flush_type() << std::flush;
    hpx::cout << "ended by endl_type" << hpx::iostreams::endl_type();

    std::cout.rdbuf(original);

    HPX_TEST_EQ(captured.str(),
        std::string("flushed by std::flush;flushed by flush_type;"
                    "flushed repeatedly;ended by endl_type\n"));

    // left in the buffer: written to std::cout when the global streams are
    // destroyed, which must not touch the stopped runtime either
    hpx::cout << "buffered at exit\n";

    return hpx::util::report_errors();
}
#endif
