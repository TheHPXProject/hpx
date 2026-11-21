//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/logging/config/defines.hpp>

#if defined(HPX_HAVE_LOGGING)

////////////////////////////////////////////////////////////////////////////////
// specific logging
#define LTM_(lvl) LHPX_(lvl, "  [TM] ")  /* thread manager */
#define LRT_(lvl) LHPX_(lvl, "  [RT] ")  /* runtime support */
#define LOSH_(lvl) LHPX_(lvl, " [OSH] ") /* one size heap */
#define LERR_(lvl) LHPX_(lvl, " [ERR] ") /* exceptions */
#define LLCO_(lvl) LHPX_(lvl, " [LCO] ") /* lcos */
#define LPCS_(lvl) LHPX_(lvl, " [PCS] ") /* performance counters */
#define LAS_(lvl) LHPX_(lvl, "  [AS] ")  /* addressing service */
#define LBT_(lvl) LHPX_(lvl, "  [BT] ")  /* bootstrap */

#define LHPX_(lvl, cat)                                                        \
    HPX_LOG_FORMAT(::hpx::util::hpx, ::hpx::util::logging::level::lvl, "{}{}", \
        ::hpx::util::logging::level::lvl, (cat)) /**/

#define LHPX_ENABLED(lvl)                                                      \
    ::hpx::util::hpx_logger()->is_enabled(::hpx::util::logging::level::lvl) /**/

#define LFATAL_                                                                \
    HPX_LOG_FORMAT(::hpx::util::hpx_error, ::hpx::util::logging::level::fatal, \
        "{} [ERR] ", ::hpx::util::logging::level::fatal)

#if defined(HPX_LOGGING_HAVE_SEPARATE_DESTINATIONS)

#define LAGAS_(lvl)                                                            \
    HPX_LOG_FORMAT(::hpx::util::agas, ::hpx::util::logging::level::lvl, "{} ", \
        ::hpx::util::logging::level::lvl) /**/

#define LAGAS_ENABLED(lvl)                                                     \
    ::hpx::util::agas_logger()->is_enabled(                                    \
        ::hpx::util::logging::level::lvl) /**/

#define LPT_(lvl)                                                              \
    HPX_LOG_FORMAT(::hpx::util::parcel, ::hpx::util::logging::level::lvl,      \
        "{} ", ::hpx::util::logging::level::lvl) /**/

#define LPT_ENABLED(lvl)                                                       \
    ::hpx::util::parcel_logger()->is_enabled(                                  \
        ::hpx::util::logging::level::lvl) /**/

#define LTIM_(lvl)                                                             \
    HPX_LOG_FORMAT(::hpx::util::timing, ::hpx::util::logging::level::lvl,      \
        "{} ", ::hpx::util::logging::level::lvl) /**/
#define LPROGRESS_                                                             \
    HPX_LOG_FORMAT(::hpx::util::timing, ::hpx::util::logging::level::fatal,    \
        " {}:{} {} ", __FILE__, __LINE__, __func__) /**/

#define LTIM_ENABLED(lvl)                                                      \
    ::hpx::util::timing_logger()->is_enabled(                                  \
        ::hpx::util::logging::level::lvl) /**/

#else

#define LAGAS_(lvl) LHPX_(lvl, "[AGAS] ")
#define LAGAS_ENABLED(lvl) LHPX_ENABLED(lvl)

#define LPT_(lvl) LHPX_(lvl, "  [PT] ")
#define LPT_ENABLED(lvl) LHPX_ENABLED(lvl)

#define LTIM_(lvl) LHPX_(lvl, " [TIM] ")
#define LPROGRESS_ LTIM_(fatal)
#define LTIM_ENABLED(lvl) LHPX_ENABLED(lvl)

#endif

#define LAPP_(lvl)                                                             \
    HPX_LOG_FORMAT(::hpx::util::app, ::hpx::util::logging::level::lvl, "{} ",  \
        ::hpx::util::logging::level::lvl) /**/

#define LAPP_ENABLED(lvl)                                                      \
    ::hpx::util::app_logger()->is_enabled(::hpx::util::logging::level::lvl) /**/

