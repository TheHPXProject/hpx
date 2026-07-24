//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/errors/define_error_info.hpp>
#include <hpx/errors/error.hpp>
#include <hpx/errors/error_code.hpp>
#include <hpx/errors/exception.hpp>
#include <hpx/errors/exception_fwd.hpp>
#include <hpx/errors/exception_info.hpp>
#include <hpx/errors/exception_list.hpp>
#include <hpx/modules/logging.hpp>

#if defined(HPX_WINDOWS)
#include <process.h>
#elif defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <typeinfo>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// Construct a hpx::exception from a \a hpx::error.
    ///
    /// \param e    The parameter \p e holds the hpx::error code the new
    ///             exception should encapsulate.
    exception::exception(hpx::error e)
      : std::system_error(make_error_code(e, throwmode::plain))
    {
        HPX_ASSERT((e >= hpx::error::success && e < hpx::error::last_error) ||
            (e & hpx::error::system_error_flag));
        if (e != hpx::error::success)
        {
            LERR_(error).format(    //-V1067
                "created exception: {}", this->exception::what());
        }
    }

    /// Construct a hpx::exception from a boost#system_error.
    exception::exception(std::system_error const& e)
      : std::system_error(e)
    {
        LERR_(error).format(    //-V1067
            "created exception: {}", this->exception::what());
    }

    /// Construct a hpx::exception from a boost#system#error_code (this is
    /// new for Boost V1.69).
    exception::exception(std::error_code const& e)
      : std::system_error(e)
    {
        LERR_(error).format(    //-V1067
            "created exception: {}", this->exception::what());
    }

    /// Construct a hpx::exception from a \a hpx::error and an error message.
    ///
    /// \param e      The parameter \p e holds the hpx::error code the new
    ///               exception should encapsulate.
    /// \param msg    The parameter \p msg holds the error message the new
    ///               exception should encapsulate.
    /// \param mode   The parameter \p mode specifies whether the returned
    ///               hpx::error_code belongs to the error category
    ///               \a hpx_category (if mode is \a throwmode::plain, this
    ///               is the default) or to the category \a hpx_category_rethrow
    ///               (if mode is \a throwmode::rethrow).
    exception::exception(hpx::error e, char const* msg, throwmode mode)
      : std::system_error(make_system_error_code(e, mode), msg)
    {
        HPX_ASSERT((e >= hpx::error::success && e < hpx::error::last_error) ||
            (e & hpx::error::system_error_flag));
        if (e != hpx::error::success)
        {
            LERR_(error).format(    //-V1067
                "created exception: {}", this->exception::what());
        }
    }

    /// Construct a hpx::exception from a \a hpx::error and an error message.
    ///
    /// \param e      The parameter \p e holds the hpx::error code the new
    ///               exception should encapsulate.
    /// \param msg    The parameter \p msg holds the error message the new
    ///               exception should encapsulate.
    /// \param mode   The parameter \p mode specifies whether the returned
    ///               hpx::error_code belongs to the error category
    ///               \a hpx_category (if mode is \a throwmode::plain, this is
    ///               the default) or to the category \a hpx_category_rethrow
    ///               (if mode is \a throwmode::rethrow).
    exception::exception(hpx::error e, std::string const& msg, throwmode mode)
      : std::system_error(make_system_error_code(e, mode), msg)
    {
        HPX_ASSERT((e >= hpx::error::success && e < hpx::error::last_error) ||
            (e & hpx::error::system_error_flag));
        if (e != hpx::error::success)
        {
            LERR_(error).format(    //-V1067
                "created exception: {}", this->exception::what());
        }
    }

    /// Destruct a hpx::exception
    ///
    /// \throws nothing
    exception::~exception() = default;

    /// The function \a get_error() returns the hpx::error code stored in the
    /// referenced instance of a hpx::exception. It returns the hpx::error code
    /// this exception instance was constructed from.
    ///
    /// \throws nothing
    error exception::get_error() const noexcept
    {
        return static_cast<error>(this->std::system_error::code().value());
    }

    /// The function \a get_error_code() returns a hpx::error_code which
    /// represents the same error condition as this hpx::exception instance.
    ///
    /// \param mode   The parameter \p mode specifies whether the returned
    ///               hpx::error_code belongs to the error category
    ///               \a hpx_category (if mode is \a throwmode::plain, this is
    ///               the default) or to the category \a hpx_category_rethrow
    ///               (if mode is \a throwmode::rethrow).
    error_code exception::get_error_code(throwmode mode) const noexcept
    {
        (void) mode;
        return {this->std::system_error::code().value(), *this};
    }

    bad_alloc_exception::bad_alloc_exception()
      : hpx::exception(hpx::error::out_of_memory)
    {
    }

    static custom_exception_info_handler_type custom_exception_info_handler;

    void set_custom_exception_info_handler(custom_exception_info_handler_type f)
    {
        custom_exception_info_handler = HPX_MOVE(f);
    }

    static pre_exception_handler_type pre_exception_handler;

    void set_pre_exception_handler(pre_exception_handler_type f)
    {
        pre_exception_handler = HPX_MOVE(f);
    }
}    // namespace hpx

