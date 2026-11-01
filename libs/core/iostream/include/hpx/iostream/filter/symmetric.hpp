//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Contains the definitions of the class templates symmetric_filter, which
// models DualUseFilter based on a model of the Symmetric Filter.

//
// Roughly, a Symmetric Filter is a class type with the following interface:
//
//   struct symmetric_filter {
//       using char_type = xxx;
//
//       bool filter(const char*& begin_in, const char* end_in,
//                   char*& begin_out, char* end_out, bool flush)
//       {
//          // Consume as many characters as possible from the interval
//          // [begin_in, end_in), without exhausting the output range
//          // [begin_out, end_out). If flush is true, write as mush output
//          // as possible.
//          // A return value of true indicates that filter should be called
//          // again. More precisely, if flush is false, a return value of
//          // false indicates that the natural end of stream has been reached
//          // and that all filtered data has been forwarded; if flush is
//          // true, a return value of false indicates that all filtered data
//          // has been forwarded.
//       }
//       void close() { /* Reset filter's state. */ }
//   };
//
// Symmetric Filter filters need not be CopyConstructable.
//

#pragma once

#include <hpx/assert.hpp>

#include <hpx/config.hpp>
#include <hpx/iostream/char_traits.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/detail/buffer.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <cstdint>
#include <ios>
#include <memory>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename SymmetricFilter,
        typename Alloc = std::allocator<char_type_of_t<SymmetricFilter>>>
    class symmetric_filter
    {
    public:
        using char_type = char_type_of_t<SymmetricFilter>;
        using traits_type = iostream::char_traits<char_type>;
        using string_type = std::basic_string<char_type, traits_type, Alloc>;

        struct category
          : dual_use
          , filter_tag
          , multichar_tag
          , closable_tag
        {
        };

        template <typename... Ts>
        explicit symmetric_filter(std::streamsize buffer_size, Ts&&... ts)
          : pimpl_(new impl(buffer_size, HPX_FORWARD(Ts, ts)...))
        {
            HPX_ASSERT(buffer_size > 0);
        }

        template <typename Source>
        std::streamsize read(Source& src, char_type* s, std::streamsize n)
        {
            if (!(state() & flags::read))
                begin_read();

            buffer_type& buf = pimpl_->buf_;
            int status = static_cast<int>(
                (state() & flags::eof) != 0 ? flags::eof : flags::good);
            char_type *next_s = s, *end_s = s + n;
            while (true)
            {
                // Invoke filter if there are unconsumed characters in buffer or
                // if filter must be flushed.
                bool flush = status == static_cast<int>(flags::eof);
                if (buf.ptr() != buf.eptr() || flush)
                {
                    char_type const* next = buf.ptr();
                    bool done = !filter().filter(
                        next, buf.eptr(), next_s, end_s, flush);
                    buf.ptr() = buf.data() + (next - buf.data());
                    if (done)
                    {
                        return detail::check_eof(
                            static_cast<std::streamsize>(next_s - s));
                    }
                }

                // If no more characters are available without blocking, or if
                // read request has been satisfied, return.
                if ((status == static_cast<int>(flags::would_block) &&
                        buf.ptr() == buf.eptr()) ||
                    next_s == end_s)
                {
                    return static_cast<std::streamsize>(next_s - s);
                }

                // Fill buffer.
                if (status == static_cast<int>(flags::good))
                    status = fill(src);
            }
        }

        template <typename Sink>
        std::streamsize write(Sink& snk, char_type const* s, std::streamsize n)
        {
            if (!(state() & flags::write))
                begin_write();

            buffer_type& buf = pimpl_->buf_;
            char_type const *next_s, *end_s;
            for (next_s = s, end_s = s + n; next_s != end_s;)
            {
                if (buf.ptr() == buf.eptr() && !flush(snk))
                    break;
                if (!filter().filter(
                        next_s, end_s, buf.ptr(), buf.eptr(), false))
                {
                    flush(snk);
                    break;
                }
            }
            return static_cast<std::streamsize>(next_s - s);
        }

        template <typename Sink>
        void close(Sink& snk, std::ios_base::openmode mode)
        {
            if (mode == std::ios_base::out)
            {
                if (!(state() & flags::write))
                    begin_write();

                // Repeatedly invoke filter() with no input.
                try
                {
                    buffer_type& buf = pimpl_->buf_;
                    char_type dummy;
                    char_type const* end = &dummy;
                    bool again = true;
                    while (again)
                    {
                        if (buf.ptr() != buf.eptr())
                            again = filter().filter(
                                end, end, buf.ptr(), buf.eptr(), true);
                        flush(snk);
                    }
                }
                catch (...)
                {
                    try
                    {
                        close_impl();
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                    }
                    throw;
                }
                close_impl();
            }
            else
            {
                close_impl();
            }
        }

        SymmetricFilter& filter()
        {
            return *pimpl_;
        }

        string_type unconsumed_input() const;

    private:
        using buffer_type = detail::buffer<char_type, Alloc>;

    private:
        buffer_type& buf()
        {
            return pimpl_->buf_;
        }
        buffer_type const& buf() const
        {
            return pimpl_->buf_;
        }

        int& state()
        {
            return pimpl_->state_;
        }

        void begin_read();
        void begin_write();

        template <typename Source>
        int fill(Source& src)
        {
            std::streamsize amt =
                iostream::read(src, buf().data(), buf().size());
            if (amt == -1)
            {
                state() = state() | flags::eof;
                return static_cast<int>(flags::eof);
            }
            buf().set(0, amt);
            return static_cast<int>(
                amt != 0 ? flags::good : flags::would_block);
        }

        // Attempts to write the contents of the buffer the given Sink. Returns
        // true if at least on character was written.
        template <typename Sink>
        bool flush(Sink& snk)
        {
            if constexpr (std::is_convertible_v<category_of_t<Sink>, output>)
            {
                std::streamsize amt =
                    static_cast<std::streamsize>(buf().ptr() - buf().data());
                std::streamsize result =
                    hpx::iostream::write(snk, buf().data(), amt);
                if (result < amt && result > 0)
                    traits_type::move(
                        buf().data(), buf().data() + result, amt - result);
                buf().set(amt - result, buf().size());
                return result != 0;
            }
            else
            {
                return true;
            }
        }

        void close_impl();

        enum class flags : std::uint8_t
        {
            read = 1,
            write = flags::read << 1,
            eof = flags::write << 1,
            good = 8,
            would_block = 16
        };

        friend constexpr int operator&(int const lhs, flags rhs) noexcept
        {
            return lhs & static_cast<int>(rhs);
        }

        friend constexpr int operator|(flags lhs, flags rhs) noexcept
        {
            return static_cast<int>(lhs) | static_cast<int>(rhs);
        }

        friend constexpr int operator|(int const lhs, flags rhs) noexcept
        {
            return lhs | static_cast<int>(rhs);
        }

        friend constexpr int operator~(flags rhs) noexcept
        {
            return ~static_cast<int>(rhs);
        }

        struct impl : SymmetricFilter
        {
            template <typename... Ts>
            explicit impl(std::streamsize buffer_size, Ts&&... ts)
              : SymmetricFilter(HPX_FORWARD(Ts, ts)...)
              , buf_(buffer_size)
              , state_(0)
            {
            }

            buffer_type buf_;
            int state_;
        };

        std::shared_ptr<impl> pimpl_;
    };

    //------------------Implementation of symmetric_filter----------------//
    template <typename SymmetricFilter, typename Alloc>
    void symmetric_filter<SymmetricFilter, Alloc>::begin_read()
    {
        HPX_ASSERT(!(state() & flags::write));
        state() = state() | flags::read;
        buf().set(0, 0);
    }

    template <typename SymmetricFilter, typename Alloc>
    void symmetric_filter<SymmetricFilter, Alloc>::begin_write()
    {
        HPX_ASSERT(!(state() & flags::read));
        state() = state() | flags::write;
        buf().set(0, buf().size());
    }

    template <typename SymmetricFilter, typename Alloc>
    void symmetric_filter<SymmetricFilter, Alloc>::close_impl()
    {
        state() = 0;
        buf().set(0, 0);
        filter().close();
    }

    template <typename SymmetricFilter, typename Alloc>
    symmetric_filter<SymmetricFilter, Alloc>::string_type
    symmetric_filter<SymmetricFilter, Alloc>::unconsumed_input() const
    {
        return string_type(buf().ptr(), buf().eptr());
    }
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
