//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file actions_base_support.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/actions_base_fwd.hpp>
#include <hpx/actions_base/actions_base_support.hpp>
#include <hpx/actions_base/traits/action_remote_result.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::actions::detail {

    /// \cond NOINTERNAL
    ///////////////////////////////////////////////////////////////////////
    // Figure out what priority the action has to be associated with a
    // dynamically specified default priority results in using the static
    // Priority.
    HPX_CXX_EXPORT template <threads::thread_priority Priority>
    struct thread_priority
    {
        constexpr static threads::thread_priority call(
            threads::thread_priority priority) noexcept
        {
            if (priority == threads::thread_priority::default_)
            {
                return Priority;
            }
            return priority;
        }
    };

    // If the static Priority is default, a dynamically specified default
    // priority results in using the normal priority.
    template <>
    struct thread_priority<threads::thread_priority::default_>
    {
        constexpr static threads::thread_priority call(
            threads::thread_priority priority) noexcept
        {
            // The mapping to 'normal' is now done at the last possible
            // moment in the scheduler.
            return priority;
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // Figure out what stacksize the action has to be associated with a
    // dynamically specified default stacksize results in using the static
    // Stacksize.
    HPX_CXX_EXPORT template <threads::thread_stacksize Stacksize>
    struct thread_stacksize
    {
        constexpr static threads::thread_stacksize call(
            threads::thread_stacksize stacksize) noexcept
        {
            if (stacksize == threads::thread_stacksize::default_)
            {
                return Stacksize;
            }
            return stacksize;
        }
    };

    // If the static Stacksize is default, a dynamically specified default
    // stacksize results in using the normal stacksize.
    template <>
    struct thread_stacksize<threads::thread_stacksize::default_>
    {
        constexpr static threads::thread_stacksize call(
            threads::thread_stacksize stacksize) noexcept
        {
            return stacksize;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action>
    std::uint32_t get_action_id()
    {
        static std::uint32_t id =
            get_action_id_from_name(get_action_name<Action>());
        return id;
    }
    /// \endcond
}    // namespace hpx::actions::detail

#include <hpx/config/warnings_suffix.hpp>