namespace hpx::detail {

    template <typename Exception>
    std::exception_ptr construct_lightweight_exception(Exception const& e,
        std::string const& func, std::string const& file, long line)
    {
        // create a std::exception_ptr object encapsulating the Exception to
        // be thrown and annotate it with all the local information we have
        try
        {
            throw_with_info(e,
                std::move(
                    hpx::exception_info().set(hpx::detail::throw_function(func),
                        hpx::detail::throw_file(file),
                        hpx::detail::throw_line(line))));
        }
        catch (...)
        {
            return std::current_exception();
        }

        // need this return to silence a warning with icc
        HPX_ASSERT(false);    // -V779
        return std::exception_ptr();
    }

    template <typename Exception>
    std::exception_ptr construct_lightweight_exception(Exception const& e)
    {
        // create a std::exception_ptr object encapsulating the Exception to
        // be thrown and annotate it with all the local information we have
        try
        {
            hpx::throw_with_info(e);
        }
        catch (...)
        {
            return std::current_exception();
        }

        // need this return to silence a warning with icc
        HPX_ASSERT(false);    // -V779
        return std::exception_ptr();
    }

    template std::exception_ptr construct_lightweight_exception(
        hpx::thread_interrupted const&);
    template std::exception_ptr construct_lightweight_exception(
        hpx::exception_list const&);

