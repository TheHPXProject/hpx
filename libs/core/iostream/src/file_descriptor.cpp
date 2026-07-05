//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/config.hpp>    // BOOST_JOIN
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/device/file_descriptor.hpp>
#include <hpx/modules/filesystem.hpp>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

// OS-specific headers for low-level i/o.

#include <fcntl.h>       // file opening flags.
#include <sys/stat.h>    // file access permissions.
#if defined(HPX_WINDOWS)
#include <io.h>    // low-level file i/o.
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#if !defined(INVALID_SET_FILE_POINTER)
#define INVALID_SET_FILE_POINTER ((DWORD) - 1)
#endif
#else
#include <string.h>       // strerror
#include <sys/types.h>    // mode_t.
#include <unistd.h>       // low-level file i/o.
#endif

namespace hpx::iostream {

    namespace detail {

        static std::ios_base::failure system_failure(char const* msg)
        {
            std::string result;

#if defined(HPX_WINDOWS)
            DWORD err;
            LPVOID lpMsgBuf;
            if ((err = ::GetLastError()) != NO_ERROR &&
                ::FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR) &lpMsgBuf, 0, nullptr) != 0)
            {
                result.reserve(std::strlen(msg) + 2 +
                    std::strlen(static_cast<LPSTR>(lpMsgBuf)));
                result.append(msg);
                result.append(": ");
                result.append(static_cast<LPSTR>(lpMsgBuf));
                ::LocalFree(lpMsgBuf);
            }
            else
            {
                result += msg;
            }
#else
            char const* system_msg = errno ? strerror(errno) : "";
            result.reserve(std::strlen(msg) + 2 + std::strlen(system_msg));
            result.append(msg);
            result.append(": ");
            result.append(system_msg);
#endif
            return std::ios_base::failure(result);
        }

        [[noreturn]] static void throw_system_failure(char const* msg)
        {
            throw system_failure(msg);
        }

        //------------------Definition of file_descriptor_impl------------------------//

        // Contains the platform dependent implementation
        struct file_descriptor_impl
        {
            // Note: These need to match file_descriptor_flags
            enum class flags : std::uint8_t
            {
                never_close = 0,
                close_on_exit = 1,
                close_on_close = 2,
                close_always = 3
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

            file_descriptor_impl();
            ~file_descriptor_impl();

            file_descriptor_impl(file_descriptor_impl const&) = delete;
            file_descriptor_impl(file_descriptor_impl&&) = delete;
            file_descriptor_impl& operator=(
                file_descriptor_impl const&) = delete;
            file_descriptor_impl& operator=(file_descriptor_impl&&) = delete;

            void open(file_handle fd, flags);
#if defined(HPX_WINDOWS)
            void open(int fd, flags);
#endif
            void open(std::filesystem::path const&, std::ios_base::openmode);

            [[nodiscard]] bool is_open() const;

            void close();
            void close_impl(bool close_flag, bool throw_);

            std::streamsize read(char* s, std::streamsize n);
            std::streamsize write(char const* s, std::streamsize n);
            std::streampos seek(stream_offset off, std::ios_base::seekdir way);

            static file_handle invalid_handle();

            file_handle handle_;
            int flags_;
        };

        //------------------Implementation of file_descriptor_impl--------------------//
        file_descriptor_impl::file_descriptor_impl()
          : handle_(invalid_handle())
          , flags_(0)
        {
        }

        file_descriptor_impl::~file_descriptor_impl()
        {
            close_impl(flags_ & flags::close_on_exit, false);
        }

        void file_descriptor_impl::open(file_handle const fd, flags f)
        {
            // Using 'close' to close the existing handle so that it will
            // throw an exception if it fails.
            //
            // Only closing after assigning the new handle, so that the
            // class will take ownership of the handle regardless of whether
            // close throws.
            file_descriptor_impl tmp;
            tmp.handle_ = handle_;
            tmp.flags_ = static_cast<int>(flags_ & flags::close_on_exit ?
                    flags::close_on_close :
                    flags::never_close);

            handle_ = fd;
            flags_ = static_cast<int>(f);

            tmp.close();
        }

#if defined(HPX_WINDOWS)
        void file_descriptor_impl::open(int const fd, flags const f)
        {
            open(reinterpret_cast<file_handle>(_get_osfhandle(fd)), f);
        }
#endif

        void file_descriptor_impl::open(
            std::filesystem::path const& p, std::ios_base::openmode const mode)
        {
            close_impl(flags_ & flags::close_on_exit, true);

#if defined(HPX_WINDOWS)
            DWORD dwDesiredAccess;
            DWORD dwCreationDisposition;

            if (!(mode &
                    (std::ios_base::in | std::ios_base::out |
                        std::ios_base::app)) ||
                ((mode & std::ios_base::trunc) &&
                    ((mode & std::ios_base::app) ||
                        !(mode & std::ios_base::out))))
            {
                throw std::ios_base::failure("bad open mode");
            }

            if (mode & std::ios_base::in)
            {
                if (mode & std::ios_base::app)
                {
                    dwCreationDisposition = OPEN_ALWAYS;
                    dwDesiredAccess = GENERIC_READ | FILE_APPEND_DATA |
                        FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
                        STANDARD_RIGHTS_WRITE | SYNCHRONIZE;
                }
                else if (mode & std::ios_base::trunc)
                {
                    dwCreationDisposition = CREATE_ALWAYS;
                    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                }
                else if (mode & std::ios_base::out)
                {
                    dwCreationDisposition = OPEN_EXISTING;
                    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                }
                else
                {
                    dwCreationDisposition = OPEN_EXISTING;
                    dwDesiredAccess = GENERIC_READ;
                }
            }
            else if (mode & std::ios_base::app)
            {
                dwCreationDisposition = OPEN_ALWAYS;
                dwDesiredAccess = FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES |
                    FILE_WRITE_EA | STANDARD_RIGHTS_WRITE | SYNCHRONIZE;
            }
            else
            {
                dwCreationDisposition = CREATE_ALWAYS;
                dwDesiredAccess = GENERIC_WRITE;
            }

            HANDLE handle = ::CreateFileA(hpx::filesystem::to_string(p).c_str(),
                dwDesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle != INVALID_HANDLE_VALUE)
            {
                handle_ = handle;
                flags_ = static_cast<int>(flags::close_always);
            }
            else
            {
                flags_ = 0;
                throw_system_failure("failed opening file");
            }
#else
            // Calculate oflag argument to open.
            int oflag = 0;
            if (!(mode &
                    (std::ios_base::in | std::ios_base::out |
                        std::ios_base::app)) ||
                ((mode & std::ios_base::trunc) &&
                    ((mode & std::ios_base::app) ||
                        !(mode & std::ios_base::out))))
            {
                throw std::ios_base::failure("bad open mode");
            }
            else if (mode & std::ios_base::in)
            {
                if (mode & std::ios_base::app)
                    oflag |= O_CREAT | O_APPEND | O_RDWR;
                else if (mode & std::ios_base::trunc)
                    oflag |= O_CREAT | O_TRUNC | O_RDWR;
                else if (mode & std::ios_base::out)
                    oflag |= O_RDWR;
                else
                    oflag |= O_RDONLY;
            }
            else
            {
                if (mode & std::ios_base::app)
                    oflag |= O_CREAT | O_APPEND | O_WRONLY;
                else
                    oflag |= O_CREAT | O_TRUNC | O_WRONLY;
            }
#ifdef _LARGEFILE64_SOURCE
            oflag |= O_LARGEFILE;
#endif

            // Calculate pmode argument to open.
            mode_t pmode =
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

            // Open file.
            int fd = ::open(p.c_str(), oflag, pmode);
            if (fd == -1)
            {
                throw system_failure("failed opening file");
            }
            else
            {
                if (mode & std::ios_base::ate)
                {
                    if (::lseek(fd, 0, SEEK_END) == -1)
                    {
                        ::close(fd);
                        throw system_failure("failed opening file");
                    }
                }
                handle_ = fd;
                flags_ = static_cast<int>(flags::close_always);
            }
#endif
        }

        bool file_descriptor_impl::is_open() const
        {
            return handle_ != invalid_handle();
        }

        void file_descriptor_impl::close()
        {
            close_impl((flags_ & flags::close_on_close) != 0, true);
        }

        void file_descriptor_impl::close_impl(
            bool const close_flag, bool const throw_)
        {
            if (handle_ != invalid_handle())
            {
                bool success = true;

                if (close_flag)
                {
#if defined(HPX_WINDOWS)
                    success = ::CloseHandle(handle_) == 1;
#else
                    success = ::close(handle_) != -1;
#endif
                }
                //  Even if the close fails, we want nothing more to do with the handle
                handle_ = invalid_handle();
                flags_ = 0;
                if (!success && throw_)
                    throw_system_failure("failed closing file");
            }
        }

        std::streamsize file_descriptor_impl::read(char* s, std::streamsize n)
        {
#if defined(HPX_WINDOWS)
            DWORD result;
            if (!::ReadFile(
                    handle_, s, static_cast<DWORD>(n), &result, nullptr))
            {
                // report EOF if the write-side of a pipe has been closed
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    result = 0;
                }
                else
                {
                    throw_system_failure("failed reading");
                }
            }
            return result == 0 ? -1 : static_cast<std::streamsize>(result);
#else
            errno = 0;
            std::streamsize result = ::read(handle_, s, n);
            if (errno != 0)
                throw_system_failure("failed reading");
            return result == 0 ? -1 : result;
#endif
        }

        std::streamsize file_descriptor_impl::write(
            char const* s, std::streamsize n)
        {
#if defined(HPX_WINDOWS)
            DWORD ignore;
            if (!::WriteFile(
                    handle_, s, static_cast<DWORD>(n), &ignore, nullptr))
                throw_system_failure("failed writing");
#else
            auto amt = ::write(handle_, s, n);
            if (amt < n)    // Handles blocking fd's only.
                throw_system_failure("failed writing");
#endif
            return n;
        }

        std::streampos file_descriptor_impl::seek(
            stream_offset off, std::ios_base::seekdir const way)
        {
#if defined(HPX_WINDOWS)
            LONG lDistanceToMove = static_cast<LONG>(off & 0xffffffff);
            LONG lDistanceToMoveHigh = static_cast<LONG>(off >> 32);
            DWORD dwResultLow =
                ::SetFilePointer(handle_, lDistanceToMove, &lDistanceToMoveHigh,
                    way == std::ios_base::beg     ? FILE_BEGIN :
                        way == std::ios_base::cur ? FILE_CURRENT :
                                                    FILE_END);

            if (dwResultLow == INVALID_SET_FILE_POINTER &&
                ::GetLastError() != NO_ERROR)
            {
                throw system_failure("failed seeking");
            }
            else
            {
                return offset_to_position(
                    (static_cast<stream_offset>(lDistanceToMoveHigh) << 32) +
                    dwResultLow);
            }
#else
            if (off > (std::numeric_limits<::off_t>::max)() ||
                off < (std::numeric_limits<::off_t>::min)())
            {
                throw std::ios_base::failure("bad offset");
            }
            stream_offset result = ::lseek(handle_, static_cast<::off_t>(off),
                (way == std::ios_base::beg        ? SEEK_SET :
                        way == std::ios_base::cur ? SEEK_CUR :
                                                    SEEK_END));
            if (result == -1)
                throw system_failure("failed seeking");
            return offset_to_position(result);
#endif
        }

        // Returns the value stored in a file_handle variable when no file is open
        file_handle file_descriptor_impl::invalid_handle()
        {
#if defined(HPX_WINDOWS)
            return INVALID_HANDLE_VALUE;
#else
            return -1;
#endif
        }
    }    // namespace detail

    //------------------Implementation of file_descriptor-------------------------//
    file_descriptor::file_descriptor()
      : pimpl_(std::make_shared<impl_type>())
    {
    }

    file_descriptor::~file_descriptor() = default;

    file_descriptor::file_descriptor(
        handle_type const fd, file_descriptor::flags const f)
      : pimpl_(std::make_shared<impl_type>())
    {
        open(fd, f);
    }

