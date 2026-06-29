//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/errors/error.hpp>
#include <hpx/errors/exception.hpp>
#include <hpx/modules/filesystem.hpp>

#include <exception>
#include <string>
#include <system_error>

namespace hpx::detail {

    [[noreturn]] void throw_exception(error errcode, std::string const& msg,
        std::string const& func, std::string const& file, long line)
    {
        filesystem::path const p(file);
        hpx::detail::throw_exception(
            hpx::exception(errcode, msg, hpx::throwmode::plain), func,
            hpx::filesystem::to_string(p), line);
    }

    [[noreturn]] void rethrow_exception(
        exception const& e, std::string const& func)
    {
        hpx::detail::throw_exception(
            hpx::exception(e.get_error(), e.what(), hpx::throwmode::rethrow),
            func, hpx::get_error_file_name(e), hpx::get_error_line_number(e));
    }

    std::exception_ptr get_exception(error errcode, std::string const& msg,
        throwmode mode, std::string const& /* func */, std::string const& file,
        long line, std::string const& auxinfo)
    {
        filesystem::path const p(file);
        return hpx::detail::get_exception(hpx::exception(errcode, msg, mode),
            hpx::filesystem::to_string(p), file, line, auxinfo);
    }

    std::exception_ptr get_exception(std::error_code const& ec,
        std::string const& /* msg */, throwmode /* mode */,
        std::string const& func, std::string const& file, long line,
        std::string const& auxinfo)
    {
        return hpx::detail::get_exception(
            hpx::exception(ec), func, file, line, auxinfo);
    }

    void throws_if(hpx::error_code& ec, error errcode, std::string const& msg,
        std::string const& func, std::string const& file, long line)
    {
        if (&ec == &hpx::throws)
        {
            hpx::detail::throw_exception(errcode, msg, func, file, line);
        }
        else
        {
            ec = make_error_code(static_cast<hpx::error>(errcode), msg,
                func.c_str(), file.c_str(), line,
                (ec.category() == hpx::get_lightweight_hpx_category()) ?
                    hpx::throwmode::lightweight :
                    hpx::throwmode::plain);
        }
    }

    void throws_bad_alloc_if(
        hpx::error_code& ec, char const* func, char const* file, long line)
    {
        if (&ec == &hpx::throws)
        {
            throw hpx::bad_alloc_exception();
        }
        else
        {
            ec = make_error_code(hpx::error::out_of_memory, "out of memory",
                func, file, line,
                (ec.category() == hpx::get_lightweight_hpx_category()) ?
                    hpx::throwmode::lightweight :
                    hpx::throwmode::plain);
        }
    }

    void rethrows_if(
        hpx::error_code& ec, exception const& e, std::string const& func)
    {
        if (&ec == &hpx::throws)
        {
            hpx::detail::rethrow_exception(e, func);
        }
        else
        {
            ec = make_error_code(e.get_error(), e.what(), func.c_str(),
                hpx::get_error_file_name(e).c_str(),
                hpx::get_error_line_number(e),
                (ec.category() == hpx::get_lightweight_hpx_category()) ?
                    hpx::throwmode::lightweight_rethrow :
                    hpx::throwmode::rethrow);
        }
    }

    [[noreturn]] void throw_thread_interrupted_exception()
    {
        throw hpx::thread_interrupted();
    }
}    // namespace hpx::detail
