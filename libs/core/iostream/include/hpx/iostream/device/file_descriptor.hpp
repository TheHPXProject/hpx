//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Inspired by fdstream.hpp, (C) Copyright Nicolai M. Josuttis 2001,
// available at http://www.josuttis.com/cppcode/fdstream.html.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/positioning.hpp>
#include <hpx/modules/filesystem.hpp>

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

#if defined(HPX_WINDOWS)
    HPX_CXX_CORE_EXPORT using file_handle = void*;    // A.k.a. HANDLE
#else
    HPX_CXX_CORE_EXPORT using file_handle = int;
#endif

    // Forward declarations
    HPX_CXX_CORE_EXPORT class file_descriptor_source;
    HPX_CXX_CORE_EXPORT class file_descriptor_sink;

    namespace detail {

        HPX_CXX_CORE_EXPORT struct file_descriptor_impl;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT file_descriptor
    {
    public:
        enum class flags : std::uint8_t
        {
            never_close_handle = 0,
            close_handle = 3
        };

        friend class file_descriptor_source;
        friend class file_descriptor_sink;

        using handle_type = file_handle;
        using char_type = char;

        struct category
          : seekable_device_tag
          , closable_tag
        {
        };

        // Default constructor
        file_descriptor();
        ~file_descriptor();

        // Constructors taking file descriptors
        file_descriptor(handle_type fd, file_descriptor::flags);
#if defined(HPX_WINDOWS)
        file_descriptor(int fd, file_descriptor::flags);
#endif

        // Constructor taking a std:: string
        explicit file_descriptor(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::in |
                std::ios_base::out);

        // Constructor taking a C-style string
        explicit file_descriptor(char const* path,
            std::ios_base::openmode mode = std::ios_base::in |
                std::ios_base::out);

        // Constructor taking a Boost.Filesystem path
        explicit file_descriptor(std::filesystem::path const& path,
            std::ios_base::openmode const mode = std::ios_base::in |
                std::ios_base::out)
        {
            init();
            open(hpx::filesystem::to_string(path), mode);
        }

        // Copy constructor
        file_descriptor(file_descriptor const& other);
        file_descriptor(file_descriptor&& other) noexcept;

        file_descriptor& operator=(file_descriptor const&);
        file_descriptor& operator=(file_descriptor&&) noexcept;

        // open overloads taking file descriptors
        void open(handle_type fd, file_descriptor::flags) const;
#if defined(HPX_WINDOWS)
        void open(int fd, file_descriptor::flags) const;
#endif

        // open overload taking a std::string
        void open(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::in |
                std::ios_base::out) const;

        // open overload taking C-style string
        void open(char const* path,
            std::ios_base::openmode mode = std::ios_base::in |
                std::ios_base::out) const;

        // open overload taking a Boost.Filesystem path
        void open(std::filesystem::path const& path,
            std::ios_base::openmode mode = std::ios_base::in |
                std::ios_base::out,
            std::ios_base::openmode = static_cast<std::ios_base::openmode>(
                0)) const;

        [[nodiscard]] bool is_open() const;
        void close() const;

        std::streamsize read(char_type* s, std::streamsize n) const;
        std::streamsize write(char_type const* s, std::streamsize n) const;
        std::streampos seek(
            stream_offset off, std::ios_base::seekdir way) const;
        [[nodiscard]] handle_type handle() const;

    private:
        void init();

        using impl_type = detail::file_descriptor_impl;
        std::shared_ptr<impl_type> pimpl_;
    };

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT file_descriptor_source
      : private file_descriptor
    {
    public:
        using handle_type = file_handle;
        using char_type = char;

        struct category
          : input_seekable
          , device_tag
          , closable_tag
        {
        };

        using file_descriptor::close;
        using file_descriptor::handle;
        using file_descriptor::is_open;
        using file_descriptor::read;
        using file_descriptor::seek;

        // Default constructor
        file_descriptor_source();
        ~file_descriptor_source();

        file_descriptor_source(file_descriptor_source const&);
        file_descriptor_source(file_descriptor_source&&) noexcept;

        file_descriptor_source& operator=(file_descriptor_source const&);
        file_descriptor_source& operator=(file_descriptor_source&&) noexcept;

        // Constructors taking file descriptors
        explicit file_descriptor_source(handle_type fd, file_descriptor::flags);
#if defined(HPX_WINDOWS)
        explicit file_descriptor_source(int fd, file_descriptor::flags);
#endif

        // Constructor taking a std:: string
        explicit file_descriptor_source(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::in);

        // Constructor taking a C-style string
        explicit file_descriptor_source(
            char const* path, std::ios_base::openmode mode = std::ios_base::in);

        // Constructor taking a Boost.Filesystem path
        explicit file_descriptor_source(std::filesystem::path const& path,
            std::ios_base::openmode mode = std::ios_base::in)
        {
            open(path, mode);
        }

        // Constructors taking file descriptors
        void open(handle_type fd, file_descriptor::flags) const;
#if defined(HPX_WINDOWS)
        void open(int fd, file_descriptor::flags) const;
#endif

        // open overload taking a std::string
        void open(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::in) const;

        // open overload taking C-style string
        void open(char const* path,
            std::ios_base::openmode mode = std::ios_base::in) const;

    private:
        // open overload taking a detail::path
        void open(
            std::filesystem::path const& path, std::ios_base::openmode) const;
    };

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT file_descriptor_sink
      : private file_descriptor
    {
    public:
        using handle_type = file_handle;
        using char_type = char;

        struct category
          : output_seekable
          , device_tag
          , closable_tag
        {
        };

        using file_descriptor::close;
        using file_descriptor::handle;
        using file_descriptor::is_open;
        using file_descriptor::seek;
        using file_descriptor::write;

        // Default constructor
        file_descriptor_sink();
        ~file_descriptor_sink();

        file_descriptor_sink(file_descriptor_sink const&);
        file_descriptor_sink(file_descriptor_sink&&) noexcept;

        file_descriptor_sink& operator=(file_descriptor_sink const&);
        file_descriptor_sink& operator=(file_descriptor_sink&&) noexcept;

        // Constructors taking file descriptors
        file_descriptor_sink(handle_type fd, file_descriptor::flags);
#if defined(HPX_WINDOWS)
        file_descriptor_sink(int fd, file_descriptor::flags);
#endif

        // Constructor taking a std:: string
        explicit file_descriptor_sink(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::out);

        // Constructor taking a C-style string
        explicit file_descriptor_sink(char const* path,
            std::ios_base::openmode mode = std::ios_base::out);

        // Constructor taking a Boost.Filesystem path
        explicit file_descriptor_sink(std::filesystem::path const& path,
            std::ios_base::openmode const mode = std::ios_base::out)
        {
            open(path, mode);
        }

        // open overloads taking file descriptors
        void open(handle_type fd, file_descriptor::flags) const;
#if defined(HPX_WINDOWS)
        void open(int fd, file_descriptor::flags) const;
#endif

        // open overload taking a std::string
        void open(std::string const& path,
            std::ios_base::openmode mode = std::ios_base::out) const;

        // open overload taking C-style string
        void open(char const* path,
            std::ios_base::openmode mode = std::ios_base::out) const;

    private:
        // open overload taking a detail::path
        void open(
            std::filesystem::path const& path, std::ios_base::openmode) const;
    };
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
