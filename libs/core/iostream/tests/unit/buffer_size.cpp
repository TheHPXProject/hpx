//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include "detail/filters.hpp"    // Must come before operations.hpp for VC6.

#include <ios>

using namespace std;
using namespace hpx::iostream;
using namespace hpx::iostream::test;

struct optimally_buffered_filter
{
    typedef char char_type;

    struct category
      : input_filter_tag
      , optimally_buffered_tag
    {
    };

    std::streamsize optimal_buffer_size() const
    {
        return default_filter_buffer_size + 1;
    }
};

void buffer_size_test()
{
    // Test   device buffer sizes.
    HPX_TEST_MSG(
        optimal_buffer_size(null_source()) == default_device_buffer_size,
        "wrong buffer size for sourcer");
    HPX_TEST_MSG(optimal_buffer_size(null_sink()) == default_device_buffer_size,
        "wrong buffer size for sink");

    // Test   filter buffer sizes.
    HPX_TEST_MSG(optimal_buffer_size(toupper_filter<output>()) ==
            default_filter_buffer_size,
        "wrong buffer size for input filter");
    HPX_TEST_MSG(optimal_buffer_size(tolower_filter<input>()) ==
            default_filter_buffer_size,
        "wrong buffer size for output filter");
    HPX_TEST_MSG(optimal_buffer_size(toupper_multichar_filter<input>()) ==
            default_filter_buffer_size,
        "wrong buffer size for multi-character input filter");
    HPX_TEST_MSG(optimal_buffer_size(tolower_multichar_filter<output>()) ==
            default_filter_buffer_size,
        "wrong buffer size for multi-character output filter");

    // Test   custom buffer size.
    HPX_TEST_MSG(optimal_buffer_size(optimally_buffered_filter()) ==
            optimally_buffered_filter().optimal_buffer_size(),
        "wrong buffer size for multi-character output filter");
}

int main(int, char*[])
{
    buffer_size_test();
    return hpx::util::report_errors();
}
