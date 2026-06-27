////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>

#include <hpx/include/async.hpp>
#include <hpx/modules/components_base.hpp>

#include "../server/managed_refcnt_checker.hpp"

namespace hpx { namespace test { namespace stubs {

    struct managed_refcnt_checker
      : components::stub_base<server::managed_refcnt_checker>
    {
        static hpx::future<void> take_reference_async(
            hpx::id_type const& this_, hpx::id_type const& gid)
        {
            typedef server::managed_refcnt_checker::take_reference_action
                action_type;
            return hpx::async<action_type>(this_, gid);
        }

        static void take_reference(
            hpx::id_type const& this_, hpx::id_type const& gid)
        {
            take_reference_async(this_, gid).get();
        }
    };

}}}    // namespace hpx::test::stubs

#endif
