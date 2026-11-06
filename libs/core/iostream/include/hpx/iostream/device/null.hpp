//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Inspired by Daryle Walker's nullbuf from his More I/O submission.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/config/defines.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/positioning.hpp>

#include <ios>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename Ch, typename Mode>
    class basic_null_device
    {
    public:
        using char_type = Ch;

        struct category
          : Mode
          , device_tag
          , closable_tag
        {
        };

        static constexpr std::streamsize read(Ch*, std::streamsize) noexcept
        {
            return -1;
        }

        static constexpr std::streamsize write(
            Ch const*, std::streamsize n) noexcept
        {
            return n;
        }

        static std::streampos seek(stream_offset, std::ios_base::seekdir,
            std::ios_base::openmode = std::ios_base::in |
                std::ios_base::out) noexcept
        {
            return -1;
        }

        static constexpr void close() noexcept {}
        static constexpr void close(std::ios_base::openmode) noexcept {}
    };

    HPX_CXX_CORE_EXPORT template <typename Ch>
    struct basic_null_source : private basic_null_device<Ch, input>
    {
        using char_type = Ch;
        using category = source_tag;

        using basic_null_device<Ch, input>::read;
        using basic_null_device<Ch, input>::close;
    };

    HPX_CXX_CORE_EXPORT using null_source = basic_null_source<char>;

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT using wnull_source = basic_null_source<wchar_t>;
#endif

    HPX_CXX_CORE_EXPORT template <typename Ch>
    struct basic_null_sink : private basic_null_device<Ch, output>
    {
        using char_type = Ch;
        using category = sink_tag;

        using basic_null_device<Ch, output>::write;
        using basic_null_device<Ch, output>::close;
    };

    HPX_CXX_CORE_EXPORT using null_sink = basic_null_sink<char>;

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT using wnull_sink = basic_null_sink<wchar_t>;
#endif
}    // namespace hpx::iostream
