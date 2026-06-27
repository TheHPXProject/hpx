//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Make HPX inspect tool happy:
//                               hpxinspect:noinclude:HPX_THROW_EXCEPTION
//                               hpxinspect:noinclude:HPX_THROWS_IF
//                               hpxinspect:noinclude:HPX_THROW_BAD_ALLOC
//                               hpxinspect:noinclude:HPX_THROWS_BAD_ALLOC_IF

#pragma once

#include <hpx/config/defines.hpp>
#include <hpx/config/export_definitions.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <type_traits>

/// \cond NOINTERNAL
///////////////////////////////////////////////////////////////////////////////
// helper macro allowing to prepend file name and line number to a generated
// exception
#define HPX_THROW_STD_EXCEPTION(except, func)                                  \
    hpx::detail::throw_exception(except, func, __FILE__, __LINE__) /**/

#define HPX_RETHROW_EXCEPTION(e, f) hpx::detail::rethrow_exception(e, f) /**/

#define HPX_RETHROWS_IF(ec, e, f) hpx::detail::rethrows_if(ec, e, f) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_GET_EXCEPTION(...)                                                 \
    HPX_GET_EXCEPTION_(__VA_ARGS__)                                            \
    /**/

#define HPX_GET_EXCEPTION_(...)                                                \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_GET_EXCEPTION_, HPX_PP_NARGS(__VA_ARGS__))(   \
        __VA_ARGS__))                                                          \
/**/
#define HPX_GET_EXCEPTION_3(errcode, f, msg)                                   \
    HPX_GET_EXCEPTION_4(errcode, hpx::throwmode::plain, f, msg)                \
/**/
#define HPX_GET_EXCEPTION_4(errcode, mode, f, msg)                             \
    hpx::detail::get_exception(errcode, msg, mode, f, __FILE__, __LINE__) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_THROW_IN_CURRENT_FUNC(errcode, msg)                                \
    HPX_THROW_EXCEPTION(errcode, HPX_ASSERTION_CURRENT_FUNCTION, msg)          \
    /**/

#define HPX_RETHROW_IN_CURRENT_FUNC(errcode, msg)                              \
    HPX_RETHROW_EXCEPTION(errcode, HPX_ASSERTION_CURRENT_FUNCTION, msg)        \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_THROWS_IN_CURRENT_FUNC_IF(ec, errcode, msg)                        \
    HPX_THROWS_IF(ec, errcode, HPX_ASSERTION_CURRENT_FUNCTION, msg)            \
    /**/

#define HPX_RETHROWS_IN_CURRENT_FUNC_IF(ec, errcode, msg)                      \
    HPX_RETHROWS_IF(ec, errcode, HPX_ASSERTION_CURRENT_FUNCTION, msg)          \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_THROW_THREAD_INTERRUPTED_EXCEPTION()                               \
    hpx::detail::throw_thread_interrupted_exception() /**/
/// \endcond

///////////////////////////////////////////////////////////////////////////////
/// \def HPX_THROW_EXCEPTION(errcode, f, msg)
/// \brief Throw a hpx::exception initialized from the given parameters
///
/// The macro \a HPX_THROW_EXCEPTION can be used to throw a hpx::exception.
/// The purpose of this macro is to prepend the source file name and line number
/// of the position where the exception is thrown to the error message.
/// Moreover, this associates additional diagnostic information with the
/// exception, such as file name and line number, locality id and thread id,
/// and stack backtrace from the point where the exception was thrown.
///
/// The parameter \p errcode holds the hpx::error code the new exception should
/// encapsulate. The parameter \p f is expected to hold the name of the
/// function exception is thrown from and the parameter \p msg holds the error
/// message the new exception should encapsulate.
///
/// \par Example:
///
/// \code
///      void raise_exception()
///      {
///          // Throw a hpx::exception initialized from the given parameters.
///          // Additionally associate with this exception some detailed
///          // diagnostic information about the throw-site.
///          HPX_THROW_EXCEPTION(hpx::error::no_success, "raise_exception",
///             "simulated error");
///      }
/// \endcode
///
#define HPX_THROW_EXCEPTION(errcode, f, ...)                                   \
    hpx::detail::throw_exception(                                              \
        errcode, hpx::util::format(__VA_ARGS__), f, __FILE__, __LINE__) /**/

