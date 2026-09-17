//  Copyright (c) 2026 Abhishek Kumar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/async_distributed/detail/locality_disconnected.hpp>

#include <exception>
#include <source_location>
#include <string>

namespace hpx::detail {

    namespace {

        std::string locality_disconnected_message(hpx::id_type const& id)
        {
            return hpx::util::format(
                "the requested locality {} was disconnected", id);
        }

        char const* locality_disconnected_function(
            char const* func, std::source_location const& location) noexcept
        {
            return func != nullptr ? func : location.function_name();
        }
    }    // namespace

    [[noreturn]] void throw_locality_disconnected(hpx::id_type const& id,
        char const* func, std::source_location const& location)
    {
        hpx::detail::throw_exception(hpx::error::locality_was_disconnected,
            locality_disconnected_message(id),
            locality_disconnected_function(func, location),
            location.file_name(), static_cast<long>(location.line()));
    }

    std::exception_ptr get_locality_disconnected_exception(
        hpx::id_type const& id, char const* func,
        std::source_location const& location)
    {
        return hpx::detail::get_exception(hpx::error::locality_was_disconnected,
            locality_disconnected_message(id), hpx::throwmode::plain,
            locality_disconnected_function(func, location),
            location.file_name(), static_cast<long>(location.line()));
    }
}    // namespace hpx::detail