#define LDEB_                                                                  \
    HPX_LOG_FORMAT(::hpx::util::debuglog, ::hpx::util::logging::level::error,  \
        "{} ", ::hpx::util::logging::level::error) /**/

#define LDEB_ENABLED                                                           \
    ::hpx::util::debuglog_logger()->is_enabled(                                \
        ::hpx::util::logging::level::error) /**/

#if defined(HPX_LOGGING_HAVE_SEPARATE_DESTINATIONS)

#define LAGAS_CONSOLE_(lvl)                                                    \
    HPX_LOG_USE_LOG(::hpx::util::agas_console,                                 \
        static_cast<::hpx::util::logging::level>(lvl))                         \
    /**/

#define LPT_CONSOLE_(lvl)                                                      \
    HPX_LOG_USE_LOG(::hpx::util::parcel_console,                               \
        static_cast<::hpx::util::logging::level>(lvl))                         \
    /**/

#define LTIM_CONSOLE_(lvl)                                                     \
    HPX_LOG_USE_LOG(::hpx::util::timing_console,                               \
        static_cast<::hpx::util::logging::level>(lvl))                         \
    /**/
#else
#define LAGAS_CONSOLE_(lvl) LHPX_CONSOLE_(lvl)
#define LPT_CONSOLE_(lvl) LHPX_CONSOLE_(lvl)
#define LTIM_CONSOLE_(lvl) LHPX_CONSOLE_(lvl)
#endif

#define LHPX_CONSOLE_(lvl)                                                     \
    HPX_LOG_USE_LOG(::hpx::util::hpx_console,                                  \
        static_cast<::hpx::util::logging::level>(lvl))                         \
    /**/

#define LAPP_CONSOLE_(lvl)                                                     \
    HPX_LOG_USE_LOG(::hpx::util::app_console,                                  \
        static_cast<::hpx::util::logging::level>(lvl))                         \
    /**/

#define LDEB_CONSOLE_                                                          \
    HPX_LOG_USE_LOG(                                                           \
        ::hpx::util::debuglog_console, ::hpx::util::logging::level::error)     \
    /**/

#else

// logging is disabled all together

// clang-format off

#define LAGAS_(lvl)           if(true) {} else ::hpx::util::detail::dummy_log
#define LPT_(lvl)             if(true) {} else ::hpx::util::detail::dummy_log
#define LTIM_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LPROGRESS_            if(true) {} else ::hpx::util::detail::dummy_log
#define LHPX_(lvl, cat)       if(true) {} else ::hpx::util::detail::dummy_log
#define LAPP_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LDEB_                 if(true) {} else ::hpx::util::detail::dummy_log

#define LTM_(lvl)             if(true) {} else ::hpx::util::detail::dummy_log
#define LRT_(lvl)             if(true) {} else ::hpx::util::detail::dummy_log
#define LOSH_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LERR_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LLCO_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LPCS_(lvl)            if(true) {} else ::hpx::util::detail::dummy_log
#define LAS_(lvl)             if(true) {} else ::hpx::util::detail::dummy_log
#define LBT_(lvl)             if(true) {} else ::hpx::util::detail::dummy_log

#define LFATAL_               if(true) {} else ::hpx::util::detail::dummy_log

#define LAGAS_CONSOLE_(lvl)   if(true) {} else ::hpx::util::detail::dummy_log
#define LPT_CONSOLE_(lvl)     if(true) {} else ::hpx::util::detail::dummy_log
#define LTIM_CONSOLE_(lvl)    if(true) {} else ::hpx::util::detail::dummy_log
#define LHPX_CONSOLE_(lvl)    if(true) {} else ::hpx::util::detail::dummy_log
#define LAPP_CONSOLE_(lvl)    if(true) {} else ::hpx::util::detail::dummy_log
#define LDEB_CONSOLE_         if(true) {} else ::hpx::util::detail::dummy_log

#define LAGAS_ENABLED(lvl)    (false)
#define LPT_ENABLED(lvl)      (false)
#define LTIM_ENABLED(lvl)     (false)
#define LHPX_ENABLED(lvl)     (false)
#define LAPP_ENABLED(lvl)     (false)
#define LDEB_ENABLED          (false)

// clang-format on

#endif
