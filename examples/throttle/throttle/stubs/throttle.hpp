//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/hpx.hpp>

#include <hpx/future.hpp>
#include <hpx/modules/components_base.hpp>

#include "../server/throttle.hpp"

#include <cstddef>

namespace throttle { namespace stubs {
    ///////////////////////////////////////////////////////////////////////////
    struct throttle : hpx::components::stub_base<server::throttle>
    {
        ///////////////////////////////////////////////////////////////////////
        static hpx::future<void> suspend_async(
            hpx::id_type const& gid, std::size_t thread_num)
        {
            // Create a future, execute the required action,
            // we simply return the initialized future, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::throttle::suspend_action action_type;
            return hpx::async<action_type>(gid, thread_num);
        }

        static void suspend(hpx::id_type const& gid, std::size_t thread_num)
        {
            suspend_async(gid, thread_num).get();
        }

        ///////////////////////////////////////////////////////////////////////
        static hpx::future<void> resume_async(
            hpx::id_type const& gid, std::size_t thread_num)
        {
            // Create a future, execute the required action,
            // we simply return the initialized future, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::throttle::resume_action action_type;
            return hpx::async<action_type>(gid, thread_num);
        }

        static void resume(hpx::id_type const& gid, std::size_t thread_num)
        {
            resume_async(gid, thread_num).get();
        }
    };
}}    // namespace throttle::stubs
