//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file error_code.hpp
/// \page hpx::error_code
/// \headerfile hpx/system_error.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/errors/error.hpp>
#include <hpx/errors/exception_fwd.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// \cond NODETAIL
    namespace detail {

        HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::exception_ptr
        access_exception(error_code const&);

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct HPX_ALWAYS_EXPORT command_line_error final
          : std::logic_error
        {
            explicit command_line_error(char const* msg)
              : std::logic_error(msg)
            {
            }

            explicit command_line_error(std::string const& msg)
              : std::logic_error(msg)
            {
            }
        };
    }    // namespace detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns generic HPX error category used for new errors.
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::error_category const&
    get_hpx_category() noexcept;

    /// \brief Returns generic HPX error category used for errors re-thrown
    ///        after the exception has been de-serialized.
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::error_category const&
    get_hpx_rethrow_category() noexcept;

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::error_category const&
    get_lightweight_hpx_category() noexcept;

    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::error_category const&
    get_lightweight_hpx_rethrow_category() noexcept;

    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::error_category const&
    get_hpx_category(throwmode mode) noexcept;

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::error_code
    make_system_error_code(error e, throwmode mode = throwmode::plain)
    {
        return {static_cast<int>(e), get_hpx_category(mode)};
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::error_condition
    make_error_condition(error e, throwmode mode)
    {
        return {static_cast<int>(e), get_hpx_category(mode)};
    }
    /// \endcond

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(
        std::exception_ptr const& e);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief A hpx::error_code represents an arbitrary error condition.
    ///
    /// The class hpx::error_code describes an object used to hold error code
    /// values, such as those originating from the operating system or other
    /// low-level application program interfaces.
    ///
    /// \note Class hpx::error_code is an adjunct to error reporting by
    /// exception
    ///
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT error_code
      : public std::error_code    //-V690
    {
    public:
        /// Construct an object of type error_code.
        ///
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws nothing
        explicit error_code(throwmode mode = throwmode::plain)
          : std::error_code(make_system_error_code(hpx::error::success, mode))
        {
        }

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws nothing
        explicit error_code(error e, throwmode mode = throwmode::plain);

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param func   The name of the function where the error was raised.
        /// \param file   The file name of the code where the error was raised.
        /// \param line   The line number of the code line where the error was
        ///               raised.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws nothing
        error_code(error e, char const* func, char const* file, long line,
            throwmode mode = throwmode::plain);

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws std#bad_alloc (if allocation of a copy of
        ///         the passed string fails).
        error_code(error e, char const* msg, throwmode mode = throwmode::plain);

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param func   The name of the function where the error was raised.
        /// \param file   The file name of the code where the error was raised.
        /// \param line   The line number of the code line where the error was
        ///               raised.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws std#bad_alloc (if allocation of a copy of
        ///         the passed string fails).
        error_code(error e, char const* msg, char const* func, char const* file,
            long line, throwmode mode = throwmode::plain);

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws std#bad_alloc (if allocation of a copy of
        ///         the passed string fails).
        error_code(
            error e, std::string const& msg, throwmode mode = throwmode::plain);

        /// Construct an object of type error_code.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param func   The name of the function where the error was raised.
        /// \param file   The file name of the code where the error was raised.
        /// \param line   The line number of the code line where the error was
        ///               raised.
        /// \param mode   The parameter \p mode specifies whether the constructed
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        ///
        /// \throws std#bad_alloc (if allocation of a copy of
        ///         the passed string fails).
        error_code(error e, std::string const& msg, char const* func,
            char const* file, long line, throwmode mode = throwmode::plain);

        error_code(error_code&&) = default;
        error_code& operator=(error_code&&) = default;

        ~error_code() = default;

        /// Return a reference to the error message stored in the hpx::error_code.
        ///
        /// \throws nothing
        [[nodiscard]] std::string get_message() const;

        /// \brief Clear this error_code object.
        /// The postconditions of invoking this method are
        /// * value() == hpx::error::success and
        ///   category() == hpx::get_hpx_category()
        void clear()
        {
            this->std::error_code::assign(
                static_cast<int>(hpx::error::success), get_hpx_category());
            exception_ = std::exception_ptr();
        }

        void assign(error e, std::string const& msg, throwmode mode);

        /// Copy constructor for error_code
        ///
        /// \note This function maintains the error category of the left hand
        ///       side if the right hand side is a success code.
        error_code(error_code const& rhs);

        /// Assignment operator for error_code
        ///
        /// \note This function maintains the error category of the left hand
        ///       side if the right hand side is a success code.
        error_code& operator=(error_code const& rhs);

    private:
        friend std::exception_ptr detail::access_exception(error_code const&);
        friend class exception;
        friend error_code make_error_code(std::exception_ptr const&);

        error_code(int err, hpx::exception const& e);
        explicit error_code(std::exception_ptr const& e);

        std::exception_ptr exception_;
    };

    /// @{
    /// \brief Returns a new error_code constructed from the given parameters.
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(
        error e, throwmode mode = throwmode::plain)
    {
        return error_code(e, mode);
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(error e,
        char const* func, char const* file, long line,
        throwmode mode = throwmode::plain)
    {
        return {e, func, file, line, mode};
    }

    /// \brief Returns error_code(e, msg, mode).
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(
        error e, char const* msg, throwmode mode = throwmode::plain)
    {
        return {e, msg, mode};
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(error e,
        char const* msg, char const* func, char const* file, long line,
        throwmode mode = throwmode::plain)
    {
        return {e, msg, func, file, line, mode};
    }

    /// \brief Returns error_code(e, msg, mode).
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(
        error e, std::string const& msg, throwmode mode = throwmode::plain)
    {
        return {e, msg, mode};
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(error e,
        std::string const& msg, char const* func, char const* file, long line,
        throwmode mode = throwmode::plain)
    {
        return {e, msg, func, file, line, mode};
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_error_code(
        std::exception_ptr const& e)
    {
        return error_code(e);
    }
    ///@}

    /// \brief Returns error_code(hpx::error::success, "success", mode).
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline error_code make_success_code(
        throwmode mode = throwmode::plain)
    {
        return error_code(mode);
    }

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT throwmode get_throwmode(
        error_code const& ec);
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>

#include <hpx/errors/throw_exception.hpp>
