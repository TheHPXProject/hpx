//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <cstddef>
#include <cstdint>
#include <system_error>

namespace hpx::parcelset {

    HPX_CXX_EXPORT template <typename ConnectionHandler>
    class parcelport_impl;

    HPX_CXX_EXPORT class HPX_EXPORT parcelhandler;

    namespace policies {

        struct message_handler;
    }

    namespace detail {

        struct create_parcel;
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////
    /// \brief Return boolean value when thread processing is completed.
    ///
    /// This returns true/false based on the background work.
    ///
    /// \param num_thread this represents the number of threads.
    /// \param mode
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool do_background_work(
        std::size_t num_thread = 0,
        parcelport_background_mode mode = parcelport_background_mode::all);

    HPX_CXX_EXPORT using write_handler_type = parcel_write_handler_type;

    ///////////////////////////////////////////////////////////////////////
    // default callback for put_parcel
    HPX_CXX_EXPORT HPX_EXPORT void default_write_handler(
        std::error_code const&, parcel const&);

    ///////////////////////////////////////////////////////////////////////
    /// Hand a parcel to the underlying parcel layer for delivery.
    HPX_CXX_EXPORT HPX_EXPORT void put_parcel(
        parcel&& p, write_handler_type&& f = &default_write_handler);

    /// Hand a parcel to the underlying parcel layer for delivery.
    /// Wait for the operation to finish before returning to the user.
    HPX_CXX_EXPORT HPX_EXPORT void sync_put_parcel(parcelset::parcel&& p);

    /// Return the maximally allowed size of an inbound message (in bytes)
    HPX_CXX_EXPORT HPX_EXPORT std::int64_t get_max_inbound_size(
        parcelport const&);
}    // namespace hpx::parcelset

#endif
