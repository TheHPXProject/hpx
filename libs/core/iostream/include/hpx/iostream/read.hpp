//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/char_traits.hpp>
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
        struct read_device_impl;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct read_filter_impl;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    int_type_of_t<T> get(T& t)
    {
        return detail::read_device_impl<T>::get(util::unwrap_ref(t));
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    std::streamsize read(T& t, char_type_of_t<T>* s, std::streamsize n)
    {
        return detail::read_device_impl<T>::read(util::unwrap_ref(t), s, n);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Source>
    std::streamsize read(
        T& t, Source& src, char_type_of_t<T>* s, std::streamsize n)
    {
        return detail::read_filter_impl<T>::read(
            util::unwrap_ref(t), src, s, n);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    bool putback(T& t, char_type_of_t<T> c)
    {
        return detail::read_device_impl<T>::putback(util::unwrap_ref(t), c);
    }

    //----------------------------------------------------------------------------//
    namespace detail {

        // Helper function for adding -1 as EOF indicator.
        HPX_CXX_CORE_EXPORT constexpr std::streamsize check_eof(
            std::streamsize n) noexcept
        {
            return n != 0 ? n : -1;
        }

        HPX_CXX_CORE_EXPORT template <typename T>
        bool true_eof([[maybe_unused]] T& t) noexcept
        {
            if constexpr (is_linked_v<T>)
            {
                return t.true_eof();
            }
            else
            {
                return true;
            }
        }

        //------------------Definition of read_device_impl----------------------------//
        HPX_CXX_CORE_EXPORT template <typename T>
        struct read_device_impl
          : std::conditional_t<is_custom_v<T>, operations<T>,
                read_device_impl<
                    dispatch_t<T, istream_tag, streambuf_tag, input>>>
        {
        };

        template <>
        struct read_device_impl<istream_tag>
        {
            template <typename T>
            static int_type_of<T>::type get(T& t)
            {
                return t.get();
            }

            template <typename T>
            static std::streamsize read(
                T& t, char_type_of<T>::type* s, std::streamsize n)
            {
                return check_eof(t.rdbuf()->sgetn(s, n));
            }

            template <typename T>
            static bool putback(T& t, char_type_of<T>::type c)
            {
                using char_type = char_type_of<T>::type;
                using traits_type = std::char_traits<char_type>;

                return !traits_type::eq_int_type(
                    t.rdbuf()->sputbackc(c), traits_type::eof());
            }
        };

        template <>
        struct read_device_impl<streambuf_tag>
        {
            template <typename T>
            static int_type_of_t<T> get(T& t)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = iostream::char_traits<char_type>;

                int_type_of_t<T> c = t.sbumpc();
                if (!traits_type::is_eof(c) || detail::true_eof(t))
                    return c;
                return traits_type::would_block();
            }

            template <typename T>
            static std::streamsize read(
                T& t, char_type_of<T>::type* s, std::streamsize n)
            {
                if (std::streamsize amt = t.sgetn(s, n); amt != 0)
                    return amt;
                if (detail::true_eof(t))
                    return -1;
                return 0;
            }

            template <typename T>
            static bool putback(T& t, char_type_of_t<T> c)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = iostream::char_traits<char_type>;

                return !traits_type::is_eof(t.sputbackc(c));
            }
        };

        template <>
        struct read_device_impl<input>
        {
            template <typename T>
            static int_type_of<T>::type get(T& t)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = iostream::char_traits<char_type>;

                char_type c;
                std::streamsize const amt = t.read(&c, 1);
                if (amt == 1)
                    return traits_type::to_int_type(c);
                if (amt == -1)
                    return traits_type::eof();
                return traits_type::would_block();
            }

            template <typename T>
            static std::streamsize read(
                T& t, char_type_of<T>::type* s, std::streamsize n)
            {
                return t.read(s, n);
            }

            template <typename T>
            static bool putback(T& t, char_type_of<T>::type c)
            {
                return t.putback(c);
            }
        };

        //------------------Definition of read_filter_impl----------------------------//
        HPX_CXX_CORE_EXPORT template <typename T>
        struct read_filter_impl
          : std::conditional_t<is_custom_v<T>, operations<T>,
                read_filter_impl<dispatch_t<T, multichar_tag, any_tag>>>
        {
        };

        template <>
        struct read_filter_impl<multichar_tag>
        {
            template <typename T, typename Source>
            static std::streamsize read(
                T& t, Source& src, char_type_of<T>::type* s, std::streamsize n)
            {
                return t.read(src, s, n);
            }
        };

        template <>
        struct read_filter_impl<any_tag>
        {
            template <typename T, typename Source>
            static std::streamsize read(
                T& t, Source& src, char_type_of_t<T>* s, std::streamsize n)
            {
                using char_type = char_type_of_t<T>;
                using traits_type = iostream::char_traits<char_type>;

                for (std::streamsize off = 0; off < n; ++off)
                {
                    typename traits_type::int_type c = t.get(src);
                    if (traits_type::is_eof(c))
                        return check_eof(off);
                    if (traits_type::would_block(c))
                        return off;
                    s[off] = traits_type::to_char_type(c);
                }
                return n;
            }
        };
    }    // namespace detail
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
