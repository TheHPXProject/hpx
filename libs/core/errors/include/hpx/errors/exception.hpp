//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file exception.hpp
/// \page hpx::exception
/// \headerfile hpx/exception.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/errors/error.hpp>
#include <hpx/errors/error_code.hpp>
#include <hpx/errors/exception_fwd.hpp>
#include <hpx/errors/exception_info.hpp>

#include <exception>
#include <functional>
#include <string>
#include <system_error>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief A hpx::exception is the main exception type used by HPX to
    ///        report errors.
    ///
    /// The hpx::exception type is the main exception type  used by HPX to
    /// report errors. Any exceptions thrown by functions in the HPX library
    /// are either of this type or of a type derived from it. This implies that
    /// it is always safe to use this type only in catch statements guarding
    /// HPX library calls.
    HPX_CXX_CORE_EXPORT class HPX_ALWAYS_EXPORT exception
      : public std::system_error
    {
    public:
        /// Construct a hpx::exception from a \a hpx::error.
        ///
        /// \param e    The parameter \p e holds the hpx::error code the new
        ///             exception should encapsulate.
        explicit exception(error e = hpx::error::success);

        /// Construct a hpx::exception from a boost#system_error.
        explicit exception(std::system_error const& e);

        /// Construct a hpx::exception from a boost#system#error_code (this is
        /// new for Boost V1.69). This constructor is required to compensate
        /// for the changes introduced as a resolution to LWG3162
        /// (https://cplusplus.github.io/LWG/issue3162).
        explicit exception(std::error_code const& e);

        /// Construct a hpx::exception from a \a hpx::error and an error message.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param mode   The parameter \p mode specifies whether the returned
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        exception(error e, char const* msg, throwmode mode = throwmode::plain);

        /// Construct a hpx::exception from a \a hpx::error and an error message.
        ///
        /// \param e      The parameter \p e holds the hpx::error code the new
        ///               exception should encapsulate.
        /// \param msg    The parameter \p msg holds the error message the new
        ///               exception should encapsulate.
        /// \param mode   The parameter \p mode specifies whether the returned
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        exception(
            error e, std::string const& msg, throwmode mode = throwmode::plain);

        /// Destruct a hpx::exception
        ///
        /// \throws nothing
        ~exception() override;

        /// The function \a get_error() returns the hpx::error code stored
        /// in the referenced instance of a hpx::exception. It returns
        /// the hpx::error code this exception instance was constructed
        /// from.
        ///
        /// \throws nothing
        [[nodiscard]] error get_error() const noexcept;

        /// The function \a get_error_code() returns a hpx::error_code which
        /// represents the same error condition as this hpx::exception instance.
        ///
        /// \param mode   The parameter \p mode specifies whether the returned
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a throwmode::plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        [[nodiscard]] error_code get_error_code(
            throwmode mode = throwmode::plain) const noexcept;
    };

    HPX_CXX_CORE_EXPORT class HPX_ALWAYS_EXPORT bad_alloc_exception
      : public hpx::exception
      , public std::bad_alloc
    {
    public:
        /// Construct a hpx::bad_alloc_exception.
        bad_alloc_exception();

        /// The function \a get_error() returns hpx::error::out_of_memory
        ///
        /// \throws nothing
        [[nodiscard]] static constexpr error get_error() noexcept
        {
            return hpx::error::out_of_memory;
        }

        /// The function \a get_error_code() returns a hpx::error_code which
        /// represents the same error condition as this hpx::exception instance.
        ///
        /// \param mode   The parameter \p mode specifies whether the returned
        ///               hpx::error_code belongs to the error category
        ///               \a hpx_category (if mode is \a throwmode::plain, this is the
        ///               default) or to the category \a hpx_category_rethrow
        ///               (if mode is \a rethrow).
        [[nodiscard]] error_code get_error_code(
            throwmode mode = throwmode::plain) const noexcept;
    };

    HPX_CXX_CORE_EXPORT using custom_exception_info_handler_type =
        std::function<hpx::exception_info(
            std::string const&, std::string const&, long, std::string const&)>;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_custom_exception_info_handler(
        custom_exception_info_handler_type f);

    HPX_CXX_CORE_EXPORT using pre_exception_handler_type =
        std::function<void()>;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_pre_exception_handler(
        pre_exception_handler_type f);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief A hpx::thread_interrupted is the exception type used by HPX to
    ///        interrupt a running HPX thread.
    ///
    /// The \a hpx::thread_interrupted type is the exception type used by HPX to
    /// interrupt a running thread.
    ///
    /// A running thread can be interrupted by invoking the interrupt() member
    /// function of the corresponding hpx::thread object. When the interrupted
    /// thread next executes one of the specified interruption points (or if it
    /// is currently blocked whilst executing one) with interruption enabled,
    /// then a hpx::thread_interrupted exception will be thrown in the interrupted
    /// thread. If not caught, this will cause the execution of the interrupted
    /// thread to terminate. As with any other exception, the stack will be
    /// unwound, and destructors for objects of automatic storage duration will
    /// be executed.
    ///
    /// If a thread wishes to avoid being interrupted, it can create an instance
    /// of \a hpx::this_thread::disable_interruption. Objects of this class disable
    /// interruption for the thread that created them on construction, and
    /// restore the interruption state to whatever it was before on destruction.
    ///
    /// \code
    ///     void f()
    ///     {
    ///         // interruption enabled here
    ///         {
    ///             hpx::this_thread::disable_interruption di;
    ///             // interruption disabled
    ///             {
    ///                 hpx::this_thread::disable_interruption di2;
    ///                 // interruption still disabled
    ///             } // di2 destroyed, interruption state restored
    ///             // interruption still disabled
    ///         } // di destroyed, interruption state restored
    ///         // interruption now enabled
    ///     }
    /// \endcode
    ///
    /// The effects of an instance of \a hpx::this_thread::disable_interruption can be
    /// temporarily reversed by constructing an instance of
    /// \a hpx::this_thread::restore_interruption, passing in the
    /// \a hpx::this_thread::disable_interruption object in question. This will restore
    /// the interruption state to what it was when the
    /// \a hpx::this_thread::disable_interruption
    /// object was constructed, and then disable interruption again when the
    /// \a hpx::this_thread::restore_interruption object is destroyed.
    ///
    /// \code
    ///     void g()
    ///     {
    ///         // interruption enabled here
    ///         {
    ///             hpx::this_thread::disable_interruption di;
    ///             // interruption disabled
    ///             {
    ///                 hpx::this_thread::restore_interruption ri(di);
    ///                 // interruption now enabled
    ///             } // ri destroyed, interruption disable again
    ///         } // di destroyed, interruption state restored
    ///         // interruption now enabled
    ///     }
    /// \endcode
    ///
    /// At any point, the interruption state for the current thread can be
    /// queried by calling \a hpx::this_thread::interruption_enabled().
    HPX_CXX_CORE_EXPORT struct HPX_ALWAYS_EXPORT thread_interrupted
      : std::exception
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Extract elements of the diagnostic information embedded in the given
    // exception.

    /// \brief Return the error message of the thrown exception.
    ///
    /// The function \a hpx::get_error_what can be used to extract the
    /// diagnostic information element representing the error message as stored
    /// in the given exception instance.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \returns    The error message stored in the exception
    ///             If the exception instance does not hold
    ///             this information, the function will return an empty string.
    ///
    /// \throws     std#bad_alloc (if one of the required allocations fails)
    ///
    /// \see        \a hpx::diagnostic_information(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error()
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_config(), \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    get_error_what(exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    get_error_what(std::exception_ptr const& e);

    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] std::string get_error_what(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_what(*xi) : std::string("<unknown>");
        });
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::string get_error_what(
        hpx::error_code const& e)
    {
        // if this is a lightweight error_code, return canned response
        if (e.category() == hpx::get_lightweight_hpx_category() ||
            e.category() == hpx::get_lightweight_hpx_rethrow_category())
        {
            return e.message();
        }

        return get_error_what<hpx::error_code>(e);
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::string get_error_what(
        std::exception const& e)
    {
        return e.what();
    }
    /// \endcond

    /// \brief Return the error code value of the exception thrown.
    ///
    /// The function \a hpx::get_error can be used to extract the
    /// diagnostic information element representing the error value code as
    /// stored in the given exception instance.
    ///
    /// \param e    The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception,
    ///             \a hpx::error_code, or \a std::exception_ptr.
    ///
    /// \returns    The error value code of the locality where the exception was
    ///             thrown. If the exception instance does not hold
    ///             this information, the function will return
    ///             \a hpx::naming#invalid_locality_id.
    ///
    /// \throws     nothing
    ///
    /// \see        \a hpx::diagnostic_information(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT error get_error(
        hpx::exception const& e);

    /// \copydoc get_error(hpx::exception const& e)
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT error get_error(
        hpx::error_code const& e);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT error get_error(
        std::exception_ptr const& e);
    /// \endcond

    /// \brief Return the function name from which the exception was thrown.
    ///
    /// The function \a hpx::get_error_function_name can be used to extract the
    /// diagnostic information element representing the name of the function
    /// as stored in the given exception instance.
    ///
    /// \returns    The name of the function from which the exception was
    ///             thrown. If the exception instance does not hold
    ///             this information, the function will return an empty string.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \throws     std#bad_alloc (if one of the required allocations fails)
    ///
    /// \see        \a hpx::diagnostic_information(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id()
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    get_error_function_name(hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] std::string get_error_function_name(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_function_name(*xi) : std::string("<unknown>");
        });
    }
    /// \endcond

    /// \brief Return the (source code) file name of the function from which
    ///        the exception was thrown.
    ///
    /// The function \a hpx::get_error_file_name can be used to extract the
    /// diagnostic information element representing the name of the source file
    /// as stored in the given exception instance.
    ///
    /// \returns    The name of the source file of the function from which the
    ///             exception was thrown. If the exception instance does
    ///             not hold this information, the function will return an empty
    ///             string.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \throws     std#bad_alloc (if one of the required allocations fails)
    ///
    /// \see        \a hpx::diagnostic_information(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT std::string
    get_error_file_name(hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] std::string get_error_file_name(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_file_name(*xi) : std::string("<unknown>");
        });
    }
    /// \endcond

    /// \brief Return the line number in the (source code) file of the function
    ///        from which the exception was thrown.
    ///
    /// The function \a hpx::get_error_line_number can be used to extract the
    /// diagnostic information element representing the line number
    /// as stored in the given exception instance.
    ///
    /// \returns    The line number of the place where the exception was
    ///             thrown. If the exception instance does not hold
    ///             this information, the function will return -1.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \throws     nothing
    ///
    /// \see        \a hpx::diagnostic_information(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name()
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT long
    get_error_line_number(hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    [[nodiscard]] long get_error_line_number(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_line_number(*xi) : -1;
        });
    }
    /// \endcond
}    // namespace hpx

#include <hpx/errors/throw_exception.hpp>

#include <hpx/config/warnings_suffix.hpp>
