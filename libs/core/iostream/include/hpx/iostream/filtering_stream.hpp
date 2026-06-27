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

#include <hpx/config.hpp>
#include <hpx/iostream/config/defines.hpp>
#include <hpx/iostream/detail/access_control.hpp>
#include <hpx/iostream/filtering_streambuf.hpp>
#include <hpx/iostream/pipeline.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <memory>
#include <streambuf>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    //--------------Definition of filtered_istream--------------------------------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr>
        struct filtering_stream_traits
        {
            // clang-format off
            using stream_type =
                util::select_t<
                    std::conjunction<std::is_convertible<Mode, input>,
                        std::is_convertible<Mode, output>>,
                        std::basic_iostream<Ch, Tr>,
                    std::is_convertible<Mode, input>,
                        std::basic_istream<Ch, Tr>,
                    util::else_t,
                        std::basic_ostream<Ch, Tr>>;

            using stream_tag =
                util::select_t<
                    std::conjunction<std::is_convertible<Mode, input>,
                        std::is_convertible<Mode, output>>,
                        iostream_tag,
                    std::is_convertible<Mode, input>,
                        istream_tag,
                    util::else_t,
                        ostream_tag>;
            // clang-format on
        };

        HPX_CXX_CORE_EXPORT template <typename Chain, typename Access>
        class filtering_stream_base
          : public access_control<chain_client<Chain>, Access>
          , public filtering_stream_traits<typename Chain::mode,
                typename Chain::char_type,
                typename Chain::traits_type>::stream_type
        {
        public:
            using chain_type = Chain;
            using client_type = access_control<chain_client<Chain>, Access>;

        protected:
            using stream_type = filtering_stream_traits<typename Chain::mode,
                typename Chain::char_type,
                typename Chain::traits_type>::stream_type;

            filtering_stream_base()
              : stream_type(nullptr)
            {
                this->set_chain(&chain_);
            }

        private:
            void notify()
            {
                this->rdbuf(chain_.empty() ? nullptr : &chain_.front());
            }

            Chain chain_;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = char,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>,
        typename Access = public_t>
    class filtering_stream
      : public detail::filtering_stream_base<chain<Mode, Ch, Tr, Alloc>, Access>
    {
    public:
        using char_type = Ch;

        struct category
          : Mode
          , closable_tag
          , detail::filtering_stream_traits<Mode, Ch, Tr>::stream_tag
        {
        };

        using traits_type = Tr;
        using int_type = traits_type::int_type;
        using off_type = traits_type::off_type;
        using pos_type = traits_type::pos_type;
        using mode = Mode;
        using chain_type = chain<Mode, Ch, Tr, Alloc>;

        filtering_stream() = default;

        template <typename CharType, typename TraitsType>
        explicit filtering_stream(
            std::basic_streambuf<CharType, TraitsType>& sb,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(sb), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_stream(std::basic_istream<CharType, TraitsType>& is,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            static_assert(!std::is_convertible_v<mode, output>,
                "std::is_convertible_v<mode, output>");
            this->push_impl(
                detail::resolve<mode, Ch>(is), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_stream(std::basic_ostream<CharType, TraitsType>& os,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            static_assert(!std::is_convertible_v<mode, input>,
                "!std::is_convertible_v<mode, input");
            this->push_impl(
                detail::resolve<mode, Ch>(os), buffer_size, pback_size);
        }

        template <typename CharType, typename TraitsType>
        explicit filtering_stream(std::basic_iostream<CharType, TraitsType>& io,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(io), buffer_size, pback_size);
        }

        template <typename Iter>
        explicit filtering_stream(util::iterator_range<Iter> const& rng,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::range_adapter<mode, util::iterator_range<Iter>>(rng),
                buffer_size, pback_size);
        }

        template <typename Pipeline, typename Concept>
        explicit filtering_stream(pipeline<Pipeline, Concept> const& p)
        {
            p.push(*this);
        }

        template <typename T>
            requires(!is_std_io_v<T>)
        explicit filtering_stream(T const& t,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->push_impl(
                detail::resolve<mode, Ch>(t), buffer_size, pback_size);
        }

        ~filtering_stream()
        {
            if (this->is_complete())
                this->rdbuf()->pubsync();
        }

        filtering_stream(filtering_stream const&) = delete;
        filtering_stream(filtering_stream&&) = default;
        filtering_stream& operator=(filtering_stream const&) = delete;
        filtering_stream& operator=(filtering_stream&&) = default;

    private:
        using client_type =
            access_control<detail::chain_client<chain<Mode, Ch, Tr, Alloc>>,
                Access>;

        template <typename T>
        void push_impl(T const& t, std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            client_type::push(t, buffer_size, pback_size);
        }
    };

    HPX_CXX_CORE_EXPORT using filtering_istream = filtering_stream<input>;
    HPX_CXX_CORE_EXPORT using filtering_ostream = filtering_stream<output>;

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch = wchar_t,
        typename Tr = std::char_traits<Ch>, typename Alloc = std::allocator<Ch>,
        typename Access = public_t>
    using wfiltering_stream = filtering_stream<Mode, Ch, Tr, Alloc, Access>;

    HPX_CXX_CORE_EXPORT using filtering_wistream = wfiltering_stream<input>;
    HPX_CXX_CORE_EXPORT using filtering_wostream = wfiltering_stream<output>;
#endif
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
