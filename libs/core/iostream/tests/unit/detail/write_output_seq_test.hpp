//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/modules/iostream.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <fstream>
#include <vector>
#include "sequence.hpp"
#include "temp_file.hpp"
#include "verification.hpp"

void write_output_sequence_test()
{
    using namespace std;
    using namespace hpx;
    using namespace hpx::iostream;
    using namespace hpx::iostream::test;

    test_file test;

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_ostream out(
            hpx::util::iterator_range(first.begin(), first.end()), 0);
        write_data_in_chars(out);
        ifstream second(test.name().c_str());
        HPX_TEST_MSG(compare_container_and_stream(first, second),
            "failed writing to filtering_ostream based on a sequence "
            "in chars with no buffer");
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_ostream out(
            hpx::util::iterator_range(first.begin(), first.end()), 0);
        write_data_in_chunks(out);
        ifstream second(test.name().c_str());
        HPX_TEST_MSG(compare_container_and_stream(first, second),
            "failed writing to filtering_ostream based on a sequence "
            "in chunks with no buffer");
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_ostream out(
            hpx::util::iterator_range(first.begin(), first.end()));
        write_data_in_chars(out);
        ifstream second(test.name().c_str());
        HPX_TEST_MSG(compare_container_and_stream(first, second),
            "failed writing to filtering_ostream based on a sequence "
            "in chars with large buffer");
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_ostream out(
            hpx::util::iterator_range(first.begin(), first.end()));
        write_data_in_chunks(out);
        ifstream second(test.name().c_str());
        HPX_TEST_MSG(compare_container_and_stream(first, second),
            "failed writing to filtering_ostream based on a sequence "
            "in chunks with large buffer");
    }
}
