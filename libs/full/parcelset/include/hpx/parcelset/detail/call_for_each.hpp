//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <cstddef>
#include <system_error>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset::detail {

    struct call_for_each
    {
        using handlers_type = std::vector<parcelport::write_handler_type>;
        using parcels_type = std::vector<parcelset::parcel>;

        handlers_type handlers_;
        parcels_type parcels_;

        call_for_each(handlers_type&& handlers, parcels_type&& parcels) noexcept
          : handlers_(HPX_MOVE(handlers))
          , parcels_(HPX_MOVE(parcels))
        {
        }

        call_for_each(call_for_each&& other) noexcept = default;
        call_for_each& operator=(call_for_each&& other) noexcept = default;

        void operator()(std::error_code const& e)
        {
            HPX_ASSERT(parcels_.size() == handlers_.size());
            for (std::size_t i = 0; i != parcels_.size(); ++i)
            {
                handlers_[i](e, parcels_[i]);
                handlers_[i].reset();
            }
        }
    };
}    // namespace hpx::parcelset::detail

#endif
