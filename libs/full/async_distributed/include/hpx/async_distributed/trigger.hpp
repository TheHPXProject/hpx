//  Copyright (c) 2007-2020 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/continuation_fwd.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <utility>

namespace hpx::actions {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Result, typename RemoteResult, typename F,
        typename... Ts>
    void trigger(typed_continuation<Result, RemoteResult>&& cont, F&& f,
        Ts&&... vs) noexcept
    {
        try
        {
            cont.trigger_value(
                HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...));
        }
        catch (...)
        {
            // make sure hpx::exceptions are propagated back to the client
            cont.trigger_error(std::current_exception());
        }
    }

    // Overload when return type is "void" aka util::unused_type
    HPX_CXX_EXPORT template <typename Result, typename F, typename... Ts>
    void trigger(typed_continuation<Result, util::unused_type>&& cont, F&& f,
        Ts&&... vs) noexcept
    {
        try
        {
            HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...);
            cont.trigger();
        }
        catch (...)
        {
            // make sure hpx::exceptions are propagated back to the client
            cont.trigger_error(std::current_exception());
        }
    }
}    // namespace hpx::actions
