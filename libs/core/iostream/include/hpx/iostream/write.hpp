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
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/dispatch.hpp>
#include <hpx/iostream/operations_fwd.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/type_support.hpp>

#include <ios>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct write_device_impl;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct write_filter_impl;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    bool put(T& t, char_type_of_t<T> c)
    {
        return detail::write_device_impl<T>::put(util::unwrap_ref(t), c);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    std::streamsize write(T& t, char_type_of_t<T> const* s, std::streamsize n)
    {
        return detail::write_device_impl<T>::write(util::unwrap_ref(t), s, n);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Sink>
    std::streamsize write(
        T& t, Sink& snk, char_type_of_t<T> const* s, std::streamsize n)
    {
        return detail::write_filter_impl<T>::write(
            util::unwrap_ref(t), snk, s, n);
    }

    namespace detail {

        //------------------Definition of write_device_impl---------------------------//
        HPX_CXX_CORE_EXPORT template <typename T>
        struct write_device_impl
          : std::conditional_t<is_custom_v<T>, operations<T>,
                write_device_impl<
                    dispatch_t<T, ostream_tag, streambuf_tag, output>>>
        {
        };

        template <>
        struct write_device_impl<ostream_tag>
        {
            template <typename T>
            static bool put(T& t, char_type_of_t<T> c)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = std::char_traits<char_type>;

                return !traits_type::eq_int_type(
                    t.rdbuf()->sputc(c), traits_type::eof());
            }

            template <typename T>
            static std::streamsize write(
                T& t, char_type_of_t<T> const* s, std::streamsize n)
            {
                return t.rdbuf()->sputn(s, n);
            }
        };

        template <>
        struct write_device_impl<streambuf_tag>
        {
            template <typename T>
            static bool put(T& t, char_type_of_t<T> c)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = std::char_traits<char_type>;

                return !traits_type::eq_int_type(
                    t.sputc(c), traits_type::eof());
            }

            template <typename T>
            static std::streamsize write(
                T& t, char_type_of_t<T> const* s, std::streamsize n)
            {
                return t.sputn(s, n);
            }
        };

        template <>
        struct write_device_impl<output>
        {
            template <typename T>
            static bool put(T& t, char_type_of_t<T> c)
            {
                return t.write(&c, 1) == 1;
            }

            template <typename T>
            static std::streamsize write(
                T& t, char_type_of_t<T> const* s, std::streamsize n)
            {
                return t.write(s, n);
            }
        };

        //------------------Definition of write_filter_impl---------------------------//
        HPX_CXX_CORE_EXPORT template <typename T>
        struct write_filter_impl
          : std::conditional_t<is_custom_v<T>, operations<T>,
                write_filter_impl<dispatch_t<T, multichar_tag, any_tag>>>
        {
        };

        template <>
        struct write_filter_impl<multichar_tag>
        {
            template <typename T, typename Sink>
            static std::streamsize write(
                T& t, Sink& snk, char_type_of_t<T> const* s, std::streamsize n)
            {
                return t.write(snk, s, n);
            }
        };

        template <>
        struct write_filter_impl<any_tag>
        {
            template <typename T, typename Sink>
            static std::streamsize write(
                T& t, Sink& snk, char_type_of_t<T> const* s, std::streamsize n)
            {
                for (std::streamsize off = 0; off < n; ++off)
                {
                    if (!t.put(snk, s[off]))
                        return off;
                }
                return n;
            }
        };
    }    // namespace detail
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