///////////////////////////////////////////////////////////////////////////////
/// \def HPX_THROW_BAD_ALLOC(f, msg)
/// \brief Throw a hpx::bad_alloc_exception initialized from the given parameters
///
/// The macro \a HPX_THROW_BAD_ALLOC can be used to throw a hpx::exception.
/// The purpose of this macro is to prepend the source file name and line number
/// of the position where the exception is thrown to the error message.
/// Moreover, this associates additional diagnostic information with the
/// exception, such as file name and line number, locality id and thread id,
/// and stack backtrace from the point where the exception was thrown.
///
/// The parameter \p errcode holds the hpx::error code the new exception should
/// encapsulate. The parameter \p f is expected to hold the name of the
/// function exception is thrown from and the parameter \p msg holds the error
/// message the new exception should encapsulate.
///
/// \par Example:
///
/// \code
///      void raise_exception()
///      {
///          // Throw a hpx::exception initialized from the given parameters.
///          // Additionally associate with this exception some detailed
///          // diagnostic information about the throw-site.
///          HPX_THROW_BAD_ALLOC("raise_exception", "simulated error");
///      }
/// \endcode
///
#define HPX_THROW_BAD_ALLOC(f)                                                 \
    hpx::detail::throw_bad_alloc_exception(f, __FILE__, __LINE__) /**/

/// \def HPX_THROWS_IF(ec, errcode, f, msg)
/// \brief Either throw a hpx::exception or initialize \a hpx::error_code from
///        the given parameters
///
/// The macro \a HPX_THROWS_IF can be used to either throw a \a hpx::exception
/// or to initialize a \a hpx::error_code from the given parameters. If
/// &ec == &hpx::throws, the semantics of this macro are equivalent to
/// \a HPX_THROW_EXCEPTION. If &ec != &hpx::throws, the \a hpx::error_code
/// instance \p ec is initialized instead.
///
/// The parameter \p errcode holds the hpx::error code from which the new
/// exception should be initialized. The parameter \p f is expected to hold the
/// name of the function exception is thrown from and the parameter \p msg
/// holds the error message the new exception should encapsulate.
///
#define HPX_THROWS_IF(ec, errcode, f, ...)                                     \
    hpx::detail::throws_if(ec, errcode, hpx::util::format(__VA_ARGS__), f,     \
        __FILE__, __LINE__) /**/

/// \def HPX_THROWS_BAD_ALLOC_IF(ec, f)
/// \brief Either throw a hpx::bad_alloc_exception or \a hpx::error_code to out_of_memory
///
/// The macro \a HPX_THROWS_BAD_ALLOC_IF can be used to either throw a
/// \a hpx::bad_alloc_exception or to initialize a \a hpx::error_code to
/// \a hpx::error::out_of_memory. If
/// &ec == &hpx::throws, the semantics of this macro are equivalent to
/// \a HPX_THROW_BAD_ALLOC. If &ec != &hpx::throws, the \a hpx::error_code
/// instance \p ec is initialized instead.
///
#define HPX_THROWS_BAD_ALLOC_IF(ec, f)                                         \
    hpx::detail::throws_bad_alloc_if(ec, f, __FILE__, __LINE__) /**/

////////////////////////////////////////////////////////////////////////////////
// From hpx/errors/define_error_info.hpp
#define HPX_DEFINE_ERROR_INFO(NAME, TYPE)                                      \
    HPX_CXX_CORE_EXPORT struct NAME : ::hpx::error_info<NAME, TYPE>            \
    {                                                                          \
        explicit NAME(TYPE const& value) noexcept(                             \
            std::is_nothrow_copy_constructible_v<TYPE>)                        \
          : error_info(value)                                                  \
        {                                                                      \
        }                                                                      \
                                                                               \
        explicit NAME(TYPE&& value) noexcept                                   \
          : error_info(HPX_MOVE(value))                                        \
        {                                                                      \
        }                                                                      \
    } /**/
