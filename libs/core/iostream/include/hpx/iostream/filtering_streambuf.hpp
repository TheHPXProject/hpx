//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/config/defines.hpp>
#include <hpx/iostream/chain.hpp>
#include <hpx/iostream/detail/access_control.hpp>
#include <hpx/iostream/detail/streambuf/chainbuf.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <exception>
#include <memory>
#include <streambuf>
#include <string>
#include <type_traits>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = char,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>,
        typename Access = public_t>
    class filtering_streambuf
      : public detail::chainbuf<chain<Mode, Ch, Tr, Alloc>, Mode, Access>
    {
    public:
        using char_type = Ch;

        struct category
          : Mode
          , closable_tag
          , streambuf_tag
        {
        };

        using traits_type = Tr;
        using int_type = traits_type::int_type;
        using off_type = traits_type::off_type;
        using pos_type = traits_type::pos_type;
        using mode = Mode;

        using chain_type = chain<Mode, Ch, Tr, Alloc>;

        filtering_streambuf() = default;

        template <typename CharType, typename TraitsType>
        explicit filtering_streambuf(
            std::basic_streambuf<CharType, TraitsType>& sb,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(sb), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_streambuf(
            std::basic_istream<CharType, TraitsType>& is,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            static_assert(!std::is_convertible_v<mode, output>,
                "!std::is_convertible_v<mode, output>");
            this->push_impl(
                detail::resolve<mode, Ch>(is), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_streambuf(
            ::std::basic_ostream<CharType, TraitsType>& os,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            static_assert(!std::is_convertible_v<mode, input>,
                "!std::is_convertible_v<mode, input>");
            this->push_impl(
                detail::resolve<mode, Ch>(os), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_streambuf(
            ::std::basic_iostream<CharType, TraitsType>& io,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(io), buffer_size, pback_size);
        }

        template <typename Iter>
        explicit filtering_streambuf(util::iterator_range<Iter> const& rng,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::range_adapter<mode, util::iterator_range<Iter>>(rng),
                buffer_size, pback_size);
        }

        template <typename Pipeline, typename Concept>
        explicit filtering_streambuf(pipeline<Pipeline, Concept> const& p)
        {
            p.push(*this);
        }

        template <typename T>
            requires(!is_std_io_v<T>)
        explicit filtering_streambuf(T const& t,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(t), buffer_size, pback_size);
        }

        ~filtering_streambuf() override
        {
            if (this->is_complete())
                this->pubsync();
        }

        filtering_streambuf(filtering_streambuf const&) = default;
        filtering_streambuf(filtering_streambuf&&) = default;
        filtering_streambuf& operator=(filtering_streambuf const&) = default;
        filtering_streambuf& operator=(filtering_streambuf&&) = default;
    };

    HPX_CXX_CORE_EXPORT using filtering_istreambuf = filtering_streambuf<input>;
    HPX_CXX_CORE_EXPORT using filtering_ostreambuf =
        filtering_streambuf<output>;

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = wchar_t,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>,
        typename Access = public_t>
    using filtering_wstreambuf =
        filtering_streambuf<Mode, Ch, Tr, Alloc, Access>;

    HPX_CXX_CORE_EXPORT using filtering_wistreambuf =
        filtering_wstreambuf<input>;
    HPX_CXX_CORE_EXPORT using filtering_wostreambuf =
        filtering_wstreambuf<output>;
#endif
}    // namespace hpx::iostream
