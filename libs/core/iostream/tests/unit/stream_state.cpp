//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright Frank Birbacher 2007
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include <ios>

using hpx::iostream::seekable_device_tag;
using hpx::iostream::stream;
using hpx::iostream::stream_offset;
using hpx::iostream::detail::bad_read;
using hpx::iostream::detail::bad_seek;
using hpx::iostream::detail::bad_write;

// This test unit uses a custom device to trigger errors. The device supports
// input, output, and seek according to the SeekableDevice concept. And each
// of the required functions throw a special detail::bad_xxx exception. This
// should trigger the iostream::stream to set the badbit status flag.
// Additionally the exception can be propagated to the caller if the exception
// mask of the stream allows exceptions.
//
// The stream offers four different functions: read, write, seekg, and seekp.
// Each of them is tested with three different error reporting concepts:
// test by reading status flags, test by propagated exception, and test by
// calling std::ios_base::exceptions when badbit is already set.
//
// In each case all of the status checking functions of a stream are checked.
//
// MSVCPRT (Visual Studio 2017, at least) does not perform exception
// handling in the seek methods (confirmed by inspecting sources).

//------------------Definition of error_device--------------------------------//

// Device whose member functions throw
struct error_device
{
    using char_type = char;
    using category = seekable_device_tag;

    explicit error_device(char const*) {}

    std::streamsize read(char_type*, std::streamsize)
    {
        throw bad_read();
    }

    std::streamsize write(char_type const*, std::streamsize)
    {
        throw bad_write();
    }

    std::streampos seek(stream_offset, std::ios_base::seekdir)
    {
        throw bad_seek();
    }
};

using test_stream = stream<error_device>;

//------------------Stream state tester---------------------------------------//
void check_stream_for_badbit(std::iostream const& str)
{
    HPX_TEST_MSG(!str.good(), "stream still good");
    HPX_TEST_MSG(!str.eof(), "eofbit set but not expected");
    HPX_TEST_MSG(str.bad(), "stream did not set badbit");
    HPX_TEST_MSG(str.fail(), "stream did not fail");
    HPX_TEST_MSG(
        str.operator!(), "stream does not report failure by operator !");
    HPX_TEST_MSG(false == static_cast<bool>(str),
        "stream does not report failure by operator void* or bool");
}

//------------------Test case generators--------------------------------------//
template <void (*const function)(std::iostream&)>
struct wrap_nothrow
{
    static void execute()
    {
        test_stream stream("foo");
        HPX_TEST_NO_THROW(function(stream));
        check_stream_for_badbit(stream);
    }
};

template <void (*const function)(std::iostream&)>
struct wrap_throw
{
    static void execute()
    {
        typedef std::ios_base ios;
        test_stream stream("foo");

        stream.exceptions(ios::failbit | ios::badbit);
        HPX_TEST_THROW(function(stream), std::exception);

        check_stream_for_badbit(stream);
    }
};

template <void (*const function)(std::iostream&)>
struct wrap_throw_delayed
{
    static void execute()
    {
        typedef std::ios_base ios;
        test_stream stream("foo");

        function(stream);
        HPX_TEST_THROW(
            stream.exceptions(ios::failbit | ios::badbit), ios::failure);

        check_stream_for_badbit(stream);
    }
};

//------------------Stream operations that throw------------------------------//
void test_read(std::iostream& str)
{
    char data[10];
    str.read(data, 10);
}

void test_write(std::iostream& str)
{
    char data[10] = {0};
    str.write(data, 10);
    //force use of streambuf
    str.flush();
}

void test_seekg(std::iostream& str)
{
    str.seekg(10);
}

void test_seekp(std::iostream& str)
{
    str.seekp(10);
}

int main(int, char*[])
{
    wrap_nothrow<&test_read>::execute();
    wrap_throw<&test_read>::execute();
    wrap_throw_delayed<&test_read>::execute();

    wrap_nothrow<&test_write>::execute();
    wrap_throw<&test_write>::execute();
    wrap_throw_delayed<&test_write>::execute();

    return hpx::util::report_errors();
}
