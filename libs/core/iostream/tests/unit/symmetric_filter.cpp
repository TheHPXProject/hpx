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

#include <cstddef>

#include "detail/closable.hpp"
#include "detail/constants.hpp"
#include "detail/filter_tests.hpp"
#include "detail/operation_sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

#include <hpx/config/warnings_prefix.hpp>

using namespace hpx::iostream;
using namespace hpx::iostream::test;
namespace io = hpx::iostream;

// Note: The filter is given an internal buffer -- unnecessary in this simple
// case -- to stress test symmetric_filter.
struct toupper_symmetric_filter_impl
{
    using char_type = char;

    explicit toupper_symmetric_filter_impl(
        std::streamsize buffer_size = default_filter_buffer_size)
      : buf_(buffer_size)
    {
        buf_.set(0, 0);
    }

    bool filter(char const*& src_begin, char const* src_end, char*& dest_begin,
        char* dest_end, bool /* flush */)
    {
        while (can_read(src_begin, src_end) || can_write(dest_begin, dest_end))
        {
            if (can_read(src_begin, src_end))
                read(src_begin, src_end);
            if (can_write(dest_begin, dest_end))
                write(dest_begin, dest_end);
        }

        bool result = buf_.ptr() != buf_.eptr();
        return result;
    }

    void close()
    {
        buf_.set(0, 0);
    }

    void read(char const*& src_begin, char const* src_end)
    {
        std::ptrdiff_t count = (std::min) (src_end - src_begin,
            static_cast<std::ptrdiff_t>(buf_.size()) -
                (buf_.eptr() - buf_.data()));
        while (count-- > 0)
            *buf_.eptr()++ = static_cast<char>(std::toupper(*src_begin++));
    }

    void write(char*& dest_begin, char* dest_end)
    {
        std::ptrdiff_t count =
            (std::min) (dest_end - dest_begin, buf_.eptr() - buf_.ptr());
        while (count-- > 0)
            *dest_begin++ = *buf_.ptr()++;
        if (buf_.ptr() == buf_.eptr())
            buf_.set(0, 0);
    }
    bool can_read(char const*& src_begin, char const* src_end)
    {
        return src_begin != src_end && buf_.eptr() != buf_.end();
    }

    bool can_write(char*& dest_begin, char* dest_end)
    {
        return dest_begin != dest_end && buf_.ptr() != buf_.eptr();
    }

    hpx::iostream::detail::buffer<char> buf_;
};

using toupper_symmetric_filter =
    symmetric_filter<toupper_symmetric_filter_impl>;

void read_symmetric_filter()
{
    test_file test;
    uppercase_file upper;
    HPX_TEST(test_input_filter(
        toupper_symmetric_filter(default_filter_buffer_size),
        file_source(test.name(), in_mode), file_source(upper.name(), in_mode)));
}

void write_symmetric_filter()
{
    test_file test;
    uppercase_file upper;
    HPX_TEST(test_output_filter(
        toupper_symmetric_filter(default_filter_buffer_size),
        file_source(test.name(), in_mode), file_source(upper.name(), in_mode)));
}

void close_symmetric_filter()
{
    // Test input
    {
        operation_sequence seq;
        chain<input> ch;
        ch.push(io::symmetric_filter<closable_symmetric_filter>(
            1, seq.new_operation(2)));
        ch.push(closable_device<input>(seq.new_operation(1)));
        HPX_TEST_NO_THROW(ch.reset());
        HPX_TEST_MSG(seq.is_success(), seq.message());
    }

    // Test output
    {
        operation_sequence seq;
        chain<output> ch;
        ch.push(io::symmetric_filter<closable_symmetric_filter>(
            1, seq.new_operation(1)));
        ch.push(closable_device<output>(seq.new_operation(2)));
        HPX_TEST_NO_THROW(ch.reset());
        HPX_TEST_MSG(seq.is_success(), seq.message());
    }
}

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)

struct wcopy_filter_impl
{
    typedef wchar_t char_type;
    bool filter(wchar_t const*& src_begin, wchar_t const* src_end,
        wchar_t*& dest_begin, wchar_t* dest_end, bool /* flush */)
    {
        if (src_begin != src_end && dest_begin != dest_end)
        {
            *dest_begin++ = *src_begin++;
        }
        return false;
    }
    void close() {}
};

using wcopy_filter = symmetric_filter<wcopy_filter_impl>;

void wide_symmetric_filter()
{
    {
        warray_source src(wide_data(), wide_data() + data_length());
        std::wstring dest;
        io::copy(src, io::compose(wcopy_filter(16), io::back_inserter(dest)));
        HPX_TEST(dest == wide_data());
    }
    {
        warray_source src(wide_data(), wide_data() + data_length());
        std::wstring dest;
        io::copy(io::compose(wcopy_filter(16), src), io::back_inserter(dest));
        HPX_TEST(dest == wide_data());
    }
}

#endif

int main(int, char*[])
{
    read_symmetric_filter();
    write_symmetric_filter();
    close_symmetric_filter();

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    wide_symmetric_filter();
#endif

    return hpx::util::report_errors();
}
