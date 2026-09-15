//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/datastructures.hpp>

#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelset/detail/message_handler_interface_functions.hpp>
#include <hpx/parcelset/message_handler_fwd.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <vector>

namespace hpx::parcelset {

    policies::message_handler* get_message_handler(char const* action,
        char const* type, std::size_t num, std::size_t interval,
        locality const& loc, error_code& ec)
    {
        return detail::get_message_handler(
            action, type, num, interval, loc, ec);
    }

    void put_parcel(parcelset::parcel&& p, write_handler_type&& f)
    {
        detail::put_parcel(HPX_MOVE(p), HPX_MOVE(f));
    }

    void sync_put_parcel(parcelset::parcel&& p)
    {
        detail::sync_put_parcel(HPX_MOVE(p));
    }

    namespace detail {

        std::vector<hpx::tuple<char const*, char const*>>&
        get_message_handler_registrations()
        {
            static std::vector<hpx::tuple<char const*, char const*>>
                message_handler_registrations;
            return message_handler_registrations;
        }
    }    // namespace detail
}    // namespace hpx::parcelset

namespace hpx {

    void register_message_handler(
        char const* message_handler_type, char const* action, error_code& ec)
    {
        if (parcelset::detail::register_message_handler != nullptr)
        {
            parcelset::detail::register_message_handler(
                message_handler_type, action, ec);
        }

        // store the request for later
        parcelset::detail::get_message_handler_registrations().emplace_back(
            message_handler_type, action);
    }

    parcelset::policies::message_handler* create_message_handler(
        char const* message_handler_type, char const* action,
        parcelset::parcelport* pp, std::size_t num_messages,
        std::size_t interval, error_code& ec)
    {
        return parcelset::detail::create_message_handler(
            message_handler_type, action, pp, num_messages, interval, ec);
    }
}    // namespace hpx

#endif
