//  Copyright (c)      2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/actions_base.hpp>

namespace hpx::actions {

    /// \cond NOINTERNAL

    HPX_CXX_EXPORT struct base_action;
    HPX_CXX_EXPORT struct HPX_EXPORT base_action_data;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action>
    struct transfer_action;

    HPX_CXX_EXPORT template <typename Action>
    struct transfer_continuation_action;
    /// \endcond
}    // namespace hpx::actions

#endif
