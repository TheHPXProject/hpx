//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <string>

#if defined(HPX_HAVE_LOGGING)
///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    // custom log destination: send generated strings to console
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT console_local
      : logging::destination::manipulator
    {
        console_local(logging::level level, logging_destination dest) noexcept
          : level_(level)
          , dest_(dest)
        {
        }

        void operator()(logging::message const& msg) override;

        friend constexpr bool operator==(
            console_local const& lhs, console_local const& rhs) noexcept
        {
            return lhs.dest_ == rhs.dest_;
        }

        logging::level level_;
        logging_destination dest_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT struct log_settings
        {
            std::string level_;
            std::string dest_;
            std::string format_;
        };

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void define_common_formatters(
            logging::writer::named_write& writer);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void define_formatters_local(
            logging::writer::named_write& writer);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT log_settings get_log_settings(
            util::section const&, char const*);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void init_logging(
            runtime_configuration& ini, bool isconsole,
            void (*set_console_dest)(logging::writer::named_write&, char const*,
                logging::level, logging_destination),
            void (*define_formatters)(logging::writer::named_write&));

        HPX_CORE_EXPORT void init_logging_local(runtime_configuration&);
    }    // namespace detail
    /// \endcond

    //////////////////////////////////////////////////////////////////////////
    /// Enable logging for given destination
    HPX_CORE_EXPORT void enable_logging(logging_destination dest,
        std::string const& lvl = "5", std::string logdest = "",
        std::string logformat = "");

    /// Disable all logging for the given destination
    HPX_CORE_EXPORT void disable_logging(logging_destination dest) noexcept;
}    // namespace hpx::util

#else    // HPX_HAVE_LOGGING

namespace hpx::util {

    namespace detail {

        HPX_CORE_EXPORT void warn_if_logging_requested(runtime_configuration&);
    }

    //////////////////////////////////////////////////////////////////////////
    HPX_CORE_EXPORT void enable_logging(logging_destination dest,
        std::string const& lvl = "5", std::string logdest = "",
        std::string logformat = "");

    HPX_CORE_EXPORT void disable_logging(logging_destination dest) noexcept;
}    // namespace hpx::util

#endif    // HPX_HAVE_LOGGING
