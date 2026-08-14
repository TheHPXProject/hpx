//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>

#include <cstdint>
#include <string>
#include <system_error>

namespace hpx::serialization {

    namespace detail {

        // Discriminates the error_category carried by a hpx::error_code so that
        // it can be reconstructed exactly on the receiving side instead of
        // always being coerced into an HPX category.
        enum class error_category_kind : std::uint8_t
        {
            hpx = 0,
            generic = 1,
            system = 2
        };

        inline error_category_kind get_error_category_kind(
            std::error_category const& category)
        {
            if (category == hpx::get_hpx_category() ||
                category == hpx::get_hpx_rethrow_category() ||
                category == hpx::get_lightweight_hpx_category() ||
                category == hpx::get_lightweight_hpx_rethrow_category())
            {
                return error_category_kind::hpx;
            }
            if (category == std::generic_category())
            {
                return error_category_kind::generic;
            }
            if (category == std::system_category())
            {
                return error_category_kind::system;
            }

            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "hpx::serialization::detail::get_error_category_kind",
                "cannot serialize hpx::error_code with unsupported error "
                "category '{}'",
                category.name());
        }

        inline std::string strip_hpx_error_message(std::string const& message)
        {
            if (auto const pos = message.rfind(": HPX(");
                pos != std::string::npos)
            {
                return message.substr(0, pos);
            }
            return message;
        }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT inline void serialize(
        output_archive& ar, hpx::error_code const& ec, unsigned)
    {
        detail::error_category_kind const kind =
            detail::get_error_category_kind(ec.category());
        ar << kind;

        int const value = ec.value();
        ar << value;

        std::string const message = ec.get_message();
        ar << message;

        throwmode const mode = hpx::get_throwmode(ec);
        ar << mode;
    }

    HPX_CXX_CORE_EXPORT inline void serialize(
        input_archive& ar, hpx::error_code& ec, unsigned)
    {
        detail::error_category_kind kind;
        ar >> kind;

        int value;
        ar >> value;

        std::string message;
        ar >> message;

        throwmode mode;
        ar >> mode;

        switch (kind)
        {
        case detail::error_category_kind::hpx:
            // The message may have ': HPX(...)' appended to it.  Remove it as
            // the constructor below re-adds it to the message.
            ec.assign(static_cast<hpx::error>(value),
                detail::strip_hpx_error_message(message), mode);
            break;

        case detail::error_category_kind::generic:
            ec.clear();
            ec.std::error_code::assign(value, std::generic_category());
            break;

        case detail::error_category_kind::system:
            ec.clear();
            ec.std::error_code::assign(value, std::system_category());
            break;

        default:
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "hpx::serialization::serialize<input_archive, "
                "hpx::error_code>",
                "invalid error_category_kind ({}) encountered while "
                "deserializing hpx::error_code",
                static_cast<int>(kind));
        }
    }
}    // namespace hpx::serialization
