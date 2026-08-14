//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Make HPX inspect tool happy:
//                               hpxinspect:noinclude:HPX_ASSERT
//                               hpxinspect:noinclude:HPX_ASSERT_MSG
//                               hpxinspect:noassert_macro

/// \page HPX_ASSERT, HPX_ASSERT_MSG
/// \headerfile hpx/assert.hpp

#pragma once

#include <hpx/config/export_definitions.hpp>

#include <hpx/modules/format.hpp>
#include <hpx/modules/preprocessor.hpp>

#if !defined(HPX_HAVE_CXX_MODULES) || defined(HPX_CORE_EXPORTS) ||             \
    (defined(HPX_COMPILE_BMI) && defined(HPX_COMPILE_CORE_WITH_MODULES))
#include <hpx/config.hpp>
#include <hpx/assertion/api.hpp>
#include <hpx/assertion/evaluate_assert.hpp>
#include <hpx/assertion/source_location.hpp>
#endif

#if defined(HPX_COMPUTE_DEVICE_CODE)
#include <assert.h>
#endif
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#if defined(DOXYGEN)
/// \def HPX_ASSERT(expr, msg)
/// \brief This macro asserts that \a expr evaluates to true.
///
/// \param expr The expression to assert on. This can either be an expression
///             that's convertible to bool or a callable which returns bool
/// \param msg The optional message that is used to give further information if
///             the assert fails. This should be convertible to a std::string
///
/// If \p expr evaluates to false, The source location and \p msg is being
/// printed along with the expression and additional. Afterward, the program is
/// being aborted. The assertion handler can be customized by calling
/// hpx::assertion::set_assertion_handler().
///
/// Asserts are enabled if \a HPX_DEBUG is set. This is the default for
/// `CMAKE_BUILD_TYPE=Debug`
#define HPX_ASSERT(expr)

/// \see HPX_ASSERT
#define HPX_ASSERT_MSG(expr, msg)
#else
/// \cond NOINTERNAL
#define HPX_ASSERT_(expr, ...)                                                 \
    (!!(expr) ? void() :                                                       \
                ::hpx::assertion::detail::handle_assert(                       \
                    HPX_CURRENT_SOURCE_LOCATION(), HPX_PP_STRINGIZE(expr),     \
                    hpx::util::format(__VA_ARGS__))) /**/

#define HPX_ASSERT_LOCKED_(l, expr, ...)                                       \
    (!!(expr) ? void() :                                                       \
                ((l).unlock(),                                                 \
                    ::hpx::assertion::detail::handle_assert(                   \
                        HPX_CURRENT_SOURCE_LOCATION(), HPX_PP_STRINGIZE(expr), \
                        hpx::util::format(__VA_ARGS__)))) /**/

#if defined(HPX_DEBUG)
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_ASSERT(expr) assert(expr)
#define HPX_ASSERT_MSG(expr, ...) HPX_ASSERT(expr)
#define HPX_ASSERT_LOCKED(l, expr) assert(expr)
#define HPX_ASSERT_LOCKED_MSG(l, expr, ...) HPX_ASSERT(expr)
#else
#define HPX_ASSERT(expr) HPX_ASSERT_(expr, "")
#define HPX_ASSERT_MSG(expr, ...) HPX_ASSERT_(expr, __VA_ARGS__)
#define HPX_ASSERT_LOCKED(l, expr) HPX_ASSERT_LOCKED_(l, expr, "")
#define HPX_ASSERT_LOCKED_MSG(l, expr, ...)                                    \
    HPX_ASSERT_LOCKED_(l, expr, __VA_ARGS__)
#endif
#else
#define HPX_ASSERT(expr)
#define HPX_ASSERT_MSG(expr, ...)
#define HPX_ASSERT_LOCKED(l, expr)
#define HPX_ASSERT_LOCKED_MSG(l, expr, ...)
#endif

#define HPX_UNREACHABLE                                                        \
    HPX_ASSERT_(false,                                                         \
        "This code is meant to be unreachable. If you are seeing this error "  \
        "message it means that you have found a bug in HPX. Please report it " \
        "on the issue tracker: https://github.com/TheHPXProject/hpx/issues."); \
    std::terminate()

#define HPX_CURRENT_SOURCE_LOCATION() std::source_location::current()
#endif
