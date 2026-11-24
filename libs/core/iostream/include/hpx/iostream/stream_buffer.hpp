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
#include <hpx/iostream/detail/streambuf/direct_streambuf.hpp>
#include <hpx/iostream/detail/streambuf/indirect_streambuf.hpp>
#include <hpx/iostream/detail/wrap_unwrap.hpp>
#include <hpx/iostream/traits.hpp>

#include <concepts>
#include <functional>
#include <ios>
#include <memory>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Tr, typename Alloc,
            typename Mode>
        struct stream_buffer_traits
          : std::conditional<
                std::is_convertible_v<category_of_t<T>, direct_tag>,
                direct_streambuf<T, Tr>, indirect_streambuf<T, Tr, Alloc, Mode>>
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename Tr, typename Alloc,
        typename Mode>
    class stream_buffer
      : public detail::stream_buffer_traits<T, Tr, Alloc, Mode>::type
    {
        static_assert(std::is_convertible_v<category_of_t<T>, Mode>);

        using base_type =
            detail::stream_buffer_traits<T, Tr, Alloc, Mode>::type;

    public:
        using char_type = char_type_of_t<T>;

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

    public:
        stream_buffer() = default;

        ~stream_buffer()
        {
            try
            {
                if (this->is_open() && this->auto_close())
                    this->close();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
            }
        }

        stream_buffer(stream_buffer const&) = default;
        stream_buffer(stream_buffer&&) = default;
        stream_buffer& operator=(stream_buffer const&) = default;
        stream_buffer& operator=(stream_buffer&&) = default;

        explicit stream_buffer(T const& t,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(detail::wrap(t), buffer_size, pback_size);
        }

        explicit stream_buffer(T& t, std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(detail::wrap(t), buffer_size, pback_size);
        }

        explicit stream_buffer(std::reference_wrapper<T> const& ref,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(ref, buffer_size, pback_size);
        }

        void open(T const& t, std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(detail::wrap(t), buffer_size, pback_size);
        }
        void open(T& t, std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(detail::wrap(t), buffer_size, pback_size);
        }
        void open(std::reference_wrapper<T> const& ref,
            std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            this->open_impl(ref, buffer_size, pback_size);
        }

        template <typename U, typename... Us>
            requires(!std::same_as<U, T>)
        explicit stream_buffer(U&& u, Us&&... us)
        {
            this->open_impl(T(HPX_FORWARD(U, u), HPX_FORWARD(Us, us)...));
        }

        template <typename U, typename... Us>
            requires(!std::same_as<U, T>)
        void open(U&& u, Us&&... us)
        {
            this->open_impl(T(HPX_FORWARD(U, u), HPX_FORWARD(Us, us)...));
        }

        T& operator*()
        {
            return *this->component();
        }
        T* operator->()
        {
            return this->component();
        }

    private:
        template <typename Dev>
        void open_impl(Dev&& dev, std::streamsize const buffer_size = -1,
            std::streamsize const pback_size = -1)
        {
            if (this->is_open())
                throw std::ios::failure("already open");
            base_type::open(HPX_FORWARD(Dev, dev), buffer_size, pback_size);
        }
    };
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
