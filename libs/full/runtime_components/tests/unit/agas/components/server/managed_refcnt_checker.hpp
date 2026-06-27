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
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>

#include <vector>

namespace hpx { namespace test { namespace server {

    struct HPX_COMPONENT_EXPORT managed_refcnt_checker
      : components::managed_component_base<managed_refcnt_checker>
    {
    private:
        hpx::id_type target_;
        std::vector<hpx::id_type> references_;

    public:
        managed_refcnt_checker()
          : target_(hpx::invalid_id)
          , references_()
        {
        }

        managed_refcnt_checker(hpx::id_type const& target)
          : target_(target)
          , references_()
        {
        }

        ~managed_refcnt_checker();

        void take_reference(hpx::id_type const& gid)
        {
            references_.push_back(gid);
        }

        HPX_DEFINE_COMPONENT_ACTION(managed_refcnt_checker, take_reference)
    };

}}}    // namespace hpx::test::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::test::server::managed_refcnt_checker::take_reference_action,
    managed_refcnt_checker_take_reference_action)

#endif
