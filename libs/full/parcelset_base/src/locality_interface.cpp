//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>

#include <hpx/parcelset_base/detail/locality_interface_functions.hpp>
#include <hpx/parcelset_base/detail/parcel_route_handler.hpp>
#include <hpx/parcelset_base/locality.hpp>
#include <hpx/parcelset_base/locality_interface.hpp>
#include <hpx/parcelset_base/parcel_interface.hpp>
#include <hpx/parcelset_base/policies/message_handler.hpp>

#include <cstdint>
#include <string>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    parcelset::parcel create_parcel()
    {
        return detail::create_parcel();
    }

    locality create_locality(std::string const& name)
    {
        return detail::create_locality(name);
    }

    bool locality_was_disconnected(std::uint32_t const id)
    {
        return detail::locality_was_disconnected(id);
    }

    namespace detail {

        void parcel_route_handler(
            std::error_code const& ec, parcelset::parcel const& p)
        {
            parcel_route_handler_func(ec, p);
        }
    }    // namespace detail
}    // namespace hpx::parcelset

#endif