#if defined(HPX_WINDOWS)
    file_descriptor::file_descriptor(
        int const fd, file_descriptor::flags const f)
      : pimpl_(new impl_type)
    {
        open(fd, f);
    }
#endif

    file_descriptor::file_descriptor(
        std::string const& path, std::ios_base::openmode const mode)
      : pimpl_(new impl_type)
    {
        open(path, mode);
    }

    file_descriptor::file_descriptor(
        char const* path, std::ios_base::openmode const mode)
      : pimpl_(new impl_type)
    {
        open(path, mode);
    }

    file_descriptor::file_descriptor(file_descriptor const& other) = default;
    file_descriptor::file_descriptor(
        file_descriptor&& other) noexcept = default;

    file_descriptor& file_descriptor::operator=(
        file_descriptor const&) = default;
    file_descriptor& file_descriptor::operator=(
        file_descriptor&&) noexcept = default;

    void file_descriptor::open(
        handle_type const fd, file_descriptor::flags f) const
    {
        pimpl_->open(fd, static_cast<detail::file_descriptor_impl::flags>(f));
    }

#if defined(HPX_WINDOWS)
    void file_descriptor::open(int const fd, file_descriptor::flags f) const
    {
        pimpl_->open(fd, static_cast<detail::file_descriptor_impl::flags>(f));
    }
