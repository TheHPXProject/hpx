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
#include <hpx/iostream/concepts.hpp>
#include <hpx/iostream/detail/dispatch.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/device/null.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <functional>
#include <ios>
#include <memory>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    HPX_CXX_CORE_EXPORT template <typename Category>
    struct device_wrapper_impl;
    HPX_CXX_CORE_EXPORT template <typename Category>
    struct flt_wrapper_impl;

    HPX_CXX_CORE_EXPORT template <typename T>
    class concept_adapter
    {
        using value_type = value_type<T>::type;

        using any_impl = std::conditional_t<is_device_v<T>,
            device_wrapper_impl<any_tag>, flt_wrapper_impl<any_tag>>;

    public:
        using char_type = char_type_of_t<T>;
        using category = category_of_t<T>;

        explicit concept_adapter(std::reference_wrapper<T> const& ref)
          : t_(ref.get())
        {
            static_assert(is_std_io_v<T>);
        }

        explicit concept_adapter(T const& t)
          : t_(t)
        {
            static_assert(!is_std_io_v<T>);
        }

        ~concept_adapter() = default;

        concept_adapter(concept_adapter const&) = delete;
        concept_adapter(concept_adapter&&) = delete;
        concept_adapter& operator=(concept_adapter const&) = delete;
        concept_adapter& operator=(concept_adapter&&) = delete;

        T& operator*()
        {
            return t_;
        }
        T* operator->()
        {
            return std::addressof(t_);
        }

        std::streamsize read(char_type* s, std::streamsize n)
        {
            return this->read(
                s, n, static_cast<basic_null_source<char_type>*>(nullptr));
        }

        template <typename Source>
        std::streamsize read(char_type* s, std::streamsize n, Source* src)
        {
            using input_tag = dispatch_t<T, input, output>;
            using input_impl = std::conditional_t<is_device_v<T>,
                device_wrapper_impl<input_tag>, flt_wrapper_impl<input_tag>>;
            return input_impl::read(t_, src, s, n);
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            return this->write(
                s, n, static_cast<basic_null_sink<char_type>*>(nullptr));
        }

        template <typename Sink>
        std::streamsize write(char_type const* s, std::streamsize n, Sink* snk)
        {
            using output_tag = dispatch_t<T, output, input>;
            using output_impl = std::conditional_t<is_device_v<T>,
                device_wrapper_impl<output_tag>, flt_wrapper_impl<output_tag>>;
            return output_impl::write(t_, snk, s, n);
        }

        std::streampos seek(stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which)
        {
            return this->seek(off, way, which,
                static_cast<basic_null_device<char_type, seekable>*>(nullptr));
        }

        template <typename Device>
        std::streampos seek(stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which, Device* dev)
        {
            if constexpr (std::is_convertible_v<category, input_seekable> ||
                std::is_convertible_v<category, output_seekable>)
            {
                return any_impl::seek(t_, dev, off, way, which);
            }
            else
            {
                throw cant_seek();
            }
        }

        void close(std::ios_base::openmode which)
        {
            this->close(which,
                static_cast<basic_null_device<char_type, seekable>*>(nullptr));
        }

        template <typename Device>
        void close(std::ios_base::openmode which, Device* dev)
        {
            any_impl::close(t_, dev, which);
        }

        template <typename Device>
        bool flush(Device* dev)
        {
            bool result = any_impl::flush(t_, dev);
            if (dev && dev->pubsync() == -1)
                result = false;
            return result;
        }

        template <typename Locale>    // Avoid dependency on <locale>
        void imbue(Locale const& loc)
        {
            if constexpr (std::is_convertible_v<category, localizable_tag>)
            {
                iostream::imbue(t_, loc);
            }
        }

        [[nodiscard]] constexpr std::streamsize optimal_buffer_size()
            const noexcept
        {
            return iostream::optimal_buffer_size(t_);
        }

    private:
        value_type t_;
    };

    //------------------Specializations of device_wrapper_impl--------------------//
    template <>
    struct device_wrapper_impl<any_tag>
    {
        template <typename Device, typename Dummy>
        static std::streampos seek(Device& dev, Dummy*, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode which)
        {
            using category = category_of_t<Device>;
            return seek(dev, off, way, which, category());
        }

        template <typename Device>
        static std::streampos seek(Device&, stream_offset,
            std::ios_base::seekdir, std::ios_base::openmode, any_tag)
        {
            throw cant_seek();
        }

        template <typename Device>
        static std::streampos seek(Device& dev, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode which,
            random_access)
        {
            return iostream::seek(dev, off, way, which);
        }

        template <typename Device, typename Dummy>
        static void close(Device& dev, Dummy*, std::ios_base::openmode which)
        {
            iostream::close(dev, which);
        }

        template <typename Device, typename Dummy>
        static bool flush(Device& dev, Dummy*)
        {
            return iostream::flush(dev);
        }
    };

    template <>
    struct device_wrapper_impl<input> : device_wrapper_impl<any_tag>
    {
        template <typename Device, typename Dummy>
        static std::streamsize read(
            Device& dev, Dummy*, char_type_of_t<Device>* s, std::streamsize n)
        {
            return iostream::read(dev, s, n);
        }

        template <typename Device, typename Dummy>
        static std::streamsize write(
            Device&, Dummy*, char_type_of_t<Device> const*, std::streamsize)
        {
            throw cant_write();
        }
    };

    template <>
    struct device_wrapper_impl<output>
    {
        template <typename Device, typename Dummy>
        static std::streamsize read(
            Device&, Dummy*, char_type_of_t<Device>*, std::streamsize)
        {
            throw cant_read();
        }

        template <typename Device, typename Dummy>
        static std::streamsize write(Device& dev, Dummy*,
            char_type_of_t<Device> const* s, std::streamsize n)
        {
            return iostream::write(dev, s, n);
        }
    };

    //------------------Specializations of flt_wrapper_impl--------------------//
    template <>
    struct flt_wrapper_impl<any_tag>
    {
        template <typename Filter, typename Device>
        static std::streampos seek(Filter& f, Device* dev, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode which)
        {
            using category = category_of_t<Device>;
            return seek(f, dev, off, way, which, category());
        }

        template <typename Filter, typename Device>
        static std::streampos seek(Filter&, Device*, stream_offset,
            std::ios_base::seekdir, std::ios_base::openmode, any_tag)
        {
            throw cant_seek();
        }

        template <typename Filter, typename Device>
        static std::streampos seek(Filter& f, Device* dev, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode which,
            random_access tag)
        {
            using category = category_of_t<Filter>;
            return seek(f, dev, off, way, which, tag, category());
        }

        template <typename Filter, typename Device>
        static std::streampos seek(Filter& f, Device* dev, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode, random_access,
            any_tag)
        {
            return f.seek(*dev, off, way);
        }

        template <typename Filter, typename Device>
        static std::streampos seek(Filter& f, Device* dev, stream_offset off,
            std::ios_base::seekdir way, std::ios_base::openmode which,
            random_access, two_sequence)
        {
            return f.seek(*dev, off, way, which);
        }

        template <typename Filter, typename Device>
        static void close(Filter& f, Device* dev, std::ios_base::openmode which)
        {
            iostream::close(f, *dev, which);
        }

        template <typename Filter, typename Device>
        static bool flush(Filter& f, Device* dev)
        {
            return iostream::flush(f, *dev);
        }
    };

    template <>
    struct flt_wrapper_impl<input>
    {
        template <typename Filter, typename Source>
        static std::streamsize read(Filter& f, Source* src,
            char_type_of_t<Filter>* s, std::streamsize n)
        {
            return iostream::read(f, *src, s, n);
        }

        template <typename Filter, typename Sink>
        static std::streamsize write(
            Filter&, Sink*, char_type_of_t<Filter> const*, std::streamsize)
        {
            throw cant_write();
        }
    };

    template <>
    struct flt_wrapper_impl<output>
    {
        template <typename Filter, typename Source>
        static std::streamsize read(
            Filter&, Source*, char_type_of_t<Filter>*, std::streamsize)
        {
            throw cant_read();
        }

        template <typename Filter, typename Sink>
        static std::streamsize write(Filter& f, Sink* snk,
            char_type_of_t<Filter> const* s, std::streamsize n)
        {
            return iostream::write(f, *snk, s, n);
        }
    };
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
