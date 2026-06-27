//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// the module itself should not register any actions which coalesce parcels
#if defined(HPX_HAVE_PARCEL_COALESCING) && defined(HPX_HAVE_NETWORKING) &&     \
    !defined(HPX_PARCEL_COALESCING_MODULE_EXPORTS) &&                          \
    !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelset/message_handler_fwd.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action>
    constexpr char const* get_action_coalescing_name() noexcept;

    HPX_CXX_EXPORT template <typename Action>
    struct register_coalescing_for_action
    {
        register_coalescing_for_action()
        {
            // ignore all errors as the module might not be available
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::register_message_handler("coalescing_message_handler",
                get_action_coalescing_name<Action>(), ec);
        }
        static register_coalescing_for_action instance_;
    };

    template <typename Action>
    register_coalescing_for_action<Action>
        register_coalescing_for_action<Action>::instance_;
}    // namespace hpx::parcelset

#endif
