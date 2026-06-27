//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/agas/addressing_service.hpp>
#include <hpx/agas/state.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::agas {

    // return whether resolver client is in state described by st
    bool router_is(state st)
    {
        auto* agas_client = naming::get_agas_client_ptr();
        if (nullptr == agas_client)
        {
            // we're probably either starting or stopping
            return st == hpx::state::starting || st == hpx::state::stopping;
        }
        return (agas_client->get_status() == st);
    }
}    // namespace hpx::agas
