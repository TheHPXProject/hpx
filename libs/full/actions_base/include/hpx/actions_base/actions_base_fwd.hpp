//  Copyright (c)      2016 Thomas Heller
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/basic_action_fwd.hpp>

#include <cstdint>

namespace hpx::actions {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    /// The type of the action defines whether this action will be executed
    /// directly or by an HPX-threads
    HPX_CXX_EXPORT enum class action_flavor : std::uint8_t {
        plain_action = 0,    ///< The action will be executed by a newly created
                             ///< thread
        direct_action = 1    ///< The action needs to be executed directly
    };

    /// The \a base_action class is an abstract class used as the base class
    /// for all action types. It's main purpose is to allow polymorphic
    /// serialization of action instances through a unique_ptr.
    HPX_CXX_EXPORT struct HPX_EXPORT base_action;

    namespace detail {

        HPX_CXX_EXPORT HPX_EXPORT std::uint32_t get_action_id_from_name(
            char const* action_name);
    }    // namespace detail

    /// \endcond
}    // namespace hpx::actions