    template <typename Exception>
    std::exception_ptr construct_custom_exception(Exception const& e,
        std::string const& func, std::string const& file, long line,
        std::string const& auxinfo)
    {
        if (!custom_exception_info_handler)
        {
            return construct_lightweight_exception(e, func, file, line);
        }

        // create a std::exception_ptr object encapsulating the Exception to
        // be thrown and annotate it with information provided by the hook
        try
        {
            throw_with_info(
                e, custom_exception_info_handler(func, file, line, auxinfo));
        }
        catch (...)
        {
            return std::current_exception();
        }

        // need this return to silence a warning with icc
        HPX_ASSERT(false);    // -V779
        return std::exception_ptr();
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Exception>
    constexpr bool is_of_lightweight_hpx_category(Exception const&) noexcept
    {
        return false;
    }

    inline bool is_of_lightweight_hpx_category(hpx::exception const& e) noexcept
    {
        hpx::error_code const& code = e.get_error_code();
        std::error_category const& category = code.category();
        return category == get_lightweight_hpx_category() ||
            category == get_lightweight_hpx_rethrow_category();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::exception_ptr access_exception(error_code const& e)
    {
        return e.exception_;
    }

    template <typename Exception>
    std::exception_ptr get_exception(Exception const& e,
        std::string const& func, std::string const& file, long line,
        std::string const& auxinfo)
    {
        if (is_of_lightweight_hpx_category(e))
        {
            return construct_lightweight_exception(e, func, file, line);
        }

        return construct_custom_exception(e, func, file, line, auxinfo);
    }

    template <typename Exception>
    void throw_exception(Exception const& e, std::string const& func,
        std::string const& file, long line)
    {
        if (pre_exception_handler)
        {
            pre_exception_handler();
        }

        std::rethrow_exception(get_exception(e, func, file, line));
    }

    void throw_bad_alloc_exception([[maybe_unused]] char const* func,
        [[maybe_unused]] char const* file, [[maybe_unused]] long line)
    {
        if (pre_exception_handler)
        {
            pre_exception_handler();
        }

        throw hpx::bad_alloc_exception();
    }

    ///////////////////////////////////////////////////////////////////////////
    template std::exception_ptr get_exception(hpx::exception const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::system_error const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::exception const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(hpx::detail::std_exception const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::bad_exception const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(hpx::detail::bad_exception const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::bad_typeid const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(hpx::detail::bad_typeid const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::bad_cast const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(hpx::detail::bad_cast const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::bad_alloc const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(hpx::detail::bad_alloc const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::logic_error const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::runtime_error const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::out_of_range const&,
        std::string const&, std::string const&, long, std::string const&);
    template std::exception_ptr get_exception(std::invalid_argument const&,
        std::string const&, std::string const&, long, std::string const&);

    ///////////////////////////////////////////////////////////////////////////
    template void throw_exception(
        hpx::exception const&, std::string const&, std::string const&, long);
    template void throw_exception(
        std::system_error const&, std::string const&, std::string const&, long);
    template void throw_exception(
        std::exception const&, std::string const&, std::string const&, long);
    template void throw_exception(hpx::detail::std_exception const&,
        std::string const&, std::string const&, long);
    template void throw_exception(std::bad_exception const&, std::string const&,
        std::string const&, long);
    template void throw_exception(hpx::detail::bad_exception const&,
        std::string const&, std::string const&, long);
    template void throw_exception(
        std::bad_typeid const&, std::string const&, std::string const&, long);
    template void throw_exception(hpx::detail::bad_typeid const&,
        std::string const&, std::string const&, long);
    template void throw_exception(
        std::bad_cast const&, std::string const&, std::string const&, long);
    template void throw_exception(hpx::detail::bad_cast const&,
        std::string const&, std::string const&, long);
    template void throw_exception(
        std::bad_alloc const&, std::string const&, std::string const&, long);
    template void throw_exception(hpx::detail::bad_alloc const&,
        std::string const&, std::string const&, long);
    template void throw_exception(
        std::logic_error const&, std::string const&, std::string const&, long);
    template void throw_exception(std::runtime_error const&, std::string const&,
        std::string const&, long);
    template void throw_exception(
        std::out_of_range const&, std::string const&, std::string const&, long);
    template void throw_exception(std::invalid_argument const&,
        std::string const&, std::string const&, long);
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// Return the error message.
    std::string get_error_what(hpx::exception_info const& xi)
    {
        // Try a cast to std::exception - this should handle boost.system
        // error codes in addition to the standard library exceptions.
        auto const* se = dynamic_cast<std::exception const*>(&xi);
        return se ? se->what() : std::string("<unknown>");
    }

    std::string get_error_what(std::exception_ptr const& e)
    {
        try
        {
            std::rethrow_exception(e);
        }
        catch (hpx::thread_interrupted const&)
        {
            return "thread_interrupted";
        }
        catch (std::exception const& ex)
        {
            return get_error_what(ex);
        }
        catch (...)
        {
            return "<unknown>";
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    error get_error(hpx::exception const& e)
    {
        return static_cast<hpx::error>(e.get_error());
    }

    error get_error(hpx::error_code const& e)
    {
        return static_cast<hpx::error>(e.value());
    }

    error get_error(std::exception_ptr const& e)
    {
        try
        {
            std::rethrow_exception(e);
        }
        catch (hpx::thread_interrupted const&)
        {
            return hpx::error::thread_cancelled;
        }
        catch (hpx::exception const& he)
        {
            return he.get_error();
        }
        catch (std::system_error const& se)
        {
            int code = se.code().value();
            if (code < hpx::error::success || code >= hpx::error::last_error)
            {
                code |= hpx::error::system_error_flag;
            }
            return static_cast<hpx::error>(code);
        }
        catch (...)
        {
            return hpx::error::unknown_error;
        }
    }

    /// Return the function name from which the exception was thrown.
    std::string get_error_function_name(hpx::exception_info const& xi)
    {
        if (std::string const* function = xi.get<hpx::detail::throw_function>())
            return *function;

        return {};
    }

    /// Return the (source code) file name of the function from which the
    /// exception was thrown.
    std::string get_error_file_name(hpx::exception_info const& xi)
    {
        if (std::string const* file = xi.get<hpx::detail::throw_file>())
            return *file;

        return "<unknown>";
    }

    /// Return the line number in the (source code) file of the function from
    /// which the exception was thrown.
    long get_error_line_number(hpx::exception_info const& xi)
    {
        if (long const* line = xi.get<hpx::detail::throw_line>())
            return *line;

        return -1;
    }

}    // namespace hpx
