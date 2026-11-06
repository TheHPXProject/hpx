//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Contains implementations of get, read, put, write and seek which
// check a device's mode at runtime instead of compile time.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/dispatch.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/read.hpp>
#include <hpx/iostream/seek.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/iostream/write.hpp>

#include <ios>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct read_write_if_impl;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct seek_if_impl;
    }    // End namespace detail.

    HPX_CXX_CORE_EXPORT template <typename T>
    int_type_of_t<T> get_if(T& t)
    {
        using tag = detail::dispatch_t<T, input, output>;
        return detail::read_write_if_impl<tag>::get(t);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    std::streamsize read_if(T& t, char_type_of_t<T>* s, std::streamsize n)
    {
        using tag = detail::dispatch_t<T, input, output>;
        return detail::read_write_if_impl<tag>::read(t, s, n);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    bool put_if(T& t, char_type_of_t<T> c)
    {
        using tag = detail::dispatch_t<T, input, output>;
        return detail::read_write_if_impl<tag>::put(t, c);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    std::streamsize write_if(
        T& t, char_type_of_t<T> const* s, std::streamsize n)
    {
        using tag = detail::dispatch_t<T, input, output>;
        return detail::read_write_if_impl<tag>::write(t, s, n);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    std::streampos seek_if(T& t, stream_offset off, std::ios_base::seekdir way,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        using tag = detail::dispatch_t<T, detail::random_access, any_tag>;
        return detail::seek_if_impl<tag>::seek(t, off, way, which);
    }

    namespace detail {

        //------------------Specializations of read_write_if_impl---------------------//
        template <>
        struct read_write_if_impl<input>
        {
            template <typename T>
            static int_type_of_t<T> get(T& t)
            {
                return iostream::get(t);
            }

            template <typename T>
            static std::streamsize read(
                T& t, char_type_of_t<T>* s, std::streamsize n)
            {
                return iostream::read(t, s, n);
            }

            template <typename T>
            static bool put(T&, char_type_of_t<T>)
            {
                throw cant_write();
            }

            template <typename T>
            static std::streamsize write(
                T&, char_type_of_t<T> const*, std::streamsize)
            {
                throw cant_write();
            }
        };

        template <>
        struct read_write_if_impl<output>
        {
            template <typename T>
            static int_type_of_t<T> get(T&)
            {
                throw cant_read();
            }

            template <typename T>
            static std::streamsize read(T&, char_type_of_t<T>*, std::streamsize)
            {
                throw cant_read();
            }

            template <typename T>
            static bool put(T& t, char_type_of_t<T> c)
            {
                return iostream::put(t, c);
            }

            template <typename T>
            static std::streamsize write(
                T& t, char_type_of_t<T> const* s, std::streamsize n)
            {
                return iostream::write(t, s, n);
            }
        };

        //------------------Specializations of seek_if_impl---------------------------//
        template <>
        struct seek_if_impl<random_access>
        {
            template <typename T>
            static std::streampos seek(T& t, stream_offset off,
                std::ios_base::seekdir way, std::ios_base::openmode which)
            {
                return iostream::seek(t, off, way, which);
            }
        };

        template <>
        struct seek_if_impl<any_tag>
        {
            template <typename T>
            static std::streampos seek(T&, stream_offset,
                std::ios_base::seekdir, std::ios_base::openmode)
            {
                throw cant_seek();
            }
        };
    }    // namespace detail
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
