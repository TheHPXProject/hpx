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

#include <hpx/config.hpp>
#include <hpx/modules/iostream.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <ios>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::test {

    template <typename Mode>
    struct toupper_filter;

    template <>
    struct toupper_filter<input> : input_filter
    {
        template <typename Source>
        int get(Source& s)
        {
            int c = hpx::iostream::get(s);
            return c != EOF && c != WOULD_BLOCK ?
                std::toupper((unsigned char) c) :
                c;
        }
    };

    template <>
    struct toupper_filter<output> : output_filter
    {
        template <typename Sink>
        bool put(Sink& s, char c)
        {
            return hpx::iostream::put(
                s, (char) std::toupper((unsigned char) c));
        }
    };

    template <typename Mode>
    struct tolower_filter;

    template <>
    struct tolower_filter<input> : input_filter
    {
        template <typename Source>
        int get(Source& s)
        {
            int c = hpx::iostream::get(s);
            return c != EOF && c != WOULD_BLOCK ?
                std::toupper((unsigned char) c) :
                c;
        }
    };

    template <>
    struct tolower_filter<output> : output_filter
    {
        template <typename Sink>
        bool put(Sink& s, char c)
        {
            return hpx::iostream::put(
                s, (char) std::tolower((unsigned char) c));
        }
    };

    template <typename Mode>
    struct toupper_multichar_filter;

    template <>
    struct toupper_multichar_filter<input> : multichar_input_filter
    {
        template <typename Source>
        std::streamsize read(Source& s, char* buf, std::streamsize n)
        {
            std::streamsize result = hpx::iostream::read(s, buf, n);
            if (result == -1)
                return -1;
            for (int z = 0; z < result; ++z)
                buf[z] = (char) std::toupper((unsigned char) buf[z]);
            return result;
        }
    };

    template <>
    struct toupper_multichar_filter<output> : multichar_output_filter
    {
        template <typename Sink>
        std::streamsize write(Sink& s, char const* buf, std::streamsize n)
        {
            std::streamsize result;
            for (result = 0; result < n; ++result)
            {
                char c = (char) std::toupper((unsigned char) buf[result]);
                if (!hpx::iostream::put(s, c))
                    break;
            }
            return result;
        }
    };

    template <typename Mode>
    struct tolower_multichar_filter;

    template <>
    struct tolower_multichar_filter<input> : multichar_input_filter
    {
        template <typename Source>
        std::streamsize read(Source& s, char* buf, std::streamsize n)
        {
            std::streamsize result = hpx::iostream::read(s, buf, n);
            if (result == -1)
                return -1;
            for (int z = 0; z < result; ++z)
                buf[z] = (char) std::tolower((unsigned char) buf[z]);
            return result;
        }
    };

    template <>
    struct tolower_multichar_filter<output> : multichar_output_filter
    {
        template <typename Sink>
        std::streamsize write(Sink& s, char const* buf, std::streamsize n)
        {
            std::streamsize result;
            for (result = 0; result < n; ++result)
            {
                char c = (char) std::tolower((unsigned char) buf[result]);
                if (!hpx::iostream::put(s, c))
                    break;
            }
            return result;
        }
    };

    struct padding_filter : dual_use_filter
    {
        explicit padding_filter(char pad_char) noexcept
          : pad_char_(pad_char)
          , use_pad_char_(false)
          , eof_(false)
        {
        }

        template <typename Source>
        int get(Source& src)
        {
            int result;
            if (use_pad_char_)
            {
                result = eof_ ? EOF : pad_char_;
                use_pad_char_ = false;
            }
            else
            {
                result = hpx::iostream::get(src);
                if (result != EOF && result != WOULD_BLOCK)
                    use_pad_char_ = true;
                eof_ = result == EOF;
            }
            return result;
        }

        template <typename Sink>
        bool put(Sink& s, char c)
        {
            if (use_pad_char_)
            {
                if (!hpx::iostream::put(s, pad_char_))
                    return false;
                use_pad_char_ = false;
            }
            if (!hpx::iostream::put(s, c))
                return false;
            if (!hpx::iostream::put(s, pad_char_))
                use_pad_char_ = true;
            return true;
        }

        char pad_char_;
        bool use_pad_char_;
        bool eof_;
    };

    struct flushable_output_filter
    {
        using char_type = char;

        struct category
          : output_filter_tag
          , flushable_tag
        {
        };

        template <typename Sink>
        bool put(Sink&, char c)
        {
            buf_.push_back(c);
            return true;
        }

        template <typename Sink>
        bool flush(Sink& s)
        {
            if (!buf_.empty())
            {
                hpx::iostream::write(
                    s, &buf_[0], (std::streamsize) buf_.size());
                buf_.clear();
            }
            return true;
        }

        std::vector<char> buf_;
    };

    struct identity_seekable_filter : filter<seekable>
    {
        template <typename Source>
        int get(Source& s)
        {
            return hpx::iostream::get(s);
        }

        template <typename Sink>
        bool put(Sink& s, char c)
        {
            return hpx::iostream::put(s, c);
        }

        template <typename Device>
        std::streampos seek(
            Device& d, stream_offset off, std::ios_base::seekdir way)
        {
            return hpx::iostream::seek(d, off, way);
        }
    };

    struct identity_seekable_multichar_filter : multichar_filter<seekable>
    {
        template <typename Source>
        std::streamsize read(Source& s, char* buf, std::streamsize n)
        {
            return hpx::iostream::read(s, buf, n);
        }
        template <typename Sink>
        std::streamsize write(Sink& s, char const* buf, std::streamsize n)
        {
            return hpx::iostream::write(s, buf, n);
        }
        template <typename Device>
        std::streampos seek(
            Device& d, stream_offset off, std::ios_base::seekdir way)
        {
            return hpx::iostream::seek(d, off, way);
        }
    };
}    // namespace hpx::iostream::test

#include <hpx/config/warnings_suffix.hpp>
