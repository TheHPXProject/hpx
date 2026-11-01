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

#include <ios>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

#include "detail/constants.hpp"
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

namespace io = hpx::iostream;

inline bool compare_type_ids(
    std::type_info const& ti1, std::type_info const& ti2)
{
    return ti1.name() == ti2.name();
}

struct indirect_source : io::source
{
    constexpr void foo() noexcept {}

    std::streamsize read(char*, std::streamsize)
    {
        return 0;
    }
};

struct direct_source
{
    using char_type = char;

    struct category
      : io::input
      , io::device_tag
      , io::direct_tag
    {
    };

    constexpr void foo() noexcept {}

    std::span<char> input_sequence()
    {
        return {static_cast<char*>(nullptr), static_cast<char*>(nullptr)};
    }
};

void compile_time_test()
{
    using namespace io;

    stream_buffer<indirect_source> indirect_buf;
    indirect_buf.open(indirect_source());
    indirect_buf->foo();

    stream_buffer<direct_source> direct_buf;
    direct_buf.open(direct_source());
    direct_buf->foo();

    stream<indirect_source> indirect_stream;
    indirect_stream.open(indirect_source());
    indirect_stream->foo();

    stream<direct_source> direct_stream;
    direct_stream.open(direct_source());
    direct_stream->foo();
}

void component_type_test()
{
    using namespace std;
    using namespace io;
    using namespace hpx::iostream::test;

    temp_file dest;
    lowercase_file lower;

    filtering_ostream out;
    out.push(tolower_filter<output>());
    out.push(tolower_multichar_filter<output>());
    out.push(file_sink(dest.name(), out_mode));

    // Check index 0.
    HPX_TEST(compare_type_ids(
        out.component_type(0), typeid(tolower_filter<output>)));
    HPX_TEST_NO_THROW(out.component<tolower_filter<output>>(0));
    HPX_TEST_NO_THROW((out.component<0, tolower_filter<output>>()));

    // Check index 1.
    HPX_TEST(compare_type_ids(
        out.component_type(1), typeid(tolower_multichar_filter<output>)));
    HPX_TEST_NO_THROW(out.component<tolower_multichar_filter<output>>(1));
    HPX_TEST_NO_THROW((out.component<1, tolower_multichar_filter<output>>()));

    // Check index 2.
    HPX_TEST(compare_type_ids(out.component_type(2), typeid(file_sink)));
    HPX_TEST_NO_THROW(out.component<file_sink>(2));
    HPX_TEST_NO_THROW((out.component<2, file_sink>()));

    // Check index 3.
    HPX_TEST_THROW(out.component_type(3), std::out_of_range);

    // Check components.

    filtering_ostream out2;
    out2.push(*(out.component<tolower_filter<output>>(0)));
    out2.push(*(out.component<tolower_multichar_filter<output>>(1)));
    out2.push(*(out.component<file_sink>(2)));
    write_data_in_chunks(out);
    out.reset();
    HPX_TEST_MSG(compare_files(dest.name(), lower.name()),
        "failed accessing components of chain");
}

int main(int, char*[])
{
    component_type_test();
    return hpx::util::report_errors();
}
