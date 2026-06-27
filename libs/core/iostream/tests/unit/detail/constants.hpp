//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Contains the definitions of several constants used by the test program.

#pragma once

#include <cstddef>
#include <cstring>
#include <iosfwd>

namespace hpx::iostream::test {

    inline constexpr std::ios_base::openmode in_mode =
        std::ios_base::in | std::ios_base::binary;
    inline constexpr std::ios_base::openmode out_mode =
        std::ios_base::out | std::ios_base::binary;

    // Chunk size for reading or writing in chunks.
    inline constexpr int chunk_size = 59;

    // Chunk size for reading or writing in chunks.
    inline constexpr int small_buffer_size = 23;

    // Number of times data is repeated in test files.
    inline constexpr std::size_t data_reps = 300;

    namespace detail {

        // Returns string which is used to generate test files.
        char const* data(char*) noexcept
        {
            constexpr char const* c =
                "!\"#$%&'()*+,-./0123456879:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\n";
            return c;
        }

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
        // Returns string which is used to generate test files.
        wchar_t const* data(wchar_t*) noexcept
        {
            constexpr wchar_t const* c =
                L"!\"#$%&'()*+,-./0123456879:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                L"[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\n";
            return c;
        }
#endif
    }    // namespace detail

    char const* narrow_data() noexcept
    {
        return detail::data((char*) nullptr);
    }

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    wchar_t const* wide_data() noexcept
    {
        return detail::data((wchar_t*) nullptr);
    }
#endif

    // Length of string returned by data().
    std::size_t data_length() noexcept
    {
        static std::size_t len = std::strlen(narrow_data());
        return len;
    }
}    // namespace hpx::iostream::test
