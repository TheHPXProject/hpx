//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2014 Jorge Lodos
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iosfwd>
#include <sstream>

#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace hpx::iostream;
using namespace hpx::iostream::test;

void verification_function_seekable_test()
{
    {
        temp_file f;
        fstream io(f.name().c_str(),
            std::ios_base::in | std::ios_base::out | std::ios_base::binary |
                std::ios_base::trunc);
        HPX_TEST_MSG(
            test_seekable_in_chars(io), "failed using test_seekable_in_chars");
        io.close();
    }

    {
        temp_file f;
        fstream io(f.name().c_str(),
            std::ios_base::in | std::ios_base::out | std::ios_base::binary |
                std::ios_base::trunc);
        HPX_TEST_MSG(test_seekable_in_chunks(io),
            "failed using test_seekable_in_chunks");
        io.close();
    }

    {
        temp_file f;
        fstream io(f.name().c_str(),
            std::ios_base::in | std::ios_base::out | std::ios_base::binary |
                std::ios_base::trunc);
        for (std::size_t i = 0; i < data_reps; ++i)
            io.write(narrow_data(), chunk_size);
        io.seekg(0, std::ios_base::beg);
        HPX_TEST_MSG(
            test_input_seekable(io), "failed using test_input_seekable");
        io.close();
    }

    {
        temp_file f;
        fstream io(f.name().c_str(),
            std::ios_base::in | std::ios_base::out | std::ios_base::binary |
                std::ios_base::trunc);
        HPX_TEST_MSG(
            test_output_seekable(io), "failed using test_output_seekable");
        io.close();
    }
}

void verification_function_dual_seekable_test()
{
    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        HPX_TEST_MSG(
            test_seekable_in_chars(ss), "failed using test_seekable_in_chars");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        HPX_TEST_MSG(test_seekable_in_chunks(ss),
            "failed using test_seekable_in_chunks");
    }

    {
        string s;
        for (std::size_t i = 0; i < data_reps; ++i)
            s.append(narrow_data(), chunk_size);
        stringstream ss(s, std::ios_base::in | std::ios_base::out);
        HPX_TEST_MSG(
            test_input_seekable(ss), "failed using test_input_seekable");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        HPX_TEST_MSG(
            test_output_seekable(ss), "failed using test_output_seekable");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        HPX_TEST_MSG(test_dual_seekable(ss), "failed using test_dual_seekable");
    }
}

void dual_seekable_test()
{
    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        filtering_stream<dual_seekable> io(ss);
        io.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        HPX_TEST_MSG(test_seekable_in_chars(io),
            "failed seeking within a string, in chars");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        filtering_stream<dual_seekable> io(ss);
        io.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        HPX_TEST_MSG(test_seekable_in_chunks(io),
            "failed seeking within a string, in chunks");
    }

    {
        string s;
        for (std::size_t i = 0; i < data_reps; ++i)
            s.append(narrow_data(), chunk_size);
        stringstream ss(s, std::ios_base::in | std::ios_base::out);
        filtering_stream<dual_seekable> io(ss);
        io.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        HPX_TEST_MSG(
            test_input_seekable(io), "failed seeking within a string source");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        filtering_stream<dual_seekable> io(ss);
        io.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        HPX_TEST_MSG(
            test_output_seekable(io), "failed seeking within a string sink");
    }

    {
        stringstream ss(std::ios_base::in | std::ios_base::out);
        filtering_stream<dual_seekable> io(ss);
        io.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        HPX_TEST_MSG(
            test_dual_seekable(io), "failed dual seeking within a string");
    }
}

int main(int, char*[])
{
    verification_function_seekable_test();
    verification_function_dual_seekable_test();
    dual_seekable_test();
    return hpx::util::report_errors();
}