#endif

    void file_descriptor::open(
        std::string const& path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    void file_descriptor::open(
        char const* path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    bool file_descriptor::is_open() const
    {
        return pimpl_->is_open();
    }

    void file_descriptor::close() const
    {
        pimpl_->close();
    }

    std::streamsize file_descriptor::read(
        char_type* s, std::streamsize const n) const
    {
        return pimpl_->read(s, n);
    }

    std::streamsize file_descriptor::write(
        char_type const* s, std::streamsize const n) const
    {
        return pimpl_->write(s, n);
    }

    std::streampos file_descriptor::seek(
        stream_offset const off, std::ios_base::seekdir const way) const
    {
        return pimpl_->seek(off, way);
    }

    file_handle file_descriptor::handle() const
    {
        return pimpl_->handle_;
    }

    void file_descriptor::init()
    {
        pimpl_.reset(new impl_type);
    }

    void file_descriptor::open(std::filesystem::path const& path,
        std::ios_base::openmode mode, std::ios_base::openmode const base) const
    {
        mode |= base;
        pimpl_->open(path, mode);
    }

    //------------------Implementation of file_descriptor_source------------------//
    file_descriptor_source::file_descriptor_source(
        handle_type const fd, file_descriptor::flags const f)
    {
        open(fd, f);
    }

    file_descriptor_source::file_descriptor_source() = default;
    file_descriptor_source::~file_descriptor_source() = default;

    file_descriptor_source::file_descriptor_source(
        file_descriptor_source const&) = default;
    file_descriptor_source::file_descriptor_source(
        file_descriptor_source&&) noexcept = default;

    file_descriptor_source& file_descriptor_source::operator=(
        file_descriptor_source const&) = default;
    file_descriptor_source& file_descriptor_source::operator=(
        file_descriptor_source&&) noexcept = default;

#if defined(HPX_WINDOWS)
    file_descriptor_source::file_descriptor_source(
        int const fd, file_descriptor::flags const f)
    {
        open(fd, f);
    }
#endif

    file_descriptor_source::file_descriptor_source(
        std::string const& path, std::ios_base::openmode const mode)
    {
        open(path, mode);
    }

    file_descriptor_source::file_descriptor_source(
        char const* path, std::ios_base::openmode const mode)
    {
        open(path, mode);
    }

    void file_descriptor_source::open(
        handle_type const fd, file_descriptor::flags const f) const
    {
        file_descriptor::open(fd, f);
    }

#if defined(HPX_WINDOWS)
    void file_descriptor_source::open(
        int const fd, file_descriptor::flags const f) const
    {
        file_descriptor::open(fd, f);
    }
#endif

    void file_descriptor_source::open(
        std::string const& path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    void file_descriptor_source::open(
        char const* path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    void file_descriptor_source::open(std::filesystem::path const& path,
        std::ios_base::openmode const mode) const
    {
        if (mode & (std::ios_base::out | std::ios_base::trunc))
            throw std::ios_base::failure("invalid mode");
        file_descriptor::open(path, mode, std::ios_base::in);
    }

    //------------------Implementation of file_descriptor_sink--------------------//
    file_descriptor_sink::file_descriptor_sink(
        handle_type const fd, file_descriptor::flags const f)
    {
        open(fd, f);
    }

    file_descriptor_sink::file_descriptor_sink() = default;
    file_descriptor_sink::~file_descriptor_sink() = default;

    file_descriptor_sink::file_descriptor_sink(
        file_descriptor_sink const&) = default;
    file_descriptor_sink::file_descriptor_sink(
        file_descriptor_sink&&) noexcept = default;

    file_descriptor_sink& file_descriptor_sink::operator=(
        file_descriptor_sink const&) = default;
    file_descriptor_sink& file_descriptor_sink::operator=(
        file_descriptor_sink&&) noexcept = default;

#if defined(HPX_WINDOWS)
    file_descriptor_sink::file_descriptor_sink(
        int const fd, file_descriptor::flags const f)
    {
        open(fd, f);
    }
#endif

    file_descriptor_sink::file_descriptor_sink(
        std::string const& path, std::ios_base::openmode const mode)
    {
        open(path, mode);
    }

    file_descriptor_sink::file_descriptor_sink(
        char const* path, std::ios_base::openmode const mode)
    {
        open(path, mode);
    }

    void file_descriptor_sink::open(
        handle_type const fd, file_descriptor::flags const f) const
    {
        file_descriptor::open(fd, f);
    }

#if defined(HPX_WINDOWS)
    void file_descriptor_sink::open(
        int const fd, file_descriptor::flags const f) const
    {
        file_descriptor::open(fd, f);
    }
#endif

    void file_descriptor_sink::open(
        std::string const& path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    void file_descriptor_sink::open(
        char const* path, std::ios_base::openmode const mode) const
    {
        open(std::filesystem::path(path), mode);
    }

    void file_descriptor_sink::open(std::filesystem::path const& path,
        std::ios_base::openmode const mode) const
    {
        if (mode & std::ios_base::in)
            throw std::ios_base::failure("invalid mode");
        file_descriptor::open(path, mode, std::ios_base::out);
    }
}    // namespace hpx::iostream
