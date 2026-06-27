//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// \cond NODETAIL
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Stores the information about the locality id the exception has been
        // raised on. This information will show up in error messages under the
        // [locality] tag.
        HPX_DEFINE_ERROR_INFO(throw_locality, std::uint32_t);    //-V835

        // Stores the information about the hostname of the locality the exception
        // has been raised on. This information will show up in error messages
        // under the [hostname] tag.
        HPX_DEFINE_ERROR_INFO(throw_hostname, std::string);

        // Stores the information about the pid of the OS process the exception
        // has been raised on. This information will show up in error messages
        // under the [pid] tag.
        HPX_DEFINE_ERROR_INFO(throw_pid, std::int64_t);    //-V835

        // Stores the information about the shepherd thread the exception has been
        // raised on. This information will show up in error messages under the
        // [shepherd] tag.
        HPX_DEFINE_ERROR_INFO(throw_shepherd, std::size_t);    //-V835

        // Stores the information about the HPX thread the exception has been
        // raised on. This information will show up in error messages under the
        // [thread_id] tag.
        HPX_DEFINE_ERROR_INFO(throw_thread_id, std::size_t);    //-V835

        // Stores the information about the HPX thread name the exception has been
        // raised on. This information will show up in error messages under the
        // [thread_name] tag.
        HPX_DEFINE_ERROR_INFO(throw_thread_name, std::string);

        // Stores the information about the stack backtrace at the point the
        // exception has been raised at. This information will show up in error
        // messages under the [stack_trace] tag.
        HPX_DEFINE_ERROR_INFO(throw_stacktrace, std::string);

        // Stores the full execution environment of the locality the exception
        // has been raised in. This information will show up in error messages
        // under the [env] tag.
        HPX_DEFINE_ERROR_INFO(throw_env, std::string);

        // Stores the full HPX configuration information of the locality the
        // exception has been raised in. This information will show up in error
        // messages under the [config] tag.
        HPX_DEFINE_ERROR_INFO(throw_config, std::string);

        // Stores the current runtime state. This information will show up in
        // error messages under the [state] tag.
        HPX_DEFINE_ERROR_INFO(throw_state, std::string);

        // Stores additional auxiliary information (such as information about
        // the current parcel). This information will show up in error messages
        // under the [auxinfo] tag.
        HPX_DEFINE_ERROR_INFO(throw_auxinfo, std::string);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::exception_info
        custom_exception_info(std::string const& func, std::string const& file,
            long line, std::string const& auxinfo);

        // Portably extract the current execution environment
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string
        get_execution_environment();

        // Report an early or late exception and locally abort execution. There
        // isn't anything more we could do.
        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void
        report_exception_and_terminate(std::exception const&);
        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void
        report_exception_and_terminate(std::exception_ptr const&);
        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void
        report_exception_and_terminate(hpx::exception const&);

        // Report an early or late exception and locally exit execution. There
        // isn't anything more we could do. The exception will be re-thrown
        // from hpx::init
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void report_exception_and_continue(
            std::exception const&);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void report_exception_and_continue(
            std::exception_ptr const&);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void report_exception_and_continue(
            hpx::exception const&);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::exception_info
        construct_exception_info(std::string const& func,
            std::string const& file, std::int64_t line,
            std::string const& back_trace, std::uint32_t node,
            std::string const& hostname, std::int64_t pid, std::size_t shepherd,
            std::size_t thread_id, std::string const& thread_name,
            std::string const& env, std::string const& config,
            std::string const& state_name, std::string const& auxinfo);

        HPX_CXX_CORE_EXPORT template <typename Exception>
        HPX_CORE_EXPORT std::exception_ptr construct_exception(
            Exception const& e, hpx::exception_info info);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void pre_exception_handler();
    }    // namespace detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Extract the diagnostic information embedded in the given
    /// exception and return a string holding a formatted message.
    ///
    /// The function \a hpx::diagnostic_information can be used to extract all
    /// diagnostic information stored in the given exception instance as a
    /// formatted string. This simplifies debug output as it composes the
    /// diagnostics into one, easy to use function call. This includes the name
    /// of the source file and line number, the sequence number of the OS-thread
    /// and the HPX-thread id, the locality id and the stack backtrace of the
    /// point where the original exception was thrown.
    ///
    /// \param xi   The parameter \p e will be inspected for all diagnostic
    ///             information elements which have been stored at the point
    ///             where the exception was thrown. This parameter can be one of
    ///             the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \returns    The formatted string holding all the available diagnostic
    ///             information stored in the given exception instance.
    ///
    /// \throws     std#bad_alloc (if any of the required allocation operations
    ///             fail)
    ///
    /// \see        \a hpx::get_error_locality_id(), \a hpx::get_error_host_name(),
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string diagnostic_information(
        exception_info const& xi);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string
    default_diagnostic_information(std::exception_ptr const& e);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string diagnostic_information(E const& e)
    {
        return invoke_with_exception_info(e, [&](exception_info const* xi) {
            if (xi)
            {
                return diagnostic_information(*xi);
            }

            if constexpr (std::is_same_v<std::exception_ptr, E>)
            {
                return default_diagnostic_information(e);
            }
            else
            {
                return std::string("<unknown>");
            }
        });
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    // Extract elements of the diagnostic information embedded in the given
    // exception.

    /// \brief Return the locality id where the exception was thrown.
    ///
    /// The function \a hpx::get_error_locality_id can be used to extract the
    /// diagnostic information element representing the locality id as stored
    /// in the given exception instance.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \returns    The locality id of the locality where the exception was
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
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::uint32_t get_error_locality_id(
        hpx::exception_info const& xi) noexcept;

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::uint32_t get_error_locality_id(E const& e) noexcept
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_locality_id(*xi) :
                        ~static_cast<std::uint32_t>(0);
        });
    }
    /// \endcond

    /// \brief Return the hostname of the locality where the exception was
    ///        thrown.
    ///
    /// The function \a hpx::get_error_host_name can be used to extract the
    /// diagnostic information element representing the host name as stored in
    /// the given exception instance.
    ///
    /// \param xi   The parameter \p e will be inspected for the requested
    ///             diagnostic information elements which have been stored at
    ///             the point where the exception was thrown. This parameter
    ///             can be one of the following types: \a hpx::exception_info,
    ///             \a hpx::error_code, \a std::exception, or
    ///             \a std::exception_ptr.
    ///
    /// \returns    The hostname of the locality where the exception was
    ///             thrown. If the exception instance does not hold
    ///             this information, the function will return and empty string.
    ///
    /// \throws     std#bad_alloc (if one of the required allocations fails)
    ///
    /// \see        \a hpx::diagnostic_information()
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error()
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string get_error_host_name(
        hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_host_name(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_host_name(*xi) : std::string();
        });
    }
    /// \endcond

    /// \brief Return the (operating system) process id of the locality where
    ///        the exception was thrown.
    ///
    /// The function \a hpx::get_error_process_id can be used to extract the
    /// diagnostic information element representing the process id as stored in
    /// the given exception instance.
    ///
    /// \returns    The process id of the OS-process which threw the exception
    ///             If the exception instance does not hold
    ///             this information, the function will return 0.
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
    ///             \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::int64_t get_error_process_id(
        hpx::exception_info const& xi) noexcept;

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::int64_t get_error_process_id(E const& e) noexcept
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_process_id(*xi) : -1;
        });
    }
    /// \endcond

    /// \brief Return the environment of the OS-process at the point the
    ///        exception was thrown.
    ///
    /// The function \a hpx::get_error_env can be used to extract the
    /// diagnostic information element representing the environment of the
    /// OS-process collected at the point the exception was thrown.
    ///
    /// \returns    The environment from the point the exception was
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
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string get_error_env(
        hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_env(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_env(*xi) : std::string("<unknown>");
        });
    }
    /// \endcond

    /// \brief Return the stack backtrace from the point the exception was thrown.
    ///
    /// The function \a hpx::get_error_backtrace can be used to extract the
    /// diagnostic information element representing the stack backtrace
    /// collected at the point the exception was thrown.
    ///
    /// \returns    The stack back trace from the point the exception was
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
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string get_error_backtrace(
        hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_backtrace(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_backtrace(*xi) : std::string();
        });
    }
    /// \endcond

    /// \brief Return the sequence number of the OS-thread used to execute
    ///        HPX-threads from which the exception was thrown.
    ///
    /// The function \a hpx::get_error_os_thread can be used to extract the
    /// diagnostic information element representing the sequence number  of the
    /// OS-thread as stored in the given exception instance.
    ///
    /// \returns    The sequence number of the OS-thread used to execute the
    ///             HPX-thread from which the exception was
    ///             thrown. If the exception instance does not hold
    ///             this information, the function will return std::size(-1).
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
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_error_os_thread(
        hpx::exception_info const& xi) noexcept;

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::size_t get_error_os_thread(E const& e) noexcept
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_os_thread(*xi) : static_cast<std::size_t>(-1);
        });
    }
    /// \endcond

    /// \brief Return the unique thread id of the HPX-thread from which the
    ///        exception was thrown.
    ///
    /// The function \a hpx::get_error_thread_id can be used to extract the
    /// diagnostic information element representing the HPX-thread id
    /// as stored in the given exception instance.
    ///
    /// \returns    The unique thread id of the HPX-thread from which the
    ///             exception was thrown. If the exception instance
    ///             does not hold this information, the function will return
    ///             std::size_t(0).
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
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread()
    ///             \a hpx::get_error_thread_description(), \a hpx::get_error(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config(),
    ///             \a hpx::get_error_state()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_error_thread_id(
        hpx::exception_info const& xi) noexcept;

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::size_t get_error_thread_id(E const& e) noexcept
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_thread_id(*xi) : static_cast<std::size_t>(-1);
        });
    }
    /// \endcond

    /// \brief Return any additionally available thread description of the
    ///        HPX-thread from which the exception was thrown.
    ///
    /// The function \a hpx::get_error_thread_description can be used to extract the
    /// diagnostic information element representing the additional thread
    /// description as stored in the given exception instance.
    ///
    /// \returns    Any additionally available thread description of the
    ///             HPX-thread from which the exception was
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
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error(), \a hpx::get_error_state(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_config()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string
    get_error_thread_description(hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_thread_description(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_thread_description(*xi) : std::string();
        });
    }
    /// \endcond

    /// \brief Return the HPX configuration information point from which the
    ///        exception was thrown.
    ///
    /// The function \a hpx::get_error_config can be used to extract the
    /// HPX configuration information element representing the full HPX
    /// configuration information as stored in the given exception instance.
    ///
    /// \returns    Any additionally available HPX configuration information
    ///             the point from which the exception was
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
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error(), \a hpx::get_error_state()
    ///             \a hpx::get_error_what(), \a hpx::get_error_thread_description()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string get_error_config(
        hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_config(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_config(*xi) : std::string();
        });
    }
    /// \endcond

    /// \brief Return the HPX runtime state information at which the exception
    ///        was thrown.
    ///
    /// The function \a hpx::get_error_state can be used to extract the
    /// HPX runtime state information element representing the state the
    /// runtime system is currently in as stored in the given exception
    /// instance.
    ///
    /// \returns    The point runtime state at the point at which the exception
    ///             was thrown. If the exception instance does not hold
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
    ///             \a hpx::get_error_process_id(), \a hpx::get_error_function_name(),
    ///             \a hpx::get_error_file_name(), \a hpx::get_error_line_number(),
    ///             \a hpx::get_error_os_thread(), \a hpx::get_error_thread_id(),
    ///             \a hpx::get_error_backtrace(), \a hpx::get_error_env(),
    ///             \a hpx::get_error(),
    ///             \a hpx::get_error_what(), \a hpx::get_error_thread_description()
    ///
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string get_error_state(
        hpx::exception_info const& xi);

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename E>
    std::string get_error_state(E const& e)
    {
        return invoke_with_exception_info(e, [](exception_info const* xi) {
            return xi ? get_error_state(*xi) : std::string();
        });
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    // \cond NOINTERNAL
    // For testing purposes we sometime expect to see exceptions, allow those
    // to go through without attaching a debugger.
    //
    // This should be used carefully as it disables the possible attaching of
    // a debugger for all exceptions, not only the expected ones.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool expect_exception(bool flag = true);
    /// \endcond

}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
