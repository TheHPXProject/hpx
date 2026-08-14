//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>
#include <sstream>
#include <string>

namespace {

    struct stream_redirect
    {
        explicit stream_redirect(std::ostream& stream)
          : stream_(stream)
          , previous_(stream.rdbuf(captured_.rdbuf()))
        {
        }

        ~stream_redirect()
        {
            stream_.rdbuf(previous_);
        }

        std::string str() const
        {
            return captured_.str();
        }

    private:
        std::ostream& stream_;
        std::ostringstream captured_;
        std::streambuf* previous_;
    };
}    // namespace

int hpx_main()
{
    HPX_TEST_EQ(&hpx::consolestream, &std::cerr);
    HPX_TEST_EQ(&hpx::get_consolestream(), &std::cerr);

    hpx::cout << "local cout smoke test" << hpx::endl;
    hpx::cerr << "local cerr smoke test" << hpx::flush;
    hpx::consolestream << "local console smoke test" << hpx::endl;
    hpx::util::format_to(hpx::consolestream, "{} {}", "format", 42)
        << hpx::flush;

    return hpx::local::finalize();
}

int main(int argc, char** argv)
{
    stream_redirect captured_cout(std::cout);
    stream_redirect captured_cerr(std::cerr);

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);

    std::string const cout_output = captured_cout.str();
    std::string const cerr_output = captured_cerr.str();

    HPX_TEST(cout_output.find("local cout smoke test\n") != std::string::npos);
    HPX_TEST(cerr_output.find("local cerr smoke test") != std::string::npos);
    HPX_TEST(
        cerr_output.find("local console smoke test\n") != std::string::npos);
    HPX_TEST(cerr_output.find("format 42") != std::string::npos);

    return hpx::util::report_errors();
}
