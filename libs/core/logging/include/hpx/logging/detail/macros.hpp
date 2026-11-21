// macros.hpp

// Boost Logging library
//
// Author: John Torjo, www.torjo.com
//
// Copyright (C) 2007 John Torjo (see www.torjo.com for email)
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.
// See http://www.torjo.com/log2/ for more details

// Make HPX inspect tool happy: hpxinspect:nounnamed

// IMPORTANT : the JT28092007_macros_HPP_DEFINED needs to remain constant
// - don't change the macro name!
#pragma once

#include <hpx/modules/preprocessor.hpp>

#include <string>

/**
@page macros Macros - how, what for?

- @ref macros_if_else_strategy
- @ref macros_using
    - @ref macros_define_declare
        - @ref HPX_DECLARE_LOG
        - @ref HPX_DEFINE_LOG
    - @ref macros_use
        - @ref HPX_LOG_USE_LOG


Simply put, you need to use macros to make sure objects (logger(s) and filter(s)) :
- are created before main
- are always created before being used

The problem we want to avoid is using a logger object before it's initialized
- this could happen
if logging from the constructor of a global/static object.

Using macros makes sure logging happens efficiently.
Basically what you want to achieve is something similar to:

@code
if ( is_filter_enabled)
    logger.gather_the_message_and_log_it();
@endcode



@section macros_if_else_strategy The if-else strategy

When gathering the message, what the macros will achieve is this:

@code
#define YOUR_COOL_MACRO_GOOD if ( !is_filter_enabled) \
; else logger.gather_the_message_and_log_it();
@endcode

The above is the correct way, instead of

@code
#define YOUR_COOL_MACRO_BAD if ( is_filter_enabled) \
logger.gather_the_message_and_log_it();
@endcode

because of

@code
if ( some_test)
  YOUR_COOL_MACRO_BAD << "some message ";
else
  whatever();
@endcode

In this case, @c whatever() will be called if @c some_test is true,
and if @c is_filter_enabled is false.

\n\n

@section macros_using Using the macros supplied with the library

There are several types of macros that this library supplies. They're explained below:

@subsection macros_define_declare Macros to declare/define logs/filters

@subsubsection HPX_DECLARE_LOG HPX_DECLARE_LOG - declaring a log

@code
HPX_DECLARE_LOG(log_name, logger_type)
@endcode

This declares a log. It should be used in a header file, to declare the log.
Note that @c logger_type only needs to be a declaration (a @c typedef, for instance)

Example:
@code
typedef logger_format_write logger_type;
HPX_DECLARE_LOG(g_l, logger_type)
@endcode


@subsubsection HPX_DEFINE_LOG HPX_DEFINE_LOG - defining a log

@code
HPX_DEFINE_LOG(log_name, logger_type, ...)
@endcode

This defines a log - and specifies some arguments to be used at its constructed.
It should be used in a source file, to define the log

Example:
@code
typedef logger_format_write logger_type;
...
HPX_DEFINE_LOG(g_l, logger_type)
@endcode

*/

////////////////////////////////////////////////////////////////////////////////
// Defining Macros

#define HPX_DECLARE_LOG(NAME)                                                  \
    ::hpx::util::logging::logger* HPX_PP_CAT(NAME, _logger)();                 \
    namespace {                                                                \
        [[maybe_unused]] inline void const* const ensure_creation_##NAME =     \
            NAME##_logger();                                                   \
    }

#define HPX_DEFINE_LOG(NAME, LEVEL)                                            \
    ::hpx::util::logging::logger* HPX_PP_CAT(NAME, _logger)()                  \
    {                                                                          \
        static ::hpx::util::logging::logger l(                                 \
            ::hpx::util::logging::level::LEVEL);                               \
        return &l;                                                             \
    }

////////////////////////////////////////////////////////////////////////////////
// Messages that were logged before initializing the log
// - cache the message (and I'll write it even if the filter is turned off)

#define HPX_LOG_USE_LOG(NAME, LEVEL)                                           \
    if (!(HPX_PP_CAT(NAME, _logger)()->is_enabled(LEVEL)))                     \
        ;                                                                      \
    else                                                                       \
        HPX_PP_CAT(NAME, _logger)()->gather()

#define HPX_LOG_FORMAT(NAME, LEVEL, FORMAT, ...)                               \
    HPX_LOG_USE_LOG(NAME, LEVEL).format(FORMAT, __VA_ARGS__)
